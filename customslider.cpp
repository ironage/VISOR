#include "customslider.h"
#include <math.h>

CustomSlider::CustomSlider(QWidget *parent) :
    QSlider(parent)
{
}

double CustomSlider::getCurrentCustomValue()
{
    double percent = ((double)(value() - minimum())) / (maximum() - minimum());
    double newValue = ((percent * (customMax - customMin)) + customMin);
    double scale = 0.01;  // i.e. round to nearest one-hundreth
    double roundedValue = floor(newValue / scale + 0.5) * scale;
    return roundedValue;
}

void CustomSlider::setCurrentCustomValue(double v) {
    if (v >= customMin && v <= customMax) {
        double percent = (double)(v - customMin) / (customMax - customMin);
        double newValue = percent * (maximum() - minimum()) + minimum();
        double scale = 0.01;  // i.e. round to nearest one-hundreth
        double roundedValue = floor(newValue / scale + 0.5) * scale;
        setValue(roundedValue);
    }
}

void CustomSlider::sliderChange(QAbstractSlider::SliderChange change)
{
    QSlider::sliderChange(change);
    if (change == SliderValueChange) {
        emit customValueChanged(QString::number(getCurrentCustomValue()));
        emit customValueChanged(getCurrentCustomValue());
    }
}

void CustomSlider::setCustomValues(double min, double max, double initialValue) {
    customMin = min;
    customMax = max;
    setCurrentCustomValue(initialValue);
}

