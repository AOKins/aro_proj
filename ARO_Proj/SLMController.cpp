// [INCUDE FILES]
// - Required
#include "stdafx.h"				// Required in source

//	- External libraries
#include "cdib.h"				// Used by blink to read in bitmaps
//	- Custom classes
#include "MainDialog.h"
#include "ImageScaler.h"
#include "CameraController.h"
#include "SLMController.h"		// Header file

#include "Utility.h"
// - System libraries
#include <string>
#include <fstream>	// used to export information to file 

// Constructor
SLMController::SLMController() {
	unsigned int bits_per_pixel = 8U;			 //8 -> small SLM, 16 -> large SLM
	bool is_LC_Nematic = true;					 //HUGE TODO: perform this setup on evey board not just the beginning
	bool RAM_write_enable = true;
	bool use_GPU_if_available = true;
	size_t max_transiet_frames = 20U;
	const char* static_regional_lut_file = NULL; // NULL -> no overdrive, actual LUT file -> yes overdrive

	// Create the sdk that lets control the board(s)
	blink_sdk = new Blink_SDK(bits_per_pixel, &numBoards, &isBlinkSuccess, is_LC_Nematic, RAM_write_enable, use_GPU_if_available, max_transiet_frames, NULL);
	// Perform initial board info retrival and settings setup
	repopulateBoardList();

	for (int i = 0; i < this->boards.size(); i++) {
		setupSLM(i);
	}

	// Load up the image arrays to each board (ASK: what this does)
	LoadSequence();

	//Start with SLMs OFF
	this->setBoardPowerALL(false);
}

// Setup performed at start of controller
// Input:
//  repopulateBoardList - boolean if true will reset the board list
//	boardIDx - index for SLM (0 based)
bool SLMController::setupSLM(int boardIdx = 0) {
	//Check if board is avaliable
	if (this->boards.size() >= boardIdx || boardIdx < 0) {
		return false;
	}

	//Read default LUT File into the SLM board so it is applied to images automatically by the hardware, doesn't check for errors
	if (!AssignLUTFile(boardIdx, "")) {
		Utility::printLine("WARNING: Failure to assign default LUT file for SLM!");
		return false;
	}
}

// Repopulate the array of boards with what is currently connected and with some default values
bool SLMController::repopulateBoardList() {
	// Clear board data
	for (int i = 0; i < this->boards.size(); i++) {
		delete this->boards[i];
	}
	this->boards.clear();

	// Go through and generate new board structs with default filenames
	for (unsigned int i = 1; i <= this->numBoards; i++) {
		SLM_Board *curBoard = new SLM_Board(true, this->blink_sdk->Get_image_width(i), this->blink_sdk->Get_image_height(i));
		int boardArea = this->blink_sdk->Get_image_width(i) * this->blink_sdk->Get_image_height(i);
		//Build paths to the calibrations for this SLM -- regional LUT included in Blink_SDK(), but need to pass NULL to that param to disable ODP. Might need to make a class.
		curBoard->LUTFileName = "linear.LUT";
		curBoard->PhaseCompensationFileName = "slm929_8bit.bmp";
		curBoard->SystemPhaseCompensationFileName = "Blank.bmp";
		//curBoard->LUT = new unsigned char[256]; // Commented out as doesn't seem to be used for anything
		curBoard->PhaseCompensationData = new unsigned char[boardArea];
		curBoard->SystemPhaseCompensationData = new unsigned char[boardArea];

		//Add board info to board list
		this->boards.push_back(curBoard);
	}
	return true;
}

// Assign and load LUT file
// Input:
//		boardIdx - index for which board (0 based index)
//		path - string to file being loaded
//				if path is "" then defaults to "./slm3986_at532_P8.LUT"
// Output: If no errors, the SLM baord at boardIdx will be assigned the LUT file at path
//		returns true if no errors
//		returns false if an error occurs while attempting to load LUT file
bool SLMController::AssignLUTFile(int boardIdx, std::string path) {
	const char* LUT_file;
	if (path != "") {
		LUT_file = path.c_str();
		Utility::printLine("INFO: Setting LUT file path to what was provided by user input!");
	}
	else {
		LUT_file = "./slm3986_at532_P8.LUT";
		Utility::printLine("INFO: Setting LUT file path to default (given path was '')!");
	}

	//Write LUT file to the board
	try {
		this->blink_sdk->Load_LUT_file(boardIdx + 1, LUT_file);
		this->boards[boardIdx]->LUTFileName = LUT_file;
		// Printing resulting file
		Utility::printLine("INFO: Loaded LUT file: " + std::string(LUT_file));
		return true;
	}
	catch (...) {
		Utility::printLine("ERROR: Failure to load LUT file: " + std::string(LUT_file));
		return false;
	}
}

/* LoadSequence: This function will load a series of two images to the PCIe board memory.
 *               The first image is written to the first frame of memory in the hardware,
 *  			 and the second image is written to the second frame.
 *
 * Modifications: Might need to change how we get a handle to the board, how we access
 *				  PhaseCompensationData, SystemCompensationData, FrameOne, and FrameTwo.
 *				  Once we're using Overdrive, I think it'll be easier because the
 *				  construction of the sdk includes a correction file, and even in the 
 *  			  old code used the same correction file. */
void SLMController::LoadSequence() {
	if (!dlg) {
		Utility::printLine("WARNING: trying to load sequence without dlg reference");
		return;
	}
		
	//loop through each PCIe board found, read in the calibrations for that SLM
	//as well as the images in the wequence list, and store the merged image/calibration
	//data in an array that will later be referenced
	for (int h = 0; h < boards.size(); h++) { // please note: BoardList has a 0-based index, but Blink's index of available boards is 1-based.

		//if the user has a nematic SLM and the user is viewing the SLM through an interferometer, 
		//then the user has the option to correct for SLM imperfections by applying a predetermined 
		//phase compensation. BNS calibrates every SLM before shipping, so each SLM ships with its 
		//own custom phase correction file. This step is either reading in that correction file, or
		//setting the data in the array to 0 for users that don't want to correct, or for users with
		//amplitude devies
		if (dlg->m_slmControlDlg.m_CompensatePhase) {
			ReadAndScaleBitmap(boards[h], boards[h]->PhaseCompensationData, boards[h]->PhaseCompensationFileName); //save to PhaseCompensationData
			ReadAndScaleBitmap(boards[h], boards[h]->SystemPhaseCompensationData, boards[h]->SystemPhaseCompensationFileName); //save to SystemPhaseCompensationData
		}
		else {
			memset(boards[h]->PhaseCompensationData, 0, boards[h]->GetArea()); //set PhaseCompensationData to 0
			memset(boards[h]->SystemPhaseCompensationData, 0, boards[h]->GetArea()); //set SystemPhaseCompensationData to 0
		}

		int total;
		//Superimpose the SLM phase compensation, the system phase compensation, and
		//the image data. Then store the image in FrameOne
		for (int i = 0; i < boards[h]->GetArea(); i++) {
			total = boards[h]->PhaseCompensationData[i] +
					boards[h]->SystemPhaseCompensationData[i];
		}

		//Superimpose the SLM phase compensation, the system phase compensation, and
		//the image data. Then store the image in FrameTwo
		for (int i = 0; i < boards[h]->GetArea(); i++) {
			total = boards[h]->PhaseCompensationData[i] +
					boards[h]->SystemPhaseCompensationData[i];
		}
	}
}

// [DESTRUCTOR]
SLMController::~SLMController() {
	//Poweroff and deallokate sdk functionality
	blink_sdk->SLM_power(false);
	blink_sdk->~Blink_SDK();

	//De-allocate all memory allocated to store board information
	for (int i = 0; i < boards.size(); i++)
		delete boards[i];
}

// [UTILITY]

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
bool SLMController::ReadAndScaleBitmap(SLM_Board *board, unsigned char *Image, std::string filename) {
	int width, height, bytespixel;

	//need a tmpImage because we cannot assume that the bitmap we
	//read in will be the correct dimensions
	unsigned char* tmpImage;

	//get a handle to our file
	CFile *pFile = new CFile();
	if (pFile == NULL)
		Utility::printLine("ERROR: allocating memory for pFile, ReadAndScaleBitmap");

	//if it is a .bmp file and we can open it
	CString file = CString(filename.c_str());
	if (pFile->Open(file, CFile::modeRead | CFile::shareDenyNone, NULL)) {
		//read in bitmap dimensions
		CDib dib;
		dib.Read(pFile);
		width = dib.GetDimensions().cx;
		height = dib.GetDimensions().cy;
		bytespixel = dib.m_lpBMIH->biBitCount;
		pFile->Close();
		delete pFile;

		//allocate our tmp array based on the bitmap dimensions
		tmpImage = new unsigned char[height*width];

		//flip the image right side up (INVERT)
		for (int i = 0; i < height; i++) {
			for (int j = 0; j < width; j++) {
				if (bytespixel == 4)
					tmpImage[((height - 1) - i)*height + j] = dib.m_lpImage[i*(height / 2) + (j / 2)];
				if (bytespixel == 8)
					tmpImage[((height - 1) - i)*height + j] = dib.m_lpImage[i*height + j];
				if (bytespixel == 16)
					tmpImage[((height - 1) - i)*height + j] = dib.m_lpImage[i * 2 * height + j * 2];
				if (bytespixel == 24)
					tmpImage[((height - 1) - i)*height + j] = dib.m_lpImage[i * 3 * height + j * 3];
			}
		}
		dib.~CDib();
	}
	//we could not open the file, or the file was not a .bmp
	else {
		//depending on if we are trying to read a bitmap to dowload
		//or if we are trying to read it for the screen, memset
		//the array to zero and return false
		memset(Image, '0', board->GetArea());
		return false;
	}

	//scale the bitmap
	unsigned char* ScaledImage = ScaleBitmap(tmpImage, height, board->imageHeight);

	//copy the scaled bitmap into the array passed into the function
	memcpy(Image, ScaledImage, board->GetArea());

	//delete tmp array to avoid mem leaks
	delete[]tmpImage;

	return true;
}

// Update the framerate for the boards according to the GUI to match camera setting
	// Returns true if no errors, false if error occurs
bool SLMController::updateFramerateFromGUI() {
	for (int i = 0; i < this->boards.size(); i++) {
		float fps;
		try	{
			unsigned short trueFrames = 3;	//3 -> non-overdrive operation, 5 -> overdrive operation (assign correct one)

			CString path("");
			this->dlg->m_cameraControlDlg.m_FramesPerSecond.GetWindowTextW(path);
			if (path.IsEmpty()) throw new std::exception();
			fps = float(_tstof(path));

			//IMPORTANT NOTE: if framerate is not the same framerate that was used in OnInitDialog AND the LC type is FLC 
			//				  then it is VERY IMPORTANT that true frames be recalculated prior to calling SetTimer such
			//				  that the FrameRate and TrueFrames are properly related
			if (!boards[i]->is_LC_Nematic) {
				trueFrames = blink_sdk->Compute_TF(float(fps));
			}
			this->blink_sdk->Set_true_frames(trueFrames);
		}
		catch (...)	{
			Utility::printLine("ERROR: Was unable to parse frame rate field for SLMs!");
			return false;
		}

	}
	return true; // no issues!
}

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
unsigned char* SLMController::ScaleBitmap(unsigned char* InvertedImage, int BitmapSize, int FinalBitmapSize) {
	int height = BitmapSize;
	int width = BitmapSize;

	//make an array to hold the scaled bitmap
	unsigned char* ScaledImage = new unsigned char[FinalBitmapSize*FinalBitmapSize];
	if (ScaledImage == NULL)
		AfxMessageBox(_T("Error allocating memory for CFile,LoadSIFRec"), MB_OK);

	//EXPAND THE IMAGE to FinalBitmapSize
	if (height < FinalBitmapSize) {
		int r, c, row, col, Index; //row and col correspond to InvertedImage
		int Scale = FinalBitmapSize / height;

		for (row = 0; row < height; row++) {
			for (col = 0; col < width; col++) {
				for (r = 0; r < Scale; r++) {
					for (c = 0; c < Scale; c++)	{
						Index = ((row*Scale) + r)*FinalBitmapSize + (col*Scale) + c;
						ScaledImage[Index] = InvertedImage[row*height + col];
					}
				}
			}
		}
	}
	//SHRINK THE IMAGE to FinalBitmapSize
	else if (height > FinalBitmapSize) {
		int Scale = height / FinalBitmapSize;
		for (int i = 0; i < height; i += Scale) {
			for (int j = 0; j < width; j += Scale) {
				ScaledImage[(i / Scale) + FinalBitmapSize*(j / Scale)] = InvertedImage[i + height*j];
			}
		}
	}
	//if the image is already the correct size, just copy the array over
	else {
		memcpy(ScaledImage, InvertedImage, FinalBitmapSize*FinalBitmapSize);
	}
	return(ScaledImage);
}

bool SLMController::IsAnyNematic() {
	for (int i = 0; 0 < boards.size(); i++) {
		if (boards[i]->is_LC_Nematic) {
			return true;
		}
	}
	return false;
}

bool SLMController::slmCtrlReady() {
	if (blink_sdk == NULL || blink_sdk == nullptr) {
		return false;
	}
	//TODO: add more conditions

	return true;
}

// Getter for board image width
// Input: boardIdx - index for board getting data from (0 based index)
// Output: the imageWidth property of the selected board (if invalid index value, returns -1)
int SLMController::getBoardWidth(int boardIdx) {
	if (boardIdx >= this->boards.size() || boardIdx < 0) {
		return -1;
	}
	return this->boards[boardIdx]->imageWidth;
}

// Getter for board image height
// Input: boardIdx - index for board getting data from (0 based index)
// Output: the imageHeight property of the selected board (if invalid index value, returns -1)
int SLMController::getBoardHeight(int boardIdx) {
	if (boardIdx >= this->boards.size() || boardIdx < 0) {
		return -1;
	}
	return this->boards[boardIdx]->imageHeight;
}

// Set power to all connected boards
// Input: isOn - power setting (true = on, false = off)
// Output: All SLMs available to the SDK are powered on
void SLMController::setBoardPowerALL(bool isOn) {
	if (blink_sdk != NULL) {
		blink_sdk->SLM_power(isOn);
		for (int i = 0; i < this->boards.size(); i++) {
			this->boards[i]->setPower(isOn);
		}
	}
	else {
		Utility::printLine("WARNING: SDK not avalible to power ON/OFF the boards!");
	}
}

// Set the power to a board
// Input: boardID - index of board being toggled (0 based index)
//		  isOn - power setting (true = on, false = off)
// Output: SLM is turned on/off accordingly
void SLMController::setBoardPower(int boardID, bool isOn) {
	if (blink_sdk != NULL) {
		blink_sdk->SLM_power(boardID+1, isOn);
		this->boards[boardID]->setPower(isOn);
	}
	else {
		Utility::printLine("WARNING: SDK not avalible to power ON/OFF the board!");
	}
}

// Write an image to a board
// Input:
//		slmNum - index for board to assing image to (0 based index)
//		image - pointer to array of image data to assign to board
// Output: Write image to board at slmNum, using that board's height for the image size
bool SLMController::writeImageToBoard(int slmNum, unsigned char * image) {
	if (slmNum < 0 || slmNum >= boards.size()) {
		return false;
	}
	else {
		return this->blink_sdk->Write_image(slmNum + 1, image, this->getBoardHeight(slmNum), false, false, 0);
	}
}
