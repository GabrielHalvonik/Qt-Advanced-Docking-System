#include "DockSnappingManager.h"

#include <queue>
#include <unordered_set>

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

std::tuple<bool, QPoint> DockSnappingManager::getSnapPoint(QWidget* preview, CDockManager* manager, const QPoint& dragStartMousePosition)
{
    const QPoint cursorPos = QCursor::pos();

    struct {
        uint8_t order;
        QPoint position;
        std::vector<CFloatingDockContainer*> snappingCandidates;
    } bestSnap { };
    int bestSnapDistance = SnapDistance;

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

            for (uint8_t j = 0; j < 8; ++j)
            {
                int distance = (previewCorners[previewCornerIndices[j]] - containerCorners[containerCornerIndices[j]]).manhattanLength();
                if (distance <= SnapDistance) {
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

    if (bestSnapDistance < SnapDistance)
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

        if ((cursorPos - (preview->mapTo({}, previewRect.topLeft()) + dragStartMousePosition)).manhattanLength() > SnapDistance) {
            pos = (cursorPos - dragStartMousePosition);
        }

        return { true, pos };
    }

    return { false, { } };
}

std::vector<CFloatingDockContainer*> DockSnappingManager::querySnappedChain(CDockManager* manager, CFloatingDockContainer* target) {
    std::vector<CFloatingDockContainer*> chain;
    std::unordered_set<CFloatingDockContainer*> visited;
    std::queue<CFloatingDockContainer*> toVisit;

    toVisit.push(target);
    visited.insert(target);

    while (!toVisit.empty())
    {
        CFloatingDockContainer* current = toVisit.front();
        toVisit.pop();
        if (current != target)
        {
            chain.push_back(current);
        }

        QRect currentRect = current->geometry();
        QPoint currentCorners[4] = {
            current->parentWidget()->mapToGlobal(currentRect.topLeft()),
            current->parentWidget()->mapToGlobal(currentRect.topRight()),
            current->parentWidget()->mapToGlobal(currentRect.bottomLeft()),
            current->parentWidget()->mapToGlobal(currentRect.bottomRight())
        };

        for (auto containerWidget : manager->dockContainers())
        {
            if (containerWidget->isFloating())
            {
                CFloatingDockContainer* candidate = containerWidget->floatingWidget();
                if (candidate && candidate != current && visited.find(candidate) == visited.end())
                {
                    QRect candidateRect = candidate->geometry();
                    QPoint candidateCorners[4] = {
                        candidate->parentWidget()->mapToGlobal(candidateRect.topLeft()),
                        candidate->parentWidget()->mapToGlobal(candidateRect.topRight()),
                        candidate->parentWidget()->mapToGlobal(candidateRect.bottomLeft()),
                        candidate->parentWidget()->mapToGlobal(candidateRect.bottomRight())
                    };

                    bool isSnapped = false;
                    for (const auto& currentCorner : currentCorners)
                    {
                        for (const auto& candidateCorner : candidateCorners)
                        {
                            int distance = (currentCorner - candidateCorner).manhattanLength();
                            if (distance <= SnapDistance)
                            {
                                isSnapped = true;
                                break;
                            }
                        }
                        if (isSnapped) break;
                    }

                    if (isSnapped)
                    {
                        toVisit.push(candidate);
                        visited.insert(candidate);
                    }
                }
            }
        }
    }

    return chain;
}

}
