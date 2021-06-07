// [DESCRIPTION]
// Implementation file for the SLM_Board class

#include "stdafx.h"
#include <string>
#include "SLM_Board.h"

SLM_Board::SLM_Board(bool isNematic, int width, int height) {
	is_LC_Nematic = isNematic;
	imageWidth = width;
	imageHeight = height;
	LUTFileName = "";
	PhaseCompensationFileName = "";
	SystemPhaseCompensationFileName = "";
}

SLM_Board::~SLM_Board() {
	delete[] FrameOne;
	delete[] FrameTwo;
	delete[] PhaseCompensationData;
	delete[] SystemPhaseCompensationData;
	delete[] LUT;
}

int SLM_Board::GetArea() {
	return imageWidth * imageHeight;
}
