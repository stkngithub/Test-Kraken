#pragma once

#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/aruco.hpp>

void DetectMarker(cv::Mat& inputImage, std::vector<cv::Vec3d>& translationVectors, std::vector<cv::Mat>& rotationMatrixs, std::vector< int >& markerIds);