#ifndef SLM_BOARD_
#define SLM_BOARD_

#include <string>

// Class to hold info for an SLM
class SLM_Board {
public:
	bool is_LC_Nematic;
	int imageWidth;
	int imageHeight;
	// Track if this board has been powered on or not (default start with false)
	bool powered_On;

	// Filepath to LUT file being used
	std::string LUTFileName;

	// Defualt constructor does not initialize anything
	SLM_Board();
	// Constructor
	SLM_Board(bool isNematic, int width, int height);
	// Getter for area (image's width*height)
	int GetArea();
	// Getter for if powered
	bool isPoweredOn();
	// Setter for if powered (should be only done within SLM_Controller!)
	void setPower(bool status);
};

#endif
