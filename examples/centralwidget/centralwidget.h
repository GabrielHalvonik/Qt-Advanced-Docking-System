#pragma once

#include <QWidget>

class QGestureEvent;

class CentralWidget : public QWidget
{
public:
    CentralWidget()
    {
        setStyleSheet("QWidget { background-color: gray; }");
    }
};

/*  // OpenGL painting widget
#include <QPainter>
#include <QOpenGLWidget>
#include <QOpenGLFunctions>

struct Delta
{
    QRect region { };
    std::vector<uchar> data { };
    // uchar* data { };
};

struct Snapshot
{
    QRect region { };
    uchar* data { };
};

class CentralWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
public:

    CentralWidget(QWidget* parent = nullptr);
    ~CentralWidget();

    static const int PageSize = 1024;

protected:
    void mousePressEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void keyPressEvent(QKeyEvent*) override;
    bool event(QEvent* event) override;

    void initializeGL() override;
    void resizeGL(int, int) override;
    void paintGL() override;

private:

    void drawBrushStroke(const QPoint&);
    QPoint interpolateSpline(const std::vector<QPoint>&, float);
    void applyTransformations(QPainter& painter);

private:
    const int bytesPerPixel = 4;

    QImage image { PageSize, PageSize, QImage::Format::Format_ARGB32 };
    QPixmap brush { 50, 50 };
    QPixmap erase { 50, 50 };

    bool isDrawing { false };
    bool isErase { false };

    std::optional<QPoint> previousPoint { };

    QTransform transform; // Transformation matrix for scaling and translation
    float scale = 1.0f;
    QPointF translation;
};

*/
