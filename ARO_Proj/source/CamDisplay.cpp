#include "../headers/stdafx.h"
#include "../headers/CamDisplay.h"
#include "../headers/Utility.h"

#include <opencv2\core\core.hpp>
#include <opencv2\highgui\highgui.hpp>
#include <opencv2\imgproc\imgproc.hpp>

#include <string>

using namespace cv;
using std::string;

CameraDisplay::CameraDisplay(int input_image_height, int input_image_width, string display_name) :	//ASK why twice larger?
							port_height_(input_image_height * 2), port_width_(input_image_width * 2), display_name_(display_name) {
	Utility::printLine("INFO: opening display " + display_name + " with - (" + std::to_string(input_image_width) + ", " + std::to_string(input_image_height) + ")");
	display_matrix_ = Mat(port_height_, port_width_, CV_8UC3);
	_isOpened = false;
}

CameraDisplay::~CameraDisplay() {
	if (_isOpened) {
		destroyWindow(display_name_);
	}
}

void CameraDisplay::OpenDisplay() {
	if (!_isOpened)	{
		namedWindow(display_name_, CV_WINDOW_AUTOSIZE); //Create a window for the display
		imshow(display_name_, display_matrix_);
		waitKey(1);
		_isOpened = true;
		Utility::printLine("INFO: a display has been opened with name - " + display_name_);
	}
}

void CameraDisplay::CloseDisplay() {
	if (_isOpened) {
		destroyWindow(display_name_);
		_isOpened = false;
	}
}

void CameraDisplay::UpdateDisplay(unsigned char* image) {
	int kk;
	for (int ie = 0; ie < (port_height_ / 2); ie++)	{
		for (int je = 0; je < (port_width_ / 2); je++) {
			kk = ie*(port_width_ / 2) + je;
			display_matrix_.at<Vec3b>(ie * 2, je * 2) = Vec3b(image[kk], image[kk], image[kk]);
			display_matrix_.at<Vec3b>(ie * 2, je * 2 + 1) = Vec3b(image[kk], image[kk], image[kk]);
			display_matrix_.at<Vec3b>(ie * 2 + 1, je * 2) = Vec3b(image[kk], image[kk], image[kk]);
			display_matrix_.at<Vec3b>(ie * 2 + 1, je * 2 + 1) = Vec3b(image[kk], image[kk], image[kk]);
		}
	}
	if (!_isOpened)	{
		OpenDisplay();
	}
	else {
		imshow(display_name_, display_matrix_);
		waitKey(1);
	}
}
