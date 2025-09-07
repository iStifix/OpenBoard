/*
 * Copyright (C) 2015-2022 Département de l'Instruction Publique (DIP-SEM)
 *
 * Copyright (C) 2013 Open Education Foundation
 *
 * Copyright (C) 2010-2013 Groupement d'Intérêt Public pour
 * l'Education Numérique en Afrique (GIP ENA)
 *
 * This file is part of OpenBoard.
 *
 * OpenBoard is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License,
 * with a specific linking exception for the OpenSSL project's
 * "OpenSSL" library (or with modified versions of it that use the
 * same license as the "OpenSSL" library).
 *
 * OpenBoard is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenBoard. If not, see <http://www.gnu.org/licenses/>.
 */




#include "UBDesktopPalette.h"

#include <QtGui>

#include "frameworks/UBPlatformUtils.h"

#include "core/UBSettings.h"
#include "core/UBSetting.h"
#include "core/UBApplication.h"
#include "core/UBApplicationController.h"

#include "board/UBDrawingController.h"

#include "gui/UBMainWindow.h"

#include "core/memcheck.h"

UBDesktopPalette::UBDesktopPalette(QWidget *parent, UBRightPalette* _rightPalette)
    : UBActionPalette(Qt::TopLeftCorner, parent)
    , mShowHideAction(NULL)
    , mDisplaySelectAction(NULL)
    , rightPalette(_rightPalette)
{
    QList<QAction*> actions;

    mActionUniboard = new QAction(QIcon(":/images/toolbar/close.png"), tr("Show OpenBoard"), this);
    connect(mActionUniboard, SIGNAL(triggered()), this, SIGNAL(uniboardClick()));

    mActionCustomSelect = new QAction(QIcon(":/images/toolbar/captureArea.png"), tr("Capture Part of the Screen"), this);
    connect(mActionCustomSelect, SIGNAL(triggered()), this, SIGNAL(customClick()));

    mDisplaySelectAction = new QAction(QIcon(":/images/toolbar/captureScreen.png"), tr("Capture the Screen"), this);
    connect(mDisplaySelectAction, SIGNAL(triggered()), this, SIGNAL(screenClick()));

    QIcon showHideIcon;
    showHideIcon.addPixmap(QPixmap(":/images/toolbar/eyeOpened.png"), QIcon::Normal , QIcon::On);
    showHideIcon.addPixmap(QPixmap(":/images/toolbar/eyeClosed.png"), QIcon::Normal , QIcon::Off);
    mShowHideAction = new QAction(showHideIcon, "", this);
    mShowHideAction->setCheckable(true);
    connect(mShowHideAction, SIGNAL(triggered(bool)), this, SLOT(showHideClick(bool)));

    // Minimize button (uses same icon as maximize)
    QIcon tabIcon(QPixmap(":/images/toolbar/stylusTab.png"));
    mMinimizeAction = new QAction(tabIcon, tr("Hide the stylus palette"), this);
    connect(mMinimizeAction, SIGNAL(triggered()), this, SLOT(onMinimizeClicked()));

    // Desired order (top→bottom): minimize/expand, pen, marker, color palette, eraser, erase-all, area capture, screen capture, back to board
    actions << mMinimizeAction;
    // mark non-tool actions as ungrouped to avoid interference with stylus grouping
    mMinimizeAction->setProperty("ungrouped", true);
    actions << UBApplication::mainWindow->actionPen;
    actions << UBApplication::mainWindow->actionMarker;
    UBApplication::mainWindow->actionColorPicker->setProperty("ungrouped", true);
    actions << UBApplication::mainWindow->actionColorPicker;
    actions << UBApplication::mainWindow->actionEraser;
    UBApplication::mainWindow->actionEraseDesktopAnnotations->setProperty("ungrouped", true);
    actions << UBApplication::mainWindow->actionEraseDesktopAnnotations;
    mActionCustomSelect->setProperty("ungrouped", true);
    actions << mActionCustomSelect;
    mDisplaySelectAction->setProperty("ungrouped", true);
    actions << mDisplaySelectAction;
    mActionUniboard->setProperty("ungrouped", true);
    actions << mActionUniboard;

    setActions(actions);
    // Desktop palette icons: 80x80, icons only (no labels)
    setButtonIconSize(QSize(80, 80));
    setToolButtonStyle(Qt::ToolButtonIconOnly);
    setBackgroundBrush(QBrush(QColor(30, 30, 30, 180)));
    setStyleSheet(
        QStringLiteral(
            "QToolButton{width:80px;height:80px;border-radius:12px;margin:2px;}"
            "QToolButton:hover{background:rgba(255,255,255,30);}"
        )
    );
    if (layout()) {
        layout()->setContentsMargins(4, 4, 4, 4);
        layout()->setSpacing(4);
    }
    // Remove outer frame/border
    setGrip(false);

    adjustSizeAndPosition();
    // Set 'erase all' icon to match main toolbar style (clearPage)
    if (UBApplication::mainWindow && UBApplication::mainWindow->actionEraseDesktopAnnotations)
        UBApplication::mainWindow->actionEraseDesktopAnnotations->setIcon(QIcon(":/images/toolbar/clearPage.png"));

    //  This palette can be minimized
    QIcon maximizeIcon;
    maximizeIcon.addPixmap(QPixmap(":/images/toolbar/stylusTab.png"), QIcon::Normal, QIcon::On);
    mMaximizeAction = new QAction(maximizeIcon, tr("Show the stylus palette"), this);
    connect(mMaximizeAction, SIGNAL(triggered()), this, SLOT(maximizeMe()));
    connect(this, SIGNAL(maximizeStart()), this, SLOT(maximizeMe()));
    connect(this, SIGNAL(minimizeStart(eMinimizedLocation)), this, SLOT(minimizeMe(eMinimizedLocation)));
    // Disable auto-minimize/restore logic; panel is fixed and should not auto-collapse
    setMinimizePermission(false);

    connect(rightPalette, SIGNAL(resized()), this, SLOT(parentResized()));
}


UBDesktopPalette::~UBDesktopPalette()
{

}


// adjustPosition is defined later to always pin the palette to left-center

void UBDesktopPalette::disappearForCapture()
{
    setWindowOpacity(0.0);
    qApp->processEvents();
}


void UBDesktopPalette::appear()
{
    setWindowOpacity(1.0);
}


void UBDesktopPalette::showHideClick(bool checked)
{
    UBApplication::applicationController->mirroringEnabled(checked);
}


void UBDesktopPalette::updateShowHideState(bool pShowEnabled)
{
    mShowHideAction->setChecked(pShowEnabled);

    if (mShowHideAction->isChecked())
        mShowHideAction->setToolTip(tr("Show Board on Secondary Screen"));
    else
        mShowHideAction->setToolTip(tr("Show Desktop on Secondary Screen"));

    if (pShowEnabled)
        raise();
}


void UBDesktopPalette::setShowHideButtonVisible(bool visible)
{
    mShowHideAction->setVisible(visible);
}


void UBDesktopPalette::setDisplaySelectButtonVisible(bool visible)
{
    mDisplaySelectAction->setVisible(visible);
}

//  Called when the palette is near the border and must be minimized
void UBDesktopPalette::minimizeMe(eMinimizedLocation location)
{
    Q_UNUSED(location);
    QList<QAction*> actions;
    clearLayout();

    actions << mMaximizeAction;
    setActions(actions);

    adjustSizeAndPosition();

    // No mask refresh here; mask is managed by controller based on tool/background
}

//  Called when the user wants to maximize the palette
void UBDesktopPalette::maximizeMe()
{
    QList<QAction*> actions;
    clearLayout();

    // Top→bottom: minimize, pen, marker, color palette, eraser, erase-all, capture area, capture screen, back to board
    actions << mMinimizeAction;
    mMinimizeAction->setProperty("ungrouped", true);
    actions << UBApplication::mainWindow->actionPen;
    actions << UBApplication::mainWindow->actionMarker;
    UBApplication::mainWindow->actionColorPicker->setProperty("ungrouped", true);
    actions << UBApplication::mainWindow->actionColorPicker;
    actions << UBApplication::mainWindow->actionEraser;
    UBApplication::mainWindow->actionEraseDesktopAnnotations->setProperty("ungrouped", true);
    actions << UBApplication::mainWindow->actionEraseDesktopAnnotations;
    mActionCustomSelect->setProperty("ungrouped", true);
    actions << mActionCustomSelect;
    mDisplaySelectAction->setProperty("ungrouped", true);
    actions << mDisplaySelectAction;
    mActionUniboard->setProperty("ungrouped", true);
    actions << mActionUniboard;

    setActions(actions);

    adjustSizeAndPosition();
    if (UBApplication::mainWindow && UBApplication::mainWindow->actionEraseDesktopAnnotations)
        UBApplication::mainWindow->actionEraseDesktopAnnotations->setIcon(QIcon(":/images/toolbar/clearPage.png"));

    // Stick to the left center (avoid auto-minimize trigger at x==5)
    QPoint p;
    if (parentWidget())
    {
        p.setX(6);
        p.setY((parentWidget()->height() - height()) / 2);
        moveInsideParent(p);
    }

    // Notify that the maximization has been done
    emit maximized();
}

void UBDesktopPalette::onMinimizeClicked()
{
    // Minimize immediately for snappier UX
    performMinimize();
}

void UBDesktopPalette::performMinimize()
{
    minimizeMe(eMinimizedLocation_Left);
}

void UBDesktopPalette::showEvent(QShowEvent *event)
{
    Q_UNUSED(event);
    // Use standard icons (no arrows) as on main toolbar
    adjustPosition();
    // Ensure pinned position
    if (parentWidget())
        moveInsideParent(QPoint(6, (parentWidget()->height() - height()) / 2));
    // No mask refresh here to avoid interfering with drawing mask
    if (UBApplication::mainWindow && UBApplication::mainWindow->actionEraseDesktopAnnotations)
        UBApplication::mainWindow->actionEraseDesktopAnnotations->setIcon(QIcon(":/images/toolbar/clearPage.png"));
}

void UBDesktopPalette::hideEvent(QHideEvent *event)
{
    Q_UNUSED(event);
    // Keep standard icons; nothing to do here
}

QPoint UBDesktopPalette::buttonPos(QAction *action)
{
    QPoint p;

    UBActionPaletteButton* pB = mMapActionToButton[action];
    if(NULL != pB)
    {
        p = pB->pos();
    }

    return p;
}

void UBDesktopPalette::adjustPosition()
{
    // Pin to left center regardless of previous position
    if (!parentWidget()) return;
    moveInsideParent(QPoint(6, (parentWidget()->height() - height()) / 2));
}


int UBDesktopPalette::getParentRightOffset()
{
    return rightPalette->width();
}

void UBDesktopPalette::parentResized()
{
    adjustPosition();
}
