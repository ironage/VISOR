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
};

class ImageStitcher : public QThread
{
    Q_OBJECT
public:
    explicit ImageStitcher(QStringList inputFiles, QObject *parent = 0);
    
signals:
    void stitchingUpdate(StitchingUpdateData* data);
public slots:

protected:
    void run();

private:
    QStringList inputFiles;
};

Q_DECLARE_METATYPE(StitchingUpdateData*);

#endif // IMAGESTITCHER_H
