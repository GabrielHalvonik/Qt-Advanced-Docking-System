#pragma once

#include <tuple>
#include <vector>

#include <QCursor>
#include <QEvent>
#include <QMouseEvent>
#include <QObject>
#include <QPoint>
#include <QRect>

namespace ads
{

class CDockManager;
class CFloatingDockContainer;

class DockSnappingManager
{
public:

    static DockSnappingManager& instance();

    std::tuple<bool, QPoint> getSnapPoint(QWidget* preview, CDockManager* manager, const QPoint& dragStartMousePosition);

    void moveSnappedDockGroup(CFloatingDockContainer* owner, const QPoint& cursorPos, const QPoint& offset);

    void storeSnappedChain(CDockManager* manager, CFloatingDockContainer* target);
    void clearSnappedChain();

    QRect calculateSnappedBoundingBox(std::vector<CFloatingDockContainer*>& containers);

public:

    const int SnapDistance = 15;

private:
    DockSnappingManager();

    std::vector<CFloatingDockContainer*> querySnappedChain(CDockManager* manager, CFloatingDockContainer* target);
    QPoint calculateOverhang(const QRect& screenBounds, const QRect& widgetBounds);

private:
    QPoint lastPosition;
    std::vector<CFloatingDockContainer*> snappedDockGroup;
};

}

