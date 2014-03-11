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

    /// output image
    cv::Mat outputImage;
    cv::Mat binaryImage(imageSize, CV_8UC1);
    inputImage.copyTo(outputImage);
    vector<Vec4i> lines;
    HoughLinesP(bwImage, lines, 1, CV_PI/180, houghVote, houghMinLength, houghMinDistance);
    for( size_t i = 0; i < lines.size(); i++ )
    {
      Vec4i l = lines[i];
      line( outputImage, Point(l[0], l[1]), Point(l[2], l[3]), Scalar(0,0,255), 3, CV_AA);
      //line( binaryImage, Point(l[0], l[1]), Point(l[2], l[3]), Scalar(0,0,255), 3, CV_AA);
    }
    ///saveImage(outputImage, "04_outputImage.jpg");

    /*
    // Find Contours
    std::vector<std::vector<cv::Point> > contours;
    cv::findContours(bwImage.clone(), contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);
    std::cout << "I'm here..." << std::endl;

    // approximation polygons
    std::vector<cv::Point> approxPolygon;
    cv::Mat outputImage = inputImage;                   // put label is destination image
    for(int i=0; i<contours.size(); i++){
        cv::approxPolyDP(cv::Mat(contours[i]), approxPolygon, cv::arcLength(cv::Mat(contours[i]), true)*0.02, true);
        //cv::approxPolyDP(cv::Mat(contours[i]), approxPolygon, cv::arcLength(cv::Mat(contours[i]), false)*0.00002, false);

        // skip small of non-convex objects
        if(std::fabs(cv::contourArea(contours[i]))<100 || !cv::isContourConvex(approxPolygon)){
            continue;
        }
        if(approxPolygon.size() == 3){
            setLabel(outputImage, "TRIANGLE", contours[i]);
            drawPolygon(outputImage, approxPolygon);
        }
        else if(approxPolygon.size() == 4){
            setLabel(outputImage, "SQUARE", contours[i]);
            drawPolygon(outputImage, approxPolygon);
        }
        else if(approxPolygon.size() == 5){
            setLabel(outputImage, "PENTAGON", contours[i]);
            drawPolygon(outputImage, approxPolygon);
        }
        else if(approxPolygon.size() == 6){
            setLabel(outputImage, "HEXAGON", contours[i]);
            drawPolygon(outputImage, approxPolygon);
        }
        else if(approxPolygon.size() > 15){
            setLabel(outputImage, "CIRCLE", contours[i]);
            drawPolygon(outputImage, approxPolygon);
        }
    }
    saveImage(outputImage, "outputImage.jpg");
    */
    return outputImage;
}
