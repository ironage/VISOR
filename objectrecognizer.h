#ifndef OBJECTRECOGNIZER_H
#define OBJECTRECOGNIZER_H
#include <opencv2/opencv.hpp>

struct RecognizerResults {
    cv::Mat input;
    cv::Mat gaussianBlur;
    cv::Mat canny;
    cv::Mat hough;
    cv::Mat canny2;
    cv::Mat output;
};

class ObjectRecognizer
{
public:
    ObjectRecognizer();
    RecognizerResults* recognizeObjects();

    cv::Mat inputImage;
    int gaussianSD;
    int cannyLow;
    int cannyHigh;
    int houghVote;
    int houghMinLength;
    int houghMinDistance;

};

#endif // OBJECTRECOGNIZER_H
