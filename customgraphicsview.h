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
    virtual void mouseMoveEvent(QMouseEvent *event);
signals:
    void mouseMove(int x, int y);
    
public slots:

private:
    double currentZoom;
    int imWidth;
    int imHeight;
    
};

#endif // CUSTOMGRAPHICSVIEW_H
