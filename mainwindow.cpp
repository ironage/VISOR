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
    ui(new Ui::MainWindow), stitcher(NULL), saveImageCounter(0), lastData(NULL), lastResult(NULL)
{
    ui->setupUi(this);
    ui->groupBox->hide();
    ui->groupBox_IS->hide();

    qRegisterMetaType<StitchingUpdateData*>();   // Allows us to use the custom class in signals/slots

    connect(ui->stitchButton, SIGNAL(clicked()), this, SLOT(stitchImagesClicked()));
    connect(ui->detectButton, SIGNAL(clicked()), this, SLOT(detectButtonClicked()));
    connect(ui->button_IS_select, SIGNAL(clicked()), this, SLOT(startImageStitchingClicked()));
    connect(ui->saveImageButton, SIGNAL(clicked()), this, SLOT(saveCurrentImage()));
    connect(ui->slider_gaussian_sd, SIGNAL(valueChanged(int)), this, SLOT(gaussianSdChanged(int)));
    connect(ui->slider_canny_low, SIGNAL(valueChanged(int)), this, SLOT(cannyLowChanged(int)));
    connect(ui->slider_canny_high, SIGNAL(valueChanged(int)), this, SLOT(cannyHighChanged(int)));
    connect(ui->slider_hough_vote, SIGNAL(valueChanged(int)), this, SLOT(houghVoteChanged(int)));
    connect(ui->slider_hough_minLength, SIGNAL(valueChanged(int)), this, SLOT(houghMinLengthChanged(int)));
    connect(ui->slider_hough_minDistance, SIGNAL(valueChanged(int)), this, SLOT(houghMinDistanceChanged(int)));
    connect(ui->slider_IS_resize, SIGNAL(valueChanged(int)), this, SLOT(IS_scaleChanged(int)));
    connect(ui->radioButtonMatches, SIGNAL(clicked()), this, SLOT(IS_radioButtonChanged()));
    connect(ui->radioButtonScene, SIGNAL(clicked()), this, SLOT(IS_radioButtonChanged()));
    connect(ui->radio_or_canny, SIGNAL(clicked()), this, SLOT(displayRecognitionResult()));
    connect(ui->radio_or_canny2, SIGNAL(clicked()), this, SLOT(displayRecognitionResult()));
    connect(ui->radio_or_gaussian, SIGNAL(clicked()), this, SLOT(displayRecognitionResult()));
    connect(ui->radio_or_hough, SIGNAL(clicked()), this, SLOT(displayRecognitionResult()));
    connect(ui->radio_or_input, SIGNAL(clicked()), this, SLOT(displayRecognitionResult()));
    connect(ui->radio_or_output, SIGNAL(clicked()), this, SLOT(displayRecognitionResult()));

    // set initial values
    ui->label_gaussian_sd->setText(QString::number(getGaussianBlurValue()));
    ui->label_canny_low->setText(QString::number(ui->slider_canny_low->value()));
    ui->label_canny_high->setText(QString::number(ui->slider_canny_high->value()));
    ui->label_hough_vote->setText(QString::number(ui->slider_hough_vote->value()));
    ui->label_hough_minLength->setText(QString::number(ui->slider_hough_minLength->value()));
    ui->label_hough_minDistance->setText(QString::number(ui->slider_hough_minDistance->value()));
    objectRecognizer.gaussianSD = getGaussianBlurValue();
    objectRecognizer.cannyLow = ui->slider_canny_low->value();
    objectRecognizer.cannyHigh = ui->slider_canny_high->value();
    objectRecognizer.houghVote = ui->slider_hough_vote->value();
    objectRecognizer.houghMinLength = ui->slider_hough_minLength->value();
    objectRecognizer.houghMinDistance = ui->slider_hough_minDistance->value();

}

MainWindow::~MainWindow()
{
    if (lastData) {
        delete lastData;
    }
    if (lastResult) {
        delete lastResult;
    }
    delete ui;
}

void MainWindow::saveCurrentImage() {
    //if (ui->display->pixmap() == NULL || ui->display->pixmap()->isNull()) return;
    QString fileName = QString("save") + QString::number(saveImageCounter) + ".jpg";
    saveImageCounter++;
    ui->display->getImage().save(fileName);
    std::cout << "saved image " << fileName.toStdString() << std::endl;
}

void MainWindow::displayImage(cv::Mat& image) {
    cvtColor(image, image,CV_BGR2RGB);
    QImage qimgOrig((uchar*)image.data, image.cols, image.rows, image.step, QImage::Format_RGB888);
    ui->display->setImage(qimgOrig);
    cvtColor(image, image, CV_RGB2BGR);
}

void MainWindow::IS_scaleChanged(int value) {
    ui->label_IS_resize->setText(QString::number(value));
    // value from the slider is read directly when creating the stitcher object
}

void MainWindow::IS_radioButtonChanged() {
    if (lastData) {
        if (ui->radioButtonMatches->isChecked()) {
            displayImage(lastData->currentFeatureMatches);
        } else {
            displayImage(lastData->currentScene);
        }
    }
}

void MainWindow::displayRecognitionResult() {
    if (lastResult == NULL) return;
    if (ui->radio_or_canny->isChecked()) {
        displayImage(lastResult->canny);
    } else if (ui->radio_or_canny2->isChecked()) {
        displayImage(lastResult->canny2);
    } else if (ui->radio_or_gaussian->isChecked()) {
        displayImage(lastResult->gaussianBlur);
    } else if (ui->radio_or_hough->isChecked()) {
        displayImage(lastResult->hough);
    } else if (ui->radio_or_input->isChecked()) {
        displayImage(lastResult->hough);
    } else if (ui->radio_or_input->isChecked()) {
        displayImage(lastResult->input);
    } else if (ui->radio_or_output->isChecked()) {
        displayImage(lastResult->output);
    }
}

void MainWindow::stitchingUpdate(StitchingUpdateData* data) {

    displayImage(data->currentScene);
    if (data->totalImages > 0) {
        ui->progressBar->setValue(((double)data->curIndex)/ data->totalImages * 100);
    }
    ui->label_IS_progress->setText(QString::number(data->curIndex) + "/" + QString::number(data->totalImages));
    //saveImage(data->currentScene, "resultAfter.png");
    if (lastData) {
        delete lastData;
    }
    lastData = data;
}

void MainWindow::startImageStitchingClicked() {
    QStringList inputFiles = QFileDialog::getOpenFileNames();
    if (inputFiles.size() < 2) return;    // don't crash on one input image

    if (stitcher) {
        stitcher->terminate();    //possibly risky way to terminate an already running stitcher
        delete stitcher;
    }
    //TODO make these take input instead of just being constant
    stitcher = new ImageStitcher(inputFiles, ui->slider_IS_resize->value() / 100.0, 1.25, 1, 1.5, 3, ImageStitcher::SURF, ImageStitcher::BRUTE_FORCE);
    connect(stitcher, SIGNAL(stitchingUpdate(StitchingUpdateData*)), this, SLOT(stitchingUpdate(StitchingUpdateData*)));
    stitcher->start();
}

void MainWindow::stitchImagesClicked() {
    ui->groupBox->hide();
    ui->groupBox_IS->show();
    ui->progressBar->setValue(0);
    ui->display->scene()->clear();
    ui->label_IS_progress->setText("");
    ui->label_IS_resize->setText(QString::number(ui->slider_IS_resize->value()));
}

void MainWindow::detectObjects() {
    ui->groupBox_IS->hide();
    ui->groupBox->show();
    ui->display->scene()->clear();
    RecognizerResults* results = objectRecognizer.recognizeObjects();
    if (lastResult != NULL) {
        delete lastResult;
    }
    lastResult = results;
    displayRecognitionResult();
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

int MainWindow::getGaussianBlurValue() {
    int value = ui->slider_gaussian_sd->value();
    if (value % 2 == 0) ++value;   // always has to be odd
    return value;
}

void MainWindow::gaussianSdChanged(int value) {
    int oddValue = getGaussianBlurValue();
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







