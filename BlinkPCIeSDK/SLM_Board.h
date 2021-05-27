#ifndef SLM_BOARD_
#define SLM_BOARD_

#include <string>

class SLM_Board
{
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

	SLM_Board(){};
	SLM_Board(bool isNematic, int width, int height);
	~SLM_Board();
	int GetArea();

};

#endif