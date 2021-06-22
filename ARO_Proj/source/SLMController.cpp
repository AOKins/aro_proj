// [INCUDE FILES]
// - Required
#include "../headers/.h"				// Required in source
#include <string>
#include <fstream>	// used to export information to file 
#include "../headers/ImageScaler.h"
#include "../headers/CameraController.h"
#include "../headers/SLMController.h"		// Header file
#include "../headers/SLM_Board.h"

//	- External libraries
#include "../headers/Blink_SDK.h"			// Camera functions
#include "../headers/cdib.h"				// Used by blink to read in bitmaps
//	- Custom classes
#include "../headers/MainDialog.h"

#include "../headers/Utility.h"
// - System libraries

SLMController::SLMController() {
	unsigned int bits_per_pixel = 8U;			 //8 -> small SLM, 16 -> large SLM
	bool is_LC_Nematic = true;					 //HUGE TODO: perform this setup on evey board not just the beginning
	bool RAM_write_enable = true;
	bool use_GPU_if_available = true;
	size_t max_transiet_frames = 20U;
	const char* static_regional_lut_file = NULL; //NULL -> no overdrive, actual LUT file -> yes overdrive

	// Create the sdk that lets control the board(s)
	blink_sdk = new Blink_SDK(bits_per_pixel, &numBoards, &isBlinkSuccess, is_LC_Nematic, RAM_write_enable, use_GPU_if_available, max_transiet_frames, NULL);
	// Perform initial board info retrival and settings setup
	setupSLM(true);
	// Load up the image arrays to each board (ASK: what this does)
	LoadSequence();
}

// Input:
//  repopulateBoardList - boolean if true will reset the board list
//	boardIDx - index for SLM (0 based)
bool SLMController::setupSLM(bool repopulateBoardList, int boardIdx) {
	boards.clear();

	//Create all board references
	if (repopulateBoardList) {
		for (int i = 1; i <= numBoards; i++) {
			SLM_Board *curBoard = new SLM_Board(true, blink_sdk->Get_image_width(i), blink_sdk->Get_image_height(i));
			int boardArea = blink_sdk->Get_image_width(i) * blink_sdk->Get_image_height(i);

			//Build paths to the calibrations for this SLM -- regional LUT included in Blink_SDK(), but need to pass NULL to that param to disable ODP. Might need to make a class.
			std::string SLMSerialNum = "linear";
			std::string SLMSerialNumphase = "slm929_8bit";
			curBoard->LUTFileName = SLMSerialNum + ".LUT";
			curBoard->PhaseCompensationFileName = SLMSerialNumphase + ".bmp";
			curBoard->SystemPhaseCompensationFileName = "Blank.bmp";
			curBoard->FrameOne = new unsigned char[boardArea];
			curBoard->FrameTwo = new unsigned char[boardArea];
			curBoard->LUT = new unsigned char[256];
			curBoard->PhaseCompensationData = new unsigned char[boardArea];
			curBoard->SystemPhaseCompensationData = new unsigned char[boardArea];

			//Add board info to board list
			boards.push_back(curBoard);
		}
	}
	//Check if board is avaliable
	if (boards.size() >= boardIdx) {
		return false;
	}

	//Read default LUT File into the SLM board so it is applied to images automatically by the hardware, doesn't check for errors
	if (!AssignLUTFile(boardIdx, "")) {
		Utility::printLine("WARNING: Failure to assign default LUT file for SLM!");
	}

	float fps;
	try	{
		CString path("");
		dlg->m_cameraControlDlg.m_FramesPerSecond.GetWindowTextW(path);
		if (path.IsEmpty()) throw new std::exception();
		fps = _tstof(path);

		//IMPORTANT NOTE: if framerate is not thesame framerate that was used in OnInitDialog AND the LC type is FLC 
		//				  then it is VERY IMPORTANT that true frames be recalculated prior to calling SetTimer such
		//				  that the FrameRate and TrueFrames are properly related
		if (!boards[boardIdx]->is_LC_Nematic) {
			trueFrames = blink_sdk->Compute_TF(float(fps));
		}
		blink_sdk->Set_true_frames(trueFrames);
	}
	catch (...)	{
		Utility::printLine("ERROR: Was unable to parse integration radius input feild!");
	}
	//Start with SLMs OFF
	blink_sdk->SLM_power(false);

	return false;
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
		blink_sdk->Load_LUT_file(boardIdx + 1, LUT_file);
		boards[boardIdx]->LUTFileName = LUT_file;
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

	int i;

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
		if (dlg->m_CompensatePhase) {
			ReadAndScaleBitmap(boards[h], boards[h]->PhaseCompensationData, boards[h]->PhaseCompensationFileName); //save to PhaseCompensationData
			ReadAndScaleBitmap(boards[h], boards[h]->SystemPhaseCompensationData, boards[h]->SystemPhaseCompensationFileName); //save to SystemPhaseCompensationData
		}
		else {
			memset(boards[h]->PhaseCompensationData, 0, boards[h]->GetArea()); //set PhaseCompensationData to 0
			memset(boards[h]->SystemPhaseCompensationData, 0, boards[h]->GetArea()); //set SystemPhaseCompensationData to 0
		}

		//Read in the first image -- this one is a bitmap
		ReadAndScaleBitmap(boards[h], boards[h]->FrameOne, "ImageOne.bmp"); //save to FrameOne

		int total;
		//Superimpose the SLM phase compensation, the system phase compensation, and
		//the image data. Then store the image in FrameOne
		for (i = 0; i < boards[h]->GetArea(); i++) {
			total = boards[h]->FrameOne[i] +
					boards[h]->PhaseCompensationData[i] +
					boards[h]->SystemPhaseCompensationData[i];

			boards[h]->FrameOne[i] = total % 256;
		}

		//Read in the second image. For the sake of an example this is a zernike file
		ReadZernikeFile(boards[h], boards[h]->FrameTwo, "ImageTwo.zrn");

		//Superimpose the SLM phase compensation, the system phase compensation, and
		//the image data. Then store the image in FrameTwo
		for (i = 0; i < boards[h]->GetArea(); i++) {
			total = boards[h]->FrameTwo[i] +
					boards[h]->PhaseCompensationData[i] +
					boards[h]->SystemPhaseCompensationData[i];

			boards[h]->FrameTwo[i] = total % 256;
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
/* AssignLUTFile: 
 * @param LUTBuf - array in which to store  the LUT File info
 * @param LUTPath - the name of the LUT file to read
 * @return TRUE if success, FALSE if failed */
bool SLMController::ReadLUTFile(unsigned char *LUTBuf, std::string LUTPath) {
	FILE *stream;
	int i, seqnum, ReturnVal, tmpLUT;
	bool errorFlag;

	//set the error flag to indicate that there are no errors so far, and open
	//the LUT file
	errorFlag = false;

	stream = fopen(LUTPath.c_str(), "r");
	if ((stream != NULL) && (errorFlag == false)) {
		//read in all 256 values
		for (i = 0; i < 256; i++) {
			ReturnVal = fscanf(stream, "%d %d", &seqnum, &tmpLUT);
			if (ReturnVal != 2 || seqnum != i || tmpLUT < 0 || tmpLUT > 255) {
				fclose(stream);
				errorFlag = true;
				break;
			}
			LUTBuf[i] = (unsigned char)tmpLUT;
		}
		fclose(stream);
	}
	//if there was an error reading in the LUT, default to a linear LUT
	if ((stream == NULL) || (errorFlag == true)) {
		for (i = 0; i < 256; i++)
			LUTBuf[i] = i;
		return false;
	}

	return true;
}

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
#define MAX_ZERNIKE_LINE 300
bool SLMController::ReadZernikeFile(SLM_Board* board, unsigned char *GeneratedImage, std::string fileName) {
	char inBuf[MAX_ZERNIKE_LINE];
	char inputString[MAX_ZERNIKE_LINE];
	char inputKey[MAX_ZERNIKE_LINE];

	int row, col, Radius;
	double x, y, divX, divY, total, XSquPlusYSqu, XPYSquSqu, divXSqu, divYSqu;
	double term1, term2, term3, term4, term5, term6;
	double term7, term8, term9, term10, term11, term12;
	double term13, term14, term15, term16, term17, term18;
	double term25, term36;
	double Piston, XTilt, YTilt, Power, AstigOne, AstigTwo;
	double ComaX, ComaY, PrimarySpherical, TrefoilX, TrefoilY, SecondaryAstigX;
	double SecondaryAstigY, SecondaryComaX, SecondaryComaY, SecondarySpherical;
	double TetrafoilX, TetrafoilY, TertiarySpherical, QuaternarySpherical;

	//set our image size based on our board spec's if we are downloading the image
	//the radius can be varied, this is the radius of the cone that is generated
	//with the zernike polynomial equations. Through testing we determined 300
	//to be a good number. This means that the edge of the cone does extend
	//beyond the edge of the SLM, but notquite all the way to the corner of the SLM.
	//It is a happy medium between getting the cone to reach the corners without
	//chopping too much off the sides. Basically this is a problem of how you 
	//force a circle to fit in a square.
	if (board->imageHeight == 512)
		Radius = 300;
	else
		Radius = 150;

	//open the zernike file to read
	std::ifstream ZernikeFile(fileName);
	if (ZernikeFile.is_open()) {
		while (ZernikeFile.getline(inBuf, MAX_ZERNIKE_LINE, '\n')) {
			//read in a line from the file
			sscanf(inBuf, "%[^= ]%*[= ]%s", inputKey, inputString);

			//get the zernikes
			if (strcmp(inputKey, "Piston") == 0)
				Piston = atof(inputString);
			else if (strcmp(inputKey, "XTilt") == 0)
				XTilt = atof(inputString);
			else if (strcmp(inputKey, "YTilt") == 0)
				YTilt = atof(inputString);
			else if (strcmp(inputKey, "Power") == 0)
				Power = atof(inputString);
			else if (strcmp(inputKey, "AstigX") == 0)
				AstigOne = atof(inputString);
			else if (strcmp(inputKey, "AstigY") == 0)
				AstigTwo = atof(inputString);
			else if (strcmp(inputKey, "ComaX") == 0)
				ComaX = atof(inputString);
			else if (strcmp(inputKey, "ComaY") == 0)
				ComaY = atof(inputString);
			else if (strcmp(inputKey, "PrimarySpherical") == 0)
				PrimarySpherical = atof(inputString);
			else if (strcmp(inputKey, "TrefoilX") == 0)
				TrefoilX = atof(inputString);
			else if (strcmp(inputKey, "TrefoilY") == 0)
				TrefoilY = atof(inputString);
			else if (strcmp(inputKey, "SecondaryAstigX") == 0)
				SecondaryAstigX = atof(inputString);
			else if (strcmp(inputKey, "SecondaryAstigY") == 0)
				SecondaryAstigY = atof(inputString);
			else if (strcmp(inputKey, "SecondaryComaX") == 0)
				SecondaryComaX = atof(inputString);
			else if (strcmp(inputKey, "SecondaryComaY") == 0)
				SecondaryComaY = atof(inputString);
			else if (strcmp(inputKey, "SecondarySpherical") == 0)
				SecondarySpherical = atof(inputString);
			else if (strcmp(inputKey, "TetrafoilX") == 0)
				TetrafoilX = atof(inputString);
			else if (strcmp(inputKey, "TetrafoilY") == 0)
				TetrafoilY = atof(inputString);
			else if (strcmp(inputKey, "TertiarySpherical") == 0)
				TertiarySpherical = atof(inputString);
			else if (strcmp(inputKey, "QuaternarySpherical") == 0)
				QuaternarySpherical = atof(inputString);
		} //end while getline

		ZernikeFile.close();

		//now generate our image based on our zernike polynomials
		y = board->imageHeight / 2;
		for (row = 0; row < board->imageHeight; row++)
		{
			//reset x
			x = (board->imageWidth / 2) * -1;
			for (col = 0; col < board->imageWidth; col++)
			{
				//build some terms that are repeated through the equations
				divX = x / Radius;
				divY = y / Radius;
				XSquPlusYSqu = divX*divX + divY*divY;
				XPYSquSqu = XSquPlusYSqu*XSquPlusYSqu;
				divXSqu = divX*divX;
				divYSqu = divY*divY;

				//figure out what each term in the equation is
				term1 = (Piston / 2);
				term2 = (XTilt / 2)*divX;
				term3 = (YTilt / 2)*divY;
				term4 = (Power / 2)*(2 * XSquPlusYSqu - 1);
				term5 = (AstigOne / 2)*(divXSqu - divYSqu);
				term6 = (AstigTwo / 2)*(2 * divX*divY);

				term7 = (ComaX / 2)*(3 * divX*XSquPlusYSqu - 2 * divX);
				term8 = (ComaY / 2)*(3 * divY*XSquPlusYSqu - 2 * divY);
				term9 = (PrimarySpherical / 2)*(1 - 6 * XSquPlusYSqu + 6 * XPYSquSqu);
				term10 = (TrefoilX / 2)*(divXSqu*divX - 3 * divX*divYSqu);
				term11 = (TrefoilY / 2)*(3 * divXSqu*divY - divYSqu*divY);
				term12 = (SecondaryAstigX / 2)*(3 * divYSqu - 3 * divXSqu + 4 * divXSqu * XSquPlusYSqu - 4 * divYSqu * XSquPlusYSqu);

				term13 = (SecondaryAstigY / 2)*(8 * divX*divY*XSquPlusYSqu - 6 * divX*divY);
				term14 = (SecondaryComaX / 2)*(3 * divX - 12 * divX*XSquPlusYSqu + 10 * divX*XPYSquSqu);
				term15 = (SecondaryComaY / 2)*(3 * divY - 12 * divY*XSquPlusYSqu + 10 * divY*XPYSquSqu);
				term16 = (SecondarySpherical / 2)*(12 * XSquPlusYSqu - 1 - 30 * XPYSquSqu + 20 * XSquPlusYSqu*XPYSquSqu);
				term17 = (TetrafoilX / 2)*(divXSqu*divXSqu - 6 * divXSqu*divYSqu + divYSqu*divYSqu);
				term18 = (TetrafoilY / 2)*(4 * divXSqu*divX*divY - 4 * divX*divY*divYSqu);

				term25 = (TertiarySpherical / 2)*(1 - 20 * XSquPlusYSqu + 90 * XPYSquSqu - 140 * XSquPlusYSqu*XPYSquSqu + 70 * XPYSquSqu*XPYSquSqu);
				term36 = (QuaternarySpherical / 2)*(-1 + 30 * XSquPlusYSqu - 210 * XPYSquSqu + 560 * XPYSquSqu*XSquPlusYSqu - 630 * XPYSquSqu*XPYSquSqu + 252 * XPYSquSqu*XPYSquSqu*XSquPlusYSqu);

				//add the terms
				total = term1 + term2 + term3 + term4 + term5 + term6 +
					term7 + term8 + term9 + term10 + term11 + term12 +
					term13 + term14 + term15 + term16 + term17 + term18 +
					term25 + term36;
				//now scale it and assign the result to an array
				if (total > 0)
					total = total - int(total);
				else
					total = total + int(total) + 1;
				GeneratedImage[row*board->imageWidth + col] = int((total)* 255);

				x++;
			}//close col loop
			y--;
		}//close row loop

		return true;
	}
	//if we could not open the zernike file
	else {
		memset(GeneratedImage, 0, board->GetArea());
		return false;
	}
}

bool SLMController::IsAnyNematic() {
	for (int i = 0; 0 < boards.size(); i++) {
		if (boards[i]->is_LC_Nematic) {
			return true;
		}
	}
	return false;
}

//index  - the image index to use to write to slm
void SLMController::ImageListBoxUpdate(int index) {
	for (int i = 0; i < boards.size(); i++)	{
		// Send the data of the selected image in the sequence list out to the SLM
		// LARGE TODO: instead of storing frames in the board struct store this 
		// info on the main dlg as a n object and pass into this function
		if (index == 0) {
			blink_sdk->Write_image(i + 1, boards[i]->FrameOne, boards[i]->imageHeight, false, false, 0.0);
		}
		if (index == 1) {
			blink_sdk->Write_image(i + 1, boards[i]->FrameTwo, boards[i]->imageHeight, false, false, 0.0);
		}
	}
}

bool SLMController::slmCtrlReady() {
	if (blink_sdk == NULL || blink_sdk == nullptr) {
		return false;
	}
	//TODO: add more conditions

	return true;
}

int SLMController::getBoardWidth(int boardIdx) {
	if (boardIdx >= boards.size()) {
		return -1;
	}
	return boards[boardIdx]->imageWidth;
}

int SLMController::getBoardHeight(int boardIdx) {
	if (boardIdx >= boards.size()) {
		return -1;
	}
	return boards[boardIdx]->imageHeight;
}

void SLMController::setBoardPower(bool isOn) {
	if (blink_sdk != NULL) {
		blink_sdk->SLM_power(isOn);
	}
	else {
		Utility::printLine("WARNING: SDK not avalible cannot power ON/OFF the boards!");
	}
}
