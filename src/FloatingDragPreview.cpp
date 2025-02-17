//============================================================================
/// \file   FloatingDragPreview.cpp
/// \author Uwe Kindler
/// \date   26.11.2019
/// \brief  Implementation of CFloatingDragPreview
//============================================================================

//============================================================================
//                                   INCLUDES
//============================================================================
#include "FloatingDragPreview.h"

#include <QEvent>
#include <QTimer>
#include <QScreen>
#include <QPainter>
#include <QToolBar>
#include <QKeyEvent>
#include <QApplication>

#include "ads_globals.h"
#include "DockWidget.h"
#include "DockOverlay.h"
#include "DockManager.h"
#include "DockWidgetTab.h"
#include "FloatingHelper.h"
#include "DockAreaWidget.h"
#include "DockAreaTabBar.h"
#include "DockAreaTitleBar.h"
#include "DockContainerWidget.h"
#include "DockSnappingManager.h"
#include "AutoHideDockContainer.h"

namespace ads
{

/**
 * Private data class (pimpl)
 */
struct FloatingDragPreviewPrivate
{
	CFloatingDragPreview *_this;
	QWidget *Content;
	CDockWidget::DockWidgetFeatures ContentFeatures;
	CDockAreaWidget *ContentSourceArea = nullptr;
	QPoint DragStartMousePosition;
	CDockManager* DockManager;
	CDockContainerWidget *DropContainer = nullptr;
	qreal WindowOpacity;
	bool Hidden = false;
	QPixmap ContentPreviewPixmap;
	bool Canceled = false;
    int DraggedTabBarIndex = 0;

	/**
	 * Private data constructor
	 */
	FloatingDragPreviewPrivate(CFloatingDragPreview *_public);
	void updateDropOverlays(const QPoint &GlobalPos);

	void setHidden(bool Value)
	{
		Hidden = Value;
		_this->update();
	}

	/**
	 * Cancel dragging and emit the draggingCanceled event
	 */
	void cancelDragging()
    {
        if (!Content) return;
        
        if (auto widget = qobject_cast<CDockWidget*>(Content); widget)
        {
            if (auto area = widget->dockAreaWidget(); area)
            {
                if (auto tab = area->titleBar()->tabBar(); tab)
                {
                    if (tab->isHidden())
                    {
                        tab->show();
                    }
                    tab->currentTab()->show();
                    area->setCurrentIndex(DraggedTabBarIndex);
                }
                
                if (auto bar = area->currentDockWidget()->titleBar(); bar)
                {
                    bar->setStyleSheet(QString("QWidget { background-color: %0; }").arg(internal::TitleBarHighlightedColor));
                        
                    QTimer::singleShot(350, [bar] {
                        bar->setStyleSheet(QString("QWidget { background-color: %0; }").arg(internal::TitleBarColor));
                    });
                }
            }
        }
        
        if (ContentSourceArea)
        {
            if (ContentSourceArea->currentDockWidget())
            {
                if (auto bar = ContentSourceArea->currentDockWidget()->titleBar(); bar)
                {
                    bar->setStyleSheet(QString("QWidget { background-color: %0; }").arg(internal::TitleBarHighlightedColor));
                    
                    QTimer::singleShot(350, [bar] {
                        bar->setStyleSheet(QString("QWidget { background-color: %0; }").arg(internal::TitleBarColor));
                    });
                }
            }
        }
        
		Canceled = true;
		Q_EMIT _this->draggingCanceled();
		DockManager->containerOverlay()->hideOverlay();
        _this->hide();
		DockManager->dockAreaOverlay()->hideOverlay();
        if (ContentSourceArea) ContentSourceArea->show();
	}

	/**
	 * Creates the real floating widget in case the mouse is released outside
	 * outside of any drop area
	 */
	void createFloatingWidget();

	/**
	 * Returns true, if the content is floatable
	 */
	bool isContentFloatable() const
	{
		return this->ContentFeatures.testFlag(CDockWidget::DockWidgetFloatable);
	}

	/**
	 * Returns true, if the content is pinnable
	 */
	bool isContentPinnable() const
	{
		return this->ContentFeatures.testFlag(CDockWidget::DockWidgetPinnable);
	}

	/**
	 * Returns the content features
	 */
	CDockWidget::DockWidgetFeatures contentFeatures() const
	{
		CDockWidget* DockWidget = qobject_cast<CDockWidget*>(Content);
		if (DockWidget)
		{
			return DockWidget->features();
		}

		CDockAreaWidget* DockArea = qobject_cast<CDockAreaWidget*>(Content);
		if (DockArea)
		{
			return DockArea->features();
		}

		return CDockWidget::DockWidgetFeatures();
	}
};

//============================================================================
void FloatingDragPreviewPrivate::updateDropOverlays(const QPoint &GlobalPos)
{
    if (!_this->isVisible() || !DockManager)
    {
        return;
    }
    
    CDockContainerWidget* top = nullptr;
    CDockAreaWidget* target = nullptr;
    
    for (auto container : DockManager->dockContainers())
    {
        for (auto area : container->openedDockAreas())
        {
            if (!area->isVisible())
            {
                continue;
            }
            
            QPoint MappedPos = area->mapFromGlobal(GlobalPos);
            if (area->rect().contains(MappedPos))
            {
                if (!top || container->isInFrontOf(top))
                {
                    top = container;
                    target = area;
                }
            }
        }
    }
    
    if (top)
    {
        auto area = DockManager->dockAreaOverlay()->showOverlay(target);
        DropContainer = top;
        if (area != InvalidDockWidgetArea || area != NoDockWidgetArea)
        {
            _this->setWindowOpacity(ads::internal::HoveredOverDockAreaOpacity);
        }
        else
        {
            _this->setWindowOpacity(1.0);
        }
    }
    else
    {
        DockManager->dockAreaOverlay()->hideOverlay();
        _this->setWindowOpacity(1.0);
    }
    
    return;
}


//============================================================================
FloatingDragPreviewPrivate::FloatingDragPreviewPrivate(CFloatingDragPreview *_public) :
	_this(_public)
{

}


//============================================================================
void FloatingDragPreviewPrivate::createFloatingWidget()
{
	CDockWidget* DockWidget = qobject_cast<CDockWidget*>(Content);
	CDockAreaWidget* DockArea = qobject_cast<CDockAreaWidget*>(Content);

	CFloatingDockContainer* FloatingWidget = nullptr;

	if (DockWidget && DockWidget->features().testFlag(CDockWidget::DockWidgetFloatable))
	{
		FloatingWidget = new CFloatingDockContainer(DockWidget);
	}
	else if (DockArea && DockArea->features().testFlag(CDockWidget::DockWidgetFloatable))
	{
		FloatingWidget = new CFloatingDockContainer(DockArea);
	}

	if (FloatingWidget)
	{
		FloatingWidget->setGeometry(_this->geometry());
		FloatingWidget->show();
		if (!CDockManager::testConfigFlag(CDockManager::DragPreviewHasWindowFrame))
		{
			QApplication::processEvents();
            
			int FrameHeight = FloatingWidget->frameGeometry().height() - FloatingWidget->geometry().height();
			QRect FixedGeometry = _this->geometry();
			FixedGeometry.adjust(0, FrameHeight, 0, 0);
			FloatingWidget->setGeometry(FixedGeometry);
		}
	}
}


//============================================================================
CFloatingDragPreview::CFloatingDragPreview(QWidget* Content, QWidget* parent) :
	QWidget(parent),
	d(new FloatingDragPreviewPrivate(this))
{
	d->Content = Content;
	d->ContentFeatures = d->contentFeatures();
	setAttribute(Qt::WA_DeleteOnClose);
	if (CDockManager::testConfigFlag(CDockManager::DragPreviewHasWindowFrame))
	{
		setWindowFlags(
			Qt::Window | Qt::WindowMaximizeButtonHint | Qt::WindowCloseButtonHint);
	}
	else
	{
		setWindowFlags(Qt::Tool | Qt::FramelessWindowHint);
		setAttribute(Qt::WA_NoSystemBackground);
		setAttribute(Qt::WA_TranslucentBackground);
	}

#if defined(Q_OS_UNIX) && !defined(Q_OS_MACOS)
    auto Flags = windowFlags();
    Flags |= Qt::WindowStaysOnTopHint | Qt::X11BypassWindowManagerHint;
    setWindowFlags(Flags);
#endif

	// Create a static image of the widget that should get undocked
	// This is like some kind preview image like it is uses in drag and drop
	// operations
	if (CDockManager::testConfigFlag(CDockManager::DragPreviewShowsContentPixmap))
	{
		d->ContentPreviewPixmap = QPixmap(Content->size());
		Content->render(&d->ContentPreviewPixmap);
	}

	connect(qApp, SIGNAL(applicationStateChanged(Qt::ApplicationState)),
		SLOT(onApplicationStateChanged(Qt::ApplicationState)));

	// The only safe way to receive escape key presses is to install an event
	// filter for the application object
	qApp->installEventFilter(this);
}


//============================================================================
CFloatingDragPreview::CFloatingDragPreview(CDockWidget* Content)
	: CFloatingDragPreview((QWidget*)Content, Content->dockManager())
{
	d->DockManager = Content->dockManager();
	if (Content->dockAreaWidget()->openDockWidgetsCount() == 1)
	{
		d->ContentSourceArea = Content->dockAreaWidget();
	}
	setWindowTitle(Content->windowTitle());
}


//============================================================================
CFloatingDragPreview::CFloatingDragPreview(CDockAreaWidget* Content)
	: CFloatingDragPreview((QWidget*)Content, Content->dockManager())
{
	d->DockManager = Content->dockManager();
	d->ContentSourceArea = Content;
    if (Content->currentDockWidget())
    {
    	setWindowTitle(Content->currentDockWidget()->windowTitle());
    }
}


//============================================================================
CFloatingDragPreview::~CFloatingDragPreview()
{
	delete d;
}

bool CFloatingDragPreview::isCanceled() const
{
    return d->Canceled;
}


//============================================================================
void CFloatingDragPreview::moveFloating()
{
    auto result = DockSnappingManager::instance().getSnapPoint(this, d->DockManager, d->DragStartMousePosition);
    if (std::get<0>(result))
    {
        move(std::get<1>(result));
    }
    else
    {
        int borderSize = (frameSize().width() - size().width()) / 2;

        const QPoint cursorPos = QCursor::pos();
        QPoint moveToPos = cursorPos - d->DragStartMousePosition - QPoint(borderSize, 0);
        move(moveToPos);
        // QPoint currentCursorPos = QCursor::pos();
        // QPoint moveToPos = currentCursorPos - d->DragStartMousePosition - QPoint(borderSize, 0) - QPoint(internal::DockMarginSize, internal::DockMarginSize);
        // QPoint offset = moveToPos - pos();
        
        // auto cursorScreen = FloatingHelper::currentlyHoveredScreen();
        // auto overhang = FloatingHelper::calculateOverhang(cursorScreen->availableGeometry(), geometry().translated(offset));
        // move(moveToPos - overhang);
        
        d->updateDropOverlays(QCursor::pos());
    }
  
}

void CFloatingDragPreview::cancelDragging()
{
    d->cancelDragging();
}


//============================================================================
void CFloatingDragPreview::startFloating(const QPoint &DragStartMousePos,
    const QSize &Size, eDragState DragState, QWidget *MouseEventHandler)
{
	Q_UNUSED(MouseEventHandler)
	Q_UNUSED(DragState)
    
    if (auto widget = qobject_cast<CDockWidget*>(d->Content); widget && widget->isTabbed())
    {
        if (auto area = widget->dockAreaWidget(); area)
        {
            CDockWidget* desired = nullptr;
            
            auto count = area->dockWidgetsCount();
            if (count > 0)
            {
                for (int i = 0; i < area->dockWidgetsCount(); ++i) {
                    if (area->dockWidget(i) == widget)
                    {
                        d->DraggedTabBarIndex = i;
                        if (i == 0)
                        {
                            desired = area->dockWidget(1);
                        }
                        else
                        {
                            desired = area->dockWidget(i - 1);
                        }
                    }
                }
            }
            
            area->setCurrentDockWidget(desired);
            
            area->titleBar()->tabBar()->currentTab()->hide();
            if (area->titleBar()->tabBar()->count() <= 2)
            {
                area->titleBar()->tabBar()->hide();
            }
        }
    }
    else if (d && d->ContentSourceArea)
    {
        d->ContentSourceArea->hide();
    }
    
	resize(Size);
	d->DragStartMousePosition = DragStartMousePos;
	moveFloating();
	show();
}


//============================================================================
void CFloatingDragPreview::finishDragging(bool)
{
	ADS_PRINT("CFloatingDragPreview::finishDragging");
    
    if (d->Canceled) return;    
    
    if (auto widget = qobject_cast<CDockWidget*>(d->Content); widget && widget->isTabbed())
    {
        if (auto area = widget->dockAreaWidget(); area)
        {
            area->setCurrentDockWidget(area->dockWidget(d->DraggedTabBarIndex));
            area->titleBar()->tabBar()->show();
        }
    }
    else if (d && d->ContentSourceArea)
    {
        d->ContentSourceArea->show();
    }
    
    auto DockDropArea = d->DockManager->dockAreaOverlay()->dropAreaUnderCursor();
    auto ContainerDropArea = d->DockManager->containerOverlay()->dropAreaUnderCursor();
	bool ValidDropArea = (DockDropArea != InvalidDockWidgetArea) || (ContainerDropArea != InvalidDockWidgetArea);
	// Non floatable auto hide widgets should stay in its current auto hide
	// state if they are dragged into a floating window
    
	if (ValidDropArea || d->isContentFloatable())
	{
		cleanupAutoHideContainerWidget(ContainerDropArea);
	}
	if (!d->DropContainer)
	{
		d->createFloatingWidget();
	}
	else if (DockDropArea != InvalidDockWidgetArea)
	{
		d->DropContainer->dropWidget(
            d->Content, DockDropArea,
            d->DropContainer->dockAreaAt(QCursor::pos()),
            d->DockManager->dockAreaOverlay()->tabIndexUnderCursor()
        );
        
        if (auto widget = qobject_cast<CDockWidget*>(d->Content); widget)
        {
            if (auto area = widget->dockAreaWidget(); area)
            {
                area->titleBar()->tabBar()->setAllTabsVisible();
            }
        }
	}
	else if (ContainerDropArea != InvalidDockWidgetArea)
	{
		CDockAreaWidget* DockArea = nullptr;
		// If there is only one single dock area, and we drop into the center
		// then we tabify the dropped widget into the only visible dock area
		if (d->DropContainer->visibleDockAreaCount() <= 1 && CenterDockWidgetArea == ContainerDropArea)
		{
			DockArea = d->DropContainer->dockAreaAt(QCursor::pos());
		}

		d->DropContainer->dropWidget(d->Content, ContainerDropArea, DockArea,
			d->DockManager->containerOverlay()->tabIndexUnderCursor());
	}
	else
	{
		d->createFloatingWidget();
	}

	close();
	d->DockManager->containerOverlay()->hideOverlay();
	d->DockManager->dockAreaOverlay()->hideOverlay();
}


//============================================================================
void CFloatingDragPreview::cleanupAutoHideContainerWidget(DockWidgetArea ContainerDropArea)
{
	auto DroppedDockWidget = qobject_cast<CDockWidget*>(d->Content);
	auto DroppedArea = qobject_cast<CDockAreaWidget*>(d->Content);
	auto AutoHideContainer = DroppedDockWidget
		? DroppedDockWidget->autoHideDockContainer()
		: DroppedArea->autoHideDockContainer();

	if (!AutoHideContainer)
	{
		return;
	}

	// If the dropped widget is already an auto hide widget and if it is moved
	// to a new side bar location in the same container, then we do not need
	// to cleanup
	if (ads::internal::isSideBarArea(ContainerDropArea)
	&& (d->DropContainer == AutoHideContainer->dockContainer()))
	{
		return;
	}

	AutoHideContainer->cleanupAndDelete();
}


//============================================================================
void CFloatingDragPreview::paintEvent(QPaintEvent* event)
{
	Q_UNUSED(event);
	if (d->Hidden)
	{
		return;
	}

	QPainter painter(this);
	painter.setOpacity(internal::DraggingDockOpacity);
	if (CDockManager::testConfigFlag(CDockManager::DragPreviewShowsContentPixmap))
	{
		painter.drawPixmap(QPoint(0, 0), d->ContentPreviewPixmap);
	}

	// If we do not have a window frame then we paint a QRubberBand like
	// frameless window
    // if (!CDockManager::testConfigFlag(CDockManager::DragPreviewHasWindowFrame))
    // {
        // QColor Color = palette().color(QPalette::Active, QPalette::Highlight);
        // QPen Pen = painter.pen();
        // Pen.setColor(Color.darker(120));
        // Pen.setStyle(Qt::SolidLine);
        // Pen.setWidth(1);
        // Pen.setCosmetic(true);
        // painter.setPen(Pen);
        // Color = Color.lighter(130);
        // Color.setAlpha(64);
        // painter.setBrush(Color);
        // painter.drawRect(rect().adjusted(0, 0, 0, 0));
    // }
}

//============================================================================
void CFloatingDragPreview::onApplicationStateChanged(Qt::ApplicationState state)
{
	if (state != Qt::ApplicationActive)
	{
		disconnect(qApp, SIGNAL(applicationStateChanged(Qt::ApplicationState)),
			this, SLOT(onApplicationStateChanged(Qt::ApplicationState)));
		d->cancelDragging();
	}
}


//============================================================================
bool CFloatingDragPreview::eventFilter(QObject *watched, QEvent *event)
{
	Q_UNUSED(watched);
    
    if ((event->type() == QEvent::Leave || event->type() == QEvent::Enter)) {
        event->accept();        
        return true;
    }
    
    if (!d->Canceled)
    {
        if (event->type() == QEvent::MouseButtonPress)
        {
            if (
                auto mouseEvent = static_cast<QMouseEvent*>(event); mouseEvent &&
                event->type() == QEvent::MouseButtonPress &&
                mouseEvent->button() == Qt::MouseButton::RightButton
            ) {
                watched->removeEventFilter(this);
                d->cancelDragging();
            }
        }
        
        if (event->type() == QEvent::KeyPress)
        {
            if (
                auto keyEvent = static_cast<QKeyEvent*>(event); keyEvent &&
                keyEvent->key() == Qt::Key_Escape
            ) {
                watched->removeEventFilter(this);
                d->cancelDragging();
            }
        }
    }
    
    return QWidget::eventFilter(watched, event);
}


} // namespace ads

//---------------------------------------------------------------------------
// EOF FloatingDragPreview.cpp
