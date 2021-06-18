#ifndef GRAPHICS_DISPLAY_H_
#define GRAPHICS_DISPLAY_H_
#include <opencv2\core\core.hpp>
#include <opencv2\highgui\highgui.hpp>
#include <opencv2\imgproc\imgproc.hpp>
#include <string>
#include "Utility.h"

class CameraDisplay {
private:
	int port_height_;
	int port_width_;
	cv::Mat display_matrix_;
	std::string display_name_;
	bool _isOpened;
public:
	CameraDisplay(int input_image_height, int input_image_width, std::string display_name);
	~CameraDisplay();
	void OpenDisplay();
	void CloseDisplay();
	void UpdateDisplay(unsigned char* image);
};

#endif
