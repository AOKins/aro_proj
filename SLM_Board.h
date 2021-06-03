#ifndef SLM_BOARD_
#define SLM_BOARD_

#include <string>

// Class to hold info for an SLM
class SLM_Board {
public:
	bool is_LC_Nematic;
	int imageWidth;
	int imageHeight;

	unsigned char*   FrameOne;
	unsigned char*   FrameTwo;
	unsigned char*   PhaseCompensationData;
	unsigned char*   SystemPhaseCompensationData;
	unsigned char*   LUT;

	std::string LUTFileName;
	std::string PhaseCompensationFileName;
	std::string SystemPhaseCompensationFileName;

	// Defualt constructor
	SLM_Board(){};
	// Constructor
	SLM_Board(bool isNematic, int width, int height);
	// Destructor
	~SLM_Board();
	// Getter for area (image's width*height)
	int GetArea();
};

#endif
