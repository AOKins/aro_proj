#ifndef SLM_CONTROLLER_H_
#define SLM_CONTROLLER_H_

#include "SLM_Board.h"
#include <vector>

// [FORWARD DEFINITIONS]
class MainDialog;
class Blink_SDK;
class SLM_Board;

// Class to encapsulate interactions with SLM boards
class SLMController {
private:
	//UI Reference
	MainDialog* dlg;

public:
	//Board control
	Blink_SDK* blink_sdk;			//Library that controlls the SLMs
	bool isBlinkSuccess = true;		//TRUE -> if SLM control wrpper was constructed correctly
	//Board parameters
	unsigned int numBoards = 0;		//Number of boards populated after creation of the SDK
	unsigned short trueFrames = 3;	//3 -> non-overdrive operation, 5 -> overdrive operation (assign correct one)

	//Board references
	std::vector<SLM_Board*> boards;
	// Constructor
	SLMController();
	// Destructor
	~SLMController();

	// Input:
	//  repopulateBoardList - boolean if true will reset the board list
	//	boardIDx - index for SLM (0 based)
	bool setupSLM(bool repopulateBoardList, int boardIdx = 0);

	/* LoadSequence: This function will load a series of two images to the PCIe board memory.
	*               The first image is written to the first frame of memory in the hardware,
	*  			 and the second image is written to the second frame.
	*
	* Modifications: Might need to change how we get a handle to the board, how we access
	*				  PhaseCompensationData, SystemCompensationData, FrameOne, and FrameTwo.
	*				  Once we're using Overdrive, I think it'll be easier because the
	*				  construction of the sdk includes a correction file, and even in the
	*  			  old code used the same correction file. */
	void LoadSequence();
	void setBoardImage(int boardIdx = 0);

	void addBoard(SLM_Board* board);
	void removeBoard(int boardIdx);

	bool IsAnyNematic();
	void ImageListBoxUpdate(int index);

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
	//   ReadAndScaleBitmap()
	//
	//   Inputs: empty array to fill, the file name to read in
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
	bool ReadAndScaleBitmap(SLM_Board* board, unsigned char *Image, std::string filename);

	bool slmCtrlReady();

	int getBoardWidth(int boardIdx);
	int getBoardHeight(int boardIdx);
	void setBoardPower(bool isOn);

	void SetMainDlg(MainDialog* dlg_) { dlg = dlg_; }
private:
	// [UTILITY]
	/* AssignLUTFile:
	* @param LUTBuf - array in which to store  the LUT File info
	* @param LUTPath - the name of the LUT file to read
	* @return TRUE if success, FALSE if failed */
	bool ReadLUTFile(unsigned char *LUTBuf, std::string LUTPath);

	///////////////////////////////////////////////////////////////////////////
	//
	//   ReadZernikeFile()
	//
	//   Inputs: empty array to fill, and the file name to read.
	//
	//   Returns: true if no errors, otherwise false
	//
	//   Purpose: This function will read in Zernike polynomials and generate
	//			  an array f data from those Zernike polynomials
	//
	//   Modifications:
	//
	/////////////////////////////////////////////////////////////////////////////
	bool ReadZernikeFile(SLM_Board* board, unsigned char *GeneratedImage, std::string fileName);
	
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
	unsigned char* ScaleBitmap(unsigned char* InvertedImage, int BitmapSize, int FinalBitmapSize);
	
};

#endif
