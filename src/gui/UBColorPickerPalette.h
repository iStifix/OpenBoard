#pragma once

#include "UBFloatingPalette.h"
#include "UBColorSquare.h"
#include "ui_colorPickerPalette.h"

#include <QPushButton>
#include <QSlider>
#include <QShowEvent>
#include <QPointer>
#include <QElapsedTimer>

class UBColorPickerPalette : public UBFloatingPalette
{
    Q_OBJECT

public:
    explicit UBColorPickerPalette(QWidget* parent = nullptr);

signals:
    void closed();

private slots:
    void onSquareColorSelected(const QColor& color);
    void updateCustomColor();
    void onWidthSliderChanged(int value);
    void refreshPalette();

protected:
    void hideEvent(QHideEvent* event) override;
    void showEvent(QShowEvent* event) override;
    bool eventFilter(QObject* obj, QEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override {}    // disable moving
    void mouseMoveEvent(QMouseEvent* event) override {}     // disable moving
    void mouseReleaseEvent(QMouseEvent* event) override {}  // disable moving

private:
    Ui::ColorPickerPalette mUi;
    QList<QPushButton*> mPresetButtons;
    int mHue;
    int mSaturation;
    bool mUpdatingPalette;

    void applyColorIndex(int index);
    void updateStrokePreview();
    class PreviewBubble : public QWidget
    {
        public:
            explicit PreviewBubble(QWidget* parent = nullptr);
            void setColor(const QColor& c);
        protected:
            void paintEvent(QPaintEvent* event) override;
    private:
        QColor mColor;
    };
    QPointer<PreviewBubble> mPreview;
    QElapsedTimer mHoverClock;
    qint64 mLastHoverTick = 0;
};
