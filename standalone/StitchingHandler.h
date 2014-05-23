#include "imagestitcher.h"
#include <QObject>

class StitchingHandler : public QObject {
Q_OBJECT
public:
        StitchingHandler(ImageStitcher::AlgorithmType algorithm, QString inputDir, QString outDir);
        void run();  
        ImageStitcher::AlgorithmType algorithm;
        bool finishedAllImages;
        int numIterations;
	QString inputDir;
	QString outputDir;
public slots:
        void stitchingUpdate(StitchingUpdateData* updateData);
	void stitchingFinished(bool success);
};


