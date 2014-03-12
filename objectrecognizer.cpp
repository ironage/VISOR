#include "objectrecognizer.h"
#include "sharedfunctions.h"

#include <opencv2/core/core.hpp>

using namespace cv;

ObjectRecognizer::ObjectRecognizer()
{
}


RecognizerResults *ObjectRecognizer::recognizeObjects() {
    RecognizerResults *results = new RecognizerResults();
    if (inputImage.empty()) return results; // otherwise it will crash.
    inputImage.copyTo(results->input);

    /// Blur Image
    cv::GaussianBlur(inputImage, results->gaussianBlur, cv::Size(gaussianSD, gaussianSD), 0, 0);

    /// Convert to Grayscale
    cv::Mat grayImage;
    cv::cvtColor(results->gaussianBlur, grayImage, CV_BGR2GRAY);

    /// Canny Edge Detector
    cv::Mat bwImage;
    cv::Canny(grayImage, bwImage, cannyLow, cannyHigh);
    inputImage.copyTo(results->canny, bwImage);  // bwImage is our mask

    /// Hough Transform
    results->hough = Mat(inputImage.size(), CV_8UC3, Scalar(0,0,0));
    vector<Vec4i> lines;
    HoughLinesP(bwImage, lines, 1, CV_PI/180, houghVote, houghMinLength, houghMinDistance);

    /// Draw Lines from Hough Transform on Clean Image
    for( size_t i = 0; i < lines.size(); i++ )
    {
        Vec4i l = lines[i];
        line(results->hough, Point(l[0], l[1]), Point(l[2], l[3]), Scalar(255,255,255), 2, 8, 0);
    }
    cv::Mat oneChannelHoughImage;
    cv::cvtColor(results->hough, oneChannelHoughImage, CV_BGR2GRAY);
/*
    /// Apply Skeleton Morphological Operator
    cv::Mat skelImage(inputImage.size(), CV_8UC1, cv::Scalar(0));
    cv::Mat tempImage(inputImage.size(), CV_8UC1);
    cv::Mat element = cv::getStructuringElement(cv::MORPH_CROSS, cv::Size(3, 3));
    bool done;
    do
    {
        cv::morphologyEx(results->hough, tempImage, cv::MORPH_OPEN, element);
        cv::bitwise_not(tempImage, tempImage);
        cv::bitwise_and(results->hough, tempImage, tempImage);
        cv::bitwise_or(skelImage, tempImage, skelImage);
        cv::erode(results->hough, results->hough, element);

        double max;
        cv::minMaxLoc(results->hough, 0, &max);
        done = (max == 0);
    } while (!done);
    cv::Mat threeChannelSkeletonImage;
    cv::cvtColor(skelImage, threeChannelSkeletonImage, CV_GRAY2BGR);*/

    /// Find Contours
    //cv::Mat contourGrayImage;
    //cv::Mat contourBwImage;                           // prehaps don't need this line
    //cv::cvtColor(results->hough, contourGrayImage, CV_BGR2GRAY);             // prehaps don't need this line
    //cv::Canny(results->hough, contourBwImage, 0, 255);   // prehaps don't need this line
    //cv::Mat cannyImage2;
    std::vector<std::vector<cv::Point> > contours;
    cv::findContours(oneChannelHoughImage.clone(), contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);
    inputImage.copyTo(results->canny2, oneChannelHoughImage);

    /// Approximation Polygons
    inputImage.copyTo(results->output);
    std::vector<cv::Point> approxPolygon;
    for(int i=0; i<contours.size(); i++){
        cv::approxPolyDP(cv::Mat(contours[i]), approxPolygon, cv::arcLength(cv::Mat(contours[i]), true)*0.01, true);

        // skip small or non-convex objects
        if(std::fabs(cv::contourArea(contours[i]))<100 || !cv::isContourConvex(approxPolygon)){
            continue;
        }
        if(approxPolygon.size() == 3){
            SharedFunctions::setLabel(results->output, "TRIANGLE", contours[i]);
            SharedFunctions::drawPolygon(results->output, approxPolygon);
        }
        else if(approxPolygon.size() == 4){
            SharedFunctions::setLabel(results->output, "SQUARE", contours[i]);
            SharedFunctions::drawPolygon(results->output, approxPolygon);
        }
        else if(approxPolygon.size() == 5){
            SharedFunctions::setLabel(results->output, "PENTAGON", contours[i]);
            SharedFunctions::drawPolygon(results->output, approxPolygon);
        }
        else if(approxPolygon.size() == 6){
            SharedFunctions::setLabel(results->output, "HEXAGON", contours[i]);
            SharedFunctions::drawPolygon(results->output, approxPolygon);
        }
        else if(approxPolygon.size() > 15){
            SharedFunctions::setLabel(results->output, "CIRCLE", contours[i]);
            SharedFunctions::drawPolygon(results->output, approxPolygon);
        }
    }
    //saveImage(outputImage, "outputImage.jpg");

    return results;
}
