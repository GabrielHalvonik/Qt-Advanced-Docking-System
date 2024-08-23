#include "DockSnappingManager.h"

#include <queue>
#include <unordered_set>

#include <QCursor>
#include <QWidget>
#include <QScreen>
#include <QApplication>

#include "DockManager.h"
#include "FloatingHelper.h"

namespace ads
{

DockSnappingManager::DockSnappingManager()
{
    // QObject::connect(qobject_cast<QGuiApplication*>(QApplication::instance()), &QGuiApplication::screenAdded, [](QScreen* screen){
    //     qInfo() << "screen added: " << screen->name();
    // });
    // QObject::connect(qobject_cast<QGuiApplication*>(QApplication::instance()), &QGuiApplication::screenRemoved, [](QScreen* screen){
    //     qInfo() << "screen removed: " << screen->name();
    // });
}

void DockSnappingManager::cancelDragging()
{
    auto source = draggingSourceWidget;
    
    if (source.widget != nullptr)
    {
        if (auto widget = dynamic_cast<QWidget*>(source.widget); widget)
        {
            draggingSourceWidget.widget->cancelDragging();
            moveSnappedDockGroup(widget, source.started, source.started - QCursor::pos() + QCursor::pos() - widget->mapToGlobal(source.offset), QGuiApplication::screenAt(source.started));
        }
    }
    
    draggingFinished();
}

DockSnappingManager& DockSnappingManager::instance()
{
    static DockSnappingManager Instance;
    return Instance;
}

void DockSnappingManager::draggingStarted(const QPoint& started, const QPoint& offset, IFloatingWidget* source)
{
    draggingSourceWidget = { started, offset, source };
    
    if (!dockScreenRelocationEventFilter.isActive)
    {
        QCoreApplication::instance()->installEventFilter(&dockScreenRelocationEventFilter);
        dockScreenRelocationEventFilter.isActive = true;
    }
}

void DockSnappingManager::draggingFinished()
{
    if (dockScreenRelocationEventFilter.isActive)
    {
        QCoreApplication::instance()->removeEventFilter(&dockScreenRelocationEventFilter);
        dockScreenRelocationEventFilter.isActive = false;
    }
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
            
            int containerWidth;
            int containerHeight;
            
            if (container->parentWidget() != nullptr)
            {
                containerWidth = container->parentWidget()->geometry().width();
                containerHeight = container->parentWidget()->geometry().height();
            }
            else
            {
                containerWidth = container->geometry().width();
                containerHeight = container->geometry().height();
            }
            
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

void DockSnappingManager::moveSnappedDockGroup(QWidget* owner, const QPoint& cursorPos, const QPoint& offset, QScreen* screen)
{
    if (screen == nullptr)
    {
        screen = owner->screen();
    }
    
    QPoint mouseOffset = cursorPos - lastPosition;
    lastPosition = cursorPos;

    auto bounds = DockSnappingManager::instance().calculateSnappedBoundingBox(snappedDockGroup);
    auto overhang = FloatingHelper::calculateOverhang(screen->availableGeometry(), bounds.translated(offset));

    QPoint delta { };

    if (!screen->availableGeometry().contains(bounds.translated(offset)))
    {
        for (auto item : snappedDockGroup)
        {
            auto widget = std::get<0>(item);
            widget->move(widget->pos() + offset - overhang);
        }

        if (snappedDockGroup.empty())
        {
            owner->move(owner->pos() + offset - overhang);
        }
    }
    else
    {
        if (overhang.x() > 0 && mouseOffset.x() < 0)
        {
            delta.setX(delta.x() + mouseOffset.x());
        }
        if (overhang.x() < 0 && mouseOffset.x() > 0)
        {
            delta.setX(delta.x() + mouseOffset.x());
        }
        if (overhang.y() > 0 && mouseOffset.y() < 0)
        {
            delta.setY(delta.y() + mouseOffset.y());
        }
        if (overhang.y() < 0 && mouseOffset.y() > 0)
        {
            delta.setY(delta.y() + mouseOffset.y());
        }

        {
            for (auto item : snappedDockGroup)
            {
                auto widget = std::get<0>(item);
                
                if (delta == QPoint())
                {
                    widget->move(widget->pos() + offset);
                }
                else
                {
                    widget->move(widget->pos() + delta);
                }
            }

            if (snappedDockGroup.empty())
            {
                if (delta == QPoint())
                {
                    owner->move(owner->pos() + offset);
                }
                else
                {
                    owner->move(owner->pos() + delta);
                }
            }
        }
    }
}

std::vector<std::tuple<CFloatingDockContainer*, QPoint>> DockSnappingManager::querySnappedChain(CDockManager* manager, CFloatingDockContainer* target)
{
    std::vector<std::tuple<CFloatingDockContainer*, QPoint>> chain;
    std::unordered_set<CFloatingDockContainer*> visited;
    std::queue<CFloatingDockContainer*> toVisit;

    toVisit.push(target);
    visited.insert(target);

    while (!toVisit.empty())
    {
        CFloatingDockContainer* current = toVisit.front();
        toVisit.pop();
        // if (current != target)
        // {
            chain.push_back({ current, current->pos() });
        // }

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
                            if (distance <= 1)
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

QRect DockSnappingManager::calculateSnappedBoundingBox(std::vector<std::tuple<CFloatingDockContainer*, QPoint>>& containers)
{
    if (containers.empty())
    {
        return QRect();
    }

    QRect boundingBox = std::get<0>(containers[0])->geometry();

    for (size_t i = 0; i < containers.size(); ++i)
    {
        boundingBox = boundingBox.united(std::get<0>(containers[i])->geometry());
    }

    return boundingBox;
}

bool DockSnappingManager::tryStoreSnappedChain(CDockManager* manager, CFloatingDockContainer* target)
{
    snappedDockGroup = querySnappedChain(manager, target);
    return snappedDockGroup.size() > 1;
}

void DockSnappingManager::clearSnappedChain()
{
    snappedDockGroup.clear();
}

bool DockSnappingManager::DockScreenRelocationEventFilter::eventFilter(QObject*, QEvent* event)
{
    if (event->type() == QEvent::KeyPress)
    {
        if (auto keyEvent = static_cast<QKeyEvent*>(event); keyEvent)
        {
            if (keyEvent->key() == Qt::Key_Escape)
            {
                owner->cancelDragging();
                
                return true;
            }
        }
    }
    else if (event->type() == QEvent::MouseButtonPress)
    {
        if (auto mouseEvent = static_cast<QMouseEvent*>(event); mouseEvent)
        {
            if (mouseEvent->button() == Qt::RightButton)
            {
                owner->cancelDragging();
                
                return true;
            }
        } 
    }
    else if (event->type() == QEvent::MouseMove)
    {
        if (auto mouseEvent = static_cast<QMouseEvent*>(event); mouseEvent)
        {
            if (auto screen = QApplication::screenAt(QCursor::pos()))
            {
                auto source = owner->draggingSourceWidget;
                if (source.widget != nullptr)
                {
                    auto cursorScreen = QApplication::screenAt(QCursor::pos());
                    if (auto widget = dynamic_cast<QWidget*>(source.widget); widget)
                    {
                        if (widget->screen() != cursorScreen)
                        {
                            owner->moveSnappedDockGroup(widget, QCursor::pos(), source.offset, cursorScreen);
                        }
                    }
                }
            }
        }
    }
    
    return false;
}

}
