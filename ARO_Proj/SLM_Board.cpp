// [DESCRIPTION]
// Implementation file for the SLM_Board class

#include "stdafx.h"
#include "SLM_Board.h"

// Constructor
SLM_Board::SLM_Board(bool isNematic, int width, int height) {
	this->is_LC_Nematic = isNematic;
	this->imageWidth = width;
	this->imageHeight = height;
	this->LUTFileName = "";
	this->PhaseCompensationFileName = "";
	this->SystemPhaseCompensationFileName = "";
}

// Destructor
SLM_Board::~SLM_Board() {
	delete[] this->PhaseCompensationData;
	delete[] this->SystemPhaseCompensationData;
	delete[] this->LUT;
}

// Return area of board image (width*height)
int SLM_Board::GetArea() {
	return this->imageWidth * this->imageHeight;
}
