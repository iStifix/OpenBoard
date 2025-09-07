// A touch-friendly rectangular color picker that lets the user select hue and
// saturation by dragging inside a gradient square. The brightness and alpha
// channels are handled externally by dedicated sliders.
#pragma once

#include <QColor>
#include <QWidget>

class UBColorSquare : public QWidget
{
    Q_OBJECT

public:
    explicit UBColorSquare(QWidget* parent = nullptr);

signals:
    void colorSelected(const QColor& color);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;

private:
    void selectColor(const QPoint& pos);
};
