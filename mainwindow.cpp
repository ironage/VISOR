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
    objectRecognizer.gaussianSD = ui->slider_gaussian_sd->value();
    objectRecognizer.cannyLow = ui->slider_canny_low->value();
    objectRecognizer.cannyHigh = ui->slider_canny_high->value();
    objectRecognizer.houghVote = ui->slider_hough_vote->value();
    objectRecognizer.houghMinLength = ui->slider_hough_minLength->value();
    objectRecognizer.houghMinDistance = ui->slider_hough_minDistance->value();

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
        stitcher->terminate();    //possibly risky way to terminate an already running stitcher
        delete stitcher;
    }
    stitcher = new ImageStitcher(inputFiles);
    connect(stitcher, SIGNAL(stitchingUpdate(StitchingUpdateData*)), this, SLOT(stitchingUpdate(StitchingUpdateData*)));
    stitcher->start();
}

void MainWindow::detectObjects() {
    Mat output = objectRecognizer.recognizeObjects();
    displayImage(output);
}

const double OR_SCALE_FACTOR = 0.5;

void MainWindow::detectButtonClicked(){
    QStringList names = QFileDialog::getOpenFileNames();

    for (int i = 0; i < names.count(); i++ ) {
        Mat object = imread( names.at(i).toStdString() );
        std::cout << names.at(i).toStdString() << std::endl;
        cv::resize(object, objectRecognizer.inputImage, Size(), OR_SCALE_FACTOR, OR_SCALE_FACTOR, INTER_AREA);
        detectObjects();
        printf("Finished O.R. iteration %d", i);
    }
}

void MainWindow::gaussianSdChanged(int value) {
    int oddValue = (2 * value) + 1;
    ui->label_gaussian_sd->setText(QString::number(oddValue));
    objectRecognizer.gaussianSD = oddValue;
    detectObjects();
}

void MainWindow::cannyLowChanged(int value) {
    ui->label_canny_low->setText(QString::number(value));
    objectRecognizer.cannyLow = value;
    detectObjects();
}

void MainWindow::cannyHighChanged(int value) {
    ui->label_canny_high->setText(QString::number(value));
    objectRecognizer.cannyHigh = value;
    detectObjects();
}

void MainWindow::houghVoteChanged(int value) {
    ui->label_hough_vote->setText(QString::number(value));
    objectRecognizer.houghVote = value;
    detectObjects();
}

void MainWindow::houghMinLengthChanged(int value) {
    ui->label_hough_minLength->setText(QString::number(value));
    objectRecognizer.houghMinLength = value;
    detectObjects();
}

void MainWindow::houghMinDistanceChanged(int value) {
    ui->label_hough_minDistance->setText(QString::number(value));
    objectRecognizer.houghMinDistance = value;
    detectObjects();

}







