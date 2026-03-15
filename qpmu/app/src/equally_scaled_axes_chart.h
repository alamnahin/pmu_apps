#ifndef QPMU_APP_EQUALLY_SCALED_AXES_CHART_H
#define QPMU_APP_EQUALLY_SCALED_AXES_CHART_H

#include <QtWidgets>
#include <QtCharts>
#include <QGraphicsSceneResizeEvent>
#include <QtGlobal>

class EquallyScaledAxesChart : public QChart
{
public:
    EquallyScaledAxesChart(QGraphicsItem *parent = nullptr,
                           Qt::WindowFlags wFlags = Qt::WindowFlags());

protected:
    void resizeEvent(QGraphicsSceneResizeEvent *event) override;
};

#endif // QPMU_APP_EQUALLY_SCALED_AXES_CHART_H