#include "DockSnappingManager.h"

#include <QCursor>
#include <QWidget>

#include "DockManager.h"

namespace ads
{

DockSnappingManager& DockSnappingManager::instance()
{
    static DockSnappingManager Instance;
    return Instance;
}

std::optional<std::tuple<QPoint, std::vector<CFloatingDockContainer*>>> DockSnappingManager::getSnapPoint(QWidget* preview, CDockManager* manager, const QPoint& dragStartMousePosition)
{
    const QPoint cursorPos = QCursor::pos();

    int snapDistance = 20;
    struct {
        u_int8_t order;
        QPoint position;
        std::vector<CFloatingDockContainer*> snappingCandidates;
    } bestSnap { };
    int bestSnapDistance = snapDistance;

    QRect previewRect = preview->geometry();
    QPoint previewCorners[4] = {
                                preview->mapTo({}, previewRect.topLeft()),
                                preview->mapTo({}, previewRect.topRight()),
                                preview->mapTo({}, previewRect.bottomLeft()),
                                preview->mapTo({}, previewRect.bottomRight()),
                                };

    int previewCornerIndices[8] { 0, 1, 2, 3, 0, 1, 2, 3 };

    for (int i = 0; i < manager->dockContainers().size(); ++i)
    {
        auto container = manager->dockContainers()[i];
        if (container->isFloating())
        {
            QPoint containerPos = container->parentWidget()->pos();
            int containerWidth = container->geometry().width();
            int containerHeight = container->geometry().height();

            QPoint containerCorners[4] = {
                containerPos,
                containerPos + QPoint(containerWidth, 0),
                containerPos + QPoint(0, containerHeight),
                containerPos + QPoint(containerWidth, containerHeight)
            };

            int containerCornerIndices[8] { 1, 0, 3, 2, 2, 3, 0, 1 };

            for (u_int8_t j = 0; j < 8; ++j)
            {
                int distance = (previewCorners[previewCornerIndices[j]] - containerCorners[containerCornerIndices[j]]).manhattanLength();
                if (distance <= snapDistance) {
                    if (distance < bestSnapDistance)
                    {
                        bestSnap = { j, containerCorners[containerCornerIndices[j]], { container->floatingWidget() } };
                        bestSnapDistance = distance;
                    }
                    else if (distance == bestSnapDistance)
                    {
                        bestSnap.snappingCandidates.push_back(container->floatingWidget());
                    }
                }
            }
        }
    }

    if (bestSnapDistance < snapDistance)
    {
        QPoint pos;

        switch (bestSnap.order)
        {
        case 0:
        case 4:
            pos = (bestSnap.position);
            break;
        case 1:
        case 5:
            pos = (bestSnap.position - QPoint(preview->geometry().width(), 0));
            break;
        case 2:
        case 6:
            pos = (bestSnap.position - QPoint(0, preview->geometry().height()));
            break;
        case 3:
        case 7:
            pos = (bestSnap.position - QPoint(preview->geometry().width(), preview->geometry().height()));
            break;
        }

        if ((cursorPos - (preview->mapTo({}, previewRect.topLeft()) + dragStartMousePosition)).manhattanLength() > snapDistance) {
            pos = (cursorPos - dragStartMousePosition);
        }

        return { { pos, bestSnap.snappingCandidates } };
    }

    return { };
}

}
