#include "DetectMarker.h"
#include "parameters.h"


#include <iostream>

#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/aruco.hpp>


void DetectMarker(cv::Mat& inputImage, std::vector<cv::Vec3d>& translationVectors, std::vector<cv::Mat>& rotationMatrixs, std::vector<int>& markerIds)
{
	// new para used in detect
	//std::vector< int > markerIds;
	std::vector< std::vector< cv::Point2f > > markerCorners, rejectedCandidates;
	cv::Ptr<cv::aruco::DetectorParameters> parameters = new cv::aruco::DetectorParameters;
	std::vector< cv::Vec3d > rvecs, tvecs;

	// choose dictionary
	cv::Ptr<cv::aruco::Dictionary> dictionary = cv::aruco::getPredefinedDictionary(cv::aruco::DICT_4X4_50);

	// detect marker
	cv::aruco::detectMarkers(inputImage, dictionary, markerCorners, markerIds, parameters, rejectedCandidates);


	if (!markerIds.empty()) {
		// draw marker
		cv::aruco::drawDetectedMarkers(inputImage, markerCorners, markerIds);

		// detect pose 
		cv::aruco::estimatePoseSingleMarkers(markerCorners, MARKER_LENGTH, cameraMatrix, distCoeffs, rvecs, tvecs);
	}

	//IDs = markerIds;
	for (int i = 0; i < markerIds.size(); i++)
	{
		// draw axis 
		cv::aruco::drawAxis(inputImage, cameraMatrix, distCoeffs, rvecs[i], tvecs[i], MARKER_LENGTH);

		//std::cout << rvecs[i] << std::endl;
		cv::Vec3d rotationVector = rvecs[i];
		cv::Vec3d translationVector = tvecs[i];

		// Transform a rotation vector to a rotation matrix
		cv::Mat rotationMatrix;
		cv::Rodrigues(rotationVector, rotationMatrix);
		//std::cout << translationVector << std::endl;

		translationVectors.push_back(translationVector);
		rotationMatrixs.push_back(rotationMatrix);

	}
}