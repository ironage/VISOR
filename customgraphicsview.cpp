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

    //Populate the scene
   /* for(int x = 0; x < 1000; x = x + 25) {
        for(int y = 0; y < 1000; y = y + 25) {

            if(x % 100 == 0 && y % 100 == 0) {
                Scene->addRect(x, y, 2, 2);

                QString pointString;
                QTextStream stream(&pointString);
                stream << "(" << x << "," << y << ")";
                QGraphicsTextItem* item = Scene->addText(pointString);
                item->setPos(x, y);
            } else {
                Scene->addRect(x, y, 1, 1);
            }
        }
    }*/

    //Set-up the view
    //setSceneRect(0, 0, 1000, 1000);

    //Use ScrollHand Drag Mode to enable Panning
    setDragMode(ScrollHandDrag);
}

void CustomGraphicsView::setImage(QImage &image)
{
    int width = image.width();//geometry().width();
    int height = image.height();//geometry().height();
    QPixmap pixmap = QPixmap::fromImage(image);
    QGraphicsScene *viewScene = new QGraphicsScene(QRectF(0, 0, width, height), 0);
    QGraphicsPixmapItem *item = viewScene->addPixmap(pixmap.scaled(QSize(
        (int)viewScene->width(), (int)viewScene->height()),
        Qt::KeepAspectRatio, Qt::SmoothTransformation));
    fitInView(QRectF(0, 0, width, height),
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
