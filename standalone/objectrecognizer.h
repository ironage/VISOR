#ifndef OBJECTRECOGNIZER_H
#define OBJECTRECOGNIZER_H
#include <opencv2/opencv.hpp>
#include <memory>

struct TargetDefinition {
	TargetDefinition(int minX, int minY, int maxX, int maxY, double latitude, double longitude, double areaInMetersSquared, std::string description) 
	: latitude(latitude), longitude(longitude), areaInMeters(areaInMetersSquared)
	{
		this->boxMinX = minX;
		this->boxMinY = minY;
		this->boxMaxX = maxX;
		this->boxMaxY = maxY;
		this->json = description;
	}
	int boxMinX;
	int boxMinY;
	int boxMaxX;
	int boxMaxY;
	double latitude;
	double longitude;
	double areaInMeters;
	std::string json;
};

struct GPSPosition {
	double latitude;
	double longitude;
};

struct RecognizerResults {
    cv::Mat input;
    cv::Mat gaussianBlur;
    cv::Mat canny;
    cv::Mat hough;
    cv::Mat canny2;
    cv::Mat output;
	std::vector<TargetDefinition> targets;
};

struct TelemetryInputs {
	double altitude;
	double heading;
	double latitude;
	double longitude;
};

class ObjectRecognizer
{
public:
    ObjectRecognizer();
	RecognizerResults* recognizeObjects(TelemetryInputs ti);
	void testTargetColours();
	std::vector<std::string> getTargetColours(const cv::Mat& input, std::vector<cv::Point> shape);
	GPSPosition pixelToGPS(int x, int y, int imageWidth, int imageHeight, double altitudeInput, double yawInput, double centerLat, double centerLon);

    cv::Mat fullSizeInputImage;
    cv::Mat inputImage;
    int gaussianSD;
    int cannyLow;
    int cannyHigh;
    int houghVote;
    int houghMinLength;
    int houghMinDistance;
    double imageScale;
    double polyDPError;

};

#endif // OBJECTRECOGNIZER_H
