#ifndef QPMU_APP_DATA_OBSERVER_H
#define QPMU_APP_DATA_OBSERVER_H

#include "app.h"
#include "qpmu/defs.h"
#include "settings_models.h"
#include "settings_widget.h"
#include "data_processor.h"

#include <QObject>
#include <QString>
#include <QColor>
#include <QTimer>

#include <array>

class DataObserver : public QObject
{
    Q_OBJECT

public:
    using USize = qpmu::USize;
    using SignalColors = std::array<QColor, qpmu::CountSignals>;

    explicit DataObserver(QObject *parent = nullptr);

    qpmu::Sample sample() const { return m_sample; }

    qpmu::Estimation estimation() const { return m_estimation; }

signals:
    void sampleUpdated(const qpmu::Sample &sample);
    void estimationUpdated(const qpmu::Estimation &estimation);

private slots:
    void update();

private:
    USize m_updateCounter = 0;
    
    qpmu::Sample m_sample = {};
    qpmu::Estimation m_estimation = {};
};

#endif // QPMU_APP_DATA_OBSERVER_H