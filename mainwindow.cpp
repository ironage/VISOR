#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <fstream>
#include <cmath>
#include <QFileDialog>

#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/stitching/stitcher.hpp>
#include "opencv2/features2d/features2d.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/nonfree/nonfree.hpp"
#include "opencv2/calib3d/calib3d.hpp"
#include "opencv2/imgproc/imgproc.hpp"

using cv::Mat;
using namespace cv;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow), stitcher(NULL)
{
    ui->setupUi(this);
    qRegisterMetaType<StitchingUpdateData*>();   // Allows us to use the custom class in signals/slots

    ui->display->setScaledContents(true);

    connect(ui->stitchButton, SIGNAL(clicked()), this, SLOT(stitchImagesClicked()));
    connect(ui->detectButton, SIGNAL(clicked()), this, SLOT(detectButtonClicked()));
    connect(ui->slider_gaussian_sd, SIGNAL(valueChanged(int)), this, SLOT(gaussianSdChanged(int)));
    connect(ui->slider_canny_low, SIGNAL(valueChanged(int)), this, SLOT(cannyLowChanged(int)));
    connect(ui->slider_canny_high, SIGNAL(valueChanged(int)), this, SLOT(cannyHighChanged(int)));
    connect(ui->slider_hough_vote, SIGNAL(valueChanged(int)), this, SLOT(houghVoteChanged(int)));
    connect(ui->slider_hough_minLength, SIGNAL(valueChanged(int)), this, SLOT(houghMinLengthChanged(int)));
    connect(ui->slider_hough_minDistance, SIGNAL(valueChanged(int)), this, SLOT(houghMinDistanceChanged(int)));
    // set initial values
    ui->label_gaussian_sd->setText(QString::number(ui->slider_gaussian_sd->value()));
    ui->label_canny_low->setText(QString::number(ui->slider_canny_low->value()));
    ui->label_canny_high->setText(QString::number(ui->slider_canny_high->value()));
    ui->label_hough_vote->setText(QString::number(ui->slider_hough_vote->value()));
    ui->label_hough_minLength->setText(QString::number(ui->slider_hough_minLength->value()));
    ui->label_hough_minDistance->setText(QString::number(ui->slider_hough_minDistance->value()));
    objectRecognitionData.gaussianSD = ui->slider_gaussian_sd->value();
    objectRecognitionData.cannyLow = ui->slider_canny_low->value();
    objectRecognitionData.cannyHigh = ui->slider_canny_high->value();
    objectRecognitionData.houghVote = ui->slider_hough_vote->value();
    objectRecognitionData.houghMinLength = ui->slider_hough_minLength->value();
    objectRecognitionData.houghMinDistance = ui->slider_hough_minDistance->value();

}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::displayImage(cv::Mat& image) {
    cvtColor(image, image,CV_BGR2RGB);
    QImage qimgOrig((uchar*)image.data, image.cols, image.rows, image.step, QImage::Format_RGB888);
    ui->display->setPixmap(QPixmap::fromImage(qimgOrig));
}

void saveImage(Mat &image, QString name) {
    cvtColor(image, image,CV_BGR2RGB);
    QImage qimgOrig((uchar*)image.data, image.cols, image.rows, image.step, QImage::Format_RGB888);
    qimgOrig.save(name);
    cvtColor(image, image, CV_RGB2BGR);
}

void MainWindow::stitchingUpdate(StitchingUpdateData* data) {

    displayImage(data->currentScene);
    //saveImage(data->currentScene, "resultAfter.png");
    delete data;

}

void MainWindow::stitchImagesClicked() {

    QStringList inputFiles = QFileDialog::getOpenFileNames();
    if (inputFiles.size() < 2) return;    // don't crash on one input image

    if (stitcher) {
        stitcher->terminate();    //possibly risky way to terminate
        delete stitcher;
    }
    stitcher = new ImageStitcher(inputFiles);
    connect(stitcher, SIGNAL(stitchingUpdate(StitchingUpdateData*)), this, SLOT(stitchingUpdate(StitchingUpdateData*)));
    stitcher->start();
    /*

    Mat object = imread( names.at(1).toStdString() );
    Mat scene  = imread( names.at(0).toStdString() );
    Mat smallObject, smallScene;
    cv::resize(object, smallObject, Size(), SCALE_FACTOR, SCALE_FACTOR, INTER_AREA);
    cv::resize(scene,  smallScene,  Size(), SCALE_FACTOR, SCALE_FACTOR, INTER_AREA);
    Mat result = stitchImages(smallObject, smallScene );

    for (int i = 2; i < names.count(); i++ ) {
        Mat object = imread( names.at(i).toStdString() );
        Mat smallObject;
        cv::resize(object, smallObject, Size(), SCALE_FACTOR, SCALE_FACTOR, INTER_AREA);
        Mat scene; result.copyTo(scene);
        QFuture<cv::Mat> future = QtConcurrent::run(stitchImages, smallObject, scene);

        result = future.result();
        printf("Finished I.S. iteration %d\n", i);
    }

    saveImage(result, "output.png"); */
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

void MainWindow::detectObjects(ObjectRecognitionData data) {
    Mat inputImage = data.image;
    if (inputImage.empty()) return; // otherwise it will crash.
    //saveImage(inputImage, "01_inputImage.jpg");

    /// input image size
    cv::Size imageSize = inputImage.size();

    /// Blur Image
    cv::Mat blurImage;
    cv::GaussianBlur(inputImage, blurImage, cv::Size(data.gaussianSD, data.gaussianSD), 0, 0);
    //saveImage(blurImage, "02_blurImage.jpeg");

    /// Convert to Grayscale
    cv::Mat grayImage;
    cv::cvtColor(blurImage, grayImage, CV_BGR2GRAY);

    /// Canny Edge Detector
    cv::Mat bwImage;
    cv::Canny(grayImage, bwImage, data.cannyLow, data.cannyHigh);

    /// save Canny Edge Image
    cv::Mat cannyImage;
    inputImage.copyTo(cannyImage, bwImage);  // bwImage is our mask
    //saveImage(cannyImage, "03_cannyImage.jpg");

    /// output image
    cv::Mat outputImage;
    cv::Mat binaryImage(imageSize, CV_8UC1);
    inputImage.copyTo(outputImage);
    vector<Vec4i> lines;
    HoughLinesP(bwImage, lines, 1, CV_PI/180, data.houghVote, data.houghMinLength, data.houghMinDistance);
    for( size_t i = 0; i < lines.size(); i++ )
    {
      Vec4i l = lines[i];
      line( outputImage, Point(l[0], l[1]), Point(l[2], l[3]), Scalar(0,0,255), 3, CV_AA);
      //line( binaryImage, Point(l[0], l[1]), Point(l[2], l[3]), Scalar(0,0,255), 3, CV_AA);
    }
    ///saveImage(outputImage, "04_outputImage.jpg");
    saveImage(outputImage, "05_binaryImage.jpg");
    displayImage(outputImage);

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

const double OR_SCALE_FACTOR = 0.5;

void MainWindow::detectButtonClicked(){
    QStringList names = QFileDialog::getOpenFileNames();

    for (int i = 0; i < names.count(); i++ ) {
        Mat object = imread( names.at(i).toStdString() );
        std::cout << names.at(i).toStdString() << std::endl;
        cv::resize(object, objectRecognitionData.image, Size(), OR_SCALE_FACTOR, OR_SCALE_FACTOR, INTER_AREA);
        detectObjects(objectRecognitionData);
        printf("Finished O.R. iteration %d", i);
    }
}

void MainWindow::gaussianSdChanged(int value) {
    int oddValue = (2 * value) + 1;
    ui->label_gaussian_sd->setText(QString::number(oddValue));
    objectRecognitionData.gaussianSD = oddValue;
    detectObjects(objectRecognitionData);
}

void MainWindow::cannyLowChanged(int value) {
    ui->label_canny_low->setText(QString::number(value));
    objectRecognitionData.cannyLow = value;
    detectObjects(objectRecognitionData);
}

void MainWindow::cannyHighChanged(int value) {
    ui->label_canny_high->setText(QString::number(value));
    objectRecognitionData.cannyHigh = value;
    detectObjects(objectRecognitionData);
}

void MainWindow::houghVoteChanged(int value) {
    ui->label_hough_vote->setText(QString::number(value));
    objectRecognitionData.houghVote = value;
    detectObjects(objectRecognitionData);
}

void MainWindow::houghMinLengthChanged(int value) {
    ui->label_hough_minLength->setText(QString::number(value));
    objectRecognitionData.houghMinLength = value;
    detectObjects(objectRecognitionData);
}

void MainWindow::houghMinDistanceChanged(int value) {
    ui->label_hough_minDistance->setText(QString::number(value));
    objectRecognitionData.houghMinDistance = value;
    detectObjects(objectRecognitionData);

}







