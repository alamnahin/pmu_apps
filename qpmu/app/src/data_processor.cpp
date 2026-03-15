#include "qpmu/defs.h"
#include "qpmu/util.h"
#include "app.h"
#include "data_processor.h"
#include "main_window.h"
#include "src/settings_models.h"

#include <QDebug>
#include <QAbstractSocket>
#include <QTcpSocket>
#include <QUdpSocket>
#include <QProcess>
#include <QFile>
#include <QMutexLocker>
#include <QLabel>

#include <array>
#include <cctype>
#include <iostream>
#include <qabstractsocket.h>

using namespace qpmu;

SampleReader::SampleReader(const SamplerSettings &settings) : m_settings(settings)
{
    if (m_settings.connection == SamplerSettings::Socket) {
        QAbstractSocket *socket;
        if (m_settings.socketConfig.socketType == SamplerSettings::TcpSocket)
            socket = new QTcpSocket(this);
        else
            socket = new QUdpSocket(this);
        m_device = socket;
        m_waitForConnected = [=] {
            return socket->state() >= QAbstractSocket::ConnectedState
                    || socket->waitForConnected(m_connectionWaitTime);
        };
        socket->connectToHost(m_settings.socketConfig.host, m_settings.socketConfig.port);
    } else if (m_settings.connection == SamplerSettings::Process) {
        auto process = new QProcess(this);
        m_device = process;
        process->setProgram(m_settings.processConfig.prog);
        process->setArguments(m_settings.processConfig.args);
        m_waitForConnected = [=] {
            return process->state() >= QProcess::Running
                    || process->waitForStarted(m_connectionWaitTime);
        };
        process->start(QProcess::ReadOnly);
    }

    if (settings.connection != SamplerSettings::None) {
        if (settings.isDataBinary) {
            m_readSample = [this](qpmu::Sample &sample) {
                auto bytesRead = m_device->read((char *)&sample, sizeof(Sample));
                return (bytesRead == sizeof(Sample));
            };
        } else {
            m_readSample = [this](qpmu::Sample &sample) {
                char line[1000];
                bool ok = true;
                if (m_device->readLine(line, sizeof(line)) > 0) {
                    std::string error;
                    sample = parseSample(line, &error);
                    if (!error.empty()) {
                        ok = false;
                        qInfo() << "Error parsing sample: " << error.c_str();
                        qInfo() << "Sample line:   " << line;
                        qInfo() << "Parsed sample: " << toString(sample).c_str();
                    }
                }
                return ok;
            };
        }
    }
}

void SampleReader::read(qpmu::Sample &outSample)
{
    int newState = 0;
    /// Check if enabled
    newState |= (Enabled * bool(m_settings.connection != SamplerSettings::None));
    /// Check if connected
    newState |= (Connected * bool((newState & Enabled) && m_waitForConnected()));
    /// Check if reading
    newState |= (DataReading
                 * bool((newState & Connected) && m_device->waitForReadyRead(m_readWaitTime)));
    /// Check if valid
    newState |= (DataValid * bool((newState & DataReading) && m_readSample(outSample)));

    m_state = newState;
}

DataProcessor::DataProcessor() : QThread()
{
    m_estimator = new PhasorEstimator(50, 1200);
    updateSampleReader();
}

void DataProcessor::updateSampleReader()
{
    SamplerSettings newSettings;
    newSettings.load();
    auto newSampler = new SampleReader(newSettings);

    QMutexLocker locker(&m_mutex);
    m_newReader = newSampler;
    /// `moveToThread` must be called from the thread where the object was
    /// created, so we do it here
    m_newReader->moveToThread(this); /// Move the new sampler to `this`, a `QThread` object
}

void DataProcessor::run()
{
    Sample sample;

    while (true) {
        if (m_newReader) {
            {
                QMutexLocker locker(&m_mutex);
                std::swap(m_reader, m_newReader);
            }
            delete m_newReader;
            m_newReader = nullptr;
        }

        if (m_reader) {
            auto oldState = m_reader->state();
            m_reader->read(sample);
            auto newState = m_reader->state();
            if (newState & SampleReader::DataValid) {
                m_estimator->updateEstimation(sample);
            }
            if (oldState != newState) {
                emit sampleReaderStateChanged(newState);
            }
        }
    }
}
