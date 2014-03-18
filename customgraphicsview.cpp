#include "customgraphicsview.h"

//Qt includes
#include <QGraphicsScene>
#include <QGraphicsTextItem>
#include <QTextStream>
#include <QScrollBar>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QDebug>

/**
* Sets up the subclassed QGraphicsView
*/
CustomGraphicsView::CustomGraphicsView(QWidget* parent) : QGraphicsView(parent), currentZoom(1.0) {

    setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);

    //Set-up the scene
    QGraphicsScene* Scene = new QGraphicsScene(this);
    setScene(Scene);

    //Use ScrollHand Drag Mode to enable Panning
    setDragMode(ScrollHandDrag);
    //Allow the mouseMoveEvent function to be called even when the mouse is not pressed.
    setMouseTracking(true);
}

void CustomGraphicsView::setImage(QImage &image)
{
    imWidth = image.width();//geometry().width();
    imHeight = image.height();//geometry().height();
    QPixmap pixmap = QPixmap::fromImage(image);
    QGraphicsScene *viewScene = new QGraphicsScene(QRectF(0, 0, imWidth, imHeight), 0);
    QGraphicsPixmapItem *item = viewScene->addPixmap(pixmap.scaled(QSize(
        (int)viewScene->width(), (int)viewScene->height()),
        Qt::KeepAspectRatio, Qt::SmoothTransformation));
    fitInView(QRectF(0, 0, imWidth, imHeight),
                            Qt::KeepAspectRatio);
    if (scene() != NULL) {
        delete scene();
    }
    setScene(viewScene);
    currentZoom = 1.0;

}

QImage CustomGraphicsView::getImage()
{
    QImage image(scene()->width(), scene()->height(), QImage::Format_ARGB32);
    image.fill(Qt::white);
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    scene()->render(&painter);
    return image;
}

/**
  * Zoom the view in and out.
  */
void CustomGraphicsView::wheelEvent(QWheelEvent* event) {

    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);

    const double zoomOutLimit = 1.0;

    // Scale the view / do the zoom
    double scaleFactor = 1.15;
    if(event->delta() > 0) {
        // Zoom in
        currentZoom /= scaleFactor;
        scale(scaleFactor, scaleFactor);
    } else {
        // Zooming out
        currentZoom /= (1.0 / scaleFactor);
        if (currentZoom > zoomOutLimit) {
            currentZoom = zoomOutLimit;
        } else {
            scale(1.0 / scaleFactor, 1.0 / scaleFactor);
        }
    }

    // Don't call superclass handler here
    // as wheel is normally used for moving scrollbars
}

void CustomGraphicsView::mouseMoveEvent(QMouseEvent *event)
{
    QGraphicsView::mouseMoveEvent(event);
    if (event) {
        QRectF rect = sceneRect();

        QPointF transformed = mapToScene(event->x(), event->y());
        transformed.setX(transformed.x() - scene()->width() / 2);
        transformed.setY(transformed.y() - scene()->height() / 2);
        emit mouseMove(transformed.x(), transformed.y());
    }
}
