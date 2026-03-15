#ifndef QPMU_GUI_UTIL_H
#define QPMU_GUI_UTIL_H

#include "qpmu/defs.h"

#include <QPainter>
#include <QPixmap>
#include <QIcon>
#include <QPair>
#include <QVector>

#include <cmath>

QPixmap circlePixmap(const QColor &color, int size);

QPixmap twoColorCirclePixmap(const QColor &color1, const QColor &color2, int size);

QPixmap rectPixmap(const QColor &color, int width, int height);

QPixmap twoColorRectPixmap(const QColor &color1, const QColor &color2, int width, int height);

QPointF unitvector(qreal angle);

#endif // QPMU_GUI_UTIL_H