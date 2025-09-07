#include "UBColorSquare.h"

#include <QImage>
#include <QMouseEvent>
#include <QPainter>
#include <QtGlobal>

#include "core/memcheck.h"

UBColorSquare::UBColorSquare(QWidget* parent)
    : QWidget(parent)
{
    setMinimumSize(200, 200);
    setMouseTracking(true);
}

void UBColorSquare::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);

    QImage img(size(), QImage::Format_RGB32);
    for (int y = 0; y < img.height(); ++y)
    {
        int sat = 255 - (255 * y / img.height());
        for (int x = 0; x < img.width(); ++x)
        {
            int hue = 359 * x / img.width();
            QColor c;
            c.setHsv(hue, sat, 255);
            img.setPixel(x, y, c.rgb());
        }
    }

    QPainter p(this);
    p.drawImage(0, 0, img);
}

void UBColorSquare::mousePressEvent(QMouseEvent* event)
{
    selectColor(event->pos());
    hoverColor(event->pos());
}

void UBColorSquare::mouseMoveEvent(QMouseEvent* event)
{
    if (event->buttons() & Qt::LeftButton)
        selectColor(event->pos());
    hoverColor(event->pos());
}

void UBColorSquare::selectColor(const QPoint& pos)
{
    int x = qBound(0, pos.x(), width() - 1);
    int y = qBound(0, pos.y(), height() - 1);
    int hue = 359 * x / width();
    int sat = 255 - (255 * y / height());
    QColor c;
    c.setHsv(hue, sat, 255);
    emit colorSelected(c);
}

void UBColorSquare::hoverColor(const QPoint& pos)
{
    int x = qBound(0, pos.x(), width() - 1);
    int y = qBound(0, pos.y(), height() - 1);
    int hue = 359 * x / width();
    int sat = 255 - (255 * y / height());
    QColor c;
    c.setHsv(hue, sat, 255);
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    emit colorHovered(c, mapToGlobal(pos));
#else
    emit colorHovered(c, mapToGlobal(pos));
#endif
}

void UBColorSquare::leaveEvent(QEvent* event)
{
    Q_UNUSED(event);
    emit hoverEnded();
}
