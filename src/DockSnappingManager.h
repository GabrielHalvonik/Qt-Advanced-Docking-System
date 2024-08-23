#pragma once

#include <tuple>
#include <vector>

#include <QObject>
#include <QPoint>
#include <QRect>

class QEvent;
class QWidget;
class QScreen;

namespace ads
{

class CDockManager;
class IFloatingWidget;
class CFloatingDockContainer;

class DockSnappingManager
{
public:

    static DockSnappingManager& instance();
    
    void draggingStarted(const QPoint& started, const QPoint& offset, IFloatingWidget* source);
    void draggingFinished();
    
    std::tuple<bool, QPoint> getSnapPoint(QWidget* preview, CDockManager* manager, const QPoint& dragStartMousePosition);
    
    void moveSnappedDockGroup(QWidget* owner, const QPoint& cursorPos, const QPoint& offset, QScreen* screen = nullptr);
    
    bool tryStoreSnappedChain(CDockManager* manager, CFloatingDockContainer* target);
    void clearSnappedChain();
    
    QRect calculateSnappedBoundingBox(std::vector<std::tuple<CFloatingDockContainer*, QPoint>>& containers);
    
public:
    const int SnapDistance = 15;
    
private:
    DockSnappingManager();
    
    void cancelDragging();
    std::vector<std::tuple<CFloatingDockContainer*, QPoint>> querySnappedChain(CDockManager* manager, CFloatingDockContainer* target);
    
private:
    QPoint lastPosition;
    struct { QPoint started; QPoint offset; IFloatingWidget* widget; } draggingSourceWidget;
    std::vector<std::tuple<CFloatingDockContainer*, QPoint>> snappedDockGroup;
    
private:
    class DockScreenRelocationEventFilter : public QObject
    {
    public:
        DockScreenRelocationEventFilter(DockSnappingManager* owner) : owner(owner) { }
        
        bool isActive = false;
        
    public:
        bool eventFilter(QObject*, QEvent* event) override;
        
    private:
        DockSnappingManager* owner;
        
    } dockScreenRelocationEventFilter { this };
};

}

