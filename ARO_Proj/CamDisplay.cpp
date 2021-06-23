#include "stdafx.h"
#include "CamDisplay.h"
#include "Utility.h"

#include <opencv2\core\core.hpp>
#include <opencv2\highgui\highgui.hpp>
#include <opencv2\imgproc\imgproc.hpp>

// Constructor
// Display has twice the dimensions of inputted image height and width
CameraDisplay::CameraDisplay(int input_image_height, int input_image_width, std::string display_name) :	//ASK why twice larger?
							port_height_(input_image_height * 2), port_width_(input_image_width * 2), display_name_(display_name) {
	Utility::printLine("INFO: opening display " + display_name + " with - (" + std::to_string(input_image_width) + ", " + std::to_string(input_image_height) + ")");
	display_matrix_ = cv::Mat(port_height_, port_width_, CV_8UC3);
	_isOpened = false;
}

// If display is open, window is destroyed
CameraDisplay::~CameraDisplay() {
	if (_isOpened) {
		cv::destroyWindow(display_name_);
	}
}

void CameraDisplay::OpenDisplay() {
	if (!_isOpened)	{
		cv::namedWindow(display_name_, CV_WINDOW_AUTOSIZE); //Create a window for the display
		imshow(display_name_, display_matrix_);
		cv::waitKey(1);
		_isOpened = true;
		Utility::printLine("INFO: a display has been opened with name - " + display_name_);
	}
}

void CameraDisplay::CloseDisplay() {
	if (_isOpened) {
		cv::destroyWindow(display_name_);
		_isOpened = false;
	}
}

// Update the display contents, if display not open it is also opened
// Input: image - pointer to data that is passed into display_matrix_
void CameraDisplay::UpdateDisplay(unsigned char* image) {
	int kk; // 1D index in display matrix (set for access into image pointer with x,y coordinates)
	for (int ie = 0; ie < (this->port_height_ / 2); ie++)	{
		for (int je = 0; je < (this->port_width_ / 2); je++) {
			kk = ie*(this->port_width_ / 2) + je;
			this->display_matrix_.at<cv::Vec3b>(ie * 2, je * 2)			= cv::Vec3b(image[kk], image[kk], image[kk]);
			this->display_matrix_.at<cv::Vec3b>(ie * 2, je * 2 + 1)		= cv::Vec3b(image[kk], image[kk], image[kk]);
			this->display_matrix_.at<cv::Vec3b>(ie * 2 + 1, je * 2)		= cv::Vec3b(image[kk], image[kk], image[kk]);
			this->display_matrix_.at<cv::Vec3b>(ie * 2 + 1, je * 2 + 1) = cv::Vec3b(image[kk], image[kk], image[kk]);
		}
	}
	if (!_isOpened)	{
		OpenDisplay();
	}
	else {
		imshow(this->display_name_, this->display_matrix_);
		cv::waitKey(1);
	}
}
