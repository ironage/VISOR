#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <fstream>
#include <cmath>
#include <QFileDialog>
#include <opencv2/opencv.hpp>
#include <opencv2/stitching/stitcher.hpp>
#include "opencv2/features2d/features2d.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/nonfree/nonfree.hpp"
#include "opencv2/calib3d/calib3d.hpp"
#include "opencv2/imgproc/imgproc.hpp"

using cv::Mat;
using namespace cv;
cv::Rect findBoundingBox(Mat &imageToCrop, bool inputGrayScale=false);


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    ui->display->setScaledContents(true);

    connect(ui->buttonOpenCamera, SIGNAL(clicked()), this, SLOT(openCameraClicked()));
    connect(ui->buttonOpenImage, SIGNAL(clicked()), this, SLOT(openImageClicked()));
    connect(ui->stitchButton, SIGNAL(clicked()), this, SLOT(stitchImagesClicked()));
    connect(ui->detectButton, SIGNAL(clicked()), this, SLOT(detectButtonClicked()));

    timer = new QTimer(this);
   // connect(timer, SIGNAL(timeout()),this, SLOT(processFrameAndUpdateGui()));
    timer->start(20);
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::processFrameAndUpdateGui() {
    Mat matOriginal;

    if (inputImages.size() > 0) {
        inputImages[0].copyTo(matOriginal);
        /*++counter;
        if (counter > 1000) {
            counter = 0;
            ++curIndex;
            inputImages[curIndex % inputImages.size()].copyTo(matOriginal);
        }*/
    } else {
        capWebcam.read(matOriginal);
        if(matOriginal.empty() == true) return;
    }
    if (matOriginal.empty() == true) return;
    // images are stored by default as Blue Gree Red, convert to RGB for display
    cvtColor(matOriginal, matOriginal,CV_BGR2RGB);

    QImage qimgOrig((uchar*)matOriginal.data, matOriginal.cols, matOriginal.rows, matOriginal.step, QImage::Format_RGB888);

    ui->display->setPixmap(QPixmap::fromImage(qimgOrig));
}

void MainWindow::openCameraClicked() {
    capWebcam.open(0);
    inputImages.clear();
}

void MainWindow::openImageClicked() {
    capWebcam.release();
    QString file = QFileDialog::getOpenFileName();
    capWebcam.open(file.toStdString());
    inputImages.clear();
}

void saveImage(Mat &image, QString name) {
    cvtColor(image, image,CV_BGR2RGB);
    QImage qimgOrig((uchar*)image.data, image.cols, image.rows, image.step, QImage::Format_RGB888);
    qimgOrig.save(name);
    cvtColor(image, image, CV_RGB2BGR);
}

cv::Mat result;
cv::Rect roi = cv::Rect(0, 0, 0, 0);
const double ROI_SIZE = 1.5;
const double STD_DEVS_TO_KEEP = 1.5;
// obj is the small image
// scene is the mosiac
void stitchImages(Mat &objImage, Mat &sceneImage ) {
    std::ofstream lengthsFile;
    std::ofstream anglesFile;
    lengthsFile.open("lengths.dat", std::ios_base::app);
    anglesFile.open("angles.dat", std::ios_base::app);
    // Pad the sceen to have sapce for the new obj
    int padding = std::max(objImage.cols, objImage.rows)/2;
    printf("max padding is %d\n", padding);
    Mat paddedScene;
    copyMakeBorder( sceneImage, paddedScene, padding, padding, padding, padding, BORDER_CONSTANT, 0 );

    // Convert imagages to gray scale to be used with openCV's detection features
    Mat grayObjImage, grayPadded;
    cvtColor( objImage,    grayObjImage, CV_BGR2GRAY );
    cvtColor( paddedScene, grayPadded, CV_BGR2GRAY );

    // only look at last image for stitching
    if (roi.height != 0) {
        roi.x += padding;
        roi.y += padding;

        int extraWidth  = roi.width  / ROI_SIZE;
        int extraHeight = roi.height / ROI_SIZE;

        roi.x -= extraWidth;
        roi.y -= extraHeight;
        roi.width  = ceil((double)roi.width  * ROI_SIZE);
        roi.height = ceil((double)roi.height * ROI_SIZE);

        if (roi.x < 0) roi.x = 0;
        if (roi.y < 0) roi.y = 0;
        if (roi.x + roi.width  >= paddedScene.cols) roi.width  = paddedScene.cols - roi.x;
        if (roi.y + roi.height >= paddedScene.rows) roi.height = paddedScene.rows - roi.y;

        //cv::rectangle(paddedScene, roi, Scalar(255, 0, 0), 3, CV_AA);
        //saveImage(paddedScene, "ROIshifted.png");
        std::cout << " ROI " << std::endl << roi << std::endl;
    } else {
        roi = cv::Rect(0, 0, grayPadded.cols, grayPadded.rows); // If not set then use the whole image.
    }
    Mat roiPointer = grayPadded(roi);

    // Detect the keypoints using SURF Detector
    int minHessian = 400;
    SurfFeatureDetector detector( minHessian );
    std::vector< KeyPoint > keypoints_object, keypoints_scene;
    detector.detect( grayObjImage, keypoints_object );
    detector.detect( roiPointer,   keypoints_scene );

    // Calculate descriptors (feature vectors)
    SurfDescriptorExtractor extractor;
    Mat descriptors_object, descriptors_scene;
    extractor.compute( grayObjImage, keypoints_object, descriptors_object );
    extractor.compute( roiPointer,   keypoints_scene,  descriptors_scene );

    // Match descriptor vectors using FLANN matcher
    FlannBasedMatcher matcher;
    std::vector< DMatch > matches;
    matcher.match( descriptors_object, descriptors_scene, matches );

    // Use only "good" matches
    // find mean and stddev of magnitude
    // and only take matches within a certain number of stddevs
    std::vector< DMatch > good_matches;

    // first find the means
    double lengthMean = 0.0;
    double angleMean  = 0.0;
    std::vector< double > angles;
    for( std::vector< DMatch >::iterator it = matches.begin(); it != matches.end(); it++ ) {
        lengthMean += (*it).distance;

        double x1 = keypoints_object[(*it).queryIdx].pt.x;
        double y1 = keypoints_object[(*it).queryIdx].pt.y;
        double x2 = keypoints_scene [(*it).trainIdx].pt.x;
        double y2 = keypoints_scene [(*it).trainIdx].pt.y;
        double angle = atan2(y2-y1,x2-x1);
        double euDistance = std::sqrt(std::pow(x1 - x2, 2) + std::pow(y1 - y2, 2));
        std::cout << "distance: " << it->distance << " euDistance " << euDistance << std::endl;

        angles.push_back(angle);
        angleMean += angle;

        lengthsFile << (*it).distance << std::endl;
        anglesFile  << angle << std::endl;
    }
    lengthMean /= matches.size();
    angleMean  /= matches.size();

    lengthsFile << "---------------------------------------" << std::endl;
    anglesFile  << "---------------------------------------" << std::endl;

    // next find the standard deviations
    double lengthStdDev = 0.0;
    double angleStdDev  = 0.0;
    // TODO maybe don't use iterators? it - begin is a bit ugly
    for( std::vector< DMatch >::iterator it = matches.begin(); it != matches.end(); it++ ) {
        lengthStdDev += ((*it).distance - lengthMean) * ((*it).distance - lengthMean);
        angleStdDev  += (angles[it - matches.begin()] - angleMean) * (angles[it - matches.begin()] - angleMean);
    }
    lengthStdDev /= matches.size();
    lengthStdDev  = sqrt(lengthStdDev);
    angleStdDev  /= matches.size();
    angleStdDev   = sqrt(angleStdDev);

    std::cout << "Length mean = " << lengthMean << " stddev = " << lengthStdDev << std::endl;
    std::cout << "Angle mean = "  << angleMean  << " stddev = " << angleStdDev  << std::endl;

    // finally prune the matches based off of stddev
    for( std::vector< DMatch >::iterator it = matches.begin(); it != matches.end(); it++ ) {
        if( (*it).distance > lengthMean + lengthStdDev*STD_DEVS_TO_KEEP ||
            (*it).distance < lengthMean - lengthStdDev*STD_DEVS_TO_KEEP ) {
            // length is out of std dev range don't add to list of good values;
            continue;
        }

        /*
        if( angles[it - matches.begin()] > angleMean + angleStdDev*STD_DEVS_TO_KEEP ||
            angles[it - matches.begin()] < angleMean - angleStdDev*STD_DEVS_TO_KEEP ) {
            // angle is out of std dev range
            continue;
        }
        */

        if( angles[it - matches.begin()] > angleMean + angleStdDev ||
            angles[it - matches.begin()] < angleMean - angleStdDev ) {
            // angle is out of std dev range
            continue;
        }


        // point passed tests adding to good matches
        good_matches.push_back(*it);
    }

    std::cout << "Found " << good_matches.size() << " goo matches" << std::endl;

    // Create a list of the good points in the object & scene
    std::vector< Point2f > obj;
    std::vector< Point2f > scene;
    for( unsigned i = 0; i < good_matches.size(); i++ ) {
        //-- Get the keypoints from the good matches
        obj.push_back( keypoints_object[ good_matches[i].queryIdx ].pt );
        scene.push_back( keypoints_scene[ good_matches[i].trainIdx ].pt );
    }

    Mat img_matches;
    drawMatches( grayObjImage, keypoints_object, roiPointer, keypoints_scene,
                 good_matches, img_matches, Scalar::all(-1), Scalar::all(-1),
                 vector<char>(), DrawMatchesFlags::NOT_DRAW_SINGLE_POINTS );
    saveImage(img_matches, "matches.png");

    // Find the Homography Matrix
    Mat H = findHomography( obj, scene, CV_RANSAC );

    std::cout << "Homography Mat" << std::endl << H << std::endl;

    // Use the Homography Matrix to warp the images
    H.row(0).col(2) += roi.x;   // Add roi offset coordinates to translation component
    H.row(1).col(2) += roi.y;
    warpPerspective(objImage,result,H,cv::Size(paddedScene.cols,paddedScene.rows));
    // result now contains the rotated/skewed/translated object image
    // this is our ROI on the next step
    roi = findBoundingBox(result);

    //cv::rectangle(result, roi, Scalar(255, 255, 255), 3, CV_AA);
    //saveImage(result, "ROI.png");

    // warpedObjDest is now a reference into the paddedScene where we are going to place the object
    //cv::Mat warpedObjDest(paddedScene,cv::Rect(0,0,paddedScene.cols,paddedScene.rows));
    // Find the non zero parts of the warped image
    cv::Mat mask = result > 0;
    result.copyTo(paddedScene,mask);
    // copy that on top of the scene
    result = paddedScene;
    cv::Rect crop = findBoundingBox(result);
    roi.x -= crop.x;
    roi.y -= crop.y;
    result = result(crop);
    std::cout << "result total: " << result.total() << "\n";
    saveImage(result, "resultAfter.png");
}

// Used to crop and to find region of interest
cv::Rect findBoundingBox(cv::Mat &inputImage, bool inputGrayScale) {

    Mat imageToFindBoundingBoxOn;

    if (!inputGrayScale) {
        cvtColor( inputImage, imageToFindBoundingBoxOn, CV_BGR2GRAY );
    } else {
        imageToFindBoundingBoxOn = inputImage;
    }

    //crop the black part of the image
    cv::Mat mask;
    vector<vector<Point> > contours; //no c++ 11 rabble rabble

    threshold(imageToFindBoundingBoxOn, mask, 1.0, 255.0, CHAIN_APPROX_SIMPLE);
    findContours(mask, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

    double maxContourArea = 0.0;
    unsigned maxContourIndex, i;
    for (i  = 0; i < contours.size(); ++i) {
        double a = contourArea(contours[i]);
        if (a > maxContourArea) {
            maxContourArea  = a;
            maxContourIndex = i;
        }
    }

    return boundingRect(contours[maxContourIndex]);
}

const double SCALE_FACTOR = 0.5;
void MainWindow::stitchImagesClicked() {

    QStringList names = QFileDialog::getOpenFileNames();
    //Mat output;

    Mat object = imread( names.at(1).toStdString() );
    Mat scene  = imread( names.at(0).toStdString() );
    Mat smallObject, smallScene;
    cv::resize(object, smallObject, Size(), SCALE_FACTOR, SCALE_FACTOR, INTER_AREA);
    cv::resize(scene,  smallScene,  Size(), SCALE_FACTOR, SCALE_FACTOR, INTER_AREA);
    stitchImages(smallObject, smallScene );

    for (int i = 2; i < names.count(); i++ ) {
        Mat object = imread( names.at(i).toStdString() );
        Mat smallObject;
        cv::resize(object, smallObject, Size(), SCALE_FACTOR, SCALE_FACTOR, INTER_AREA);
        Mat scene; result.copyTo(scene);
        stitchImages(smallObject, scene);
        printf("Finished I.S. iteration %d\n", i);
    }

    saveImage(result, "output.png");
}

// O.R. code starts

void setLabel(cv::Mat& im, const std::string label, std::vector<cv::Point>& contour)
{
    int fontface = cv::FONT_HERSHEY_SIMPLEX;
    double scale = 0.4;
    int thickness = 1;
    int baseline = 0;

    cv::Size text = cv::getTextSize(label, fontface, scale, thickness, &baseline);
    cv::Rect r = cv::boundingRect(contour);

    cv::Point pt(r.x + ((r.width - text.width) / 2), r.y + ((r.height + text.height) / 2));
    cv::rectangle(im, pt + cv::Point(0, baseline), pt + cv::Point(text.width, -text.height), CV_RGB(255,255,255), CV_FILLED);
    cv::putText(im, label, pt, fontface, scale, CV_RGB(0,0,0), thickness, 8);
}

// draws a blue polygon on and image
void drawPolygon(cv::Mat& image, std::vector<cv::Point> points){

    for(unsigned int i=0; i<points.size(); i++){
        if(i==points.size()-1){
            cv::line(image, points[0],points[points.size()-1], cv::Scalar(255,0,0), 5, 8, 0);
        }
        else{
            cv::line(image, points[i],points[i+1], cv::Scalar(255,0,0), 3, CV_AA);
        }
    }
}

void detectObjects(cv::Mat &inputImage){
    saveImage(inputImage, "01_inputImage.jpg");

    // Blur Image
    cv::Mat blurImage;
    cv::GaussianBlur(inputImage, blurImage, cv::Size(11,11), 0, 0);
    saveImage(blurImage, "02_blurImage.jpeg");

    // Convert to Grayscale
    cv::Mat grayImage;
    cv::cvtColor(blurImage, grayImage, CV_BGR2GRAY);

    // Canny Edge Detector
    cv::Mat bwImage;
    cv::Canny(grayImage, bwImage, 50, 130);

    // save Canny Edge Image
    cv::Mat cannyImage;
    inputImage.copyTo(cannyImage, bwImage);  // bwImage is our mask
    saveImage(cannyImage, "03_cannyImage.jpg");

    // output image
    cv::Mat outputImage = inputImage;
    vector<Vec4i> lines;
    HoughLinesP(bwImage, lines, 1, CV_PI/180, 120, 5, 60);
    for( size_t i = 0; i < lines.size(); i++ )
    {
      Vec4i l = lines[i];
      line( outputImage, Point(l[0], l[1]), Point(l[2], l[3]), Scalar(0,0,255), 3, CV_AA);
    }
    saveImage(outputImage, "04_outputImage.jpg");

    /*
    // Find Contours
    std::vector<std::vector<cv::Point> > contours;
    cv::findContours(bwImage.clone(), contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);
    std::cout << "I'm here..." << std::endl;

    // approximation polygons
    std::vector<cv::Point> approxPolygon;
    cv::Mat outputImage = inputImage;                   // put label is destination image
    for(int i=0; i<contours.size(); i++){
        cv::approxPolyDP(cv::Mat(contours[i]), approxPolygon, cv::arcLength(cv::Mat(contours[i]), true)*0.02, true);
        //cv::approxPolyDP(cv::Mat(contours[i]), approxPolygon, cv::arcLength(cv::Mat(contours[i]), false)*0.00002, false);

        // skip small of non-convex objects
        if(std::fabs(cv::contourArea(contours[i]))<100 || !cv::isContourConvex(approxPolygon)){
            continue;
        }
        if(approxPolygon.size() == 3){
            setLabel(outputImage, "TRIANGLE", contours[i]);
            drawPolygon(outputImage, approxPolygon);
        }
        else if(approxPolygon.size() == 4){
            setLabel(outputImage, "SQUARE", contours[i]);
            drawPolygon(outputImage, approxPolygon);
        }
        else if(approxPolygon.size() == 5){
            setLabel(outputImage, "PENTAGON", contours[i]);
            drawPolygon(outputImage, approxPolygon);
        }
        else if(approxPolygon.size() == 6){
            setLabel(outputImage, "HEXAGON", contours[i]);
            drawPolygon(outputImage, approxPolygon);
        }
        else if(approxPolygon.size() > 15){
            setLabel(outputImage, "CIRCLE", contours[i]);
            drawPolygon(outputImage, approxPolygon);
        }
    }
    saveImage(outputImage, "outputImage.jpg");
    */
}

void MainWindow::detectButtonClicked(){
    QStringList names = QFileDialog::getOpenFileNames();

    for (int i = 0; i < names.count(); i++ ) {
        Mat object = imread( names.at(i).toStdString() );
        std::cout << names.at(i).toStdString() << std::endl;
        Mat resizedImage;
        cv::resize(object, resizedImage, Size(), SCALE_FACTOR, SCALE_FACTOR, INTER_AREA);
        detectObjects(resizedImage);
        printf("Finished O.R. iteration %d", i);
    }
}







