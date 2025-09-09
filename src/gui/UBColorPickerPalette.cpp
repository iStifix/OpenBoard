#include "UBColorPickerPalette.h"

#include "board/UBDrawingController.h"
#include "board/UBBoardController.h"
#include "core/UBApplication.h"
#include "gui/UBMainWindow.h"
#include "core/UBSettings.h"
#include "core/memcheck.h"

#include <QColor>
#include <QShowEvent>
#include <QMouseEvent>
#include <QTouchEvent>
#include <QSignalBlocker>
#include <QPainter>
#include <QPainterPath>
#include <QGuiApplication>
#include <QScreen>

UBColorPickerPalette::UBColorPickerPalette(QWidget* parent)
    : UBFloatingPalette(Qt::TopLeftCorner, parent)
    , mHue(0)
    , mSaturation(0)
    , mUpdatingPalette(false)
{
    mUi.setupUi(this);
    // Make palette non-draggable
    setGrip(false);
    // Touch support: accept touch on palette; rely on synthesized mouse for colorSquare
    setAttribute(Qt::WA_AcceptTouchEvents, true);

    mPresetButtons << mUi.colorButton0 << mUi.colorButton1 << mUi.colorButton2
                   << mUi.colorButton3 << mUi.colorButton4;

    for (int i = 0; i < mPresetButtons.size(); ++i)
    {
        QPushButton* button = mPresetButtons.at(i);
        connect(button, &QPushButton::clicked, this, [this, i]() {
            applyColorIndex(i);
            updateActiveHighlight();
        });
    }

    connect(mUi.customButton, &QPushButton::clicked, this, [this]() {
        applyColorIndex(5);
        updateActiveHighlight();
    });

    mUi.widthSlider->setRange(1, 90);
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
    connect(mUi.widthSlider, &QSlider::valueChanged, this, [this](int){ updateStrokePreview(); });
    connect(mUi.brightnessSlider, &QSlider::valueChanged, this, &UBColorPickerPalette::updateCustomColor);

    connect(UBDrawingController::drawingController(), &UBDrawingController::colorPaletteChanged,
            this, &UBColorPickerPalette::refreshPalette);
    connect(UBDrawingController::drawingController(), &UBDrawingController::colorIndexChanged,
            this, [this](int idx){ Q_UNUSED(idx); updateActiveHighlight(); });

    if (UBApplication::boardController)
        connect(UBApplication::boardController, &UBBoardController::penColorChanged,
                this, &UBColorPickerPalette::refreshPalette);

    refreshPalette();
    updateStrokePreview();
    updateActiveHighlight();

    // Preview bubble initialization (top-level window to allow going outside palette bounds)
    mPreview = new PreviewBubble(nullptr);
    mPreview->hide();

    // Live hover updates over gradient square (throttled for performance)
    mHoverClock.start();
    mLastHoverTick = -1000;
    connect(mUi.colorSquare, &UBColorSquare::colorHovered, this, [this](const QColor& c, const QPoint& globalPos){
        qint64 now = mHoverClock.elapsed();
        if (now - mLastHoverTick < 16) return; // ~60 FPS cap
        mLastHoverTick = now;
        if (!mPreview)
            return;
        QColor previewColor = c;
        // Apply current brightness to hovered color for accurate preview
        previewColor.setHsv(previewColor.hue(), previewColor.saturation(), mUi.brightnessSlider->value());
        mPreview->setColor(previewColor);
        const int d = 128;
        const int margin = 12;
        // Prefer showing strictly above the finger
        int gx = globalPos.x() - d/2;
        int gy = globalPos.y() - d - margin;
        // Clamp to screen to keep bubble visible on screen, but allow going outside the palette
        QScreen* scr = QGuiApplication::screenAt(globalPos);
        QRect bounds = scr ? scr->geometry() : QGuiApplication::primaryScreen()->geometry();
        gx = qMax(bounds.left() + 2, qMin(gx, bounds.right() - d - 2));
        gy = qMax(bounds.top() + 2, qMin(gy, bounds.bottom() - d - 2));
        if (!mPreview->isVisible())
        {
            mPreview->resize(d, d);
            mPreview->move(gx, gy);
            mPreview->show();
        }
        else
        {
            mPreview->move(gx, gy);
        }
        mPreview->raise();
    });
    connect(mUi.colorSquare, &UBColorSquare::hoverEnded, this, [this](){ if (mPreview) mPreview->hide(); });
}

void UBColorPickerPalette::onSquareColorSelected(const QColor& color)
{
    mHue = color.hue();
    mSaturation = color.saturation();
    // Avoid heavy updates during drag; apply on mouse release or click
    if (!(QGuiApplication::mouseButtons() & Qt::LeftButton))
    {
        updateCustomColor();
        updateStrokePreview();
    }
}

void UBColorPickerPalette::updateCustomColor()
{
    if (!UBApplication::boardController)
        return;
    QColor c;
    c.setHsv(mHue, mSaturation, mUi.brightnessSlider->value());
    mUi.customButton->setStyleSheet(buttonStyleFor(c, true));

    mUpdatingPalette = true;
    UBDrawingController::drawingController()->setPenColor(true, c, 5);
    UBDrawingController::drawingController()->setPenColor(false, c, 5);
    UBDrawingController::drawingController()->setMarkerColor(true, c, 5);
    UBDrawingController::drawingController()->setMarkerColor(false, c, 5);
    applyColorIndex(5);
    mUpdatingPalette = false;
    updateActiveHighlight();
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
    // Stop capturing global events
    qApp->removeEventFilter(this);
    if (mPreview) mPreview->hide();
    emit closed();
}

void UBColorPickerPalette::showEvent(QShowEvent* event)
{
    UBFloatingPalette::showEvent(event);
    // Capture global clicks to auto-close when clicking outside or starting to draw
    qApp->installEventFilter(this);
    refreshPalette();
    mUi.widthSlider->setValue(static_cast<int>(UBDrawingController::drawingController()->currentToolWidth()));
    if (mPreview) mPreview->hide();
    updateActiveHighlight();
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
        mPresetButtons.at(i)->setStyleSheet(buttonStyleFor(c, false));
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
        mUi.customButton->setStyleSheet(buttonStyleFor(c, false));
        UBDrawingController::drawingController()->setMarkerColor(true, darkColors.at(5), 5);
        UBDrawingController::drawingController()->setMarkerColor(false, lightColors.at(5), 5);
    }
    else
    {
        {
            QSignalBlocker blocker(mUi.brightnessSlider);
            mUi.brightnessSlider->setValue(255);
        }
        mUi.customButton->setStyleSheet(buttonStyleFor(Qt::white, false));
    }

    mUpdatingPalette = false;
    updateStrokePreview();
    updateActiveHighlight();

}

void UBColorPickerPalette::updateStrokePreview()
{
    if (!mUi.strokePreview)
        return;
    // Just a number, no px suffix — максимально компактно
    int penW = mUi.widthSlider->value();
    mUi.strokePreview->setPixmap(QPixmap());
    mUi.strokePreview->setText(QString::number(penW));
}

QString UBColorPickerPalette::buttonStyleFor(const QColor& c, bool active) const
{
    QString base = QString("background:%1; border-radius:12px;").arg(c.name());
    if (active) {
        QColor rgb = c.toRgb();
        int luma = (299*rgb.red() + 587*rgb.green() + 114*rgb.blue()) / 1000; // perceived brightness
        QString border = (luma > 180) ? "#000000" : "#FFFFFF";
        return base + QString(" border:4px solid %1;").arg(border);
    } else {
        return base + " border:2px solid rgba(255,255,255,64);";
    }
}

void UBColorPickerPalette::updateActiveHighlight()
{
    if (mUpdatingPalette)
        return;

    int idx = UBDrawingController::drawingController()->currentToolColorIndex();
    if (idx < 0) idx = mActiveIndex; // keep previous if tool is not drawing
    mActiveIndex = idx;

    bool dark = UBSettings::settings()->isDarkBackground();
    QList<QColor> lightColors = UBSettings::settings()->penColors(false);
    QList<QColor> darkColors = UBSettings::settings()->penColors(true);

    for (int i = 0; i < mPresetButtons.size() && i < lightColors.size() && i < darkColors.size(); ++i)
    {
        QColor c = dark ? darkColors.at(i) : lightColors.at(i);
        bool active = (i == mActiveIndex);
        mPresetButtons.at(i)->setStyleSheet(buttonStyleFor(c, active));
    }

    // Custom button highlight (index 5)
    if (lightColors.size() > 5 && darkColors.size() > 5)
    {
        QColor c = dark ? darkColors.at(5) : lightColors.at(5);
        bool active = (mActiveIndex == 5);
        mUi.customButton->setStyleSheet(buttonStyleFor(c, active));
    }
}

// PreviewBubble implementation
UBColorPickerPalette::PreviewBubble::PreviewBubble(QWidget* parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_TransparentForMouseEvents, true);
    setAttribute(Qt::WA_NoSystemBackground, true);
    setAttribute(Qt::WA_ShowWithoutActivating, true);
    setWindowFlags(Qt::ToolTip | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground, true);
}

void UBColorPickerPalette::PreviewBubble::setColor(const QColor& c)
{
    if (mColor != c)
    {
        mColor = c;
        update();
    }
}

void UBColorPickerPalette::PreviewBubble::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    QRectF r = rect().adjusted(2,2,-2,-2);
    // Shadow
    QPainterPath shadowPath; shadowPath.addEllipse(r.adjusted(2,2,2,2));
    p.fillPath(shadowPath, QColor(0,0,0,40));
    // Fill
    p.setBrush(mColor);
    p.setPen(Qt::NoPen);
    p.drawEllipse(r);
    // Soft highlight ring
    QPen ring(QColor(255,255,255,180)); ring.setWidth(3);
    p.setPen(ring);
    p.setBrush(Qt::NoBrush);
    p.drawEllipse(r);
}

bool UBColorPickerPalette::eventFilter(QObject* obj, QEvent* event)
{
    Q_UNUSED(obj);
    if (!isVisible())
        return false;

    switch (event->type())
    {
        case QEvent::MouseButtonPress:
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
        {
            auto* me = static_cast<QMouseEvent*>(event);
            QPoint gp = me->globalPosition().toPoint();
            QPoint lp = mapFromGlobal(gp);
            // If click is outside palette, check if it's on the Color Picker button.
            if (!rect().contains(lp))
            {
                bool onColorButton = false;
                if (UBApplication::mainWindow && UBApplication::mainWindow->actionColorPicker)
                {
                    const auto objs = UBApplication::mainWindow->actionColorPicker->associatedObjects();
                    for (QObject* obj : objs)
                    {
                        if (auto ab = qobject_cast<QAbstractButton*>(obj))
                        {
                            QPoint btnTopLeft = ab->mapToGlobal(QPoint(0, 0));
                            QRect btnRect(btnTopLeft, ab->size());
                            if (btnRect.contains(gp)) { onColorButton = true; break; }
                        }
                    }
                }
                if (!onColorButton)
                    hide();
            }
            break;
        }
#else
        {
            auto* me = static_cast<QMouseEvent*>(event);
            QPoint gp = me->globalPos();
            QPoint lp = mapFromGlobal(gp);
            // If click is outside palette, check if it's on the Color Picker button.
            if (!rect().contains(lp))
            {
                bool onColorButton = false;
                if (UBApplication::mainWindow && UBApplication::mainWindow->actionColorPicker)
                {
                    const auto wgs = UBApplication::mainWindow->actionColorPicker->associatedWidgets();
                    for (QWidget* w : wgs)
                    {
                        if (auto ab = qobject_cast<QAbstractButton*>(w))
                        {
                            QPoint btnTopLeft = ab->mapToGlobal(QPoint(0, 0));
                            QRect btnRect(btnTopLeft, ab->size());
                            if (btnRect.contains(gp)) { onColorButton = true; break; }
                        }
                    }
                }
                if (!onColorButton)
                    hide();
            }
            break;
        }
#endif
        case QEvent::MouseButtonRelease:
            // Apply final color selection after dragging
            updateCustomColor();
            updateStrokePreview();
            break;
        case QEvent::TouchBegin:
        case QEvent::TouchUpdate:
        case QEvent::TouchEnd:
        {
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
            auto* te = static_cast<QTouchEvent*>(event);
            bool anyInside = false;
            for (const auto& p : te->points())
            {
                QPoint gp = p.globalPosition().toPoint();
                QPoint lp = mapFromGlobal(gp);
                if (rect().contains(lp)) { anyInside = true; break; }
            }
            if (!anyInside)
            {
                // Do not auto-close if the touch is on the Color Picker button itself.
                bool onColorButton = false;
                if (UBApplication::mainWindow && UBApplication::mainWindow->actionColorPicker)
                {
                    const auto objs = UBApplication::mainWindow->actionColorPicker->associatedObjects();
                    for (QObject* obj : objs)
                    {
                        if (auto ab = qobject_cast<QAbstractButton*>(obj))
                        {
                            QPoint btnTopLeft = ab->mapToGlobal(QPoint(0, 0));
                            QRect btnRect(btnTopLeft, ab->size());
                            for (const auto& p : te->points())
                            {
                                QPoint gp2 = p.globalPosition().toPoint();
                                if (btnRect.contains(gp2)) { onColorButton = true; break; }
                            }
                            if (onColorButton) break;
                        }
                    }
                }
                if (!onColorButton)
                    hide();
            }
#else
            auto* te = static_cast<QTouchEvent*>(event);
            bool anyInside = false;
            const auto points = te->touchPoints();
            for (const auto& p : points)
            {
                QPoint gp = p.screenPos().toPoint();
                QPoint lp = mapFromGlobal(gp);
                if (rect().contains(lp)) { anyInside = true; break; }
            }
            if (!anyInside)
            {
                // Do not auto-close if the touch is on the Color Picker button itself.
                bool onColorButton = false;
                if (UBApplication::mainWindow && UBApplication::mainWindow->actionColorPicker)
                {
                    const auto wgs = UBApplication::mainWindow->actionColorPicker->associatedWidgets();
                    for (QWidget* w : wgs)
                    {
                        if (auto ab = qobject_cast<QAbstractButton*>(w))
                        {
                            QPoint btnTopLeft = ab->mapToGlobal(QPoint(0, 0));
                            QRect btnRect(btnTopLeft, ab->size());
                            for (const auto& p : points)
                            {
                                QPoint gp2 = p.screenPos().toPoint();
                                if (btnRect.contains(gp2)) { onColorButton = true; break; }
                            }
                            if (onColorButton) break;
                        }
                    }
                }
                if (!onColorButton)
                    hide();
            }
#endif
            break;
        }
        default:
            break;
    }
    return false;
}
