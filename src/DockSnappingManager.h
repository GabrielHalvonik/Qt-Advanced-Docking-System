#pragma once

#include <tuple>
#include <vector>

#include <QCursor>
#include <QEvent>
#include <QMouseEvent>
#include <QWidget>
#include <QObject>
#include <QPoint>
#include <QRect>
#include <QApplication>
#include <QScreen>

namespace ads
{

class CDockManager;
class CFloatingDockContainer;

class DockSnappingManager
{
public:

    static DockSnappingManager& instance();
    
    void draggingStarted(const QPoint& offset, QWidget* source);
    void draggingFinished();
    
    std::tuple<bool, QPoint> getSnapPoint(QWidget* preview, CDockManager* manager, const QPoint& dragStartMousePosition);
    
    void moveSnappedDockGroup(QWidget* owner, const QPoint& cursorPos, const QPoint& offset, QScreen* screen = nullptr);
    
    bool tryStoreSnappedChain(CDockManager* manager, CFloatingDockContainer* target);
    void clearSnappedChain();
    
    QRect calculateSnappedBoundingBox(std::vector<CFloatingDockContainer*>& containers);
    
public:
    const int SnapDistance = 15;
    
private:
    DockSnappingManager();
    std::vector<CFloatingDockContainer*> querySnappedChain(CDockManager* manager, CFloatingDockContainer* target);
    
private:
    QPoint lastPosition;
    struct { QPoint offset; QWidget* widget; } draggingSourceWidget;
    std::vector<CFloatingDockContainer*> snappedDockGroup;
    
private:
    class DockScreenRelocationEventFilter : public QObject
    {
    public:
        DockScreenRelocationEventFilter(DockSnappingManager* owner) : owner(owner) { }
        
        bool isActive = false;
    public:
        bool eventFilter(QObject*, QEvent* event) override
        {
            auto mouseEvent = static_cast<QMouseEvent*>(event);
            if (mouseEvent != nullptr)
            {
                if (auto screen = QApplication::screenAt(QCursor::pos()))
                {
                    auto source = owner->draggingSourceWidget;
                    if (source.widget != nullptr)
                    {
                        auto cursorScreen = QApplication::screenAt(QCursor::pos());
                        if (source.widget->screen() != cursorScreen)
                        {
                            owner->moveSnappedDockGroup(source.widget, QCursor::pos(), source.offset, cursorScreen);
                        }
                    }
                }
            }
            return false;
        }
    private:
        DockSnappingManager* owner;
    } dockScreenRelocationEventFilter { this };
};

}

