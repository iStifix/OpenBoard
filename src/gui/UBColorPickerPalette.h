#pragma once

#include "UBFloatingPalette.h"
#include "UBColorSquare.h"
#include "ui_colorPickerPalette.h"

#include <QPushButton>
#include <QSlider>
#include <QShowEvent>

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

private:
    Ui::ColorPickerPalette mUi;
    QList<QPushButton*> mPresetButtons;
    int mHue;
    int mSaturation;
    bool mUpdatingPalette;

    void applyColorIndex(int index);
};
