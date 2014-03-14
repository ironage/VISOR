#ifndef CUSTOMSLIDER_H
#define CUSTOMSLIDER_H

#include <QSlider>
#include <QString>

class CustomSlider : public QSlider
{
    Q_OBJECT
public:
    explicit CustomSlider(QWidget *parent = 0);
    double getCurrentCustomValue();
signals:
    void customValueChanged(QString text);
    void customValueChanged(double value);
public slots:
    void setCustomValues(double min, double max, double initialValue = -1);
    void setCurrentCustomValue(double v);
protected:
    virtual void sliderChange(SliderChange change);
    double customMin;
    double customMax;
};

#endif // CUSTOMSLIDER_H
