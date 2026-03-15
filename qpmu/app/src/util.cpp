#include "util.h"

#include <QDebug>

QPixmap circlePixmap(const QColor &color, int size)
{
    QPixmap pixmap(size, size);
    pixmap.fill(Qt::transparent); // Fill the pixmap with transparent color

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);

    // Set the color and draw a solid circle
    painter.setPen(Qt::NoPen);
    painter.setBrush(color);
    painter.drawEllipse(0, 0, size, size);
    painter.end();

    return pixmap;
}

QPixmap twoColorCirclePixmap(const QColor &color1, const QColor &color2, int size)
{
    QPixmap pixmap(size, size);
    pixmap.fill(Qt::transparent); // Fill the pixmap with transparent color

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);

    // Set the color and draw a solid circle
    painter.setPen(Qt::NoPen);
    painter.setBrush(color1);
    painter.drawPie(0, 0, size, size, 90 * 16, 270 * 16);
    painter.setBrush(color2);
    painter.drawPie(0, 0, size, size, 0 * 16, -90 * 16);
    painter.drawPie(0, 0, size, size, 0 * 16, +90 * 16);
    painter.end();

    return pixmap;
}

QPixmap rectPixmap(const QColor &color, int width, int height)
{
    QPixmap pixmap(width, height);
    pixmap.fill(Qt::transparent); // Fill the pixmap with transparent color

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);

    // Set the color and draw a solid circle
    painter.setPen(Qt::NoPen);
    painter.setBrush(color);
    painter.drawRect(0, 0, width, height);
    painter.end();

    return pixmap;
}

QPixmap twoColorRectPixmap(const QColor &color1, const QColor &color2, int width, int height)
{
    QPixmap pixmap(width, height);
    pixmap.fill(Qt::transparent); // Fill the pixmap with transparent color

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);

    // Set the color and draw a solid circle
    painter.setPen(Qt::NoPen);
    painter.setBrush(color1);
    painter.drawRect(0, 0, width / 2, height);
    painter.setBrush(color2);
    painter.drawRect(width / 2, 0, width / 2, height);
    painter.end();

    return pixmap;
}

QPointF unitvector(qreal angle)
{
    return QPointF(std::cos(angle), std::sin(angle));
}
