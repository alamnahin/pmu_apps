#ifndef QPMU_APP_H
#define QPMU_APP_H

#include "qpmu/defs.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QSettings>
#include <QTimer>
#include <QIODevice>
#include <QStringList>

QT_FORWARD_DECLARE_CLASS(DataProcessor)
QT_FORWARD_DECLARE_CLASS(DataObserver)
QT_FORWARD_DECLARE_CLASS(MainWindow)

class App : public QApplication
{
#define APP (static_cast<App *>(QCoreApplication::instance()))

    Q_OBJECT

public:
    static constexpr quint32 TimerIntervalMs = 100;
    static constexpr quint32 DataViewUpdateIntervalMs = 200;

    App(int &argc, char **argv);

    QTimer *timer() const
    {
        return m_timer;
    }
    DataProcessor *dataProcessor() const
    {
        return m_dataProcessor;
    }
    DataObserver *dataObserver() const
    {
        return m_dataObserver;
    }
    MainWindow *mainWindow() const
    {
        return m_mainWindow;
    }

private:
    QTimer *m_timer = nullptr;
    MainWindow *m_mainWindow = nullptr;
    DataProcessor *m_dataProcessor = nullptr;
    DataObserver *m_dataObserver = nullptr;
};

#endif // QPMU_APP_H