#ifndef SLM_CONTROLLER_H_
#define SLM_CONTROLLER_H_

#include "SLM_Board.h"
#include "Blink_SDK.h"

#include <vector>

class MainDialog;

// Class to encapsulate interactions with SLM boards
class SLMController {
private:
	//UI Reference
	MainDialog* dlg;
	bool phaseCompensationToggle;
public:
	//Board control
	Blink_SDK* blink_sdk;			//Library that controls the SLMs
	bool isBlinkSuccess = true;		//TRUE -> if SLM control wrapper was constructed correctly
	//Board parameters
	unsigned int numBoards = 0;		//Number of boards populated after creation of the SDK

	//Board references
	std::vector<SLM_Board*> boards;
	// Constructor
	SLMController();
	// Destructor
	~SLMController();

	// Setup performed at start of controller
	// Input:
	//  repopulateBoardList - boolean if true will reset the board list
	//	boardIDx - index for SLM (0 based)
	bool setupSLM(int boardIdx);

	// Repopulate the array of boards with what is currently connected and with some default values
	bool repopulateBoardList();

	void setBoardImage(int boardIdx = 0);

	void addBoard(SLM_Board* board);
	void removeBoard(int boardIdx);

	bool IsAnyNematic();

	// Update the framerate for the boards according to the GUI to match camera setting
	// Returns true if no errors, false if error occurs
	bool updateFromGUI();


	// Assign and load LUT file
	// Input:
	//		boardIdx - index for which board (0 based index)
	//		path - string to file being loaded
	//				if path is "" then defaults to "./slm3986_at532_P8.LUT"
	// Output: If no errors, the SLM baord at boardIdx will be assigned the LUT file at path
	//		returns true if no errors
	//		returns false if an error occurs while attempting to load LUT file
	bool AssignLUTFile(int boardIdx, std::string);

	//////////////////////////////////////////////////////////
	//
	//   LoadPhaseCompensationData()
	//
	//   Inputs: board to assign phase compensation data with, the file name to read in
	//
	//   Returns: true if no errors, otherwise false
	//
	//   Purpose: This function will read in the bitmap and x-axis flip it. If there is a 
	//			  problem reading in the bitmap, we fill the array with zeros. This function
	//			  then calls ScaleBitmap so we can scale the bitmap to an image size based on
	//			  the board type.
	//
	//   Modifications: 
	//
	//////////////////////////////////////////////////////////
	bool LoadPhaseCompensationData(SLM_Board* board, std::string filename);

	bool slmCtrlReady();

	int getBoardWidth(int boardIdx);
	int getBoardHeight(int boardIdx);
	void setBoardPowerALL(bool isOn);
	// Set power to a specific board
	// Input:
	//		boardID - index for which board (0 based index)
	//		isOn - boolean for if on or off toggle
	void setBoardPower(int boardID, bool isOn);

	void SetMainDlg(MainDialog* dlg_) { dlg = dlg_; }

	// Write an image to a board
	// Input:
	//		slmNum - index for which board (0 based index)
	//		image - pointer to array of image to assign to board
	// Output: Write image to board at slmNum, using that board's height for the image size
	bool writeImageToBoard(int slmNum, unsigned char * image);

private:

	////////////////////////////////////////////////////////////////////////
	//
	//   ScaleBitmap()
	//
	//   Inputs: the array that holds the image to scale, the current bitmap size, 
	//			 the final bitmap size
	//
	//   Returns: the scaled image
	//
	//   Purpose: This will scale the bitmap from its initial size to a 128x128
	//			  if load is set to false. Otherwise the image is scaled to a 
	//			  512x512 if the board type is set to 512ASLM, or 256x256 if the 
	//			  board type is set to 256ASLM
	//
	//   Modifications:
	//
	/////////////////////////////////////////////////////////////////////////
	unsigned char* SLMController::ScaleBitmap(unsigned char* InvertedImage, int BitmapSizeHeight, int BitmapSizeWidth, int FinalBitmapSizeHeight, int FinalBitmapSizeWidth);
};

#endif
