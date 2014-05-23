#include "objectrecognizer.h"
#include "sharedfunctions.h"

//The following allows us to use M_PI in visual studio
#define _USE_MATH_DEFINES
#include <math.h>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <iomanip>
#include <sstream>

using namespace cv;

ObjectRecognizer::ObjectRecognizer()
{

}

std::vector<double> getSideLengths(const std::vector<cv::Point>& vertices) {
	std::vector<double> lengths;
	if (vertices.size() < 3) {
		std::cout << "invalid polygon passed to getSideLengths() with number of vertices " << vertices.size() << std::endl;
		return lengths;
	}
	for (unsigned int i = 0; i < vertices.size(); i++) {
		cv::Point p1 = vertices.at(i);
		// This will loop around on the last iteration to link the last and first vertices
		cv::Point p2 = vertices.at((i + 1) % vertices.size());	
		double distance = std::sqrt(std::pow(p1.x - p2.x, 2) + std::pow(p1.y - p2.y, 2));
		lengths.push_back(distance);
	}
	return lengths;
}

std::vector<double> getInteriorAngles(const std::vector<cv::Point>& vertices) {
	std::vector<double> angles;
	if (vertices.size() < 3) {
		std::cout << "invalid polygon passed to getInteriorAngles() with number of vertices " << vertices.size() << std::endl;
		return angles;
	}
	for (unsigned int i = 0; i < vertices.size(); i++) {
		cv::Point p1 = vertices.at(i);
		cv::Point p2 = vertices.at((i + 1) % vertices.size());
		cv::Point p3 = vertices.at((i + 2) % vertices.size());
		double c = std::sqrt(std::pow(p1.x - p3.x, 2) + std::pow(p1.y - p3.y, 2));
		double a = std::sqrt(std::pow(p1.x - p2.x, 2) + std::pow(p1.y - p2.y, 2));
		double b = std::sqrt(std::pow(p2.x - p3.x, 2) + std::pow(p2.y - p3.y, 2));
		//law of cosines: theta = cos-1( (a^2 + b^2 - c^2) / (2*a*b) )
		double theta = std::acos((std::pow(a, 2) + std::pow(b, 2) - std::pow(c, 2)) / (2 * a * b));
		angles.push_back(theta);
	}
	return angles;
}


void testSideLengthsAndAngles() {
	std::vector<cv::Point> vertices = { cv::Point(-13, 15), cv::Point(12, 15), cv::Point(13, 3), cv::Point(1, -13), cv::Point(-9, -4) };

	std::vector<double> lengths = getSideLengths(vertices);
	std::vector<double> angles = getInteriorAngles(vertices);

	for (unsigned int i = 0; i < lengths.size(); i++) {
		std::cout << "length: " << lengths.at(i) << std::endl;
	}
	for (unsigned int i = 0; i < angles.size(); i++) {
		std::cout << "angle: " << angles.at(i) << std::endl;
	}
}

double getElevationFromGround(double seaLevelElevation) {
	return seaLevelElevation - 269; // sea level elevation at southPort Manitoba... CHANGE THIS FOR OTHER LOCATIONS
}

double getMetersPerPixel(double altitude) {
	// These numbers change based on the camera
	double metersPerPixel = altitude * (0.00000155) / 0.012; // all units in meters. = altitude * (dimension of pixel) / focal length
	return metersPerPixel;
}

//const double FOV_degrees = 29.6;
GPSPosition ObjectRecognizer::pixelToGPS(int x, int y, int imageWidth, int imageHeight, double altitudeInput, double yawInput, double centerLat, double centerLon) {
	//SUPER UBER HACK FOR BROKEN CAMERA DRIVERS!!!!!
	//center gps coordinates moved to be 512 pixels from the left side of the image
	x = x + (256 * imageScale);	

	//double FOV = getFOVInRadians(altitudeInput, imageWidth, imageHeight);
	//double metersPerPix = (double)altitudeInput * atan(FOV) / imageWidth;
	double groundLevelElevation = getElevationFromGround(altitudeInput);
	//double swath = 2 * groundLevelElevation * tan(FOV / 2);
	//double metersPerPix = (swath / imageWidth) * (swath / imageWidth);
	double metersPerPixel = getMetersPerPixel(groundLevelElevation);
	double yaw = -1 * yawInput * M_PI / 180.0; // convert to radians
	double offsetX = cos(yaw)*x + sin(yaw)*y;
	double offsetY = -1 * sin(yaw)*x + cos(yaw)*y;

	// offsets in meters
	offsetX *= metersPerPixel;
	offsetY *= metersPerPixel;

	//Earth’s radius
	int R = 6378137;

	// delta lat/long in radians
	double dLat = (double)offsetY / R;
	double dLon = (double)offsetX / (R*cos(M_PI*centerLat / 180.0));

	//OffsetPosition, decimal degrees
	double latitude = centerLat + dLat * 180.0 / M_PI;
	double longitude = centerLon + dLon * 180.0 / M_PI;

	GPSPosition gps;
	gps.latitude = latitude;
	gps.longitude = longitude;
	return gps;
}

double pixelAreaToMeters(double area, double altitudeInput, int imageWidth, int imageHeight) {
	//double FOV = getFOVInRadians(altitudeInput, imageWidth, imageHeight);
	double groundLevelElevation = getElevationFromGround(altitudeInput);
	//double swath = 2 * groundLevelElevation * tan(FOV / 2);
	//double metersPerPix = (swath / imageWidth) * (swath / imageWidth);
	double metersPerPixel = getMetersPerPixel(groundLevelElevation);
	double metersSquaredPerPixel = (metersPerPixel * metersPerPixel);
	return area * metersSquaredPerPixel;
}

void overlayExtraData(double lat, double lon, double area, std::vector<std::vector<cv::Point> > contours, cv::Mat& img) {
	std::stringstream ss;
	ss << "LAT: " << lat;

    int fontface = cv::FONT_HERSHEY_SIMPLEX;
    double scale = 0.4;
    int thickness = 1;
    int baseline = 0;

    cv::Size text = cv::getTextSize(ss.str(), fontface, scale, thickness, &baseline);
    cv::Rect r = cv::boundingRect(contours);
    r.y += text.height;

    cv::Point pt(r.x + ((r.width - text.width) / 2), r.y + ((r.height + text.height) / 2));
    cv::rectangle(img, pt + cv::Point(0, baseline), pt + cv::Point(text.width, -text.height), CV_RGB(255,255,255), CV_FILLED);
    cv::putText(img, ss.str(), pt, fontface, scale, CV_RGB(0,0,0), thickness, 8);

}

RecognizerResults *ObjectRecognizer::recognizeObjects(TelemetryInputs ti) {
    RecognizerResults *results = new RecognizerResults();
    if (fullSizeInputImage.empty()) return results; // otherwise it will crash.

    cv::resize(fullSizeInputImage, inputImage, Size(), imageScale, imageScale, INTER_AREA);

    inputImage.copyTo(results->input);

    /// Blur Image
    cv::GaussianBlur(inputImage, results->gaussianBlur, cv::Size(gaussianSD, gaussianSD), 0, 0);

    /// Convert to Grayscale
    cv::Mat grayImage;
    cv::cvtColor(results->gaussianBlur, grayImage, CV_BGR2GRAY);

    /// Canny Edge Detector
    cv::Mat cannyImage, cannyImageDialate;
    cv::Canny(grayImage, cannyImage, cannyLow, cannyHigh);

    /// Dialate Canny Image
    int dilation_size = 2;
    Mat element = getStructuringElement( MORPH_RECT,
                                         Size( 2*dilation_size + 1, 2*dilation_size+1 ),
                                         Point( dilation_size, dilation_size ) );
    cv::dilate(cannyImage, cannyImageDialate, element);
    inputImage.copyTo(results->canny, cannyImageDialate);  // output, inputmask

    /// Hough Transform
    results->hough = Mat(inputImage.size(), CV_8UC3, Scalar(0,0,0));
    vector<Vec4i> lines;
    HoughLinesP(cannyImageDialate, lines, 1, CV_PI/180, houghVote, houghMinLength, houghMinDistance);

    /// Draw Lines from Hough Transform on Clean Image
    for( size_t i = 0; i < lines.size(); i++ )
    {
        Vec4i l = lines[i];
        line(results->hough, Point(l[0], l[1]), Point(l[2], l[3]), Scalar(255,255,255), 2, 8, 0);
    }
    cv::Mat oneChannelHoughImage;
    cv::cvtColor(results->hough, oneChannelHoughImage, CV_BGR2GRAY); // source, destination

    /// OR the canny image and hough image
    cv::Mat orImage(inputImage.size(), CV_8UC1, cv::Scalar(0));
    cv::bitwise_or(cannyImage, oneChannelHoughImage, orImage);
    cvtColor(orImage, results->canny2, CV_GRAY2BGR);

    /// Find Contours
    std::vector<std::vector<cv::Point> > contours;
    cv::findContours(orImage.clone(), contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);

    /// Approximation Polygons
    inputImage.copyTo(results->output);
    std::vector<cv::Point> approxPolygon;
    for(unsigned int i=0; i<contours.size(); i++)
	{
        cv::approxPolyDP(cv::Mat(contours[i]), approxPolygon, cv::arcLength(cv::Mat(contours[i]), true)*polyDPError, true);
		
		std::string description = "";
		std::string shape = "undetermined";
		std::string objectType = "unknown";
		double areaInMeters = pixelAreaToMeters(cv::contourArea(contours[i]), ti.altitude, inputImage.cols, inputImage.rows);
        
			// skip small or non-convex objects
        if(std::fabs(cv::contourArea(contours[i]))<100 || !cv::isContourConvex(approxPolygon)){
            continue;
        }
        if(approxPolygon.size() == 3){
            SharedFunctions::setLabel(results->output, "TRIANGLE", contours[i]);
            SharedFunctions::drawPolygon(results->output, approxPolygon);
			shape = "triangle";
        }
        else if(approxPolygon.size() == 4){
			std::vector<double> sideLengths = getSideLengths(approxPolygon);
			std::vector<double> angles = getInteriorAngles(approxPolygon);

			const double PI = 3.14159265;
			bool allAnglesApproxRight = true;
			double rightAngle = PI / 2.0;
			double angleTolerance = PI / 18.0;	// error tolerance is within 1/18 of PI (10 degrees)
			for (unsigned int angleIndex = 0; angleIndex < angles.size(); angleIndex++) {
				if (std::abs(angles.at(angleIndex) - rightAngle) > angleTolerance) allAnglesApproxRight = false;
			}
			if (allAnglesApproxRight) {	//square or rectangle
				bool allSidesApproxEqual = true;
				double avg1 = (sideLengths.at(0) + sideLengths.at(2)) / 2;
				double avg2 = (sideLengths.at(1) + sideLengths.at(3)) / 2;
				double sideRatio = avg1 / avg2;
				double ratioError = 0.15;
				if ((sideRatio < (1.0 - ratioError)) || (sideRatio >(1.0 + ratioError))) {
					allSidesApproxEqual = false;
				}
				if (allSidesApproxEqual) {
					SharedFunctions::setLabel(results->output, "SQUARE", contours[i]);
					SharedFunctions::drawPolygon(results->output, approxPolygon);
					shape = "square";
				} else {
					SharedFunctions::setLabel(results->output, "RECTANGLE", contours[i]);
					SharedFunctions::drawPolygon(results->output, approxPolygon);
					shape = "rectangle";
				}
				const double TRUCK_AREA = 18; // meters squared
				const double HUMAN_AREA = 1.44; // meters squared
				const double ERROR_TOLERANCE = 2; // meters squared
				if (std::abs(TRUCK_AREA - areaInMeters) < ERROR_TOLERANCE) {
					objectType = "truck";
				} else if (std::abs(HUMAN_AREA - areaInMeters) < ERROR_TOLERANCE) {
					objectType = "human";
				} else if (areaInMeters >= 25 && areaInMeters <= 60) {
					objectType = "rockslide";
				} else if (areaInMeters >= 80) {	// 90 meters squared area
					objectType = "crop";
				}
			}
			else {	//quadralateral
				SharedFunctions::setLabel(results->output, "QUADRILATERAL", contours[i]);
				SharedFunctions::drawPolygon(results->output, approxPolygon);
				shape = "quadrilateral";
			}
        }
        else if(approxPolygon.size() == 5){
            SharedFunctions::setLabel(results->output, "PENTAGON", contours[i]);
            SharedFunctions::drawPolygon(results->output, approxPolygon);
			shape = "pentagon";
        }
        else if(approxPolygon.size() == 6){
            SharedFunctions::setLabel(results->output, "HEXAGON", contours[i]);
            SharedFunctions::drawPolygon(results->output, approxPolygon);
			shape = "hexagon";
        }
        else if(approxPolygon.size() > 15){
            SharedFunctions::setLabel(results->output, "CIRCLE", contours[i]);
            SharedFunctions::drawPolygon(results->output, approxPolygon);
			shape = "circle";
        }

		// bounds.tl is the top left corner
		// bounds.br is the bottom right corner
		// These wil get assigned to the targets minimum and maximum for the bounding square/box
		cv::Rect bounds = boundingRect(approxPolygon);
		cv::Point targetCenter(bounds.x + (bounds.width / 2), bounds.y + (bounds.height / 2));
		int halfImgWidth = inputImage.cols / 2;
		int halfImgHeight = inputImage.rows / 2;
		GPSPosition topLeft = pixelToGPS(bounds.x - halfImgWidth, bounds.y - halfImgHeight, inputImage.cols, inputImage.rows, ti.altitude, ti.heading, ti.latitude, ti.longitude);
		GPSPosition bottomRight = pixelToGPS(bounds.x + bounds.width - halfImgWidth, bounds.y + bounds.height - halfImgHeight, inputImage.cols, inputImage.rows, ti.altitude, ti.heading, ti.latitude, ti.longitude);

		std::vector<std::string> colours = getTargetColours(results->input, approxPolygon);
		GPSPosition targetPosition = pixelToGPS(targetCenter.x - halfImgWidth, targetCenter.y - halfImgHeight, inputImage.cols, inputImage.rows, ti.altitude, ti.heading, ti.latitude, ti.longitude);

	//	overlayExtraData(targetPosition.latitude, targetPosition.longitude, areaInMeters, contours, results->output);

		std::stringstream ss;
		ss << std::setprecision(10);	// print doubles to 10 decimal precision
		ss << "{ \"shape\" : \"" << shape << "\", "
			<< "\"primaryColour\" : \"" << colours.at(0) << "\", "
			<< "\"secondaryColour\" : \"" << colours.at(1) << "\","
			<< "\"area\" : \"" << areaInMeters << "\","
			<< "\"targetLatitude\" : \"" << targetPosition.latitude << "\","
			<< "\"targetLongitude\" : \"" << targetPosition.longitude << "\","
			<< "\"objectType\" : \"" << objectType << "\","
			<< "\"pixelBoundTopLeftX\" : \"" << bounds.x << "\","
			<< "\"pixelBoundTopLeftY\" : \"" << bounds.y << "\","
			<< "\"pixelBoundBottomRightX\" : \"" << bounds.x + bounds.width << "\","
			<< "\"pixelBoundBottomRightY\" : \"" << bounds.y + bounds.height << "\","
			<< "\"gpsBoundTopLeftLat\" : \"" << topLeft.latitude << "\","
			<< "\"gpsBoundTopLeftLon\" : \"" << topLeft.longitude << "\","
			<< "\"gpsBoundBottomRightLat\" : \"" << bottomRight.latitude << "\","
			<< "\"gpsBoundBottomRightLon\" : \"" << bottomRight.longitude << "\""
			<< "}";

		description.append(ss.str());

		results->targets.push_back(TargetDefinition(bounds.x, bounds.y, bounds.width, bounds.height, targetPosition.latitude, targetPosition.longitude, areaInMeters, description));
    }

    return results;
}

void ObjectRecognizer::testTargetColours() {
	Mat image = imread("hueSpectrum.jpg", CV_LOAD_IMAGE_COLOR);
	int width = 29; // (IMAGE_WIDTH = 700) / (NUM_COLOURS = 24)
	for (int i = 0; i < 27; i++) {	// 27 segments including the added black grey white sections at the end of the image
		cv::Rect bounds(i * width, 0, width, image.rows);
		std::vector<cv::Point> vertices = { cv::Point(bounds.x, bounds.y), cv::Point(bounds.x + bounds.width, bounds.y), cv::Point(bounds.x + bounds.width, bounds.y + bounds.height), cv::Point(bounds.x, bounds.y + bounds.height) };
		//SharedFunctions::drawPolygon(image, vertices);
		//imwrite("hue.jpg", image);
		std::vector<std::string> colours = getTargetColours(image, vertices);
		if (colours.size() == 2) {
			std::cout << "iteration " << i << " with bounds (" << bounds.x << "," << bounds.y << "," << bounds.width << "," << bounds.height << ") produces primary colour " << colours[0] << " secondary colour " << colours[1] << std::endl;
		}
	}
}

bool pointInPolygon(int x, int y, const std::vector<cv::Point>& vertices) {
	int j = vertices.size() - 1;
	bool oddNodes = false;

	for (unsigned int i = 0; i< vertices.size(); i++) {
		if ((vertices.at(i).y < y && vertices.at(j).y >= y
			|| vertices.at(j).y < y && vertices.at(i).y >= y)
			&& (vertices.at(i).x <= x || vertices.at(j).x <= x)) {
			oddNodes ^= (vertices.at(i).x + (y - vertices.at(i).y) / (vertices.at(j).y - vertices.at(i).y)*(vertices.at(j).x - vertices.at(i).x)<x);
		}
		j = i;
	}

	return oddNodes;
}

std::string convertBinIndexToColour(int index, int NUM_COLOURS, int blackIndex, int whiteIndex, int greyIndex) {
	std::string colour = "unknown";

	if (index == blackIndex) {
		colour = "black";
	}
	else if (index == whiteIndex) {
		colour = "white";
	}
	else if (index == greyIndex) {
		colour = "grey";
	}
	else {
		//opencv hues are implemented in the range from 0-180 not 0-255
		//these descriptions match the opencv HSV hue space
		std::vector<std::string> hueColours = { "red", "warm red", "orange", "warm yellow", "yellow", "cool yellow", "yellow green", "warm green", "green", "cool green", "green cyan", "warm cyan", "cyan", "cool cyan", "blue cyan", "cool blue", "blue", "warm blue", "violet", "cool magenta", "magenta", "warm magenta", "red magenta", "cool red" };
		if (index < (int)hueColours.size()) {
			colour = hueColours.at(index);
		}
	}
	return colour;
}

std::vector<std::string> ObjectRecognizer::getTargetColours(const cv::Mat& input, std::vector<cv::Point> vertices) {
	const int NUM_COLOURS = 24;	// separate the hues (0-180) into 24 bins
	const int NUM_BINS = NUM_COLOURS + 3;
	//special cases required for shades
	int whiteIndex = NUM_COLOURS;
	int greyIndex = NUM_COLOURS+1;
	int blackIndex = NUM_COLOURS+2;
	int bins[NUM_BINS];
	for (int i = 0; i < NUM_BINS; i++) {
		bins[i] = 0;
	}

	cv::Mat hsvImage;
	cvtColor(input, hsvImage, cv::COLOR_BGR2HSV);

	std::vector<std::string> colours;
	cv::Rect bounds = boundingRect(vertices);
	cv::Vec3b currentColour;
	for (int i = bounds.x; i < bounds.x + bounds.width && i < hsvImage.cols; i++) {
		for (int j = bounds.y; j < bounds.y + bounds.height && j < hsvImage.rows; j++) {
			if (pointInPolygon(i, j, vertices)) {
				currentColour = hsvImage.at<cv::Vec3b>(j, i);	//[0] == hue, [1] = saturation, [2] = value
				if (currentColour[0] < 2) {	// handle white, black, grey
					if (currentColour[2] < 50) {
						bins[blackIndex]++;
					} else if (currentColour[2] > 200) {
						bins[whiteIndex]++;
					} else {
						bins[greyIndex]++;
					}
				} else { // otherwise just put it into equally spaced bins
					int binIndex = (int)(currentColour[0] * NUM_COLOURS / 180.0);
					bins[binIndex] ++; 
				}
			}
		}
	}

	//find largest two bins
	int largestIndex = 0;
	int secondLargestIndex = 1;
	for (int i = 0; i < NUM_BINS; i++) {
		if (bins[i] > bins[largestIndex]) {
			largestIndex = i;
		}
	}
	for (int i = 0; i < NUM_BINS; i++) {
		if (bins[i] > bins[secondLargestIndex] && i != largestIndex) {
			secondLargestIndex = i;
		}
	}

	std::string primary = convertBinIndexToColour(largestIndex, NUM_COLOURS, blackIndex, whiteIndex, greyIndex);
	std::string secondary = convertBinIndexToColour(secondLargestIndex, NUM_COLOURS, blackIndex, whiteIndex, greyIndex);

	colours.push_back(primary);
	colours.push_back(secondary);

	return colours;
}
