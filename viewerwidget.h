#ifndef VIEWERWIDGET_H
#define VIEWERWIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QTimer>
#include <QMatrix4x4>
#include "uglylab_sharedmemory.h"  // contains AgentData, SharedBuffer, openSharedBuffer()
#include "vec3.h"  // contains Vec3 class definition

class ViewerWidget : public QOpenGLWidget, protected QOpenGLFunctions {
    Q_OBJECT

public:
    explicit ViewerWidget(SharedBuffer* shm, QWidget* parent = nullptr);
    ~ViewerWidget();
    void resetView();

    void setGridBuffer(void* buffer, GridDataType type);

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;


private:
    QTimer timer;
    std::vector<AgentData> visibleAgents;
    void readSharedMemory();
    SharedBuffer* shm = nullptr;
    QMatrix4x4 viewMatrix;
    float zoom = 1.0f;
    float yaw = 0.0f, pitch = 0.0f;
    float panX = 0.0f, panY = 0.0f;

    QPoint lastMousePos;

    void* gridDataRaw = nullptr;
    GridDataType gridType = GRID_TYPE_INT;

    template<typename T>
    Vec3 defaultColorMap(T value, T min, T max) {
        float t = float(value - min) / float(max - min + 1e-5f);
        return Vec3{t, 0.2f, 1.0f - t};  // blue to pink gradient
    }

    template<typename T>
    void drawGrid(const SharedGrid<T>* grid, T minVal, T maxVal);


signals:
    void stepUpdated(int);
};
#endif // VIEWERWIDGET_H
