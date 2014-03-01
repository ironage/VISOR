// The "Square Detector" program.
// It loads several images sequentially and tries to find squares in
// each image

#include "opencv2/core/core.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"

#include <tesseract/baseapi.h>
#include <leptonica/allheaders.h>


#include <iostream>
#include <math.h>
#include <string.h>

using namespace cv;
using namespace std;

#define PI 3.141592653

class ShapeSide {
public:
	ShapeSide(const Point& p1, const Point& p2) : p1(p1), p2(p2) {
		Point directionVector;
		directionVector.x = abs(p1.x - p2.x);
		directionVector.y = abs(p1.y - p2.y);
		length = sqrt(
				pow((double) directionVector.x, 2)
						+ pow((double) directionVector.y, 2));
		angleRads = atan2((double) directionVector.y,
				(double) directionVector.x);
	}
	int getLength() const {
		return length;
	}
	double getAngleInRadians() const {
		return angleRads;
	}
	void setAngleRads(double newAngle) { //TODO: check sign
		int degrees = ((int) (newAngle * PI / 180)) % 360;
		angleRads = ((double) degrees) * 180 / PI;
	}
	bool compareLengths(const ShapeSide& other,
			unsigned int lengthErrorTolerance = 30) {
		if (other.getLength() < length + lengthErrorTolerance
				&& other.getLength() > length - lengthErrorTolerance) {
			return true;
		}
		return false;
	}
	bool compareAngles(const ShapeSide& other,
			double anglePercentError = 0.10) {
		if (anglePercentError > 1) {
			anglePercentError = 1;
		}
		if (anglePercentError < 0) {
			anglePercentError = 0;
		}
		double otherAngle = other.getAngleInRadians();
		double angleError = PI * 2.0 * anglePercentError;
		if (otherAngle < angleRads + angleError
				&& otherAngle > angleRads - angleError) {
			return true;
		}
		return false;
	}
	bool compare(const ShapeSide& other, unsigned int lengthErrorTolerance = 30,
			double anglePercentError = 0.10) {
		return compareAngles(other, anglePercentError)
				&& compareLengths(other, lengthErrorTolerance);
	}
	Point getP1() { return p1; }
	Point getP2() { return p2; }
private:
	double angleRads;
	int length;
	Point p1, p2;
};

class Shape {
public:
	Shape(vector<Point> v) :
			vertices(v), colour(0, 0, 0) {
		for (unsigned int i = 0; i < vertices.size(); i++) {
			unsigned int nextIndex = (i + 1) >= vertices.size() ? 0 : (i + 1);
			sides.push_back(ShapeSide(vertices[i], vertices[nextIndex]));
		}
	}
	virtual void draw(Mat& image) {
		const Point* p = &vertices[0];
		int n = (int) vertices.size();
		polylines(image, &p, &n, 1, true, colour, 3, CV_AA);
	}
	virtual ~Shape() {
	}
	virtual vector<ShapeSide> getSides() const {
		return sides;
	}
	virtual int getNumVertices() const {
		return vertices.size();
	}
	virtual bool compareCenters(const Shape& other, int pixelTolerance) const {
		Point center1 = getCenter();
		Point center2 = other.getCenter();
		if (center1.x > center2.x - pixelTolerance && center1.x < center2.x + pixelTolerance
				&& center1.y > center2.y - pixelTolerance && center1.y < center2.y + pixelTolerance) {
			return true;
		}
		return false;
	}
	virtual Point getCenter() const {
		int avgX = 0;
		int avgY = 0;
		for (int i = 0; i < vertices.size(); i++) {
			avgX += vertices[i].x;
			avgY += vertices[i].y;
		}
		return Point(avgX / vertices.size(), avgY / vertices.size());
	}
	Scalar getColour() { return colour; }
protected:
	vector<Point> vertices;
	vector<ShapeSide> sides;
	Scalar colour;
};
bool operator<(const Shape& lhs, const Shape& rhs) {
	std::cout << "operator < is used!\n";
	return lhs.getNumVertices() < rhs.getNumVertices();
}
bool operator==(const Shape& lhs, const Shape& rhs) {
	return (lhs.getNumVertices() == rhs.getNumVertices()) && (lhs.compareCenters(rhs, 3));
}

class Quad: public Shape {
public:
	Quad(vector<Point> points) :
			Shape(points), type(OTHER) {
		colour = Scalar(0, 255, 0);
		setupQuadType();
	}

	enum QuadType {
		SQUARE, RECT, OTHER
	};
	QuadType getType() {
		return type;
	}
private:
	void setupQuadType() {
		if (sides[0].compare(sides[2]) && sides[1].compare(sides[3])
				&& sides[0].compareLengths(sides[1])
				&& sides[2].compareLengths(sides[3])) {
			type = SQUARE;
			colour = Scalar(255, 0, 0);
		} else if (sides[0].compare(sides[2]) && sides[1].compare(sides[3])) {
			type = RECT;
			colour = Scalar(100, 100, 100);
		} else {
			type = OTHER;
		}
	}
	QuadType type;
};

class Triangle: public Shape {
public:
	Triangle(vector<Point> points) :
			Shape(points) {
		colour = Scalar(0, 0, 255);
	}
};

class Pentagon: public Shape {
public:
	Pentagon(vector<Point> points) :
			Shape(points) {
		colour = Scalar(0, 100, 100);
	}
};

void help() {
	cout
			<< "\nA program using pyramid scaling, Canny, contours, contour simpification and\n"
					"memory storage (it's got it all folks) to find\n"
					"squares in a list of images pic1-6.png\n"
					"Returns sequence of squares detected on the image.\n"
					"the sequence is stored in the specified memory storage\n"
					"Call:\n"
					"./squares\n"
					"Using OpenCV version " << CV_VERSION << "\n" << endl;
}

int thresh = 50, N = 11;
const char* wndname = "Square Detection Demo";

// helper function:
// finds a cosine of angle between vectors
// from pt0->pt1 and from pt0->pt2
double angle(Point pt1, Point pt2, Point pt0) {
	double dx1 = pt1.x - pt0.x;
	double dy1 = pt1.y - pt0.y;
	double dx2 = pt2.x - pt0.x;
	double dy2 = pt2.y - pt0.y;
	return (dx1 * dx2 + dy1 * dy2)
			/ sqrt((dx1 * dx1 + dy1 * dy1) * (dx2 * dx2 + dy2 * dy2) + 1e-10);
}

void removeAll(vector<Shape*>& shapes) {
	for (unsigned int i = 0; i < shapes.size(); i++) {
		delete (shapes[i]);
	}
	shapes.clear();
}

void checkForShapes(vector<Point> contour, vector<Shape*>& shapes) {
	vector<Point> approx;
	// approximate contour with accuracy proportional
	// to the contour perimeter
	approxPolyDP(Mat(contour), approx, arcLength(Mat(contour), true) * 0.02,
			true);

	// square contours should have 4 vertices after approximation
	// relatively large area (to filter out noisy contours)
	// and be convex.
	// Note: absolute value of an area is used because
	// area may be positive or negative - in accordance with the
	// contour orientation
	if (approx.size() == 4 && fabs(contourArea(Mat(approx))) > 1000
			&& isContourConvex(Mat(approx))) {
		double maxCosine = 0;

		for (int j = 2; j < 5; j++) {
			// find the maximum cosine of the angle between joint edges
			double cosine = fabs(angle(approx[j % 4], approx[j - 2], approx[j - 1]));
			maxCosine = MAX(maxCosine, cosine);
		}

		// if cosines of all angles are small
		// (all angles are ~90 degree) then write quandrange
		// vertices to resultant sequence
		if (maxCosine < 0.3) {
			Shape* s = new Quad(approx);
			shapes.push_back(s);
		}
	} else if (approx.size() == 3 && fabs(contourArea(Mat(approx))) > 500
			&& isContourConvex(Mat(approx))) {
		Shape* s = new Triangle(approx);
		shapes.push_back(s);
	} else if (approx.size() == 5) {
		Shape* s = new Pentagon(approx);
		bool sidesAreEqual = true;
		for (int i = 1; i < s->getSides().size(); i++) {
			if (!s->getSides()[i - 1].compareLengths(s->getSides()[i])) {
				sidesAreEqual = false;
				break;
			}
		}
		if (sidesAreEqual) {
			shapes.push_back(s);
		}
	}
}

// returns sequence of squares detected on the image.
// the sequence is stored in the specified memory storage
void findShapes(const Mat& image, vector<Shape*>& shapes) {
	removeAll(shapes);

	Mat pyr, timg, gray0(image.size(), CV_8U), gray;

	// down-scale and upscale the image to filter out the noise
	pyrDown(image, pyr, Size(image.cols / 2, image.rows / 2));
	pyrUp(pyr, timg, image.size());
	//imshow(wndname, pyr);
	//waitKey();
	vector<vector<Point> > contours;

	// find shapes in every color plane of the image
	for (int c = 0; c < 3; c++) {
		int ch[] = { c, 0 };
		mixChannels(&timg, 1, &gray0, 1, ch, 1);

		//cvtColor(timg,gray0,CV_RGB2GRAY);


		// try several threshold levels
		for (int l = 0; l < N; l++) {
			// hack: use Canny instead of zero threshold level.
			// Canny helps to catch squares with gradient shading
			if (l == 0) {
				// apply Canny. Take the upper threshold from slider
				// and set the lower to 0 (which forces edges merging)
				Canny(gray0, gray, 0, thresh, 5);
				// dilate canny output to remove potential
				// holes between edge segments
				dilate(gray, gray, Mat(), Point(-1, -1));
			} else {
				// apply threshold if l!=0:
				//     tgray(x,y) = gray(x,y) < (l+1)*255/N ? 255 : 0
				gray = gray0 >= (l + 1) * 255 / N;
			}

			//imshow(wndname, gray);
			//waitKey();
			// find contours and store them all as a list
			findContours(gray, contours, CV_RETR_LIST, CV_CHAIN_APPROX_SIMPLE);

			// test each contour
			for (size_t i = 0; i < contours.size(); i++) {
				checkForShapes(contours[i], shapes);
			}
		}
	}
}

//TODO: this is an O(n^2) operation, so improve it if performance becomes an issue.
void removeDuplicates(vector<Shape*>& shapes) {
	//shapes are the same if they are the same type, and have vertices in approximately the same place
	int shapesBefore = shapes.size();
	//std::unique requires that identical elements be next to each other so we have to sort the vector first.
    std::sort(shapes.begin(), shapes.end());
    shapes.erase(std::unique(shapes.begin(), shapes.end()), shapes.end());
    std::cout << "Removed " << shapesBefore - shapes.size() << " duplicate shapes\n";
}

// the function draws all the shapes in the image
void drawShapes(Mat& image, const vector<Shape*>& shapes) {
	for (size_t i = 0; i < shapes.size(); i++) {
		shapes[i]->draw(image);
		//draw the center of the shape
		//circle(image, shapes[i]->getCenter(), 5, shapes[i]->getColour());
	}

	imshow(wndname, image);
}

void loadImage(tesseract::TessBaseAPI* api, const Mat& image) {

/*    uchar* camData = new uchar[inputImage.total()*4];
    Mat continuousRGBA(inputImage.size(), CV_8UC4, camData);
    cv::cvtColor(inputImage, continuousRGBA, CV_BGR2RGBA, 4);

    Pix *pix = pixReadMem(camData, inputImage.total()*4);*/
    // Open input image with leptonica library
    //Pix *image = pixRead("/home/jstone/workspace/VisionTests/text2.jpg");

	api->SetImage((uchar*)image.data, image.size().width, image.size().height, image.channels(), image.step1());
}

std::string getTextFromImage(Mat& inputImage) {
    char *outText;

    tesseract::TessBaseAPI *api = new tesseract::TessBaseAPI();
    // Initialize tesseract-ocr with English, without specifying tessdata path
    if (api->Init(NULL, "eng")) {
        fprintf(stderr, "Could not initialize tesseract.\n");
        exit(1);
    }

    loadImage(api, inputImage);

    // Get OCR result
    outText = api->GetUTF8Text();
    printf("OCR output:\n%s", outText);

    // Destroy used object and release memory
    api->End();
    std::string text(outText);
    delete [] outText;
    return text;
}

int main(int /*argc*/, char** /*argv*/) {
	//static const char* names[] = { "pic1.png", "pic2.png", "pic3.png",
	//		"pic4.png", "pic5.png", "pic6.png", "pic7.png", "pic8.png", "pic9.png", 0 };
	static const char* names[] = { "IMG_0582.JPG", "IMG_0589.JPG", "IMG_0596.JPG", "IMG_0603.JPG", "IMG_0705.JPG", "IMG_0712.JPG", "IMG_0747.JPG", 0 };


	help();
	namedWindow(wndname, 1);
	vector<Shape*> shapes;

	for (int i = 0; names[i] != 0; i++) {
		std::cout << "reading image" << names[i] << "\n";
		Mat image = imread(names[i], 1);
		if (image.empty()) {
			cout << "Couldn't load " << names[i] << endl;
			continue;
		}
		std::cout << "text found: \"" << getTextFromImage(image) << "\"\n";

		findShapes(image, shapes);
		removeDuplicates(shapes);
		drawShapes(image, shapes);
		std::cout << "shapes found: " << shapes.size() << "\n";

		int c = waitKey();
		if ((char) c == 27)
			break;
	}
	removeAll(shapes);
	return 0;
}
