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
};

class ImageStitcher : public QThread
{
    Q_OBJECT
public:
    ImageStitcher(QStringList inputFiles, double scaleFactor, QObject *parent = 0);
    
signals:
    void stitchingUpdate(StitchingUpdateData* data);
public slots:

protected:
    void run();

private:
    QStringList inputFiles;
    const double SCALE_FACTOR;
};

Q_DECLARE_METATYPE(StitchingUpdateData*);

#endif // IMAGESTITCHER_H
