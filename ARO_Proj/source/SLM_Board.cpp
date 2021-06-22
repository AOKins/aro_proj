// [DESCRIPTION]
// Implementation file for the SLM_Board class

#include "../headers/stdafx.h"
#include "../headers/SLM_Board.h"

#include <string>

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
