#pragma once

#include <tuple>
#include <vector>

#include <QCursor>
#include <QEvent>
#include <QMouseEvent>
#include <QObject>
#include <QPoint>
#include <QRect>

QT_FORWARD_DECLARE_CLASS(QWidget)

class CursorRestrictionFilter : public QObject {

public:
    CursorRestrictionFilter(QObject* parent = nullptr)
        : QObject(parent) {}

    inline void setBoundsToCheck(const QRect& screenBound, const QRect& snappedBound)
    {
        currentScreenBound = screenBound;
        currentSnappedBound = snappedBound;
    }

protected:
    bool eventFilter(QObject* obj, QEvent* event) override
    {
        if (event->type() == QEvent::MouseMove)
        {
            QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
            // QPoint cursorPos = mouseEvent->globalPos();

            qInfo() << "MM: " << currentSnappedBound << " in " << currentScreenBound;
            if (!currentScreenBound.contains(currentSnappedBound))
            {
                QPoint newPos = mouseEvent->globalPos();

                if (currentSnappedBound.x() < currentScreenBound.left())
                {
                    newPos.setX(currentScreenBound.left());
                }
                if (currentSnappedBound.x() > currentScreenBound.right())
                {
                    newPos.setX(currentScreenBound.right());
                }
                if (currentSnappedBound.y() < currentScreenBound.top())
                {
                    newPos.setY(currentScreenBound.top());
                }
                if (currentSnappedBound.y() > currentScreenBound.bottom())
                {
                    newPos.setY(currentScreenBound.bottom());
                }

                qInfo() << "QCursor::setPos(newPos);";
                // QCursor::setPos({0, 0});
                return true; // Consume the event
            }
        }
        return QObject::eventFilter(obj, event);
    }

private:
    QRect currentScreenBound;
    QRect currentSnappedBound;
};


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

public:
    CursorRestrictionFilter* cursorRestrictionFilter;

private:
    DockSnappingManager();
};

}

