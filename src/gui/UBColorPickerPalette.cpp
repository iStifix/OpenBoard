#include "UBColorPickerPalette.h"

#include "board/UBDrawingController.h"
#include "board/UBBoardController.h"
#include "core/UBApplication.h"
#include "core/UBSettings.h"
#include "core/memcheck.h"

#include <QColor>
#include <QShowEvent>
#include <QSignalBlocker>

UBColorPickerPalette::UBColorPickerPalette(QWidget* parent)
    : UBFloatingPalette(Qt::TopLeftCorner, parent)
    , mHue(0)
    , mSaturation(0)
    , mUpdatingPalette(false)
{
    mUi.setupUi(this);

    mPresetButtons << mUi.colorButton0 << mUi.colorButton1 << mUi.colorButton2
                   << mUi.colorButton3 << mUi.colorButton4;

    for (int i = 0; i < mPresetButtons.size(); ++i)
    {
        QPushButton* button = mPresetButtons.at(i);
        connect(button, &QPushButton::clicked, this, [this, i]() {
            applyColorIndex(i);
        });
    }

    connect(mUi.customButton, &QPushButton::clicked, this, [this]() {
        applyColorIndex(5);
    });

    mUi.widthSlider->setRange(1, 30);
    mUi.widthSlider->setValue(static_cast<int>(UBDrawingController::drawingController()->currentToolWidth()));
    mUi.widthSlider->setTickPosition(QSlider::NoTicks);
    mUi.brightnessSlider->setRange(0, 255);

    QString sliderStyle =
        "QSlider::groove:horizontal {height:30px; background:#606060; border-radius:15px;}"
        "QSlider::handle:horizontal {background:white; border:1px solid #8f8f8f; width:40px; height:40px; margin:-5px 0; border-radius:20px;}"
        "QSlider::groove:vertical {width:30px; background:#606060; border-radius:15px;}"
        "QSlider::handle:vertical {background:white; border:1px solid #8f8f8f; width:40px; height:40px; margin:0 -5px; border-radius:20px;}";
    mUi.widthSlider->setStyleSheet(sliderStyle);
    mUi.brightnessSlider->setStyleSheet(sliderStyle);

    connect(mUi.colorSquare, &UBColorSquare::colorSelected, this, &UBColorPickerPalette::onSquareColorSelected);
    connect(mUi.widthSlider, &QSlider::valueChanged, this, &UBColorPickerPalette::onWidthSliderChanged);
    connect(mUi.brightnessSlider, &QSlider::valueChanged, this, &UBColorPickerPalette::updateCustomColor);

    connect(UBDrawingController::drawingController(), &UBDrawingController::colorPaletteChanged,
            this, &UBColorPickerPalette::refreshPalette);

    if (UBApplication::boardController)
        connect(UBApplication::boardController, &UBBoardController::penColorChanged,
                this, &UBColorPickerPalette::refreshPalette);

    refreshPalette();
}

void UBColorPickerPalette::onSquareColorSelected(const QColor& color)
{
    mHue = color.hue();
    mSaturation = color.saturation();
    updateCustomColor();
}

void UBColorPickerPalette::updateCustomColor()
{
    if (!UBApplication::boardController)
        return;
    QColor c;
    c.setHsv(mHue, mSaturation, mUi.brightnessSlider->value());
    mUi.customButton->setStyleSheet(QString("background:%1;").arg(c.name()));

    mUpdatingPalette = true;
    UBDrawingController::drawingController()->setPenColor(true, c, 5);
    UBDrawingController::drawingController()->setPenColor(false, c, 5);
    UBDrawingController::drawingController()->setMarkerColor(true, c, 5);
    UBDrawingController::drawingController()->setMarkerColor(false, c, 5);
    applyColorIndex(5);
    mUpdatingPalette = false;
}

void UBColorPickerPalette::applyColorIndex(int index)
{
    if (!UBApplication::boardController || !UBApplication::boardController->activeScene())
        return;

    UBDrawingController* dc = UBDrawingController::drawingController();
    UBStylusTool::Enum tool = (UBStylusTool::Enum)dc->stylusTool();
    UBApplication::boardController->setColorIndex(index);

    if (tool == UBStylusTool::Marker)
    {
        UBSettings::settings()->setPenColorIndex(index);
        UBApplication::boardController->setPenColorOnDarkBackground(UBSettings::settings()->penColors(true).at(index));
        UBApplication::boardController->setPenColorOnLightBackground(UBSettings::settings()->penColors(false).at(index));
    }
    else
    {
        UBSettings::settings()->setMarkerColorIndex(index);
        UBApplication::boardController->setMarkerColorOnDarkBackground(UBSettings::settings()->markerColors(true).at(index));
        UBApplication::boardController->setMarkerColorOnLightBackground(UBSettings::settings()->markerColors(false).at(index));
    }
}

void UBColorPickerPalette::onWidthSliderChanged(int value)
{
    UBSettings::settings()->boardPenFineWidth->set(value);
    UBSettings::settings()->boardMarkerFineWidth->set(value);
    UBSettings::settings()->setPenWidthIndex(UBWidth::Fine);
    UBSettings::settings()->setMarkerWidthIndex(UBWidth::Fine);

    UBDrawingController::drawingController()->setLineWidthIndex(UBWidth::Fine);
}

void UBColorPickerPalette::hideEvent(QHideEvent* event)
{
    UBFloatingPalette::hideEvent(event);
    emit closed();
}

void UBColorPickerPalette::showEvent(QShowEvent* event)
{
    UBFloatingPalette::showEvent(event);
    refreshPalette();
    mUi.widthSlider->setValue(static_cast<int>(UBDrawingController::drawingController()->currentToolWidth()));
}

void UBColorPickerPalette::refreshPalette()
{
    if (mUpdatingPalette)
        return;

    if (!UBApplication::boardController)
        return;

    mUpdatingPalette = true;

    bool dark = UBSettings::settings()->isDarkBackground();
    QList<QColor> lightColors = UBSettings::settings()->penColors(false);
    QList<QColor> darkColors = UBSettings::settings()->penColors(true);

    for (int i = 0; i < mPresetButtons.size() && i < lightColors.size() && i < darkColors.size(); ++i)
    {
        QColor c = dark ? darkColors.at(i) : lightColors.at(i);
        mPresetButtons.at(i)->setStyleSheet(QString("background:%1;").arg(c.name()));
        UBDrawingController::drawingController()->setMarkerColor(true, darkColors.at(i), i);
        UBDrawingController::drawingController()->setMarkerColor(false, lightColors.at(i), i);
    }

    if (lightColors.size() > 5 && darkColors.size() > 5)
    {
        QColor c = dark ? darkColors.at(5) : lightColors.at(5);
        mHue = c.hue();
        mSaturation = c.saturation();
        {
            QSignalBlocker blocker(mUi.brightnessSlider);
            mUi.brightnessSlider->setValue(c.value());
        }
        mUi.customButton->setStyleSheet(QString("background:%1;").arg(c.name()));
        UBDrawingController::drawingController()->setMarkerColor(true, darkColors.at(5), 5);
        UBDrawingController::drawingController()->setMarkerColor(false, lightColors.at(5), 5);
    }
    else
    {
        {
            QSignalBlocker blocker(mUi.brightnessSlider);
            mUi.brightnessSlider->setValue(255);
        }
        mUi.customButton->setStyleSheet(QString("background:%1;").arg(Qt::white));
    }

    mUpdatingPalette = false;
}
