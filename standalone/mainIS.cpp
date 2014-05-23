#include "imagestitcher.h"
#include "StitchingHandler.h"
#include <stdlib.h>
#include <iostream>
#include <fstream>

#include <QtGui/QApplication>
#include <QDir>
#include <QString>
#include <QStringList>

#include <sys/types.h>
#include <sys/stat.h>

const QString OUT_IMG_IS_DIR = "imageOutputIS/";

//this is LINUX specific currently...
//create these output directories if they don't exist
void createDirs() {
	mkdir(OUT_IMG_IS_DIR.toStdString().c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
}

void failOnArguments(std::string description) {
        std::cout << "Invalid arguments. " <<  description << "\n";
        std::cout << "Usage: imageInputDirectory algorithmType\n";
        std::cout << "For example ./IS inputImageDir\n";
	std::cout << "algorithm types include: CUMULATIVE COMPOUND REDUCE FULL\n";
	std::cout << "if the algorithm type is omitted it will default to FULL\n";
        exit(1);
}

void parseArguments(int argc, char* argv[], QString* folderPath, ImageStitcher::AlgorithmType* type) {
        if (argc != 2 && argc != 3) {
                failOnArguments("Incorrect number of arguments.");
        }
	(*type) = ImageStitcher::FULL_MATCHES;
	(*folderPath) = QString(argv[1]);
	if (argc == 3) {
		if (strncmp(argv[2], "CUMULATIVE", 9) == 0) {
			(*type) = ImageStitcher::CUMULATIVE;
		} else if (strncmp(argv[2], "COMPOUND", 7) == 0) {
			(*type) = ImageStitcher::COMPOUND_HOMOGRAPHY;
		} else if (strncmp(argv[2], "REDUCE", 6) == 0) {
			(*type) = ImageStitcher::REDUCE;
		} else if (strncmp(argv[2], "FULL", 4) == 0) {
			(*type) = ImageStitcher::FULL_MATCHES;
		}
	}
}

int main(int argc, char* argv[]) {

	createDirs();
	
	ImageStitcher::AlgorithmType algorithm;
	QString folderPath;
	parseArguments(argc, argv, &folderPath, &algorithm);

	StitchingHandler handler(algorithm, folderPath, OUT_IMG_IS_DIR);
	handler.run();

	return 0;
}

