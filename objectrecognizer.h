#ifndef OBJECTRECOGNIZER_H
#define OBJECTRECOGNIZER_H
#include <opencv2/opencv.hpp>

class ObjectRecognizer
{
public:
    ObjectRecognizer();
    cv::Mat recognizeObjects();

    cv::Mat inputImage;
    int gaussianSD;
    int cannyLow;
    int cannyHigh;
    int houghVote;
    int houghMinLength;
    int houghMinDistance;

};

#endif // OBJECTRECOGNIZER_H
