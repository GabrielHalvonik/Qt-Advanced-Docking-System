#ifndef FLOATINGHELPER_H
#define FLOATINGHELPER_H

#include <QPoint>
#include <QRect>

class QScreen;

namespace ads
{

class FloatingHelper
{
public:
    FloatingHelper() = delete;
    FloatingHelper(const FloatingHelper&) = delete;
    
    static QPoint calculateOverhang(const QRect& screenBounds, const QRect& widgetBounds);
    static QScreen* currentlyHoveredScreen();
};

}
    
#endif  // FLOATINGHELPER_H
