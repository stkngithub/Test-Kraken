#pragma once

#include <vector>

#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/aruco.hpp>

#define MARKER_LENGTH 0.02f

// The intrinsic parameters  (3x3 matrix)
static const cv::Mat cameraMatrix = (cv::Mat_<float>(3, 3) << 6.7987358758420328e+02f, 0.0f, 3.1950000000000000e+02f,
														0.0f, 6.7987358758420328e+02f, 2.3950000000000000e+02f,
														0.0f, 0.0f, 1.0f);

// The distortion coefficients  (1x5 vector)
static const cv::Mat distCoeffs = (cv::Mat_<float>(1, 5) << -9.4532608901286876e-03f, -9.4759037803977220e-01f, 0.0f, 0.0f, 2.7752296281859339e+00f);

