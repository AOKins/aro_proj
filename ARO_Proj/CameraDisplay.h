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
	// Constructor
	// Display has twice the dimensions of inputted image height and width
	CameraDisplay(int input_image_height, int input_image_width, std::string display_name);

	// If display is open, window is destroyed
	~CameraDisplay();
	
	// Open the window display with set window dimensions
	void OpenDisplay(int window_width, int window_height);
	// Resize the display with set window dimensions
	void resizeDisplay(int new_width, int new_height);

	void CloseDisplay();

	// Update the display contents, if display not open it is also opened
	// Input: image - pointer to data that is passed into display_matrix_
	void UpdateDisplay(unsigned char* image);
};

#endif
