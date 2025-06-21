#include "mainwindow.h"
#include <QMenuBar>
#include <QMessageBox>
#include <QDebug>
#include <QFileDialog>
#include <QFileInfo>
#include <QStatusBar>
#include "uglylab_sharedmemory.h"
#include <QThread>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), simulatorProcess(nullptr), running(false) {
    cmd = openOrCreateCommandBuffer();
    shm = openOrCreateSharedBuffer();

    createActions();
    createToolbar();
    createMenuBar();

    viewer = new ViewerWidget(shm, this);
    setCentralWidget(viewer);

    QSettings settings("Uglylab", "Viewer");
    simulatorPath = settings.value("simulatorPath", "./simulator").toString();

    connect(viewer, &ViewerWidget::stepUpdated, this, &MainWindow::updateStep);
    statusBar(); // Initialize status bar
    statusBar()->showMessage("Step: 0");
    resize(1200, 800);
}

MainWindow::~MainWindow() {
    if (cmd) {
        cmd->command.store(CMD_TERMINATE);
        QThread::msleep(50);  // Allow immediate command to be processed
    }

    if (simulatorProcess) {
        simulatorProcess->waitForFinished(1000);  // ✅ Let it exit on its own
        if (simulatorProcess->state() != QProcess::NotRunning) {
            simulatorProcess->terminate();        // ✅ Fallback only if needed
            simulatorProcess->waitForFinished(1000);
        }
        delete simulatorProcess;
    }

    shm_unlink(SHM_NAME);
    shm_unlink(CMD_SHM_NAME);
}


void MainWindow::createActions() {
    actionStartStop = new QAction(QIcon::fromTheme("media-playback-start"), "Start", this);
    actionStartStop->setCheckable(true);
    connect(actionStartStop, &QAction::toggled, this, &MainWindow::onStartStopToggled);

    actionStep = new QAction(QIcon::fromTheme("go-next"), "Step", this);
    connect(actionStep, &QAction::triggered, this, &MainWindow::onStepClicked);

    actionReset = new QAction(QIcon::fromTheme("view-refresh"), "Reset", this);
    connect(actionReset, &QAction::triggered, this, &MainWindow::onResetClicked);

}

void MainWindow::createToolbar() {
    toolbar = addToolBar("Simulation Controls");
    toolbar->addAction(actionStartStop);
    toolbar->addAction(actionStep);
    toolbar->addAction(actionReset);

}

void MainWindow::createMenuBar() {
    QMenu* fileMenu = menuBar()->addMenu("File");
    fileMenu->addAction("Select Simulator...", this, &MainWindow::chooseSimulatorPath);
    fileMenu->addSeparator();
    fileMenu->addAction("Quit", this, &QWidget::close);
}

void MainWindow::onStartStopToggled() {
    if (!cmd) return;

    if (!running) {
        cmd->command.store(CMD_START);
        actionStartStop->setText("Stop");
    } else {
        cmd->command.store(CMD_STOP);
        actionStartStop->setText("Start");
    }

    running = !running;
}

void MainWindow::onStepClicked() {
    if (!cmd) return;
    cmd->command.store(CMD_STEP);
}

void MainWindow::onResetClicked() {
    if (cmd) {
        cmd->command.store(CMD_RESET);
    }
    viewer->resetView();
    statusBar()->showMessage("Step: 0");
}


void MainWindow::chooseSimulatorPath() {
    QString path = QFileDialog::getOpenFileName(this, "Select Simulator Executable");
    if (!path.isEmpty()) {
        QFileInfo info(path);
        if (info.exists() && info.isExecutable()) {
            simulatorPath = path;
            QSettings settings("Uglylab", "Viewer");
            settings.setValue("simulatorPath", simulatorPath);
            launchSimulator();
        } else {
            QMessageBox::warning(this, "Invalid File", "The selected file is not an executable.");
        }
    }
}

void MainWindow::launchSimulator() {
    if (simulatorProcess) {
        simulatorProcess->terminate();
        simulatorProcess->waitForFinished(3000);
        delete simulatorProcess;
    }

    simulatorProcess = new QProcess(this);
    simulatorProcess->setProgram(simulatorPath);
    // 🔽 Capture all output from the simulator
    simulatorProcess->setProcessChannelMode(QProcess::MergedChannels);
    connect(simulatorProcess, &QProcess::readyReadStandardOutput, this, [this]() {
        qDebug() << simulatorProcess->readAllStandardOutput();
    });

    // 🔽 Catch launch and runtime errors
    connect(simulatorProcess, &QProcess::errorOccurred, this, [](QProcess::ProcessError error) {
        qWarning() << "Simulator process error:" << error;
    });

    simulatorProcess->start();

    if (!simulatorProcess->waitForStarted(3000)) {
        QMessageBox::critical(this, "Error", "Failed to start simulator.");
        delete simulatorProcess;
        simulatorProcess = nullptr;
    } else {
        qDebug() << "Simulator started: " << simulatorPath;
        // Wait briefly for simulator to populate grid request (non-blocking version)
        QTimer::singleShot(100, this, [this]() {
            if (cmd && cmd->gridRequested.load() && !cmd->gridReady.load()) {
                void* buffer = createGridBufferFromCommand(cmd);
                if (buffer) {
                    gridBuffer = buffer;
                    viewer->setGridBuffer(gridBuffer, static_cast<GridDataType>(cmd->gridType.load()));  // This lets viewer draw grid
                    cmd->gridReady.store(true);
                    qDebug() << "Grid shared memory created successfully.";
                } else {
                    QMessageBox::critical(this, "Grid Error", "Failed to create grid shared memory.");
                }
            } else {
                qDebug() << "No grid requested by simulator.";
            }
            // 🔽 After grid setup (if any), tell simulator to perform initial write
            if (cmd) {
                cmd->command.store(CMD_INITIALIZE);
                qDebug() << "Sent CMD_INITIALIZE to simulator.";
            }
        });


    }
}

void MainWindow::updateStep(int step) {
    statusBar()->showMessage(QString("Step: %1").arg(step));
}


