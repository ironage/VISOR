#include "sharedfunctions.h"

#include <QImage>

using namespace cv;

SharedFunctions::SharedFunctions()
{
}

// draw a text box at a certain point in an image
void SharedFunctions::setLabel(cv::Mat& im, const std::string label, std::vector<cv::Point>& contour)
{
    int fontface = cv::FONT_HERSHEY_SIMPLEX;
    double scale = 1;
    int thickness = 3;
    int baseline = 0;

    cv::Size text = cv::getTextSize(label, fontface, scale, thickness, &baseline);
    cv::Rect r = cv::boundingRect(contour);

    cv::Point pt(r.x + ((r.width - text.width) / 2), r.y + ((r.height + text.height) / 2));
    cv::rectangle(im, pt + cv::Point(0, baseline), pt + cv::Point(text.width, -text.height), CV_RGB(255,255,255), CV_FILLED);
    cv::putText(im, label, pt, fontface, scale, CV_RGB(0,0,0), thickness, 8);
}

// draws a blue polygon on and image
void SharedFunctions::drawPolygon(cv::Mat& image, std::vector<cv::Point> points){

    for(unsigned int i=0; i<points.size(); i++){
        if(i==points.size()-1){
            cv::line(image, points[0],points[points.size()-1], cv::Scalar(0,0,255), 5, 8, 0);
        }
        else{
            cv::line(image, points[i],points[i+1], cv::Scalar(0,0,255), 3, CV_AA);
        }
    }
}

// Used to crop and to find region of interest
cv::Rect SharedFunctions::findBoundingBox(cv::Mat &inputImage, bool inputGrayScale) {

    Mat imageToFindBoundingBoxOn;

    if (!inputGrayScale) {
        cvtColor( inputImage, imageToFindBoundingBoxOn, CV_BGR2GRAY );
    } else {
        imageToFindBoundingBoxOn = inputImage;
    }

    //crop the black part of the image
    cv::Mat mask;
    vector<vector<Point> > contours; //no c++ 11 rabble rabble

    threshold(imageToFindBoundingBoxOn, mask, 1.0, 255.0, CHAIN_APPROX_SIMPLE);
    findContours(mask, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

    double maxContourArea = 0.0;
    unsigned maxContourIndex, i;
    for (i  = 0; i < contours.size(); ++i) {
        double a = contourArea(contours[i]);
        if (a > maxContourArea) {
            maxContourArea  = a;
            maxContourIndex = i;
        }
    }

    return boundingRect(contours[maxContourIndex]);
}

void SharedFunctions::saveImage(Mat &image, QString name) {
    cvtColor(image, image,CV_BGR2RGB);
    QImage qimgOrig((uchar*)image.data, image.cols, image.rows, image.step, QImage::Format_RGB888);
    qimgOrig.save(name);
    cvtColor(image, image, CV_RGB2BGR);
}
