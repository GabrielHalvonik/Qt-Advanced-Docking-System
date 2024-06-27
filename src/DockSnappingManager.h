#pragma once

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

    std::optional<QPoint> getSnapPoint(QWidget* preview, CDockManager* manager, const QPoint& dragStartMousePosition);
    std::vector<CFloatingDockContainer*> querySnappedChain(CDockManager* manager, CFloatingDockContainer* target);

private:
    DockSnappingManager() = default;
};

}
