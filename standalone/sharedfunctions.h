#ifndef SHAREDFUNCTIONS_H
#define SHAREDFUNCTIONS_H

#include <opencv2/opencv.hpp>

class SharedFunctions
{
public:
    static void setLabel(cv::Mat& im, const std::string label, std::vector<cv::Point>& contour);
    static void drawPolygon(cv::Mat& image, std::vector<cv::Point> points);
    static cv::Rect findBoundingBox(cv::Mat &inputImage, bool inputGrayScale = false);
    static void saveImage(cv::Mat &image, std::string name);
private:
    SharedFunctions();  // the methods are all static so there is no need to instantiate this class
};

#endif // SHAREDFUNCTIONS_H
