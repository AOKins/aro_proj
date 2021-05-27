#ifndef SLM_CONTROLLER_H_
#define SLM_CONTROLLER_H_

#include "SLM_Board.h"
#include <vector>

// [FORWARD DEFINITIONS]
class MainDialog;
class Blink_SDK;
class SLM_Board;

class SLMController
{
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
	SLMController();
	~SLMController();

	bool setupSLM(bool repopulateBoardList, int boardIdx = 0);
	void LoadSequence();
	void setBoardImage(int boardIdx = 0);

	void addBoard(SLM_Board* board);
	void removeBoard(int boardIdx);

	bool IsAnyNematic();
	void ImageListBoxUpdate(int index);
	void AssignLUTFile(int boardIdx, std::string);
	bool ReadAndScaleBitmap(SLM_Board* board, unsigned char *Image, std::string filename);

	bool slmCtrlReady();

	int getBoardWidth(int boardIdx);
	int getBoardHeight(int boardIdx);
	void setBoardPower(bool isOn);

	void SetMainDlg(MainDialog* dlg_) { dlg = dlg_; }
private:
	bool ReadLUTFile(unsigned char *LUTBuf, std::string LUTPath);
	bool ReadZernikeFile(SLM_Board* board, unsigned char *GeneratedImage, std::string fileName);
	unsigned char* ScaleBitmap(unsigned char* InvertedImage, int BitmapSize, int FinalBitmapSize);


};

#endif