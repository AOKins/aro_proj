// [DESCRIPTION]
// Implementation file for the SLM_Board class

#include "stdafx.h"
#include "SLM_Board.h"

// Constructor
SLM_Board::SLM_Board(bool isNematic, int width, int height) {
	this->is_LC_Nematic = isNematic;
	this->imageWidth = width;
	this->imageHeight = height;
	this->powered_On = false;

	int boardArea = width * height;
	//Build paths to the calibrations for this SLM -- regional LUT included in Blink_SDK(), but need to pass NULL to that param to disable ODP. Might need to make a class.
	this->LUTFileName = "linear.LUT";
	this->PhaseCompensationFileName = "slm929_8bit.bmp";
	this->PhaseCompensationData = new unsigned char[boardArea];
}

// Destructor
SLM_Board::~SLM_Board() {
	delete[] this->PhaseCompensationData;
}

// Return area of board image (width*height)
int SLM_Board::GetArea() {
	return this->imageWidth * this->imageHeight;
}

bool SLM_Board::isPoweredOn() {
	return this->powered_On;
}

void SLM_Board::setPower(bool status) {
	this->powered_On = status;
}
