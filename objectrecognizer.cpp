#include "objectrecognizer.h"
#include "sharedfunctions.h"

#include <opencv2/core/core.hpp>

using namespace cv;

ObjectRecognizer::ObjectRecognizer()
{
}

/*
  TODO:
  - Input Image is not displayed properly in GUI. Currently same as Hough Image.. Can remove
  */

RecognizerResults *ObjectRecognizer::recognizeObjects() {
    RecognizerResults *results = new RecognizerResults();
    if (fullSizeInputImage.empty()) return results; // otherwise it will crash.

    cv::resize(fullSizeInputImage, inputImage, Size(), imageScale, imageScale, INTER_AREA);

    inputImage.copyTo(results->input);

    /// Blur Image
    cv::GaussianBlur(inputImage, results->gaussianBlur, cv::Size(gaussianSD, gaussianSD), 0, 0);

    /// Convert to Grayscale
    cv::Mat grayImage;
    cv::cvtColor(results->gaussianBlur, grayImage, CV_BGR2GRAY);

    /// Canny Edge Detector
    cv::Mat cannyImage, cannyImageDialate;
    cv::Canny(grayImage, cannyImage, cannyLow, cannyHigh);

    /// Dialate Canny Image
    int dilation_size = 2;
    Mat element = getStructuringElement( MORPH_RECT,
                                         Size( 2*dilation_size + 1, 2*dilation_size+1 ),
                                         Point( dilation_size, dilation_size ) );
    cv::dilate(cannyImage, cannyImageDialate, element);
    inputImage.copyTo(results->canny, cannyImageDialate);  // output, inputmask

    /// Hough Transform
    results->hough = Mat(inputImage.size(), CV_8UC3, Scalar(0,0,0));
    vector<Vec4i> lines;
    HoughLinesP(cannyImageDialate, lines, 1, CV_PI/180, houghVote, houghMinLength, houghMinDistance);

    /// Draw Lines from Hough Transform on Clean Image
    for( size_t i = 0; i < lines.size(); i++ )
    {
        Vec4i l = lines[i];
        line(results->hough, Point(l[0], l[1]), Point(l[2], l[3]), Scalar(255,255,255), 2, 8, 0);
    }
    cv::Mat oneChannelHoughImage;
    cv::cvtColor(results->hough, oneChannelHoughImage, CV_BGR2GRAY); // source, destination

    /// OR the canny image and hough image
    cv::Mat orImage(inputImage.size(), CV_8UC1, cv::Scalar(0));
    cv::bitwise_or(cannyImage, oneChannelHoughImage, orImage);
    cvtColor(orImage, results->canny2, CV_GRAY2BGR);

    /// Find Contours
    std::vector<std::vector<cv::Point> > contours;
    cv::findContours(orImage.clone(), contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);

    /// Approximation Polygons
    inputImage.copyTo(results->output);
    std::vector<cv::Point> approxPolygon;
    for(int i=0; i<contours.size(); i++){
        cv::approxPolyDP(cv::Mat(contours[i]), approxPolygon, cv::arcLength(cv::Mat(contours[i]), true)*polyDPError, true);

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
    return results;
}
