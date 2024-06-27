#pragma once

#include <tuple>
#include <vector>

#include <QPoint>

QT_FORWARD_DECLARE_CLASS(QWidget)

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

    const int SnapDistance = 15;

private:
    DockSnappingManager() = default;
};

}
