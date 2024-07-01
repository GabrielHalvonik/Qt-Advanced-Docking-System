#include "centralwidget.h"

/*  // OpenGL painting widget

float distance(const QPoint& p1, const QPoint& p2)
{
    float dx = p2.x() - p1.x();
    float dy = p2.y() - p1.y();
    return std::sqrt(dx * dx + dy * dy);
}

QPoint interpolate(QPoint source, QPoint destination, float percentage)
{
    float x = (1 - percentage) * source.x() + percentage * destination.x();
    float y = (1 - percentage) * source.y() + percentage * destination.y();
    return QPoint(static_cast<int>(x), static_cast<int>(y));
}

CentralWidget::CentralWidget(QWidget* parent) : QOpenGLWidget(parent)
{
    image.fill(Qt::white);

    brush.fill(Qt::transparent);
    QPainter brushPainter { &brush };
    brushPainter.setPen(QColor::fromRgba(0xFF000000));
    brushPainter.setBrush(QBrush(QColor::fromRgba(0xFF000000)));
    brushPainter.drawEllipse(0, 0, 50, 50);

    // brush.load(":/brushes/brush.png");

    erase.fill(Qt::transparent);
    QPainter erasePainter { &erase };
    erasePainter.setPen(QColor::fromRgba(0xFFFFFFFF));
    erasePainter.setBrush(QBrush(QColor::fromRgba(0xFFFFFFFF)));
    erasePainter.drawEllipse(0, 0, 50, 50);

}

CentralWidget::~CentralWidget() { }

void CentralWidget::initializeGL()
{
    initializeOpenGLFunctions();
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    setAttribute(Qt::WA_AcceptTouchEvents);
}

void CentralWidget::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);
}

void CentralWidget::paintGL()
{
    QPainter painter(this);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    applyTransformations(painter);
    painter.drawImage(0, 0, image);

}

void CentralWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton)
    {
        isDrawing = true;

        drawBrushStroke(event->pos());
        previousPoint = event->pos();
        update();
    }
}

void CentralWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (isDrawing)
    {
        drawBrushStroke(event->pos());
        previousPoint = event->pos();
        update();
    }
}

void CentralWidget::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton)
    {
        isDrawing = false;

        previousPoint = { };

    }
}

void CentralWidget::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Tab)
    {
        isErase = !isErase;
    }
}

void CentralWidget::drawBrushStroke(const QPoint& point)
{

    QPainter imagePainter(&image);

    if (!previousPoint.has_value())
    {
        imagePainter.drawPixmap(point.x() - brush.width() / 2, point.y() - brush.height() / 2, isErase ? erase : brush);
    }
    else
    {
        const float segmentLength = 10.0f;
        float dist = distance(previousPoint.value(), point);
        int segments = static_cast<int>(dist / segmentLength) + 1;
        for (int j = 0; j <= segments; ++j)
        {
            float t = static_cast<float>(j) / segments;
            QPoint p = interpolate(previousPoint.value(), point, t);
            imagePainter.drawPixmap(p.x() - brush.width() / 2, p.y() - brush.height() / 2, isErase ? erase : brush);
        }
    }
}

void CentralWidget::applyTransformations(QPainter& painter)
{
    QTransform transform;
    transform.translate(translation.x(), translation.y());
    transform.scale(scale, scale);
    painter.setTransform(transform);
}

bool CentralWidget::event(QEvent* event)
{
    return QOpenGLWidget::event(event);
}

*/
