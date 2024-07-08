#include "floatinghelper.h"

#include <QCursor>
#include <QScreen>
#include <QGuiApplication>
#include "QtCore/qdebug.h"

namespace ads
{

QPoint FloatingHelper::calculateOverhang(const QRect& screenBounds, const QRect& widgetBounds)
{
    int overhangX = 0;
    int overhangY = 0;
    
    if (widgetBounds.left() < screenBounds.left())
    {
        overhangX = widgetBounds.left() - screenBounds.left();
    }
    else if (widgetBounds.right() > screenBounds.right())
    {
        overhangX = widgetBounds.right() - screenBounds.right();
    }
    
    if (widgetBounds.top() < screenBounds.top())
    {
        overhangY = widgetBounds.top() - screenBounds.top();
    }
    else if (widgetBounds.bottom() > screenBounds.bottom())
    {
        overhangY = widgetBounds.bottom() - screenBounds.bottom();
    }
    
    return QPoint(overhangX, overhangY);   
}

QScreen* FloatingHelper::currentlyHoveredScreen()
{
    QPoint cursorPos = QCursor::pos();
    
    for (auto screen : QGuiApplication::screens())
    {
        QRect screenGeometry = screen->geometry();
        
        int left = screenGeometry.left();
        int right = screenGeometry.right();
        int top = screenGeometry.top();
        int bottom = screenGeometry.bottom();
        // qInfo() << "yy " << cursorPos.y() << " : btm " << right;
        if (cursorPos.x() >= left && cursorPos.x() <= right + 1 &&
            cursorPos.y() >= top && cursorPos.y() <= bottom + 1)
        {
            // qInfo() << screenGeometry << " contains " << cursorPos;
            return screen;
        }
    }
    
    return nullptr;
}

}
