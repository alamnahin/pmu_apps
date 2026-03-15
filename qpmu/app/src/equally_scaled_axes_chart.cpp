#include "equally_scaled_axes_chart.h"
#include <qnamespace.h>

EquallyScaledAxesChart::EquallyScaledAxesChart(QGraphicsItem *parent, Qt::WindowFlags wFlags)
    : QChart(parent, wFlags)
{
}

void EquallyScaledAxesChart::resizeEvent(QGraphicsSceneResizeEvent *event)
{

    QChart::resizeEvent(event);

    auto p1 = mapToPosition(QPointF(0, 0));
    auto p2 = mapToPosition(QPointF(1, 1));
    auto w2h = std::abs(p2.x() - p1.x()) / std::abs(p2.y() - p1.y());

    auto s = event->newSize();

    if (w2h > 1.0) {
        // too wide
        s.setWidth(s.width() / w2h);
    } else {
        // too high
        s.setHeight(s.height() * w2h);
    }

    resize(s);
}
