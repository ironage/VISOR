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

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void processFrameAndUpdateGui();
    void openCameraClicked();
    void openImageClicked();
    void stitchImagesClicked();

    
private:
    //For getting frames from camera/video/picture
    cv::VideoCapture capWebcam;
    std::vector<cv::Mat> inputImages;
    int curIndex;
    long int counter;
    Ui::MainWindow *ui;
    QTimer* timer;

};

#endif // MAINWINDOW_H
