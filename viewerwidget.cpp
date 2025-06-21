#include "viewerwidget.h"
#include <QDebug>
#include <QMouseEvent>

ViewerWidget::ViewerWidget(SharedBuffer *shm, QWidget* parent)
    : QOpenGLWidget(parent),
      shm(shm)  {
    connect(&timer, &QTimer::timeout, this, QOverload<>::of(&ViewerWidget::update));
    timer.start(16); // ~60 FPS
}

ViewerWidget::~ViewerWidget() {}

void ViewerWidget::initializeGL() {
    initializeOpenGLFunctions();
    glEnable(GL_PROGRAM_POINT_SIZE);
    glClearColor(0, 0, 0, 1);
}

void ViewerWidget::resizeGL(int w, int h) {
    glViewport(0, 0, w, h);
}

void ViewerWidget::paintGL() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    readSharedMemory();  // Update visibleAgents

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    float aspect = float(width()) / float(height());
    float viewSize = 100.0f;  // logical view size, adjust if needed

    // Keep world units, but scale based on zoom
    glOrtho(-viewSize * aspect * zoom, viewSize * aspect * zoom,
            -viewSize * zoom, viewSize * zoom,
            -1000, 1000);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // Apply pan and rotation
    glTranslatef(panX, panY, 0);
    glRotatef(pitch, 1.0f, 0.0f, 0.0f);
    glRotatef(yaw,   0.0f, 1.0f, 0.0f);

    // Draw agents
    glBegin(GL_POINTS);
    for (const AgentData& agent : visibleAgents) {
        switch (agent.species_id) {
        case 1: glColor3f(0, 1, 0); break; // green
        case 2: glColor3f(1, 0, 0); break; // red
        default: glColor3f(1, 1, 1); break;
        }
        glVertex3f(agent.x, agent.y, agent.z);
    }
    glEnd();

    // Draw base axes
    glBegin(GL_LINES);

    // X axis (red)
    glColor3f(1, 0, 0);
    glVertex3f(0, 0, 0);
    glVertex3f(20, 0, 0);

    // Y axis (green)
    glColor3f(0, 1, 0);
    glVertex3f(0, 0, 0);
    glVertex3f(0, 20, 0);

    // Z axis (blue)
    glColor3f(0, 0, 1);
    glVertex3f(0, 0, 0);
    glVertex3f(0, 0, 20);

    glEnd();

    if (gridDataRaw) {
        switch (gridType) {
        case GRID_TYPE_INT:
            drawGrid(static_cast<SharedGrid<int>*>(gridDataRaw), 0, 10);
            break;
        case GRID_TYPE_FLOAT:
            drawGrid(static_cast<SharedGrid<float>*>(gridDataRaw), 0.0f, 1.0f);
            break;
        case GRID_TYPE_BOOL:
            drawGrid(static_cast<SharedGrid<bool>*>(gridDataRaw), false, true);
            break;
        }
    }

}


void ViewerWidget::readSharedMemory() {
    if (!shm) return;

    visibleAgents.clear();

    int index = shm->visible_buffer_index.load();
    auto& chunks = shm->buffers[index];

    int current_frame = chunks[0].frame_index;
    int total_chunks = chunks[0].total_chunks;

    int currentStep = shm->currentStep.load();
    emit stepUpdated(currentStep);  // Custom signal

    for (int i = 0; i < total_chunks; ++i) {
        if (chunks[i].frame_index != current_frame || chunks[i].ready.load() == 0)
            continue;

        for (int j = 0; j < chunks[i].agents_in_chunk; ++j) {
            visibleAgents.push_back(chunks[i].agents[j]);
        }
    }
}

template<typename T>
void ViewerWidget::drawGrid(const SharedGrid<T>* grid, T minVal, T maxVal) {
    if (!grid) return;
    glPointSize(3.0f);
    glBegin(GL_POINTS);
    for (int z = 0; z < grid->zSize; ++z)
        for (int y = 0; y < grid->ySize; ++y)
            for (int x = 0; x < grid->xSize; ++x) {
                T v = grid->data[x + grid->xSize * (y + grid->ySize * z)];
                if (v == T()) continue;

                Vec3 color = defaultColorMap(v, minVal, maxVal);
                glColor3f(color.x, color.y, color.z);

                Vec3 pos {
                    (x + 0.5f) * grid->cellSize,
                    (y + 0.5f) * grid->cellSize,
                    (z + 0.5f) * grid->cellSize
                };
                glVertex3f(pos.x, pos.y, pos.z);
            }
    glEnd();
}

void ViewerWidget::mousePressEvent(QMouseEvent* event) {
    lastMousePos = event->pos();
}

void ViewerWidget::mouseMoveEvent(QMouseEvent* event) {
    QPoint delta = event->pos() - lastMousePos;
    lastMousePos = event->pos();

    if (event->buttons() & Qt::LeftButton) {
        yaw += delta.x() * 0.5f;
        pitch += delta.y() * 0.5f;
        update();
    }
    else if (event->buttons() & (Qt::RightButton | Qt::MiddleButton)) {
        float viewSize = 100.0f;  // must match glOrtho()
        float unitsPerPixel = (2.0f * viewSize * zoom) / height();

        panX += delta.x() * unitsPerPixel;
        panY -= delta.y() * unitsPerPixel;

        update();
    }
}

void ViewerWidget::wheelEvent(QWheelEvent* event) {
    zoom *= std::pow(1.001, event->angleDelta().y());
    zoom = std::clamp(zoom, 0.1f, 100.0f);
    update();
}
void ViewerWidget::resetView() {
    yaw = 0.0f;
    pitch = 0.0f;
    panX = 0.0f;
    panY = 0.0f;
    zoom = 1.0f;
    update();
}

void ViewerWidget::setGridBuffer(void *buffer, GridDataType type)
{
    gridDataRaw = buffer;
    gridType = type;
}

