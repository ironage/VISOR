#include "objectrecognizer.h"
#include "sharedfunctions.h"

#include <opencv2/core/core.hpp>

using namespace cv;

ObjectRecognizer::ObjectRecognizer()
{
}


Mat ObjectRecognizer::recognizeObjects() {
    if (inputImage.empty()) return Mat(); // otherwise it will crash.
    //saveImage(inputImage, "01_inputImage.jpg");

    /// input image size
    cv::Size imageSize = inputImage.size();

    /// Blur Image
    cv::Mat blurImage;
    cv::GaussianBlur(inputImage, blurImage, cv::Size(gaussianSD, gaussianSD), 0, 0);
    //saveImage(blurImage, "02_blurImage.jpeg");

    /// Convert to Grayscale
    cv::Mat grayImage;
    cv::cvtColor(blurImage, grayImage, CV_BGR2GRAY);

    /// Canny Edge Detector
    cv::Mat bwImage;
    cv::Canny(grayImage, bwImage, cannyLow, cannyHigh);

    /// save Canny Edge Image
    cv::Mat cannyImage;
    inputImage.copyTo(cannyImage, bwImage);  // bwImage is our mask
    //saveImage(cannyImage, "03_cannyImage.jpg");

    /// Hough Transform
    Mat lineImage(imageSize, CV_8UC3, Scalar(255,255,255));
    vector<Vec4i> lines;
    HoughLinesP(bwImage, lines, 1, CV_PI/180, houghVote, houghMinLength, houghMinDistance);
    for( size_t i = 0; i < lines.size(); i++ )
    {
      Vec4i l = lines[i];
      line( lineImage, Point(l[0], l[1]), Point(l[2], l[3]), Scalar(0,0,0), 3, CV_AA);
    }
    ///saveImage(outputImage, "04_outputImage.jpg");


    /// Find Contours
    cv::Mat contourGrayImage, contourBwImage;                           // prehaps don't need this line
    cv::cvtColor(lineImage, contourGrayImage, CV_BGR2GRAY);             // prehaps don't need this line
    cv::Canny(contourGrayImage, contourBwImage,cannyLow, cannyHigh);   // prehaps don't need this line
    std::vector<std::vector<cv::Point> > contours;
    cv::findContours(contourBwImage.clone(), contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);

    cv::Mat cannyImage2;
    inputImage.copyTo(cannyImage2, contourBwImage);

    /// Approximation Polygons
    cv::Mat outputImage;
    inputImage.copyTo(outputImage);
    std::vector<cv::Point> approxPolygon;
    for(int i=0; i<contours.size(); i++){
        cv::approxPolyDP(cv::Mat(contours[i]), approxPolygon, cv::arcLength(cv::Mat(contours[i]), true)*0.01, true);

        // skip small or non-convex objects
        if(std::fabs(cv::contourArea(contours[i]))<100 || !cv::isContourConvex(approxPolygon)){
            continue;
        }
        if(approxPolygon.size() == 3){
            SharedFunctions::setLabel(outputImage, "TRIANGLE", contours[i]);
            SharedFunctions::drawPolygon(outputImage, approxPolygon);
        }
        else if(approxPolygon.size() == 4){
            SharedFunctions::setLabel(outputImage, "SQUARE", contours[i]);
            SharedFunctions::drawPolygon(outputImage, approxPolygon);
        }
        else if(approxPolygon.size() == 5){
            SharedFunctions::setLabel(outputImage, "PENTAGON", contours[i]);
            SharedFunctions::drawPolygon(outputImage, approxPolygon);
        }
        else if(approxPolygon.size() == 6){
            SharedFunctions::setLabel(outputImage, "HEXAGON", contours[i]);
            SharedFunctions::drawPolygon(outputImage, approxPolygon);
        }
        else if(approxPolygon.size() > 15){
            SharedFunctions::setLabel(outputImage, "CIRCLE", contours[i]);
            SharedFunctions::drawPolygon(outputImage, approxPolygon);
        }
    }
    //saveImage(outputImage, "outputImage.jpg");

    return lineImage;
}
