#ifndef QPMU_APP_MAIN_WINDOW_H
#define QPMU_APP_MAIN_WINDOW_H

#include "app.h"

#include <QMainWindow>

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

private:
};

#endif // QPMU_APP_MAIN_WINDOW_H
