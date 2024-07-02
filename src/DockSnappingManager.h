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
    std::vector<CFloatingDockContainer*> querySnappedChain(CDockManager* manager, CFloatingDockContainer* target);
    QRect calculateSnappedBoundingBox(std::vector<CFloatingDockContainer*>& containers);

    const int SnapDistance = 15;

private:
    DockSnappingManager();
};

}

