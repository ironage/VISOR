#include "imagestitcher.h"
#include "sharedfunctions.h"

#include <opencv2/opencv.hpp>
#include <opencv2/stitching/stitcher.hpp>
#include "opencv2/features2d/features2d.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/nonfree/nonfree.hpp"
#include "opencv2/calib3d/calib3d.hpp"
#include "opencv2/imgproc/imgproc.hpp"

#include <QImage>
#include <fstream>


using namespace cv;

StitchingUpdateData* stitchImages(Mat &objImage, Mat &sceneImage);

StitchingUpdateData::StitchingUpdateData() : QObject(NULL)
{
}

ImageStitcher::ImageStitcher(QStringList inputFiles,
                             double scaleFactor, double roiSize, double angleStdDevs, double lenStdDevs, double distMins,
                             ImageStitcher::FeatureDetector featureDetector, ImageStitcher::FeatcherMatcher featureMatcher,
                             QObject *parent) :
    QThread(parent), inputFiles(inputFiles), SCALE_FACTOR(scaleFactor), ROI_SIZE(roiSize), STD_ANGLE_DEVS_TO_KEEP(angleStdDevs),
    STD_LEN_DEVS_TO_KEEP(lenStdDevs), NUM_MIN_DIST_TO_KEEP(distMins), F_DETECTOR(featureDetector), F_MATCHER(featureMatcher)
{
}

void ImageStitcher::run() {

    cv::Mat result = imread(inputFiles.at(0).toStdString());
    cv::resize(result, result, Size(), SCALE_FACTOR, SCALE_FACTOR, INTER_AREA);

    for (int i = 1; i < inputFiles.count(); i++ ) {
        cv::Mat object = imread( inputFiles.at(i).toStdString() );
        cv::Mat smallObject;
        cv::resize(object, smallObject, Size(), SCALE_FACTOR, SCALE_FACTOR, INTER_AREA);
        cv::Mat scene; result.copyTo(scene);
        StitchingUpdateData* update = stitchImages(smallObject, scene);
        if( !update->success ) {
            return;
        }
        update->currentScene.copyTo(result);
        update->curIndex = i + 1;
        update->totalImages = inputFiles.size();
        emit stitchingUpdate(update);
        printf("Finished I.S. iteration %d\n", i);
    }
}


cv::Rect roi = cv::Rect(0, 0, 0, 0);

// obj is the small image
// scene is the mosiac
StitchingUpdateData* ImageStitcher::stitchImages(Mat &objImage, Mat &sceneImage) {
    StitchingUpdateData* updateData = new StitchingUpdateData();
    updateData->success = true;
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

        int newWidth = roi.width * ROI_SIZE;
        roi.x = roi.x - ((newWidth - roi.width) / 2);
        roi.width = newWidth;
        int newHeight = roi.height * ROI_SIZE;
        roi.y = roi.y - ((newHeight - roi.height) / 2);
        roi.height = newHeight;

        if (roi.x < 0) roi.x = 0;
        if (roi.y < 0) roi.y = 0;
        if (roi.x + roi.width  >= paddedScene.cols) roi.width  = paddedScene.cols - roi.x;
        if (roi.y + roi.height >= paddedScene.rows) roi.height = paddedScene.rows - roi.y;

        //cv::rectangle(paddedScene, roi, Scalar(255, 0, 0), 3, CV_AA);
        //saveImage(paddedScene, "ROIshifted.png");
        //std::cout << " ROI " << std::endl << roi << std::endl;
    } else {
        roi = cv::Rect(0, 0, grayPadded.cols, grayPadded.rows); // If not set then use the whole image.
    }
    Mat roiPointer = grayPadded(roi);

    std::vector< KeyPoint > keypoints_object, keypoints_scene;
    Mat descriptors_object, descriptors_scene;

    switch( F_DETECTOR ) {
        case ImageStitcher::SURF: {
            // Detect the keypoints using SURF Detector
            int minHessian = 400;
            SurfFeatureDetector detector( minHessian );
            detector.detect( grayObjImage, keypoints_object );
            detector.detect( roiPointer,   keypoints_scene );

            // Calculate descriptors (feature vectors)
            SurfDescriptorExtractor extractor;
            extractor.compute( grayObjImage, keypoints_object, descriptors_object );
            extractor.compute( roiPointer,   keypoints_scene,  descriptors_scene );
            break;
        }
        case ImageStitcher::ORB: {
            cv::ORB orb(5000); // max features default is 500
            orb( grayObjImage, Mat(), keypoints_object, descriptors_object );
            orb( roiPointer,   Mat(), keypoints_scene,  descriptors_scene );
            break;
        }
    }

    std::vector< DMatch > matches;
    switch( F_MATCHER ) {
        case ImageStitcher::FLANN: {
            // Match descriptor vectors using FLANN matcher
            FlannBasedMatcher matcher;
            matcher.match( descriptors_object, descriptors_scene, matches );
            break;
        }
        case ImageStitcher::BRUTE_FORCE: {
            int normType = F_DETECTOR == ImageStitcher::ORB ? NORM_HAMMING : NORM_L2;
            BFMatcher matcher(normType);
            matcher.match( descriptors_object, descriptors_scene, matches );
            break;
        }
    }

    // Use only "good" matches
    // find mean and stddev of magnitude
    // and only take matches within a certain number of stddevs
    std::vector< DMatch > good_matches;

    // first find the means and the minimum openCV heuristic distance
    double distanceMin  = 100.0;
    double angleMean  = 0.0;
    double lengthsMean = 0.0;
    std::vector< double > angles;
    std::vector< double > lengths;

    for( unsigned i = 0; i < matches.size(); i++ ) {
        // distance is opencv score so best match is the minimum value
        if( matches[i].distance < distanceMin ) distanceMin = matches[i].distance;

        //calculate the angle of the match and store it (to save doing calculation again)
        double x1 = keypoints_object[matches[i].queryIdx].pt.x;
        double y1 = keypoints_object[matches[i].queryIdx].pt.y;
        double x2 = keypoints_scene [matches[i].trainIdx].pt.x;
        double y2 = keypoints_scene [matches[i].trainIdx].pt.y;
        double angle = atan2(y2-y1,x2-x1);
        angles.push_back(angle);
        angleMean += angle;

        //calculate the euclidian distance between the features
        double euDistance = std::sqrt(std::pow(x1 - x2, 2) + std::pow(y1 - y2, 2));
        lengths.push_back(euDistance);
        lengthsMean += euDistance;

        //lengthsFile << matches[i].distance << std::endl;
        //anglesFile  << angle << std::endl;
    }
    angleMean    /= matches.size();
    lengthsMean  /= matches.size();

    lengthsFile << "---------------------------------------" << std::endl;
    anglesFile  << "---------------------------------------" << std::endl;

    // next find the standard deviations
    double angleStdDev    = 0.0;
    double lengthsStdDev  = 0.0;

    for( unsigned i = 0; i < matches.size(); i++ ) {
        angleStdDev    += (angles[i]  - angleMean)    * (angles[i]  - angleMean);
        lengthsStdDev  += (lengths[i] - lengthsMean)  * (lengths[i] - lengthsMean);
    }
    angleStdDev    /= matches.size();
    angleStdDev     = sqrt(angleStdDev);
    lengthsStdDev  /= matches.size();
    lengthsStdDev   = sqrt(lengthsStdDev);

    std::cout << "Angle mean = "  << angleMean   << " stddev = " << angleStdDev   << std::endl;
    std::cout << "Length mean = " << lengthsMean << " stddev = " << lengthsStdDev << std::endl;

    // finally prune the matches based off of stddev
    for( unsigned i = 0; i < matches.size(); i++ ) {
        if( matches[i].distance > NUM_MIN_DIST_TO_KEEP*distanceMin) {
            // length is out of std dev range don't add to list of good values;
            continue;
        }
        if( angles[i] > angleMean + angleStdDev*STD_ANGLE_DEVS_TO_KEEP || //*STD_DEVS_TO_KEEP ||
            angles[i] < angleMean - angleStdDev*STD_ANGLE_DEVS_TO_KEEP ){//*STD_DEVS_TO_KEEP ) {
            // angle is out of std dev range
            continue;
        }
        if ( lengths[i] > lengthsMean + lengthsStdDev*STD_LEN_DEVS_TO_KEEP ||
             lengths[i] < lengthsMean - lengthsStdDev*STD_LEN_DEVS_TO_KEEP) {
            // euculidian distance is out of std dev range
            continue;
        }
        // point passed tests adding to good matches
        good_matches.push_back(matches[i]);
    }

    // need at least 4 matches to do homography
    if( good_matches.size() < 4 ) {
        updateData->success = false;
        std::cout << "Fatal error detector did not find 4 good matches I.S cannot proceed" << std::endl;
        return updateData;
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
    img_matches.copyTo(updateData->currentFeatureMatches);
    //saveImage(img_matches, "matches.png");

    // Find the Homography Matrix
    Mat H = findHomography( obj, scene, CV_RANSAC );

    std::cout << "Homography Mat" << std::endl << H << std::endl;

    // Use the Homography Matrix to warp the images
    H.row(0).col(2) += roi.x;   // Add roi offset coordinates to translation component
    H.row(1).col(2) += roi.y;
    Mat result;
    warpPerspective(objImage,result,H,cv::Size(paddedScene.cols,paddedScene.rows));
    // result now contains the rotated/skewed/translated object image
    // this is our ROI on the next step
    roi = SharedFunctions::findBoundingBox(result);

    //cv::rectangle(result, roi, Scalar(255, 255, 255), 3, CV_AA);
    //saveImage(result, "ROI.png");

    // warpedObjDest is now a reference into the paddedScene where we are going to place the object
    //cv::Mat warpedObjDest(paddedScene,cv::Rect(0,0,paddedScene.cols,paddedScene.rows));
    // Find the non zero parts of the warped image
    cv::Mat mask = result > 0;
    result.copyTo(paddedScene,mask);
    // copy that on top of the scene
    result = paddedScene;
    cv::Rect crop = SharedFunctions::findBoundingBox(result);
    roi.x -= crop.x;
    roi.y -= crop.y;
    result = result(crop);
    std::cout << "result total: " << result.total() << "\n";
    result.copyTo(updateData->currentScene);
    return updateData;
}
