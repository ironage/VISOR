#ifndef IMAGESTITCHER_H
#define IMAGESTITCHER_H

#include <QMetaType>
#include <QThread>
#include <QStringList>

#include <opencv2/opencv.hpp>

// This has to be a QObject so it can be passed through signals/slots
class StitchingUpdateData : public QObject {
    Q_OBJECT
public:
    StitchingUpdateData();
    cv::Mat currentScene;
    cv::Mat currentFeatureMatches;
    int curIndex;
    int totalImages;
    bool success;
};

class ImageStitcher : public QThread
{
    Q_OBJECT
public:
    enum FeatureDetector
    {
        SURF,
        ORB
    };

    enum FeatcherMatcher{
        FLANN,
        BRUTE_FORCE
    };

    ImageStitcher(QStringList inputFiles,
                  double scaleFactor, double roiSize, double angleStdDevs, double lenStdDevs, double distMins,
                  ImageStitcher::FeatureDetector featureDetector, ImageStitcher::FeatcherMatcher featureMatcher,
                  QObject *parent = 0);



    
signals:
    void stitchingUpdate(StitchingUpdateData* data);
public slots:

protected:
    void run();

private:
    QStringList inputFiles;
    const double SCALE_FACTOR;
    //TODO for now keeping old values in here but eventually should just make good ones default
    const double ROI_SIZE;               // = 1.5;
    const double STD_ANGLE_DEVS_TO_KEEP; // = 1.5;
    const double STD_LEN_DEVS_TO_KEEP;   //   = 3;
    const double NUM_MIN_DIST_TO_KEEP;   //   = 3;
    const ImageStitcher::FeatureDetector F_DETECTOR;
    const ImageStitcher::FeatcherMatcher F_MATCHER;

    StitchingUpdateData* stitchImages(cv::Mat &objImage, cv::Mat &sceneImage);
};

Q_DECLARE_METATYPE(StitchingUpdateData*);

#endif // IMAGESTITCHER_H
