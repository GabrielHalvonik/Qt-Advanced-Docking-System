/*******************************************************************************
** Qt Advanced Docking System
** Copyright (C) 2017 Uwe Kindler
** 
** This library is free software; you can redistribute it and/or
** modify it under the terms of the GNU Lesser General Public
** License as published by the Free Software Foundation; either
** version 2.1 of the License, or (at your option) any later version.
** 
** This library is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
** Lesser General Public License for more details.
** 
** You should have received a copy of the GNU Lesser General Public
** License along with this library; If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/


//============================================================================
//                                   INCLUDES
//============================================================================
#include "DockOverlay.h"

#include <QPointer>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QMoveEvent>
#include <QPainter>
#include <QGridLayout>
#include <QCursor>
#include <QIcon>
#include <QLabel>
#include <QtGlobal>
#include <QDebug>
#include <QMap>
#include <QWindow>
#include <QApplication>

#include "DockAreaWidget.h"
#include "DockAreaTitleBar.h"
#include "DockContainerWidget.h"
#include "AutoHideSideBar.h"
#include "DockManager.h"
#include "DockAreaTabBar.h"

namespace ads
{
static const int AutoHideAreaWidth = 32;
static const int AutoHideAreaMouseZone = 8;
static const int InvalidTabIndex = -2;

/**
 * Private data class of CDockOverlay
 */
struct DockOverlayPrivate
{
	CDockOverlay* _this;

	DockWidgetAreas AllowedAreas = InvalidDockWidgetArea;
	CDockOverlayCross* Cross;
	QPointer<QWidget> TargetWidget;
	DockWidgetArea LastLocation = InvalidDockWidgetArea;
	bool DropPreviewEnabled = true;
	CDockOverlay::eMode Mode = CDockOverlay::ModeDockAreaOverlay;
	int TabIndex = InvalidTabIndex;

    struct {
        DockWidgetArea Area = DockWidgetArea::NoDockWidgetArea;
        QWidget* DockAreaWidget = nullptr;
        QFrame* DockTargetFrame = nullptr;
    } LastlyHoveredDropArea;

	/**
	 * Private data constructor
	 */
	DockOverlayPrivate(CDockOverlay* _public) : _this(_public) {}

	/**
	 * Returns the overlay width / height depending on the visibility
	 * of the sidebar
	 */
	int sideBarOverlaySize(SideBarLocation sideBarLocation);

	/**
	 * The area where the mouse is considered in the sidebar
	 */
	int sideBarMouseZone(SideBarLocation sideBarLocation);
};

/**
 * Private data of CDockOverlayCross class
 */
struct DockOverlayCrossPrivate
{
	CDockOverlayCross* _this;
	CDockOverlay::eMode Mode = CDockOverlay::ModeDockAreaOverlay;
	CDockOverlay* DockOverlay;
    QHash<DockWidgetArea, QWidget*> DropIndicatorWidgets;
    QGridLayout* GridLayout;
	QColor IconColors[5];
	bool UpdateRequired = false;
	double LastDevicePixelRatio = 0.1;

	/**
	 * Private data constructor
	 */
	DockOverlayCrossPrivate(CDockOverlayCross* _public) : _this(_public) {}

	/**
	 *
	 * @param area
	 * @return
	 */
    QRect areaGridPosition(const DockWidgetArea area);

    /**
     * Helper function that returns the drop indicator width depending on the
     * operating system
     */
    qreal dropIndicatiorWidth(QLabel* l) const
    {
    #if defined(Q_OS_UNIX) && !defined(Q_OS_MACOS)
        Q_UNUSED(l)
        return 40;
    #else
        return static_cast<qreal>(l->fontMetrics().height()) * 3.f;
    #endif
    }

	//============================================================================
    QWidget* createDropIndicatorWidget(DockWidgetArea DockWidgetArea,
        CDockOverlayCross* parent)
	{
        auto frame = new QWidget(parent);
        frame->setAttribute(Qt::WA_TransparentForMouseEvents);
        // frame->setStyleSheet("QFrame { background-color: blue; }");
        frame->setMouseTracking(false);
        return frame;
	}

};


//============================================================================
int DockOverlayPrivate::sideBarOverlaySize(SideBarLocation sideBarLocation)
{
	auto Container = qobject_cast<CDockContainerWidget*>(TargetWidget.data());
	auto SideBar = Container->autoHideSideBar(sideBarLocation);
	if (!SideBar || !SideBar->isVisibleTo(Container))
	{
		return AutoHideAreaWidth;
	}
	else
	{
		return (SideBar->orientation() == Qt::Horizontal) ? SideBar->height() : SideBar->width();
	}
}


//============================================================================
int DockOverlayPrivate::sideBarMouseZone(SideBarLocation sideBarLocation)
{
	auto Container = qobject_cast<CDockContainerWidget*>(TargetWidget.data());
	auto SideBar = Container->autoHideSideBar(sideBarLocation);
	if (!SideBar || !SideBar->isVisibleTo(Container))
	{
		return AutoHideAreaMouseZone;
	}
	else
	{
		return (SideBar->orientation() == Qt::Horizontal) ? SideBar->height() : SideBar->width();
	}
}


//============================================================================
CDockOverlay::CDockOverlay(QWidget* parent, eMode Mode) :
	QFrame(parent),
	d(new DockOverlayPrivate(this))
{
	d->Mode = Mode;
    auto layout = new QHBoxLayout();
    d->Cross = new CDockOverlayCross(this);
    layout->addWidget(d->Cross);
    setLayout(layout);
#if defined(Q_OS_UNIX) && !defined(Q_OS_MACOS)
	setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::X11BypassWindowManagerHint);
#else
	setWindowFlags(Qt::Tool | Qt::FramelessWindowHint);
#endif
    setWindowOpacity(1);
	setWindowTitle("DockOverlay");
    setAttribute(Qt::WA_NoSystemBackground);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_TransparentForMouseEvents);

	d->Cross->setVisible(false);
	setVisible(false);

    setMouseTracking(false);
}


//============================================================================
CDockOverlay::~CDockOverlay()
{
	delete d;
}

QWidget *CDockOverlay::targetDropWidget() const
{
    return d->TargetWidget;
}


//============================================================================
void CDockOverlay::setAllowedAreas(DockWidgetAreas areas)
{
	if (areas == d->AllowedAreas)
	{
        return;
	}
	d->AllowedAreas = areas;
}


//============================================================================
void CDockOverlay::setAllowedArea(DockWidgetArea area, bool Enable)
{
	auto AreasOld = d->AllowedAreas;
	d->AllowedAreas.setFlag(area, Enable);
	if (AreasOld != d->AllowedAreas)
	{
		d->Cross->reset();
	}
}


//============================================================================
DockWidgetAreas CDockOverlay::allowedAreas() const
{
    DockWidgetAreas result = DockWidgetArea::InvalidDockWidgetArea;
    
    // if (d->TargetWidget.data())
    // qInfo() << "🔑area : " << d->TargetWidget.data()->metaObject()->className();

    if (qobject_cast<CDockManager*>(d->TargetWidget.data()))
    {
        result = DockWidgetArea::NoDockWidgetArea;
    }
    else if (auto area = qobject_cast<CDockAreaWidget*>(d->TargetWidget.data()); area)
    {
        if (area->isCentralWidgetArea())
        {
            result = DockWidgetArea::OuterDockAreas;
        }
        else if (auto container = area->dockContainer(); container && container->isFloating())
        {
            result = DockWidgetArea::CenterDockWidgetArea;
        }
    }
    if (result == DockWidgetArea::InvalidDockWidgetArea)
    {
        result = DockWidgetArea::AllDockAreas;
    }
    
    d->AllowedAreas = result;
    
	return d->AllowedAreas;
}


//============================================================================
DockWidgetArea CDockOverlay::dropAreaUnderCursor() const
{
	d->TabIndex = InvalidTabIndex;
	if (!d->TargetWidget)
	{
		return InvalidDockWidgetArea;
	}
    
    DockWidgetArea Result = d->Cross->cursorLocation();

    if (Result != InvalidDockWidgetArea)
    {
		return Result;
	}
    
	auto CursorPos = QCursor::pos();
	auto DockArea = qobject_cast<CDockAreaWidget*>(d->TargetWidget.data());
    
	if (!DockArea && CDockManager::autoHideConfigFlags().testFlag(CDockManager::AutoHideFeatureEnabled))
	{
		auto Rect = rect();
		const QPoint pos = mapFromGlobal(QCursor::pos());
		if ((pos.x() < d->sideBarMouseZone(SideBarLeft))
		  && d->AllowedAreas.testFlag(LeftAutoHideArea))
		{
			Result = LeftAutoHideArea;
		}
		else if (pos.x() > (Rect.width() - d->sideBarMouseZone(SideBarRight))
			  && d->AllowedAreas.testFlag(RightAutoHideArea))
		{
			Result = RightAutoHideArea;
		}
		else if (pos.y() < d->sideBarMouseZone(SideBarTop)
			&& d->AllowedAreas.testFlag(TopAutoHideArea))
		{
			Result = TopAutoHideArea;
		}
		else if (pos.y() > (Rect.height() - d->sideBarMouseZone(SideBarBottom))
			&& d->AllowedAreas.testFlag(BottomAutoHideArea))
		{
			Result = BottomAutoHideArea;
		}

		auto SideBarLocation = ads::internal::toSideBarLocation(Result);
		if (SideBarLocation != SideBarNone)
		{
			auto Container = qobject_cast<CDockContainerWidget*>(d->TargetWidget.data());
			auto SideBar = Container->autoHideSideBar(SideBarLocation);
			if (SideBar->isVisible())
			{
				d->TabIndex = SideBar->tabInsertIndexAt(SideBar->mapFromGlobal(CursorPos));
			}
		}
		return Result;
	}
	else if (!DockArea)
	{
		return Result;
	}

	if (DockArea->allowedAreas().testFlag(CenterDockWidgetArea)
	 && !DockArea->titleBar()->isHidden()
	 && DockArea->titleBarGeometry().contains(DockArea->mapFromGlobal(CursorPos)))
	{
		auto TabBar = DockArea->titleBar()->tabBar();
		d->TabIndex = TabBar->tabInsertIndexAt(TabBar->mapFromGlobal(CursorPos));
		return DockWidgetArea::CenterDockWidgetArea;
	}
    
	return Result;
}


//============================================================================
int CDockOverlay::tabIndexUnderCursor() const
{
	return d->TabIndex;
}


//============================================================================
DockWidgetArea CDockOverlay::visibleDropAreaUnderCursor() const
{
	if (isHidden() || !d->DropPreviewEnabled)
	{
		return InvalidDockWidgetArea;
	}
	else
	{
		return dropAreaUnderCursor();
	}
}


//============================================================================
void CDockOverlay::clearDockDropStrip()
{
    d->LastlyHoveredDropArea.Area = DockWidgetArea::NoDockWidgetArea;
    d->LastlyHoveredDropArea.DockAreaWidget = nullptr;
    if (d->LastlyHoveredDropArea.DockTargetFrame)
    {
        d->LastlyHoveredDropArea.DockTargetFrame->close();
        d->LastlyHoveredDropArea.DockTargetFrame = nullptr;
    }
    d->TargetWidget.clear();
    d->Cross->hide();
    d->Cross->show();
}

//============================================================================
DockWidgetArea CDockOverlay::showOverlay(QWidget* target)
{
    d->TargetWidget = target;

    if (d->LastlyHoveredDropArea.DockAreaWidget && !allowedAreas().testFlag(d->LastlyHoveredDropArea.Area))
    {
        clearDockDropStrip();
        return DockWidgetArea::NoDockWidgetArea;
    }

    const QPoint pos = (QCursor::pos());

    if (d->LastlyHoveredDropArea.DockAreaWidget) {
        auto position = d->LastlyHoveredDropArea.DockAreaWidget->mapToGlobal(QPoint());
        auto geometry = QRect(position.x(), position.y(), d->LastlyHoveredDropArea.DockAreaWidget->width(), d->LastlyHoveredDropArea.DockAreaWidget->height());
        if (geometry.contains(pos))
        {
            return d->LastlyHoveredDropArea.Area;
        }
        else if (d->LastlyHoveredDropArea.Area != DockWidgetArea::NoDockWidgetArea)
        {
            clearDockDropStrip();
        }
    }

    resize(target->size());
    move(target->mapToGlobal(target->rect().topLeft()));

    d->Cross->resize(target->size());
    d->Cross->show();
    d->Cross->updatePosition();

    auto area = d->Cross->cursorLocation();
    auto item = d->Cross->d->DropIndicatorWidgets.value(area);

    auto calculateOverlayBounds = [this](const QRect& rect, DockWidgetArea area) -> std::optional<QRect> {
        if (!allowedAreas().testFlag(area)) return { };
        auto size = ads::internal::DockingAreaStripDetectSize;

        if (area == DockWidgetArea::TopDockWidgetArea)
        {
            return QRect(rect.x() - size, rect.y(), rect.width() + size * 2, rect.height() * 2);
        }
        if (area == DockWidgetArea::BottomDockWidgetArea)
        {
            return QRect(rect.x() - size, rect.y() - size, rect.width() + size * 2, rect.height() + size);
        }
        if (area == DockWidgetArea::LeftDockWidgetArea)
        {
            return QRect(rect.x(), rect.y() - size, rect.width() + size, rect.height() + size * 2);
        }
        if (area == DockWidgetArea::RightDockWidgetArea)
        {
            return QRect(rect.x() - size, rect.y() - size, rect.width() + size, rect.height() + size * 2);
        }
        if (area == DockWidgetArea::CenterDockWidgetArea)
        {
            return QRect(rect.x() - size, rect.y() - size, rect.width() + size * 2, /*rect.height() + size*/ d->TargetWidget.data()->height());
        }
        return { };
    };

    if (item) {
        d->LastlyHoveredDropArea.Area = area;
        d->LastlyHoveredDropArea.DockAreaWidget = item;
        d->LastlyHoveredDropArea.DockTargetFrame = new QFrame(d->Cross);

        QString stops = (area == TopDockWidgetArea) ? "x1:0, y1:0, x2:0, y2:1" :
                        (area == BottomDockWidgetArea) ? "x1:0, y1:1, x2:0, y2:0" :
                        (area == LeftDockWidgetArea) ? "x1:0, y1:0, x2:1, y2:0" :
                        (area == RightDockWidgetArea) ? "x1:1, y1:0, x2:0, y2:0" :
                                                        "x1:1, y1:1, x2:1, y2:1";

        if (auto bounds = calculateOverlayBounds(item->geometry(), area); bounds.has_value())
        {
            d->LastlyHoveredDropArea.DockTargetFrame->setGeometry(bounds.value());
            // d->LastlyHoveredDropArea.DockTargetFrame->setGeometry(item->pos().x(), item->pos().y(), item->size().width(), item->size().height());
            // d->LastlyHoveredDropArea.DockTargetFrame->resize(item->size());
            // d->LastlyHoveredDropArea.DockTargetFrame->move(item->pos());
            if (area == CenterDockWidgetArea)
            {
                d->LastlyHoveredDropArea.DockTargetFrame->setStyleSheet(
                    QString(
                        "QFrame {"
                        "    border-top: %1px solid %3;"
                        "    border-right: %2px solid %3;"
                        "    border-bottom: %2px solid %3;"
                        "    border-left: %2px solid %3;"
                        "    background-color: transparent;"
                        "}"
                        ).arg(
                            QString::number(ads::internal::DefaultDockTitleBarHeight),
                            QString::number(int(ads::internal::DockingAreaStripDetectSize / 2)),
                            ads::internal::HighlightDropAreaColorSpreadMiddle
                        )
                    );
            }
            else
            {
                d->LastlyHoveredDropArea.DockTargetFrame->setStyleSheet(
                    QString(
                        "QFrame {"
                        "    background: qlineargradient("
                        "        spread:pad, "
                        "        %1, "
                        "        stop:0 %2, "
                        "        stop:1 %3 "
                        "    );"
                        "}"
                    ).arg(stops, ads::internal::HighlightDropAreaColorSpreadStart, ads::internal::HighlightDropAreaColorSpreadEnd)
                );
            }
            d->LastlyHoveredDropArea.DockTargetFrame->show();
        }
    } else {
        clearDockDropStrip();
    }

    return d->LastlyHoveredDropArea.Area;
}


//============================================================================
void CDockOverlay::hideOverlay()
{
    clearDockDropStrip();

	hide();
	d->TargetWidget.clear();
	d->LastLocation = InvalidDockWidgetArea;
}


//============================================================================
void CDockOverlay::enableDropPreview(bool Enable)
{
	d->DropPreviewEnabled = Enable;
	update();
}


//============================================================================
bool CDockOverlay::dropPreviewEnabled() const
{
	return d->DropPreviewEnabled;
}


//============================================================================
void CDockOverlay::paintEvent(QPaintEvent* event)
{
    QFrame::paintEvent(event);
    // Q_UNUSED(event);
 //    // Draw rect based on location
    // if (!d->DropPreviewEnabled)
 //    {
    // 	d->DropAreaRect = QRect();
    // 	return;
    // }
    
    // QRect r = rect();
    // const DockWidgetArea da = dropAreaUnderCursor();
    
    // double Factor = (CDockOverlay::ModeContainerOverlay == d->Mode) ?
    // 	3 : 2;

    // switch (da)
    // {
 //        case TopDockWidgetArea: r.setHeight(r.height() / Factor); break;
 //        case RightDockWidgetArea: r.setX(r.width() * (1 - 1 / Factor)); break;
 //        case BottomDockWidgetArea: r.setY(r.height() * (1 - 1 / Factor)); break;
 //        case LeftDockWidgetArea: r.setWidth(r.width() / Factor); break;
 //        case CenterDockWidgetArea: r = rect(); break;
 //        case LeftAutoHideArea: r.setWidth(d->sideBarOverlaySize(SideBarLeft)); break;
 //        case RightAutoHideArea: r.setX(r.width() - d->sideBarOverlaySize(SideBarRight)); break;
 //        case TopAutoHideArea: r.setHeight(d->sideBarOverlaySize(SideBarTop)); break;
 //        case BottomAutoHideArea: r.setY(r.height() - d->sideBarOverlaySize(SideBarBottom)); break;
 //        default: {
 //            d->DropAreaRect = QRect();
 //            return;
 //        }
 //    }

    // QPainter painter(this);
 //    QColor Color = palette().color(QPalette::Active, QPalette::Highlight);
 //    QPen Pen = painter.pen();
 //    Pen.setColor(Color.darker(120));
 //    Pen.setStyle(Qt::SolidLine);
 //    Pen.setWidth(1);
 //    Pen.setCosmetic(true);
 //    painter.setPen(Pen);
 //    Color = Color.lighter(130);
 //    Color.setAlpha(64);
 //    painter.setBrush(Color);
 //    painter.drawRect(r.adjusted(0, 0, -1, -1));
    // d->DropAreaRect = r;
}


//============================================================================
void CDockOverlay::showEvent(QShowEvent* e)
{
	d->Cross->show();
	QFrame::showEvent(e);
}


//============================================================================
void CDockOverlay::hideEvent(QHideEvent* e)
{
	d->Cross->hide();
	QFrame::hideEvent(e);
}


//============================================================================
bool CDockOverlay::event(QEvent *e)
{
	bool Result = Super::event(e);
    // if (e->type() == QEvent::Resize)
    // {
    //     qInfo() << "💦resize";
    //     // d->Cross->setupOverlayCross(d->Mode);
    // }
	return Result;
}


//============================================================================
static int areaAlignment(const DockWidgetArea area)
{
	switch (area)
	{
        case TopDockWidgetArea: return (int) Qt::AlignJustify | Qt::AlignTop;
        case RightDockWidgetArea: return (int) Qt::AlignRight | Qt::AlignJustify;
        case BottomDockWidgetArea: return (int) Qt::AlignJustify | Qt::AlignBottom;
        case LeftDockWidgetArea: return (int) Qt::AlignLeft | Qt::AlignJustify;
        case CenterDockWidgetArea:  return (int) Qt::AlignCenter;
        default: return Qt::AlignCenter;
	}
}

//============================================================================
// DockOverlayCrossPrivate
//============================================================================
QRect DockOverlayCrossPrivate::areaGridPosition(const DockWidgetArea area)
{
    switch (area)
    {
        // case TopDockWidgetArea: return QRect(1, 0, 1, 1);
        // case BottomDockWidgetArea: return QRect(1, 3, 1, 1);
        // case LeftDockWidgetArea: return QRect(0, 1, 1, 2);
        // case RightDockWidgetArea: return QRect(2, 1, 1, 2);
        // case CenterDockWidgetArea: return QRect(1, 1, 1, 1);
        // default: return QRect(1, 2, 1, 1);
        case TopDockWidgetArea: return QRect(0, 1, 1, 1);
        case BottomDockWidgetArea: return QRect(3, 1, 1, 1);
        case LeftDockWidgetArea: return QRect(1, 0, 2, 1);
        case RightDockWidgetArea: return QRect(1, 2, 2, 1);
        case CenterDockWidgetArea: return QRect(1, 1, 1, 1);
        default: return QRect(2, 1, 1, 1);
    }

    // if (CDockOverlay::ModeDockAreaOverlay == Mode)
    // {
    //     switch (area)
    //     {
    //         case TopDockWidgetArea: return QPoint(1, 2);
    //         case RightDockWidgetArea: return QPoint(2, 3);
    //         case BottomDockWidgetArea: return QPoint(3, 2);
    //         case LeftDockWidgetArea: return QPoint(2, 1);
    //         case CenterDockWidgetArea: return QPoint(2, 2);
    //         default: return QPoint();
    //     }
    // }
    // else
    // {
    //     switch (area)
    //     {
    //         case TopDockWidgetArea: return QPoint(0, 2);
    //         case RightDockWidgetArea: return QPoint(2, 4);
    //         case BottomDockWidgetArea: return QPoint(4, 2);
    //         case LeftDockWidgetArea: return QPoint(2, 0);
    //         case CenterDockWidgetArea: return QPoint(2, 2);
    //         default: return QPoint();
    //     }
    // }
}


//============================================================================
CDockOverlayCross::CDockOverlayCross(CDockOverlay* overlay) :
    QWidget(overlay),
	d(new DockOverlayCrossPrivate(this))
{
	d->DockOverlay = overlay;
#if defined(Q_OS_UNIX) && !defined(Q_OS_MACOS)
	setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::X11BypassWindowManagerHint);
#else
	setWindowFlags(Qt::Tool | Qt::FramelessWindowHint);
#endif
	setWindowTitle("DockOverlayCross");
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_TransparentForMouseEvents);

    d->GridLayout = new QGridLayout();
    d->GridLayout->setContentsMargins(0, 0, 0, 0);
    d->GridLayout->setSpacing(0);

    d->GridLayout->setColumnStretch(0, 0);
    d->GridLayout->setColumnStretch(1, 1);
    d->GridLayout->setColumnStretch(2, 0);
    d->GridLayout->setRowStretch(0, 0);
    d->GridLayout->setRowStretch(1, 0);
    d->GridLayout->setRowStretch(2, 1);
    d->GridLayout->setRowStretch(3, 0);
    d->GridLayout->setColumnMinimumWidth(0, ads::internal::DockingAreaStripDetectSize);
    d->GridLayout->setColumnMinimumWidth(2, ads::internal::DockingAreaStripDetectSize);
    d->GridLayout->setRowMinimumHeight(0, ads::internal::DockingAreaStripDetectSize);
    d->GridLayout->setRowMinimumHeight(1, ads::internal::DefaultDockTitleBarHeight - ads::internal::DockingAreaStripDetectSize);
    d->GridLayout->setRowMinimumHeight(3, ads::internal::DockingAreaStripDetectSize);

    createAreaWidgets();

    setLayout(d->GridLayout);

    setMouseTracking(false);
}


//============================================================================
CDockOverlayCross::~CDockOverlayCross()
{
	delete d;
}

void CDockOverlayCross::createAreaWidgets()
{
    QHash<DockWidgetArea, QWidget*> areaWidgets;

    areaWidgets.insert(TopDockWidgetArea, d->createDropIndicatorWidget(TopDockWidgetArea, this));
    areaWidgets.insert(RightDockWidgetArea, d->createDropIndicatorWidget(RightDockWidgetArea, this));
    areaWidgets.insert(BottomDockWidgetArea, d->createDropIndicatorWidget(BottomDockWidgetArea, this));
    areaWidgets.insert(LeftDockWidgetArea, d->createDropIndicatorWidget(LeftDockWidgetArea, this));
    areaWidgets.insert(CenterDockWidgetArea, d->createDropIndicatorWidget(CenterDockWidgetArea, this));

    setAreaWidgets(areaWidgets);
}


//============================================================================
void CDockOverlayCross::setupOverlayCross(CDockOverlay::eMode Mode)
{
	d->Mode = Mode;
    
#if QT_VERSION >= 0x050600
	d->LastDevicePixelRatio = devicePixelRatioF();
#else
    d->LastDevicePixelRatio = devicePixelRatio();
#endif
	d->UpdateRequired = false;
}


//============================================================================
void CDockOverlayCross::updateOverlayIcons()
{
    if (windowHandle() && windowHandle()->devicePixelRatio() == d->LastDevicePixelRatio)
	{
		return;
	}
    
	for (auto Widget : d->DropIndicatorWidgets)
	{
        // d->updateDropIndicatorIcon(Widget);
	}
#if QT_VERSION >= 0x050600
	d->LastDevicePixelRatio = devicePixelRatioF();
#else
    d->LastDevicePixelRatio = devicePixelRatio();
#endif
}


//============================================================================
void CDockOverlayCross::setIconColor(eIconColor ColorIndex, const QColor& Color)
{
	d->IconColors[ColorIndex] = Color;
	d->UpdateRequired = true;
}


//============================================================================
QColor CDockOverlayCross::iconColor(eIconColor ColorIndex) const
{
	return d->IconColors[ColorIndex];
}


//============================================================================
void CDockOverlayCross::setAreaWidgets(const QHash<DockWidgetArea, QWidget*>& widgets)
{
	// Delete old widgets.
    QMutableHashIterator<DockWidgetArea, QWidget*> i(d->DropIndicatorWidgets);
    
	while (i.hasNext())
	{
		i.next();
        QWidget* widget = i.value();
        d->GridLayout->removeWidget(widget);
		delete widget;
		i.remove();
	}
    
	// Insert new widgets into grid.
	d->DropIndicatorWidgets = widgets;
    QHashIterator<DockWidgetArea, QWidget*> i2(d->DropIndicatorWidgets);

	while (i2.hasNext())
	{
		i2.next();
		const DockWidgetArea area = i2.key();
		QWidget* widget = i2.value();
        auto p = d->areaGridPosition(area);
        d->GridLayout->addWidget(widget, p.x(), p.y(), p.width(), p.height());
	}
    // d->GridLayout->setRowStretch(2, 1);
    // d->GridLayout->setColumnStretch(2, 1);

    CDockAreaWidget* area = nullptr;
    if ((area = qobject_cast<CDockAreaWidget*>(d->DockOverlay->d->TargetWidget)) && area->isCentralWidgetArea())
    {
        // d->GridLayout->setContentsMargins(4, 4, 4, 4);
    }
    else
    {
        // d->GridLayout->setContentsMargins(4, TitleOffset, 4, 4);
    }

    // d->GridLayout->setRowStretch(0, 1);
    // d->GridLayout->setRowStretch(1, 1);
    // d->GridLayout->setRowStretch(2, 1);
    // d->GridLayout->setRowStretch(3, 1);
    // d->GridLayout->setRowStretch(4, 0);

    // d->GridLayout->setColumnStretch(0, 1);
    // d->GridLayout->setColumnStretch(1, 0);
    // d->GridLayout->setColumnStretch(2, 0);
    // d->GridLayout->setColumnStretch(3, 0);
    // d->GridLayout->setColumnStretch(4, 1);
    
    // if (CDockOverlay::ModeDockAreaOverlay == d->Mode)
    // {
    // 	d->GridLayout->setRowStretch(0, 1);
    // 	d->GridLayout->setRowStretch(1, 0);
    // 	d->GridLayout->setRowStretch(2, 0);
    // 	d->GridLayout->setRowStretch(3, 0);
    // 	d->GridLayout->setRowStretch(4, 1);

    // 	d->GridLayout->setColumnStretch(0, 1);
    // 	d->GridLayout->setColumnStretch(1, 0);
    // 	d->GridLayout->setColumnStretch(2, 0);
    // 	d->GridLayout->setColumnStretch(3, 0);
    // 	d->GridLayout->setColumnStretch(4, 1);
    // }
    // else
    // {
    // 	d->GridLayout->setRowStretch(0, 0);
    // 	d->GridLayout->setRowStretch(1, 1);
    // 	d->GridLayout->setRowStretch(2, 1);
    // 	d->GridLayout->setRowStretch(3, 1);
    // 	d->GridLayout->setRowStretch(4, 0);

    // 	d->GridLayout->setColumnStretch(0, 0);
    // 	d->GridLayout->setColumnStretch(1, 1);
    // 	d->GridLayout->setColumnStretch(2, 1);
    // 	d->GridLayout->setColumnStretch(3, 1);
    // 	d->GridLayout->setColumnStretch(4, 0);
    // }
    
    reset();
}

//============================================================================
DockWidgetArea CDockOverlayCross::cursorLocation() const
{
	const QPoint pos = mapFromGlobal(QCursor::pos());
    QHashIterator<DockWidgetArea, QWidget*> i(d->DropIndicatorWidgets);
	while (i.hasNext())
	{
		i.next();
		if (d->DockOverlay->allowedAreas().testFlag(i.key())
			&& i.value()
			&& i.value()->isVisible()
			&& i.value()->geometry().contains(pos))
		{
            if (d->DockOverlay->d->TargetWidget)
			return i.key();
		}
	}
	return InvalidDockWidgetArea;
}


//============================================================================
void CDockOverlayCross::showEvent(QShowEvent*)
{
    if (d->UpdateRequired)
	{
		setupOverlayCross(d->Mode);
	}
    // this->updatePosition();
}

//============================================================================
bool CDockOverlayCross::eventFilter(QObject *obj, QEvent *event) {
    if (event->type() == QEvent::MouseMove) {
        this->setCursor(Qt::ArrowCursor);
        return true; // Event handled
    }
    // Pass the event to the base class
    return QWidget::eventFilter(obj, event);
}

//============================================================================
void CDockOverlayCross::updatePosition()
{

#if QT_VERSION >= 0x050600
    d->LastDevicePixelRatio = devicePixelRatioF();
#else
    d->LastDevicePixelRatio = devicePixelRatio();
#endif
    resize(d->DockOverlay->size());
	QPoint TopLeft = d->DockOverlay->pos();
    QPoint Offest((this->width() - d->DockOverlay->width()) / 2,
        (this->height() - d->DockOverlay->height()) / 2);
    QPoint CrossTopLeft = TopLeft - Offest;
	move(CrossTopLeft);
}


//============================================================================
void CDockOverlayCross::reset()
{
    // QList<DockWidgetArea> allAreas;
    // allAreas << TopDockWidgetArea << RightDockWidgetArea
    //          << BottomDockWidgetArea << LeftDockWidgetArea << CenterDockWidgetArea;
    // const DockWidgetAreas allowedAreas = d->DockOverlay->allowedAreas();
    // // Update visibility of area widgets based on allowedAreas.
    // for (int i = 0; i < allAreas.count(); ++i)
    // {
    //     auto p = d->areaGridPosition(allAreas.at(i));
    //     QLayoutItem* item = d->GridLayout->itemAtPosition(p.x(), p.y());
    //     QWidget* w = nullptr;
    //     if (item && (w = item->widget()) != nullptr)
    //     {
    //         w->setVisible(allowedAreas.testFlag(allAreas.at(i)));
    //         w->update();
    //     }
    // }
}


//============================================================================
void CDockOverlayCross::setIconColors(const QString& Colors)
{
	static const QMap<QString, int> ColorCompenentStringMap{
		{"Frame", CDockOverlayCross::FrameColor},
		{"Background", CDockOverlayCross::WindowBackgroundColor},
		{"Overlay", CDockOverlayCross::OverlayColor},
		{"Arrow", CDockOverlayCross::ArrowColor},
		{"Shadow", CDockOverlayCross::ShadowColor}};

#if (QT_VERSION < QT_VERSION_CHECK(5, 14, 0))
    auto SkipEmptyParts = QString::SkipEmptyParts;
#else
    auto SkipEmptyParts = Qt::SkipEmptyParts;
#endif
    auto ColorList = Colors.split(' ', SkipEmptyParts);
	for (const auto& ColorListEntry : ColorList)
	{
        auto ComponentColor = ColorListEntry.split('=', SkipEmptyParts);
		int Component = ColorCompenentStringMap.value(ComponentColor[0], -1);
		if (Component < 0)
		{
			continue;
		}
		d->IconColors[Component] = QColor(ComponentColor[1]);
	}

	d->UpdateRequired = true;
}

//============================================================================
QString CDockOverlayCross::iconColors() const
{
	return QString();
}



} // namespace ads
//----------------------------------------------------------------------------

