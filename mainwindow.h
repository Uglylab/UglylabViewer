#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QToolBar>
#include <QAction>
#include <QProcess>
#include <QSettings>
#include "viewerwidget.h"  // Your QOpenGLWidget subclass

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onStartStopToggled();
    void onStepClicked();
    void onResetClicked();
    void chooseSimulatorPath();
    void updateStep(int step);

private:
    void createActions();
    void createToolbar();
    void createMenuBar();
    void launchSimulator();

    CommandBuffer* cmd;  // Shared memory command buffer
    SharedBuffer* shm;  // Shared memory visualization buffer
    void* gridBuffer = nullptr;  // Pointer to the grid data in shared memory
    ViewerWidget* viewer;          // OpenGL viewport
    QToolBar* toolbar;
    QAction* actionStartStop;
    QAction* actionStep;
    QAction* actionSelectSimulator;
    QAction* actionReset;


    QProcess* simulatorProcess;
    QString simulatorPath = "./simulator";  // default path
    bool running;
};
#endif // MAINWINDOW_H
