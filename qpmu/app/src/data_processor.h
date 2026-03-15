#ifndef QPMU_APP_DATA_PROCESSOR_H
#define QPMU_APP_DATA_PROCESSOR_H

#include "qpmu/defs.h"
#include "qpmu/estimator.h"
#include "settings_models.h"
#include "app.h"

#include <QThread>
#include <QIODevice>
#include <QAbstractSocket>
#include <QProcess>
#include <QMutex>
#include <QMutexLocker>

#include <functional>
#include <array>

class SampleReader : public QObject
{
    Q_OBJECT
public:
    enum StateFlag {
        Enabled = 1 << 0,
        Connected = 1 << 1,
        DataReading = 1 << 2,
        DataValid = 1 << 3,
    };

    SampleReader() = default;
    SampleReader(const SamplerSettings &settings);
    virtual ~SampleReader()
    {
        if (m_device) {
            m_device->close();
        }
    }

    static QString stateString(int state)
    {
        return QStringLiteral("%1, %2, %3, %4")
                .arg(bool(state & Enabled) ? "Enabled" : "")
                .arg(bool(state & Connected) ? "Connected" : "")
                .arg(bool(state & DataReading) ? "DataReading" : "")
                .arg(bool(state & DataValid) ? "DataValid" : "");
    }

    SamplerSettings settings() const { return m_settings; }
    void read(qpmu::Sample &outSample);
    int state() const { return m_state; }

private:
    SamplerSettings m_settings;
    QIODevice *m_device = nullptr;
    int m_state = 0;

    std::function<bool()> m_waitForConnected = nullptr;
    std::function<bool(qpmu::Sample &)> m_readSample = nullptr;

    int m_connectionWaitTime = 3000;
    int m_readWaitTime = 1000;
};

class DataProcessor : public QThread
{
    Q_OBJECT

public:
    explicit DataProcessor();

    void run() override;

    int sampleReaderState()
    {
        QMutexLocker locker(&m_mutex);
        return m_reader->state();
    }
    const qpmu::Estimation &lastEstimation()
    {
        QMutexLocker locker(&m_mutex);
        return m_estimator->lastEstimation();
    }
    const qpmu::Sample &lastSample()
    {
        QMutexLocker locker(&m_mutex);
        return m_estimator->lastSample();
    }

    void updateSampleReader();

signals:
    void sampleReaderStateChanged(int);

private:
    QMutex m_mutex;
    qpmu::PhasorEstimator *m_estimator = nullptr;

    SampleReader *m_reader = nullptr;
    SampleReader *m_newReader = nullptr;
};

#endif // QPMU_APP_DATA_PROCESSOR_H
