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
    //saveImage(inputImage, "01_inputImage.jpg");
    inputImage.copyTo(results->input);

    /// input image size
    cv::Size imageSize = inputImage.size();

    /// Blur Image
    cv::GaussianBlur(inputImage, results->gaussianBlur, cv::Size(gaussianSD, gaussianSD), 0, 0);
    //saveImage(blurImage, "02_blurImage.jpeg");

    /// Convert to Grayscale
    cv::Mat grayImage;
    cv::cvtColor(results->gaussianBlur, grayImage, CV_BGR2GRAY);

    /// Canny Edge Detector
    cv::Mat bwImage;
    cv::Canny(grayImage, bwImage, cannyLow, cannyHigh);

    /// save Canny Edge Image
    inputImage.copyTo(results->canny, bwImage);  // bwImage is our mask
    //saveImage(cannyImage, "03_cannyImage.jpg");

    /// Hough Transform
    results->hough = Mat(imageSize, CV_8UC3, Scalar(255,255,255));
    vector<Vec4i> lines;
    HoughLinesP(bwImage, lines, 1, CV_PI/180, houghVote, houghMinLength, houghMinDistance);
    for( size_t i = 0; i < lines.size(); i++ )
    {
      Vec4i l = lines[i];
      line( results->hough, Point(l[0], l[1]), Point(l[2], l[3]), Scalar(0,0,0), 3, CV_AA);
    }
    ///saveImage(outputImage, "04_outputImage.jpg");


    /// Find Contours
    cv::Mat contourGrayImage, contourBwImage;                           // prehaps don't need this line
    cv::cvtColor(results->hough, contourGrayImage, CV_BGR2GRAY);             // prehaps don't need this line
    cv::Canny(contourGrayImage, contourBwImage,cannyLow, cannyHigh);   // prehaps don't need this line
    std::vector<std::vector<cv::Point> > contours;
    cv::findContours(contourBwImage.clone(), contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);

    inputImage.copyTo(results->canny2, contourBwImage);

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
