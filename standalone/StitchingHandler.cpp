#include "StitchingHandler.h"

#include <QDir>
#include <QString>
#include <QStringList>
#include <unistd.h>

StitchingHandler::StitchingHandler(ImageStitcher::AlgorithmType algorithm, QString inputDir, QString outDir) 
		: algorithm(algorithm), finishedAllImages(false), numIterations(0), inputDir(inputDir), outputDir(outDir) {
}

void StitchingHandler::run() {

		QDir directory(inputDir);
	        QStringList inputFiles = directory.entryList(QDir::Files | QDir::NoSymLinks | QDir::Readable);
                if (inputFiles.size() < 2) return;    // don't crash on one input image

                double angleParam = 1.0;
                double lengthParam = 1.0;
                double heuristicParam = 3.0;
                bool stepMode = false;
                double imageScale = 0.50;

		QStringList fullPathNames;
		//std::cout << "input files follow: ";
		for (int i = 0; i < inputFiles.size(); i++) {
			fullPathNames << QString(directory.absolutePath() + "/" + inputFiles.at(i));
			//std::cout << fullPathNames.at(i).toStdString() << " " ;
		}

                ImageStitcher* stitcher = new ImageStitcher(fullPathNames, imageScale, 1.25, angleParam, lengthParam, heuristicParam, ImageStitcher::SURF, ImageStitcher::BRUTE_FORCE, stepMode, algorithm, outputDir);
                //connect(stitcher, SIGNAL(stitchingUpdate(StitchingUpdateData*)), this, SLOT(stitchingUpdate(StitchingUpdateData*)));
                //connect(stitcher, SIGNAL(stitchingFinished(bool)), this, SLOT(stitchingFinished(bool)));
                stitcher->start();
                while (stitcher->finishedStitching == false) {
			usleep( 100000 );
                }   
}  

void StitchingHandler::stitchingFinished(bool success) {
	finishedAllImages = true;
	std::cout << "finished all images with success " << success << std::endl;
} 

void StitchingHandler::stitchingUpdate(StitchingUpdateData* updateData) {
                QString outputName = outputDir;
                if (algorithm == ImageStitcher::CUMULATIVE) {
                        outputName += "CUMULATIVE";
                } else if (algorithm == ImageStitcher::COMPOUND_HOMOGRAPHY) {
                        outputName += "COMPOUND";
                } else if (algorithm == ImageStitcher::REDUCE) {
                        outputName += "REDUCE";
                } else {
                        outputName += "FULL";
                }
                outputName = outputName + "_" + QString::number(numIterations);
                cv::imwrite(outputName.toStdString().c_str(), updateData->currentScene);
                numIterations++;
		std::cout << "finished iteration " << numIterations << " output file: " << outputName.toStdString() << std::endl;
}

