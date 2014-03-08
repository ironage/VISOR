#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

namespace Ui {
class MainWindow;
}

struct ObjectRecognitionData {
    cv::Mat image;
    int gaussianSD;
    int cannyLow;
    int cannyHigh;
    int houghVote;
    int houghMinLength;
    int houghMinDistance;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void displayImage(cv::Mat& image);
    void processFrameAndUpdateGui();
    void openCameraClicked();
    void openImageClicked();
    void stitchImagesClicked();
    void detectButtonClicked();
    void gaussianSdChanged(int value);
    void cannyLowChanged(int value);
    void cannyHighChanged(int value);
    void houghVoteChanged(int value);
    void houghMinLengthChanged(int value);
    void houghMinDistanceChanged(int value);
    
private:
    void detectObjects(ObjectRecognitionData data);
    //For getting frames from camera/video/picture
    cv::VideoCapture capWebcam;
    std::vector<cv::Mat> inputImages;
    int curIndex;
    long int counter;
    Ui::MainWindow *ui;
    QTimer* timer;
    ObjectRecognitionData objectRecognitionData;

};

#endif // MAINWINDOW_H
