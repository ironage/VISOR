#ifndef CUSTOMGRAPHICSVIEW_H
#define CUSTOMGRAPHICSVIEW_H

#include <QGraphicsView>

class CustomGraphicsView : public QGraphicsView
{
    Q_OBJECT
public:
    explicit CustomGraphicsView(QWidget *parent = 0);
    void setImage(QImage& image);
    QImage getImage();
protected:
    //Take over the interaction
    virtual void wheelEvent(QWheelEvent* event);
signals:
    
public slots:

private:
    double currentZoom;
    
};

#endif // CUSTOMGRAPHICSVIEW_H
