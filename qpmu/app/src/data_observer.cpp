#include "data_observer.h"
#include "src/settings_models.h"

DataObserver::DataObserver(QObject *parent) : QObject(parent)
{
    connect(APP->timer(), &QTimer::timeout, this, &DataObserver::update);
}

void DataObserver::update()
{
    ++m_updateCounter;
    if (m_updateCounter * App::TimerIntervalMs >= App::DataViewUpdateIntervalMs) {
        m_updateCounter = 0;
        emit sampleUpdated(APP->dataProcessor()->lastSample());
        emit estimationUpdated(APP->dataProcessor()->lastEstimation());
    }
}