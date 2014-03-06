#include "mainwindow.h"
#include "ui_mainwindow.h"
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
// obj is the small image
// scene is the mosiac
void stitchImages(Mat &objImage, Mat &sceneImage ) {

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
    double max_dist = 0; double min_dist = 100;

    //-- Quick calculation of max and min distances between keypoints
    for( int i = 0; i < descriptors_object.rows; i++ ) {
        double dist = matches[i].distance;
        if( dist < min_dist ) min_dist = dist;
        if( dist > max_dist ) max_dist = dist;
    }

    printf("-- Max dist : %f \n", max_dist );
    printf("-- Min dist : %f \n", min_dist );

    //-- Use only "good" matches (i.e. whose distance is less than 3*min_dist )
    std::vector< DMatch > good_matches;

    for( int i = 0; i < descriptors_object.rows; i++ ) {
         if( matches[i].distance < 3*min_dist ) {
             good_matches.push_back( matches[i]);
         }
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

    for(int i=0; i<points.size(); i++){
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







