#include "objectrecognizer.h"
#include <stdlib.h>
#include <iostream>
#include <fstream>

#include <sys/types.h>
#include <sys/stat.h>

const std::string OUT_IMG_DIR = "outputs/";
const std::string OUT_DATA_DIR = "data/";

//this is LINUX specific currently...
//create these output directories if they don't exist
void createDirs() {
	mkdir(OUT_IMG_DIR.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	mkdir(OUT_DATA_DIR.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
}

void failOnArguments(std::string description) {
        std::cout << "Invalid arguments. " <<  description << "\n";
        std::cout << "Usage: imagePath gpsLAT gpsLON altitude heading\n";
        std::cout << "For example ./OR inputImage.jpg 48.23232 28.2322 397 270\n";
        exit(1);
}

void parseArguments(int argc, char* argv[], TelemetryInputs *data, std::string* imageName) {
        if (argc != 6) {
                failOnArguments("Incorrect number of arguments.");
        }

	(*imageName) = argv[1];
	data->latitude = atof(argv[2]);
	data->longitude = atof(argv[3]);
	data->altitude = atof(argv[4]);
	data->heading = atof(argv[5]);
}

int main(int argc, char* argv[]) {

	createDirs();
	
	TelemetryInputs input;
	std::string imageName;
	parseArguments(argc, argv, &input, &imageName);

	ObjectRecognizer objRec;
	//set default parameters:
	objRec.gaussianSD = 21;
    	objRec.cannyLow = 78;
    	objRec.cannyHigh = 137;
    	objRec.houghVote = 30;
    	objRec.houghMinLength = 0;
    	objRec.houghMinDistance = 40;
    	objRec.imageScale = 1.0;
    	objRec.polyDPError = 0.03;

	cv::Mat image = cv::imread(imageName, CV_LOAD_IMAGE_COLOR);
	objRec.fullSizeInputImage = image;

	RecognizerResults* results = objRec.recognizeObjects(input);

	std::string outputFileName = OUT_IMG_DIR;
	std::string outputDataName = OUT_DATA_DIR;
	unsigned int found = imageName.find_last_of("/\\");
	if (found != std::string::npos) {
		outputFileName += imageName.substr(found+1);
		outputDataName += imageName.substr(found+1);
	} else {
		outputFileName += imageName;
		outputDataName += imageName;
	}
	if (outputDataName.size() >= 3) {
		outputDataName.replace(outputDataName.size() - 3, 3, "txt");
	}
	cv::imwrite(outputFileName, results->output);

	std::filebuf fb;
	fb.open(outputDataName, std::ios::out);
	std::ostream dataOut(&fb);
	for (int i = 0; i < results->targets.size(); i++) {
		dataOut << results->targets.at(i).json << std::endl;
	}
	fb.close();

	delete results;
	return 0;
}

