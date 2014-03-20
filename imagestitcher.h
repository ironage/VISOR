#ifndef IMAGESTITCHER_H
#define IMAGESTITCHER_H

#include <QMetaType>
#include <QThread>
#include <QStringList>
#include <QMutex>

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

class StitchingMatchesUpdateData {
public:
    StitchingMatchesUpdateData() {}
    cv::Mat object;
    cv::Mat scene;
    std::vector<cv::KeyPoint> objFeatures;
    std::vector<cv::KeyPoint> sceneFeatures;
    std::vector<cv::DMatch> matches;
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
                  bool stepModeState, QObject *parent = 0);
    void nextStep(double angle, double length, double heuristic);
    void setStepMode(bool inputStepMode);
    static std::vector<cv::DMatch> pruneMatches(const std::vector<cv::DMatch>& allMatches,
                const std::vector<cv::KeyPoint> &keypoints_object, const std::vector<cv::KeyPoint> &keypoints_scene,
                double angleThreshold, double distanceThreshold, double heuristicThreshold);
signals:
    void stitchingUpdate(StitchingUpdateData* data);
    void stitchingUpdateMatches(StitchingMatchesUpdateData data);
public slots:

protected:
    void run();

private:
    QStringList inputFiles;
    const double SCALE_FACTOR;
    //TODO for now keeping old values in here but eventually should just make good ones default
    const double ROI_SIZE;               // = 1.5;
    double STD_ANGLE_DEVS_TO_KEEP; // = 1.5;
    double STD_LEN_DEVS_TO_KEEP;   //   = 3;
    double NUM_MIN_DIST_TO_KEEP;   //   = 3;
    const ImageStitcher::FeatureDetector F_DETECTOR;
    const ImageStitcher::FeatcherMatcher F_MATCHER;
    QMutex lock;
    bool currentlyPaused;   // protected by lock
    bool stepMode;  // protected by lock
    bool useROI;
    cv::Rect roi;

    StitchingUpdateData* stitchImages(cv::Mat &objImage, cv::Mat &sceneImage);
    void pauseThreadUntilReady();
};

Q_DECLARE_METATYPE(StitchingUpdateData*)
Q_DECLARE_METATYPE(StitchingMatchesUpdateData)


#endif // IMAGESTITCHER_H
