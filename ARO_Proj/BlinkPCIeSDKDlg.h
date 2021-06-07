#pragma once

// [DESCRIPTION]
// BlinkPCIeSDKDlg.h : header file for the main dialog of the program


// [INCUDE FILES]
//	- Dialog and and IDE
#include "afxwin.h"
#include "afxdialogex.h"
#include <stdlib.h>
#include <stdio.h>
#include "resource.h"

//	- External libraries
#include "Blink_SDK.h"			// Camera functions
#include "cdib.h"				// Used by blink to read in bitmaps

#include "Spinnaker.h"			// Spatial light modulator functions
#include "SpinGenApi\SpinnakerGenApi.h"

#include <opencv2\highgui\highgui.hpp>	//image processing
#include <opencv2\imgproc\imgproc.hpp>

//	- Board/Camera Storage
#include "SLM_Board.h"

// - Aglogrithm Related
#include "Population.h"
#include "SGA_Population.h"

//	- Helper
#include "Utility.h"			// Collection of static helper functions
#include "Timing.h"				// Allows to properly pace slm updates
#include "ImgScaleManager.h"
#include "CamDisplay.h"

//	- Profiling
#include <Windows.h>			// Used for profiling capabilities (timers)
#include "TimeStamp.h"			// Captures a time and message for progiling purposes
#include <conio.h>				
#include <chrono>
#include <math.h>
#include <thread>

//	- System
#include <fstream>				// Used to export information to file 
#include <string>				// Used as the primary "letters" datatype
#include <vector>				// Used for image value storage


//[NAMESPACES]
using namespace std;
using namespace cv;
using namespace Spinnaker;
using namespace Spinnaker::GenApi;
using namespace Spinnaker::GenICam;


class CBlinkPCIeSDKDlg : public CDialog
{
public:
	// [CONSTRUCTOR(S)]
	// Standard Constructor
	CBlinkPCIeSDKDlg(CWnd* pParent = nullptr);


	// [IMPORT]
	void LoadSequence();
	bool ReadLUTFile(unsigned char* LUTBuf, CString LUTPath);
	bool ReadZernikeFile(unsigned char *GeneratedImage, CString fileName);


	// [PROCCESSING]
	double FindAverageValue(unsigned char *Image, int* target, size_t width, size_t height);

	bool ReadAndScaleBitmap(unsigned char* buffer, CString ImagePath);
	unsigned char* ScaleBitmap(unsigned char* InvertedImage, int BitmapSize, int FinalBitmapSize);

	void GenerateTargetMatrix_SinglePoint(int *target, int width, int height, int radius);
	void GenerateTargetMatrix_LoadFromFile(int *target, int width, int height);

	// [IMAGE ACQUISITION]
	Spinnaker::ImagePtr AcquireImages(Spinnaker::CameraPtr pCam, Spinnaker::GenApi::INodeMap &nodeMap, Spinnaker::GenApi::INodeMap &nodeMapTLDevice);
	int ConfigureExposure(Spinnaker::GenApi::INodeMap &nodeMap, double exposureTimeToSet);
	int ConfigureCustomImageSettings(Spinnaker::CameraPtr pCam, Spinnaker::GenApi::INodeMap &nodeMap, int FPS);

	
	double frequency;
	vector<TimeStamp> recordedTimes;
	LARGE_INTEGER startTime;
	TimeStamp RecordTime(LARGE_INTEGER start, double frequency, std::string label);

	int ImgHeight;
	int ImgWidth;

	int populationCounter;
	Population<int>* populationPointer;
	SGAPopulation<int>* SGAPopulationPointer;
	ImageScaleManager* ImgScaleMngr;
	CameraDisplay* camDisplay;
	Spinnaker::CameraPtr pCam;
	unsigned char* camImage;
	size_t width, height; //camera image dimensions
	int* targetMatrix;
	double exposureTime;

	unsigned char ary[512 * 512];

	vector<SLM_Board*> SLM_board_list;

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_BLINKPCIESDK_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	BOOL m_CompensatePhase;
	CButton m_SlmPwrButton;
	CButton m_StartStopButton;
	CButton m_CompensatePhaseCheckbox;
	CListBox m_ImageListBox;
	CButton m_uGAButton;
	CButton m_SGAButton;
	CButton m_OptButton;
	CEdit m_BSEdit;
	CEdit m_NBEdit;
	CEdit m_IREdit;
	CEdit m_ISTEdit;
	CEdit m_MFEdit;
	CEdit m_MSEEdit;
	CEdit m_MFEEdit;

protected:
	afx_msg void OnSlmPwrButton();
	afx_msg void OnSelchangeImageListbox();
	afx_msg void OnStartStopButton();
	virtual void OnOK();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnClose();
	afx_msg void OnCompensatePhaseCheckbox();
public:
	afx_msg void OnBnClickedUgaButton();
	afx_msg void OnBnClickedSgaButton();
	afx_msg void OnBnClickedOptButton();
	afx_msg void OnBnClickedSetlut();
	afx_msg void OnBnClickedSetwfc();
private:
	CString m_ReadyRunning;
};
