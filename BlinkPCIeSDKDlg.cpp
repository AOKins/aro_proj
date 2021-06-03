// [DESCRIPTION]
// BlinkPCIeSDKDlg.cpp: implementation file for the main dialog of the program
// Authors: Benjamin Richardson, Rebecca Tucker, Kostiantyn Makrasnov and More @ ISP ASL

// [DEFINITIONS/ABRIVIATIONS]
// MFC - Microsoft Foundation Class library - Used to design UI in C++
// SLM - Spatial Light Modulator - the device that contains parameters which we want to optimize (takes in image an reflects light based on that image)
// LC  - Liquid Crystal - the type of diplay the SLM has

// [REFERENCES]
// 1) Thread sleep example http://www.enseignement.polytechnique.fr/informatique/INF478/docs/Cpp/en/cpp/thread/sleep_for.html 

// [INCUDE FILES]
#include "stdafx.h"				// Required in source
#include "BlinkPCIeSDKDlg.h"	// Header file for dialog functions

//[GLOBAL VARIABLES]
// - General
ofstream rtime;			//Used to store training time info, then writing it to file

// - BlinkSDK
static Blink_SDK * blink_sdk;
unsigned int numBoards = 0U;	//Number of boards populated after creation of the SDK
int frameRate = 200;			//200 FPS or 200 HZ (valid range 1 - 1000)
unsigned short trueFrames = 3;	//3 -> non-overdrive operation, 5 -> overdrive operation (assign correct one)
bool is_LC_Nematic = true;			//false -> Ferroelectric LC, true -> Nematic LC


////USED FOR DEBUG IN MFC (most likely unneeded)
////#ifdef _DEBUG
////#define new DEBUG_NEW
////#undef THIS_FILE
////static char THIS_FILE[] = __FILE__;
////#endif


// [CONSTRUCTOR/COMPONENT EVENTS]
// Constructor for dialog
CBlinkPCIeSDKDlg::CBlinkPCIeSDKDlg(CWnd* pParent) : CDialog(IDD_BLINKPCIESDK_DIALOG, pParent), m_CompensatePhase(FALSE), m_ReadyRunning(_T(""))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

// Reference UI components
void CBlinkPCIeSDKDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Check(pDX, IDC_COMPENSATE_PHASE_CHECKBOX, m_CompensatePhase);
	DDX_Control(pDX, IDC_SLM_PWR_BUTTON, m_SlmPwrButton);
	DDX_Control(pDX, IDC_START_STOP_BUTTON, m_StartStopButton);
	DDX_Control(pDX, IDC_COMPENSATE_PHASE_CHECKBOX, m_CompensatePhaseCheckbox);
	DDX_Control(pDX, IDC_IMAGE_LISTBOX, m_ImageListBox);
	DDX_Control(pDX, IDC_UGA_BUTTON, m_uGAButton);
	DDX_Control(pDX, IDC_SGA_BUTTON, m_SGAButton);
	DDX_Control(pDX, IDC_OPT_BUTTON, m_OptButton);
	DDX_Control(pDX, IDC_EDIT_BS, m_BSEdit);
	DDX_Control(pDX, IDC_EDIT_NB, m_NBEdit);
	DDX_Control(pDX, IDC_EDIT_IR, m_IREdit);
	DDX_Control(pDX, IDC_EDIT_IST, m_ISTEdit);
	DDX_Control(pDX, IDC_EDIT_MF, m_MFEdit);
	DDX_Control(pDX, IDC_EDIT_MSE, m_MSEEdit);
	DDX_Control(pDX, IDC_EDIT_MFE, m_MFEEdit);
}

//Set functions that respond to user action
BEGIN_MESSAGE_MAP(CBlinkPCIeSDKDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_SLM_PWR_BUTTON, OnSlmPwrButton)
	ON_LBN_SELCHANGE(IDC_IMAGE_LISTBOX, OnSelchangeImageListbox)
	ON_BN_CLICKED(IDC_START_STOP_BUTTON, OnStartStopButton)
	ON_WM_TIMER()
	ON_WM_CLOSE()
	ON_BN_CLICKED(IDC_COMPENSATE_PHASE_CHECKBOX, OnCompensatePhaseCheckbox)
	ON_BN_CLICKED(IDC_UGA_BUTTON, &CBlinkPCIeSDKDlg::OnBnClickedUgaButton)
	ON_BN_CLICKED(IDC_SGA_BUTTON, &CBlinkPCIeSDKDlg::OnBnClickedSgaButton)
	ON_BN_CLICKED(IDC_OPT_BUTTON, &CBlinkPCIeSDKDlg::OnBnClickedOptButton)
	ON_BN_CLICKED(IDC_SETLUT, &CBlinkPCIeSDKDlg::OnBnClickedSetlut)
	ON_BN_CLICKED(IDC_SETWFC, &CBlinkPCIeSDKDlg::OnBnClickedSetwfc)
END_MESSAGE_MAP()



//////////////////////////////////////////////////////////////
//
//   OnInitDDialog()
//
//   Inputs: none
//
//   Returns: true or false depending on if an error was encountered
//
//   Purpose: This is called when the dialog is first constructed. This is a good place to 
//			  initialize variables for the program. First build the board class and set the
//			  run parameters. Then pre-load images to the hardware.
//
//   Modifications: With the new Blink SDK, opening communication has a new, more streamlined function 
//					(Blink_SDK(...)).Calculating TrueFrames for the FLC has its own function. Assumes 
//					only one board for now.
//
///////////////////////////////////////////////////////
BOOL CBlinkPCIeSDKDlg::OnInitDialog()
{
	//[CONSOLE OUTPUT]
	//Enable console output
	#ifdef _DEBUG
	if (!AllocConsole())
		AfxMessageBox(LPCTSTR("Output will not be shown!"));
	#endif

	//[BLINK SDK SETUP]
	//Construct a Blink_SDK instance w/o overdrive capability (TODO: implement overdrive)
	unsigned int bits_per_pixel = 8U;			 //8 -> small SLM, 16 -> large SLM
	bool isBlinkSuccess = true;
	bool RAM_write_enable = true;
	bool use_GPU_if_available = true;
	size_t max_transiet_frames = 20U;
	const char* static_regional_lut_file = NULL; //NULL -> no overdrive, actual LUT file -> yes overdrive

	blink_sdk = new Blink_SDK(bits_per_pixel, &numBoards, &isBlinkSuccess, is_LC_Nematic, RAM_write_enable, use_GPU_if_available, max_transiet_frames, NULL);

	if (!isBlinkSuccess)
		Utility::printLine("Blink SDK failed to construct -> Reopen Appplication");

	//[UI SETUP]
	CDialog::OnInitDialog();

	// Set the icon for this dialog (automatic if main window not a dialog)
	SetIcon(m_hIcon, TRUE);			// Set big icon

	// [CAMERA/BLINK SETUP]
	// Set true frames for this SLM
	// NOTE: if working with FLC SLM then TrueFrames must be calculated based on hardware parameters and FrameRate
	// Blink does this for us with blink_sdk->Compute_TF(float(FrameRate))
	if (!is_LC_Nematic)
		trueFrames = blink_sdk->Compute_TF(float(frameRate));
	blink_sdk->Set_true_frames(trueFrames);

	SLM_Board *pBoard = new SLM_Board;

	int board = 1; //TODO: setup other boards (1 based index)
	ImgWidth = blink_sdk->Get_image_width(1);
	ImgHeight = blink_sdk->Get_image_height(1);
	exposureTime = 2000; // in microseconds

	//Build paths to the calibrations for this SLM -- regional LUT included in Blink_SDK(), but need to pass NULL to that param to disable ODP. Might need to make a class.
	string SLMSerialNum = "linear"; // "slm929_at532_P8' -- original code had listed as CString, but I got errors. Need more research.
	string SLMSerialNumphase = "slm929_8bit"; // See above
	pBoard->LUTFileName.Format(_T("%s.LUT"), SLMSerialNum);
	pBoard->PhaseCompensationFileName.Format(_T("%s.bmp"), SLMSerialNumphase);
	pBoard->SystemPhaseCompensationFileName.Format(_T("Blank.bmp"));
	pBoard->FrameOne = new unsigned char[ImgWidth*ImgHeight];
	pBoard->FrameTwo = new unsigned char[ImgWidth*ImgHeight];
	pBoard->LUT = new unsigned char[256];
	pBoard->PhaseCompensationData = new unsigned char[ImgWidth*ImgHeight];
	pBoard->SystemPhaseCompensationData = new unsigned char[ImgWidth*ImgHeight];

	//Add setup information to board(s)
	SLM_board_list.push_back(pBoard);

	//Read the LUT file and write it to the hardware (images are processed through the LUT in the hardware) -- Load_LUT_file(int board, const char *LUT_file)
	ReadLUTFile(pBoard->LUT, pBoard->LUTFileName);
	const char* LUT_file = (const char*)pBoard->LUT;
	blink_sdk->Load_LUT_file(board, LUT_file);

	//load up the image arrays to each board
	LoadSequence();

	//add Image names to the listbox and select the first image
	m_ImageListBox.AddString(_T("ImageOne.bmp"));
	m_ImageListBox.AddString(_T("ImageTwo.zrn"));
	m_ImageListBox.SetCurSel(0);

	//Call OnSelchangeImageListbox to send the currently selected image from
	//	the PCIe card memory board to the SLM. User will immediately see first image in sequence. 
	OnSelchangeImageListbox();

	//initialize power button to indicate the power is currently off
	m_SlmPwrButton.SetWindowTextW(_T("Turn power ON"));

	//initialize sequencing button to indicate that we are not currently sequencing
	m_StartStopButton.SetWindowTextW(_T("START"));

	//if the liquid crystal type is nematic, then allow the user the option to
	//compensate for phase imperfections by applying a phase compensation image
	if (!is_LC_Nematic)
		m_CompensatePhaseCheckbox.ShowWindow(false);
	else
		m_CompensatePhaseCheckbox.ShowWindow(true);

	m_BSEdit.SetWindowTextW(_T("16"));
	m_NBEdit.SetWindowTextW(_T("32"));
	m_ISTEdit.SetWindowTextW(_T("2000"));
	m_IREdit.SetWindowTextW(_T("2"));
	m_MFEdit.SetWindowTextW(_T("0"));
	m_MSEEdit.SetWindowTextW(_T("60"));
	m_MFEEdit.SetWindowTextW(_T("0"));


	return TRUE;  // return TRUE  unless you set the focus to a control
}

/////////////////////////////////////////////////////////////////////
//
//   OnSysCommand() - MFC function
//
//   Inputs: none
//
//   Returns: none
//
//   Purpose: MFC function that calls the AboutBox dialog
//
//   Modifications: 
//
/////////////////////////////////////////////////////////////////////
void CBlinkPCIeSDKDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	CDialog::OnSysCommand(nID, lParam);
}

////////////////////////////////////////////////////////////////////////////
//
//   OnPaint() - MFC function
//
//   Inputs: none
//
//   Returns: none
//
//   Purpose: If you add a minimize button to your dialog, you will need the code below
//			  to draw the icon.  For MFC applications using the document/view model,
//			  this is automatically done for you by the framework.
//
//   Modifications:
//
/////////////////////////////////////////////////////////////////////////
void CBlinkPCIeSDKDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

/////////////////////////////////////////////////////////////////////////////
//
//   OnQueryDragIcon() - MFC function
//   
//   Inputs: none
//
//   Returns: handle to the cursor
//
//   Purpose: The system calls this function to obtain the cursor to display while the user drags
//			  the minimized window.
//
//   Modifications:
//
/////////////////////////////////////////////////////////////////////////////
HCURSOR CBlinkPCIeSDKDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

/////////////////////////////////////////////////////////////
//
//   AcquireImages()
//
//   Inputs: camera pointer, node map, TL device nodemap,
//			 address of image
//
//   Returns: 0 if successful, -1 else
//
//   Purpose: Acquire one image from a device
//
//   Modifications: Taken from Acquisition.cpp in Spinnaker SDK
//
/////////////////////////////////////////////////////////////
ImagePtr CBlinkPCIeSDKDlg::AcquireImages(CameraPtr pCam, INodeMap &nodeMap, INodeMap &nodeMapTLDevice)
{
	recordedTimes.push_back(RecordTime(startTime, frequency, "   AcquireImages -> Beginning"));
	QueryPerformanceCounter(&startTime);

	int result = 0;
	ImagePtr convertedImage = Image::Create();

	recordedTimes.push_back(RecordTime(startTime, frequency, "   AcquireImages -> Created Blank Image Pointer"));
	QueryPerformanceCounter(&startTime);

	int countReleased = 0;
	while (pCam->GetNumImagesInUse() != 0)
	{
		pCam->GetNextImage()->Release();
		countReleased++;
	}

	recordedTimes.push_back(RecordTime(startTime, frequency, "   AcquireImages -> Released " + to_string(countReleased) + " Old Image(s)"));
	QueryPerformanceCounter(&startTime);

	try
	{
		//set acquisition mode to continuous
		//retrieve enumeration node from nodemap
		CEnumerationPtr ptrAcquisitionMode = nodeMap.GetNode("AcquisitionMode");
		if (!IsAvailable(ptrAcquisitionMode) || !IsWritable(ptrAcquisitionMode))
		{
			Utility::printLine("Unable to set acquisition mode (enum retrieval).");
			return -1;
		}

		recordedTimes.push_back(RecordTime(startTime, frequency, "   AcquireImages -> Set Acquisition mode to SingleFrame"));
		QueryPerformanceCounter(&startTime);

		//retrieve entry node from enumeration node
		CEnumEntryPtr ptrAcquisitionModeContinuous = ptrAcquisitionMode->GetEntryByName("Continuous");//"SingleFrame");
		if (!IsAvailable(ptrAcquisitionModeContinuous) || !IsReadable(ptrAcquisitionModeContinuous))
		{
			Utility::printLine("Unable to set acquisition mode (entry retrieval).");
			return -1;
		}

		recordedTimes.push_back(RecordTime(startTime, frequency, "   AcquireImages -> Retrieved Entry Node"));
		QueryPerformanceCounter(&startTime);

		//retrieve integer value from entry node
		int64_t acquisitionModeContinuous = ptrAcquisitionModeContinuous->GetValue();

		recordedTimes.push_back(RecordTime(startTime, frequency, "   AcquireImages -> Retrieved Integer Value From Entry Node"));
		QueryPerformanceCounter(&startTime);

		//set value from entry node as new value of enum node
		ptrAcquisitionMode->SetIntValue(acquisitionModeContinuous);

		recordedTimes.push_back(RecordTime(startTime, frequency, "   AcquireImages -> Set value of entry node as new value of enum node"));
		QueryPerformanceCounter(&startTime);

		////begin acquisition
		//pCam->BeginAcquisition();
		//recordedTimes.push_back(RecordTime(startTime, frequency, "   AcquireImages -> Began Acquisition"));
		//QueryPerformanceCounter(&startTime);

		////retrieve device serial for filename
		//gcstring deviceSerialNumber("");
		//CStringPtr ptrStringSerial = nodeMapTLDevice.GetNode("DeviceSerialNumber");
		//if (IsAvailable(ptrStringSerial) && IsReadable(ptrStringSerial))
		//{
		//	deviceSerialNumber = ptrStringSerial->GetValue();
		//}
		//
		//retrieve image
		try
		{
			//retrieve next received image
			ImagePtr pResultImage = pCam->GetNextImage();

			recordedTimes.push_back(RecordTime(startTime, frequency, "   AcquireImages -> Retrieved Taken Image"));
			QueryPerformanceCounter(&startTime);

			//ensure image completion
			if (pResultImage->IsIncomplete())
			{
				recordedTimes.push_back(RecordTime(startTime, frequency, "   AcquireImages -> Ensured Image Completion"));
				QueryPerformanceCounter(&startTime);

				Utility::printLine("Image incomplete: " + string(Image::GetImageStatusDescription(pResultImage->GetImageStatus())));
			}
			else
			{
				recordedTimes.push_back(RecordTime(startTime, frequency, "   AcquireImages -> Ensured Image Completion"));
				QueryPerformanceCounter(&startTime);

				//Print The dimensions of the image (should be XX by YY)
				Utility::printLine("Current Image Dimensions : " + to_string(pResultImage->GetWidth()) + " by " + to_string(pResultImage->GetHeight()) + "\n");

				//copy image to pImage pointer
				convertedImage = pResultImage->Convert(PixelFormat_Mono8); // TODO try see if there is any performance gain if use -> , HQ_LINEAR);

				recordedTimes.push_back(RecordTime(startTime, frequency, "   AcquireImages -> Converted Image"));
				QueryPerformanceCounter(&startTime);
			}

			//release image
			pResultImage->Release();

			recordedTimes.push_back(RecordTime(startTime, frequency, "   AcquireImages -> Released Image"));
			QueryPerformanceCounter(&startTime);
		}
		catch (Spinnaker::Exception &e)
		{
			Utility::printLine("ERROR: " + string(e.what()));
			result = -1;
		}
		//end acquisition
		//pCam->EndAcquisition();

		//recordedTimes.push_back(RecordTime(startTime, frequency, "   AcquireImages -> Ended Acquisition"));
		//QueryPerformanceCounter(&startTime);
	}
	catch (Spinnaker::Exception &e)
	{
		Utility::printLine("ERROR: " + string(e.what()));
		result = -1;
	}
	return convertedImage;
}

//////////////////////////////////////////////////////////////////
//
//   ConfigureExposure()
//
//   Inputs: node map, custom exposure time
//
//   Returns: 0 if successful, -1 else
//
//   Purpose: configure a custom exposure time. Automatic exposure
//			  is turned off, then the custom setting is applied.
//
//   Modifications: taken from Exposure.cpp in Spinnaker SDK
//
///////////////////////////////////////////////////////////////////
int CBlinkPCIeSDKDlg::ConfigureExposure(INodeMap &nodeMap, double exposureTimeToSet)
{
	if (exposureTimeToSet < 4.9) //HUGE TODO WHY DOES IT GO TO 3.9 
		exposureTimeToSet = 4.9;

	Utility::printLine("Configuring exposure to: " + to_string(exposureTimeToSet));

	int result = 0;
	try
	{
		//turn off automatic exposure mode
		CEnumerationPtr ptrExposureAuto = nodeMap.GetNode("ExposureAuto");
		if (!IsAvailable(ptrExposureAuto) || !IsWritable(ptrExposureAuto))
		{
			Utility::printLine("Unable to disable automatic exposure (node retrieval)");
			return -1;
		}
		CEnumEntryPtr ptrExposureAutoOff = ptrExposureAuto->GetEntryByName("Off");
		if (!IsAvailable(ptrExposureAutoOff) || !IsReadable(ptrExposureAutoOff))
		{
			Utility::printLine("Unable to disable automatic exposure (enum entry retrieval)");
			return -1;
		}

		ptrExposureAuto->SetIntValue(ptrExposureAutoOff->GetValue());

		//set exposure manually
		CFloatPtr ptrExposureTime = nodeMap.GetNode("ExposureTime");
		if (!IsAvailable(ptrExposureTime) || !IsWritable(ptrExposureTime))
		{
			Utility::printLine("Unable to set exposure time.");
			return -1;
		}
		//ensure new time does not exceed max
		const double exposureTimeMax = ptrExposureTime->GetMax();
		if (exposureTimeToSet > exposureTimeMax)
			exposureTimeToSet = exposureTimeMax;

		ptrExposureTime->SetValue(exposureTimeToSet);
	}
	catch (Spinnaker::Exception &e)
	{
		Utility::printLine("Cannot Set Exposure Time:\n" + string(e.what()));
		return -1;
	}

	return result;
}

///////////////////////////////////////////////////////////////////////////
//
//   ConfigureCustomImageSettings()
//
//   Inputs: camera pointer, Frames per Second
//
//   Outputs: 0 if successful, -1 else
//
//   Purpose: Configures the camera to run at desired frame rate, and 
//			  return images with correct AOI
//
//   Modifications: Taken from ImageFormatControl_Quickspin in the Spinnaker SDK,
//					logic for FPS added
//
////////////////////////////////////////////////////////////////////////////
int CBlinkPCIeSDKDlg::ConfigureCustomImageSettings(CameraPtr pCam, INodeMap &nodeMap, int FPS)
{
	//Check if passed in camera is valid
	Utility::printLine();
	if (pCam.IsValid())
		Utility::printLine("CAM VALID");
	else
	{
		Utility::printLine("CAM INVALID");
		return -1;
	}

	int result = 0;
	int x0 = 752;	    //  Must be a factor of 4 (like 752)
	int y0 = 752;		//	Must be a factor of 2 (like 752)
	int width = 64;		//	Must be a factor of 32 (like 64)
	int height = 64;	//	Must be a factor of 2  (like 64)
	try
	{
		Utility::printLine("[BEGGINING TO CONFIGURE CAMERA SETTINGS]");

		//apply mono 8 pixel format
		if (pCam->PixelFormat != NULL && pCam->PixelFormat.GetAccessMode() == RW)
		{
			pCam->PixelFormat.SetValue(PixelFormat_Mono8);
			Utility::printLine("Pixel format set to mono...");
		}
		else
		{
			Utility::printLine("Pixel format not available...");
			result = -1;
		}

		//apply initial zero offset in x direction
		if (pCam->OffsetX != NULL && pCam->OffsetX.GetAccessMode() == RW)
		{
			pCam->OffsetX.SetValue(0);
			Utility::printLine("OffsetX set at: 0");
		}
		else
		{
			Utility::printLine("OffsetX not available for initial setup");
			result = -1;
		}


		//apply initial zero offset in y direction
		if (pCam->OffsetY != NULL && pCam->OffsetY.GetAccessMode() == RW)
		{
			pCam->OffsetY.SetValue(0);
			Utility::printLine("OffsetY set at: 0");
		}
		else
		{
			Utility::printLine("OffsetY not available for initial setup");
			result = -1;
		}

		//apply width
		if (pCam->Width != NULL && pCam->Width.GetAccessMode() == RW && pCam->Width.GetInc() != 0 && pCam->Width.GetMax() != 0)
		{
			pCam->Width.SetValue(width);
			Utility::printLine("Image width set at: " + to_string(width));
		}
		else
		{
			Utility::printLine("Width not available");
			result = -1;
		}

		//apply height
		if (pCam->Height != NULL && pCam->Height.GetAccessMode() == RW && pCam->Height.GetInc() != 0 && pCam->Height.GetMax() != 0)
		{
			pCam->Height.SetValue(height);
			Utility::printLine("Image height set at: " + to_string(height));
		}
		else
		{
			Utility::printLine("Height not available");
			result = -1;
		}

		//apply offset in x direction
		if (pCam->OffsetX != NULL && pCam->OffsetX.GetAccessMode() == RW)
		{
			pCam->OffsetX.SetValue(x0);
			Utility::printLine("OffsetX set at: " + to_string(x0));
		}
		else
		{
			Utility::printLine("OffsetX not available");
			result = -1;
		}

		//apply offset in y direction
		if (pCam->OffsetY != NULL && pCam->OffsetY.GetAccessMode() == RW)
		{
			pCam->OffsetY.SetValue(y0);
			Utility::printLine("OffsetY set at: " + to_string(y0));
		}
		else
		{
			Utility::printLine("OffsetY not available");
			result = -1;
		}

		//set the frame rate
		CBooleanPtr AcquisitionFrameRateEnable = pCam->GetNodeMap().GetNode("AcquisitionFrameRateEnable");

		if (IsAvailable(AcquisitionFrameRateEnable) && IsReadable(AcquisitionFrameRateEnable))
		{
			AcquisitionFrameRateEnable->SetValue(1);

			if (pCam->AcquisitionFrameRate != NULL && pCam->AcquisitionFrameRate.GetAccessMode() == RW)
			{
				pCam->AcquisitionFrameRate.SetValue(FPS);
				Utility::printLine("Frame rate set at:" + FPS);
			}
			else
			{
				Utility::printLine("Frame rate setup not available");
				result = -1;
			}
		}
		else
		{
			Utility::printLine("Manual frame rate enabling not available because:");
			Utility::printLine("IS Avaliable: " + to_string(IsAvailable(AcquisitionFrameRateEnable)) + " | IS Readable: " + to_string(IsReadable(AcquisitionFrameRateEnable)));
		}

		//set gamma
		CBooleanPtr AcquisitionGammaEnable = pCam->GetNodeMap().GetNode("GammaEnable");
		if (IsAvailable(AcquisitionGammaEnable) && IsReadable(AcquisitionGammaEnable))
		{
			AcquisitionFrameRateEnable->SetValue(1);

			if (pCam->Gamma != NULL && pCam->Gamma.GetAccessMode() == RW)
			{
				pCam->Gamma.SetValue(100);
				Utility::printLine("Gamma was set to 100");
			}
			else
			{
				Utility::printLine("Gamma not available");
				result = -1;
			}
		}
		else
		{
			Utility::printLine("Manual gamma enabling not available because:");
			Utility::printLine("IS Avaliable: " + to_string(IsAvailable(AcquisitionGammaEnable)) + " | IS Readable: " + to_string(IsReadable(AcquisitionGammaEnable)));

		}
	}
	catch (Spinnaker::Exception &e)
	{
		Utility::printLine("Error Setting Camera Options:\n" + string(e.what()));
		result = -1;
	}
	return result;
}

////////////////////////////////////////////////////////////////////////////
//
//   LoadSequence()
//
//   Inputs: none
//
//   Returns: none
//
//   Purpose: This function will load a series of two images to the PCIe boad memory. The
//			  first image is written to the first frame of memory in the hardware, and the
//			  second image is written to the second frame.
//
//   Modifications: Might need to change how we get a handle to the board, how we access
//					PhaseCompensationData, SystemCompensationData, FrameOne, and FrameTwo.
//					Once we're using Overdrive, I think it'll be easier because the
//					construction of the sdk includes a correction file, and even in the 
//					old code used the same correction file.
//
///////////////////////////////////////////////////////////////////////////
void CBlinkPCIeSDKDlg::LoadSequence()
{
	int i;

	//loop through each PCIe board found, read in the calibrations for that SLM
	//as well as the images in the wequence list, and store the merged image/calibration
	//data in an array that will later be referenced
	for (int blist = 0; blist < numBoards; blist++) //please note: BoardList has a 0-based index, but Blink's index of available boards is 1-based.
	{
		SLM_Board* pBoard = SLM_board_list[0];
		//if the user has a nematic SLM and the user is viewing the SLM through an interferometer, 
		//then the user has the option to correct for SLM imperfections by applying a predetermined 
		//phase compensation. BNS calibrates every SLM before shipping, so each SLM ships with its 
		//own custom phase correction file. This step is either reading in that correction file, or
		//setting the data in the array to 0 for users that don't want to correct, or for users with
		//amplitude devies
		if (m_CompensatePhase)
		{
			ReadAndScaleBitmap(pBoard->PhaseCompensationData, pBoard->PhaseCompensationFileName); //save to PhaseCompensationData
			ReadAndScaleBitmap(pBoard->SystemPhaseCompensationData, pBoard->SystemPhaseCompensationFileName); //save to SystemPhaseCompensationData
		}
		else
		{
			memset(pBoard->PhaseCompensationData, 0, ImgHeight*ImgWidth); //set PhaseCompensationData to 0
			memset(pBoard->SystemPhaseCompensationData, 0, ImgHeight*ImgWidth); //set SystemPhaseCompensationData to 0
		}

		//Read in the first image -- this one is a bitmap
		ReadAndScaleBitmap(pBoard->FrameOne, _T("ImageOne.bmp")); //save to FrameOne

		int total;
		//Superimpose the SLM phase compensation, the system phase compensation, and
		//the image data. Then store the image in FrameOne
		for (i = 0; i < ImgHeight*ImgWidth; i++)
		{
			total = pBoard->FrameOne[i] +
				pBoard->PhaseCompensationData[i] +
				pBoard->SystemPhaseCompensationData[i];

			pBoard->FrameOne[i] = total % 256;
		}

		//Read in the second image. For the sake of an example this is a zernike file
		ReadZernikeFile(pBoard->FrameTwo, _T("ImageTwo.zrn"));

		//Superimpose the SLM phase compensation, the system phase compensation, and
		//the image data. Then store the image in FrameTwo
		for (i = 0; i < ImgHeight*ImgWidth; i++)
		{
			total = pBoard->FrameTwo[i] +
				pBoard->PhaseCompensationData[i] +
				pBoard->SystemPhaseCompensationData[i];

			pBoard->FrameTwo[i] = total % 256;
		}
	}
}

///////////////////////////////////////////////
//
//   OnSlmPwrButton()
//
//   Inputs: none
//
//   Returns: none
//
//   Purpose: Turn on or off our SLM power
//
//   Modifications: New SDK does not have a GetPower() function, so
//					we need a new way of checking the power state.
//					This method assumes SLM is off initially, and uses 
//					the button text to check power state.
//
///////////////////////////////////////////////
void CBlinkPCIeSDKDlg::OnSlmPwrButton()
{
	//Check power state using the button text (see modification note), and set the power to the opposite
	for (int board = 1; board <= numBoards; board++) //Blink sdk uses a 1-based index for the boards
	{
		CString PowerState;
		m_SlmPwrButton.GetWindowTextW(PowerState);
		CStringA pState(PowerState);

		if (strcmp(pState, "Turn power ON") == 0) //the strings are equal, power is off
		{
			blink_sdk->SLM_power(board, true); //turn the SLM on
			m_SlmPwrButton.SetWindowTextW(_T("Turn power OFF")); //update button
		}
		else
		{
			blink_sdk->SLM_power(board, false); //turn the SLM off
			m_SlmPwrButton.SetWindowTextW(_T("Turn power ON")); //update button
		}
	}
}

//////////////////////////////////////////////
//
//   OnSelchangeImageListbox()
//
//   Inputs: none
//
//   Returns: none
//
//   Purpose: This function allows the user to select an image from the image list, then see the image on the SLM
//
//   Modifications:
//
//////////////////////////////////////////////
void CBlinkPCIeSDKDlg::OnSelchangeImageListbox()
{
	//figure out which image in the list was just selected
	int sel = m_ImageListBox.GetCurSel();
	if (sel == LB_ERR) //nothing selected
		return;

	for (int blist = 0; blist < numBoards; blist++) //please note: BoardList is 0-based, but the Blink index for the boards is 1-based. 
		//The data stored in BoardList[0] corresponds to board = 1 for the sdk functions
	{
		int board = blist + 1;
		SLM_Board* pBoard = SLM_board_list[0];

		//send the data of the selected image in the sequence list out to the SLM
		if (sel == 0)
			blink_sdk->Write_image(board, pBoard->FrameOne, ImgHeight, false, false, 0.0);
		if (sel == 1)
			blink_sdk->Write_image(board, pBoard->FrameTwo, ImgHeight, false, false, 0.0);
	}
}

/////////////////////////////////////////////
//
//   OnStartStopButton()
//
//   Inputs: none
//
//   Returns: none
// 
//   Purpose: Check if the system is running. If so, ask the system to stop by calling
//			  SetRunMode with MEMORY_MODE. Otherwise, disable buttons so the user 
//			  can't edit while running, and start the system running. Call SetTimer
//			  so that OnTimer is called to sync our display and hardware.
//
//   Modifications:
//
///////////////////////////////////////////////
void CBlinkPCIeSDKDlg::OnStartStopButton()
{
	//There's a lot of code in the last version, but it's all commented out. It was replaced by the uGA and SGA functions.
	//This button is now just used in those functions to check if we're sequencing.
	return;
}

/////////////////////////////////////////////////
//
//   OnTimer()
//
//   Inputs: none
//
//   Returns: none
//
//   Purpose: This function is called periodically after the user has started the
//			  software sequencing through the images in the image list. This is
//			  used to update the dialog with the current state of the hardware.
//			  If the hardware is displaying image 1 on the SLM, that image will be
//			  selected in the listbox.
//
//   Modifications: Spinnaker code modified from Acquisition.cpp.
//
/////////////////////////////////////////////////
void CBlinkPCIeSDKDlg::OnTimer(UINT_PTR nIDEvent)
{
	//take picture

	//retrieve singleton reference to system object
	SystemPtr system = System::GetInstance();
	//retrieve list of cameras from the system
	CameraList camList = system->GetCameras();
	unsigned int numCameras = camList.GetSize();
	//make an image pointer
	ImagePtr pImage = Image::Create();
	//finish if there are no cameras
	if (numCameras == 0)
	{
		//clear camera list before releasing system
		camList.Clear();
		system->ReleaseInstance();
		Utility::printLine("Only ONE camera, please!");
	}
	int result = 0;
	int err = 0;
	try
	{
		//create a shared pointer to the camera--the pointer will automatically clean up after the try
		pCam = camList.GetByIndex(0);
		INodeMap &nodeMapTLDevice = pCam->GetTLDeviceNodeMap();
		pCam->Init();
		INodeMap &nodeMap = pCam->GetNodeMap();

		err = ConfigureExposure(nodeMap, exposureTime);
		if (err < 0)
		{
			MessageBox(_T("Could not configure exposure"), _T("ERROR"), MB_ICONERROR);
		}

		pImage = AcquireImages(pCam, nodeMap, nodeMapTLDevice);
		pCam->DeInit();

	}
	catch (Spinnaker::Exception &e)
	{
		Utility::printLine("ERROR: " + string(e.what()));
	}
	//save image to char vector
	width = pImage->GetWidth();
	height = pImage->GetHeight();
	camImage = static_cast<unsigned char*>(pImage->GetData());
	//clear the cam list, and release the memory and system
	pCam = NULL;
	camList.Clear();
	pImage->Release();
	system->ReleaseInstance();

	//find fitness
	double fitness = FindAverageValue(camImage, targetMatrix, width, height);
	populationPointer->SetFitness(fitness);
	rtime << fitness << endl;
	camDisplay->UpdateDisplay(camImage);

	if (populationCounter < 5) populationCounter++;
	else
	{
		populationCounter = 1;
		populationPointer->NextGeneration(0);
	}

	//send the data of the currently selected image in the sequence list out to the SLM
	ImgScaleMngr->TranslateImage(populationPointer->GetImage(), ary);
	for (int blist = 0; blist < numBoards; blist++)
	{
		int board = blist + 1; //sdk uses 1-based index for boards
		blink_sdk->Write_image(board, ary, 512, false, false, 0.0);
	}

	CDialog::OnTimer(nIDEvent);
}

//////////////////////////////////////////////////
//
//   FindAverageValue()
//
//   Inputs: Image -> the camera image to use
//			 target -> area to find average within, I believe
//			 width -> the width of the camera image in pixels
//			 height -> the height of the camera image in pixels
//
//   Returns: The average intensity within the calculated area
//
//   Purpose: Calculate an average intensity from an image taken by the camera
//			  which can be used as a fitness value
//
//   Modifications: Changed from "peakvalue()," most of the stuff was cut out
//
//////////////////////////////////////////////////
double CBlinkPCIeSDKDlg::FindAverageValue(unsigned char *Image, int* target, size_t width, size_t height)
{
	Mat m_ary(width, height, CV_8UC1, Image);
	double sum = 0;
	double area = 0;
	int x0, y0, r;
	x0 = 32;
	y0 = 32;
	r = 2;

	for (int i = x0 - r; i < x0 + r; i++)
	{
		for (int j = y0 - r; j < y0 + r; j++)
		{
			sum += m_ary.at<unsigned char>(i, j);// *target[j*width + i];
			area += 1;
		}
	}
	//////////////////////////test
	/*ofstream efile("target.txt", ios::app);
	efile << sum << "  " << area << endl;
	efile.close();*/
	/////////////////

	return sum / area;
}

//////////////////////////////////////////////////
//
//   GenerateTargetMatrix_SinglePoint()
//
//   Inputs: target -> where it's saved
//			 width -> the width of the camera image in pixels
//			 height -> the height of the camera image in pixels
//			 radius -> the radius of the single point
//
//   Returns: none
//
//   Purpose: This function is used to create an image of a single dot,
//			  centered in the image.
//
//   Modifications:
//
//////////////////////////////////////////////////
void CBlinkPCIeSDKDlg::GenerateTargetMatrix_SinglePoint(int *target, int width, int height, int radius)
{
	int centerX = width / 2;
	int centerY = height / 2;
	int ymin = centerY - radius;
	int ymax = centerY + radius;
	double xmin, xmax;
	for (int i = 0; i < width * height; i++)
	{
		target[i] = -1;
	}
	for (int i = ymin; i < ymax; i++)
	{
		xmin = centerX - sqrt(pow(radius, 2) - pow(i - centerY, 2)); // calculate the dimensions of roughly circle
		xmax = centerX + sqrt(pow(radius, 2) - pow(i - centerY, 2));
		for (int j = xmin; j < xmax; j++)
		{
			target[i*width + j] = 500;
		}
	}
	ofstream efile("targetmat.txt", ios::app);
	for (int i = 0; i < width; i++)
	{
		for (int j = 0; j < height; j++)
		{
			efile << target[j*width + i] << ' ';
		}
		efile << endl;
	}
	efile.close();
}

//////////////////////////////////////////////////
//
//   GenerateTargetMatrix_LoadFromFile()
//
//   Inputs: target -> where we save the matrix
//			 width -> width of camera image in pixels
//			 height -> height of camera image in pixels
//
//   Returns: none
//
//   Purpose: Reads a file to create a target matrix.
//
//   Modifications: removed the radius input, as it was never used.
//
///////////////////////////////////////////////////
void CBlinkPCIeSDKDlg::GenerateTargetMatrix_LoadFromFile(int *target, int width, int height)
{
	//Load file
	string targfilename = "DRAWN_TARGET.TXT";
	ifstream targfile(targfilename);
	if (targfile.fail())
	{
		throw new exception("Failed Target Matrix Load: Could Not Open File");
	}
	vector<vector<int>> targvec;
	int targetwidth = 0;
	while (!targfile.eof())
	{
		string line;
		getline(targfile, line);
		if (line.length() > targetwidth) //find widest point
		{
			targetwidth = line.length();
		}

		vector<int> targline;
		for (int i = 0; i < line.length(); i++)
		{
			if (line[i] == ' ')
			{
				targline.push_back(0);
			}
			else
			{
				targline.push_back(1);
			}
		}
		targvec.push_back(targline);
	}
	//calculate size and middle
	int targetheight = targvec.size();
	int xStart = (width / 2) - (targetwidth / 2);
	int yStart = (height / 2) - (targetheight / 2);

	if (xStart < 0 || yStart < 0)
	{
		throw new exception("Failed Target Matrix Load: Target Matrix Too Big");
	}

	//apply loaded mat to target

	for (int i = 0; i < width * height; i++)
	{
		target[i] = 0;
	}

	for (int i = 0; i < targetheight; i++)
	{
		for (int j = 0; j < targetwidth; j++)
		{
			target[((yStart + i) * width) + (xStart + j)] = targvec[i][j];
		}
	}

	ofstream efile("targetmat.txt", ios::app);
	for (int i = 0; i < height; i++)
	{
		for (int j = 0; j < width; j++)
		{
			efile << target[i*width + j] << ' ';
		}
		efile << endl;
	}
	efile.close();
}

/////////////////////////////////////////////////
//
//   OnClose()
//
//   Inputs: none
//
//   Returns: none
//
//   Purpose: This function is used to clean up at the close of the program. 
//			  It is called when the user clicks the little X in the upper corner. 
//			  MAKE SURE THE SLM POWER IS OFF BEFORE LEAVING THE PROGRAM!!
//
//   Modifications:
//
///////////////////////////////////////////////////
void CBlinkPCIeSDKDlg::OnClose()
{
	//Cancel console output
#ifdef _DEBUG
	if (!FreeConsole())
		AfxMessageBox(LPCTSTR("Could not free the console!"));
#endif

	for (int blist = 0; blist < numBoards; blist++)
	{
		//get a handle to the BoardList
		SLM_Board* pBoard = SLM_board_list[0];

		int board = blist + 1; //BoardList is 0-indexed, Blink sdk uses a 1-based index for the boards
		//shut off SLM power
		blink_sdk->SLM_power(board, false);
		//deconstruct the instance
		blink_sdk->~Blink_SDK();
		//clean up allocated memory
		delete[]pBoard->FrameOne;
		delete[]pBoard->FrameTwo;
		delete[]pBoard->PhaseCompensationData;
		delete[]pBoard->SystemPhaseCompensationData;
		delete[]pBoard->LUT;
		delete pBoard;
	}
	CDialog::OnClose();
}

/////////////////////////////////////////////////////////
//
//   OnOK()
//
//   Inputs: none
//
//   Returns: none
//
//   Purpose: This function is used so you don't accidentally exit the program
//			  by hitting the enter button. This function now causes nothing to 
//			  happen if enter is clicked. It forces the user to purposely exit
//			  the program using the exit button, which will ensure that the proper
//			  shutdown procedure of the hardware is followed. This will prevent
//			  accidental damage to the hardware and/or SLM.
//
//   Modifications:
//
////////////////////////////////////////////////////////////
void CBlinkPCIeSDKDlg::OnOK()
{

}

/////////////////////////////////////////////////////
//
//   OnCompensatePhaseCheckbox()
//
//   Inputs: none
//
//   Returns: none
//
//   Purpose: This function is called if the ser clicks on the checkbox labeled "apply phase compensation."
//			  This will superimpose a phase correction image with the desired image
//			  being downloaded to the SLM. This option should ony be used if the user
//			  is driving a nematic SLM using a phase setup (interferometer). Using the
//			  appropriate phase correction file will compensate for slight imperfections
//			  (curvature) in the SLM, thus making the SLM appear nearly flat.
//
//   Modifications:
//
//////////////////////////////////////////////////
void CBlinkPCIeSDKDlg::OnCompensatePhaseCheckbox()
{
	UpdateData(true);
	//re-load the sequence so that it is consistent with the current state of the
	//phase correction chechkbox. If the box was checked, the phase correction image
	//will be superimposed. If it was un-checked, the phase correction will be removed.
	LoadSequence();

	//load the currently selected image to the SLM
	OnSelchangeImageListbox();
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
bool CBlinkPCIeSDKDlg::ReadAndScaleBitmap(unsigned char *Image, CString filename)
{
	int width, height, bytespixel;

	//need a tmpImage because we cannot assume that the bitmap we
	//read in will be the correct dimensions
	unsigned char* tmpImage;

	//get a handle to our file
	CFile *pFile = new CFile();
	if (pFile == NULL)
		MessageBox(_T("Error allocating memory for pFile, ReadAndScaleBitmap"), _T("MEMORY ERROR"), MB_ICONERROR);

	//if it is a .bmp file and we can open it
	if (pFile->Open(filename, CFile::modeRead | CFile::shareDenyNone, NULL))
	{
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
		for (int i = 0; i < height; i++)
		{
			for (int j = 0; j < width; j++)
			{
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
	else
	{
		//depending on if we are trying to read a bitmap to dowload
		//or if we are trying to read it for the screen, memset
		//the array to zero and return false
		memset(Image, '0', ImgWidth*ImgHeight);
		return false;
	}

	//scale the bitmap
	unsigned char* ScaledImage = ScaleBitmap(tmpImage, height, ImgHeight);

	//copy the scaled bitmap into the array passed into the function
	memcpy(Image, ScaledImage, ImgWidth*ImgHeight);

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
unsigned char* CBlinkPCIeSDKDlg::ScaleBitmap(unsigned char* InvertedImage, int BitmapSize, int FinalBitmapSize)
{
	int height = BitmapSize;
	int width = BitmapSize;

	//make an array to hold the scaled bitmap
	unsigned char* ScaledImage = new unsigned char[FinalBitmapSize*FinalBitmapSize];
	if (ScaledImage == NULL)
		AfxMessageBox(_T("Error allocating memory for CFile,LoadSIFRec"), MB_OK);

	//EXPAND THE IMAGE to FinalBitmapSize
	if (height < FinalBitmapSize)
	{
		int r, c, row, col, Index; //row and col correspond to InvertedImage
		int Scale = FinalBitmapSize / height;

		for (row = 0; row < height; row++)
		{
			for (col = 0; col < width; col++)
			{
				for (r = 0; r < Scale; r++)
				{
					for (c = 0; c < Scale; c++)
					{
						Index = ((row*Scale) + r)*FinalBitmapSize + (col*Scale) + c;
						ScaledImage[Index] = InvertedImage[row*height + col];
					}
				}
			}
		}
	}
	//SHRINK THE IMAGE to FinalBitmapSize
	else if (height > FinalBitmapSize)
	{
		int Scale = height / FinalBitmapSize;
		for (int i = 0; i < height; i += Scale)
		{
			for (int j = 0; j < width; j += Scale)
			{
				ScaledImage[(i / Scale) + FinalBitmapSize*(j / Scale)] = InvertedImage[i + height*j];
			}
		}
	}
	//if the image is already the correct size, just copy the array over
	else
	{
		memcpy(ScaledImage, InvertedImage, FinalBitmapSize*FinalBitmapSize);
	}
	return(ScaledImage);
}

////////////////////////////////////////////////////////////////////////////
//
//   ReadLUTFile()
//  
//   Inputs: the name of the LUT file to read, and an array in which to store it
//
//   Returns: true if read successfully; false if linear.lut was generated
//
//   Purpose: This will read in the LUT file. This is a look up table through which
//			  we process our images. It maps the pixel values to the values specified
//			  by the LUT. FOr exampe, with linear.lut, we have a direct mapping; if 
//			  the pixel value is 255, linear.lut will keep it at 255. However, 
//			  skew.lut will alter the pixel values. With skew.lut, if the initial
//			  pixel value is 60, it is mapped to a value of 139.
//
//   Modifications:
//
/////////////////////////////////////////////////////////////////////////////
bool CBlinkPCIeSDKDlg::ReadLUTFile(unsigned char *LUTBuf, CString LUTPath)
{
	FILE *stream;
	int i, seqnum, ReturnVal, tmpLUT;
	bool errorFlag;

	//set the error flag to indicate that there are no errors so far, and open
	//the LUT file
	errorFlag = false;
	CStringA filename(LUTPath);

	stream = fopen((const char*)filename, "r");
	if ((stream != NULL) && (errorFlag == false))
	{
		//read in all 256 values
		for (i = 0; i < 256; i++)
		{
			ReturnVal = fscanf(stream, "%d %d", &seqnum, &tmpLUT);
			if (ReturnVal != 2 || seqnum != i || tmpLUT < 0 || tmpLUT > 255)
			{
				fclose(stream);
				errorFlag = true;
				break;
			}
			LUTBuf[i] = (unsigned char)tmpLUT;
		}
		fclose(stream);
	}
	//if there was an error reading in the LUT, default to a linear LUT
	if ((stream == NULL) || (errorFlag == true))
	{
		for (i = 0; i < 256; i++)
			LUTBuf[i] = i;
		return false;
	}

	return TRUE;
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
bool CBlinkPCIeSDKDlg::ReadZernikeFile(unsigned char *GeneratedImage, CString fileName)
{
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
	if (ImgHeight == 512)
		Radius = 300;
	else
		Radius = 150;

	//open the zernike file to read
	ifstream ZernikeFile(fileName);
	if (ZernikeFile.is_open())
	{
		while (ZernikeFile.getline(inBuf, MAX_ZERNIKE_LINE, '\n'))
		{
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
		y = ImgHeight / 2;
		for (row = 0; row < ImgHeight; row++)
		{
			//reset x
			x = (ImgWidth / 2) * -1;
			for (col = 0; col < ImgWidth; col++)
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
				GeneratedImage[row*ImgWidth + col] = int((total)* 255);

				x++;
			}//close col loop
			y--;
		}//close row loop

		return true;
	}
	//if we could not open the zernike file
	else
	{
		memset(GeneratedImage, 0, ImgHeight*ImgWidth);
		return false;
	}
}

/////////////////////////////////////////////////////////
//
//   OnBnClickedUgaButton()
//
//   Inputs: none
//
//   Returns: none
//
//   Purpose: run the sequence for a uGA process
//
//   Modifications:
//
////////////////////////////////////////////////////////

void CBlinkPCIeSDKDlg::OnBnClickedUgaButton()
{
	Utility::printLine("UGA BUTTON CLICKED!");

	//check if we are currently sequencing. If not, start; if so, stop.
	CString RunState;
	m_StartStopButton.GetWindowText(RunState);
	CStringA WindowText(RunState);
	if (strcmp((const char*)WindowText, "START") == 0)
	{
		//Image Size
		int cameraImageWidth = 64;
		int cameraImageHeight = 64;
		double FPS = 300;
		double initialExposureTime = 2000;

		try
		{
			CString path;
			m_ISTEdit.GetWindowTextW(path);
			if (path.IsEmpty()) throw new exception();
			initialExposureTime = _tstof(path);
		}
		catch (...)
		{
			MessageBox(_T("Error Parsing Initial Shutter Time"), _T("PARSING ERROR"), MB_ICONERROR);
			return;

		}

		//[GENETIC ALGORITHM PARAMETERS]
		//Percentage of similarity between two genomes to be considered thesame (need value of less than 1 to converge)
		double acceptedSimilarity = .97;
		//Max allowed fitness value - NOTE: if reached will half the exposure time (TODO: check this feature)
		double maxFitnessValue = 30;

		int popImageWidth = 32;
		try
		{
			CString path;
			m_NBEdit.GetWindowTextW(path);
			if (path.IsEmpty()) throw new exception();
			popImageWidth = _tstof(path);
		}
		catch (...)
		{
			MessageBox(_T("Error Parsing Number of Bins"), _T("PARSING ERROR"), MB_ICONERROR);
			return;
		}
		int popImageHeight = popImageWidth;
		int popImageDensity = 1; //bin parameters

		//stop conditions
		double fitnessToStop = 0;
		try
		{
			CString path;
			m_MFEdit.GetWindowTextW(path);
			if (path.IsEmpty()) throw new exception();
			fitnessToStop = _tstof(path);
		}
		catch (...)
		{
			MessageBox(_T("Error Parsing Minimum Fitness"), _T("PARSING ERROR"), MB_ICONERROR);
			return;
		}
		double secondsToStop = 60;
		try
		{
			CString path;
			m_MSEEdit.GetWindowTextW(path);
			if (path.IsEmpty()) throw new exception();
			secondsToStop = _tstof(path);
		}
		catch (...)
		{
			MessageBox(_T("Error Parsing Minimum Seconds Elapsed"), _T("PARSING ERROR"), MB_ICONERROR);
			return;
		}
		int functionEvalsToStop = 0;
		try
		{
			CString path;
			m_MFEEdit.GetWindowTextW(path);
			if (path.IsEmpty()) throw new exception();
			functionEvalsToStop = _tstof(path);
		}
		catch (...)
		{
			MessageBox(_T("Error Parsing Minimum Function Evaluations"), _T("PARSING ERROR"), MB_ICONERROR);
			return;
		}
		int gennum = 3000; //max number of generations to run, assuming no other stop conditions are met (each gen is 5 func evals)
		bool saveImages = false; //set to true to save images of the fittest individual of each gen
		bool displayCameraImage = true; //set to true to open a window showing the camera image

		//bin parameters
		int binSizeX = 16;
		try
		{
			CString path;
			m_BSEdit.GetWindowTextW(path);
			if (path.IsEmpty()) throw new exception();
			binSizeX = _tstof(path);
		}
		catch (...)
		{
			MessageBox(_T("Error Parsing Bin Size"), _T("PARSING ERROR"), MB_ICONERROR);
			return;
		}
		int binSizeY = binSizeX;
		int numBinsX = popImageWidth;
		int numBinsY = popImageHeight;

		int targetRadius = 2;
		try
		{
			CString path;
			m_IREdit.GetWindowTextW(path);
			if (path.IsEmpty()) throw new exception();
			targetRadius = _tstof(path);
		}
		catch (...)
		{
			MessageBox(_T("Error Parsing Integration Radius"), _T("PARSING ERROR"), MB_ICONERROR);
			return;
		}
		//END PARAMETERS

		//indicate we are currently sequencing
		m_StartStopButton.EnableWindow(false);
		//disable some of the buttons in the dialog so that the user
		//cannot do anything while sequencing, other than stop sequencing
		m_ImageListBox.EnableWindow(false);
		m_SlmPwrButton.EnableWindow(false);
		m_uGAButton.EnableWindow(false);
		m_SGAButton.EnableWindow(false);
		m_OptButton.EnableWindow(false);

		rtime.open("rtime.txt", ios::app);

		//*****************!! NOTE - DON'T DAMAGE THE SLM !! *****************************
		//if your frame rate is not the same frame rate that was used in OnInitDialog, and
		//if the LC type is FLC then it is VERY IMPORTANT that true frames be recalculated
		//prior to calling SetTimer such that the Frame Rate and True Frames are properly
		//related
		if (!is_LC_Nematic) //FLC
		{
			trueFrames = blink_sdk->Compute_TF(float(frameRate));
		}
		blink_sdk->Set_true_frames(trueFrames);

		targetMatrix = new int[cameraImageHeight*cameraImageWidth];
		GenerateTargetMatrix_SinglePoint(targetMatrix, cameraImageWidth, cameraImageHeight, targetRadius);

		float FrameRateMs = (1.0 / (float)frameRate) * 1000.0;

		//start camera
		int resultCam = 0;
		//retrieve singleton reference to system object
		SystemPtr system = System::GetInstance();
		//retrieve a list of cameras from the system
		CameraList camList = system->GetCameras();
		unsigned int numCameras = camList.GetSize();
		//ensure only one camera
		if (numCameras != 1)
		{
			//clear camera list before releasing system
			camList.Clear();
			system->ReleaseInstance();
			Utility::printLine("only 1 camera, please");
		}

		try
		{
			pCam = camList.GetByIndex(0);
			//retrieve TL device nodemap
			INodeMap &nodeMapTLDevice = pCam->GetTLDeviceNodeMap();
			//initialize camera
			pCam->Init();
			//retrieve GenICam nodemap
			INodeMap &nodeMap = pCam->GetNodeMap();

			//configure cam and image properties
			int resultImage = 0;
			ConfigureCustomImageSettings(pCam, nodeMap, FPS);

			unsigned char *camImg = new unsigned char;
			double exposureTime = initialExposureTime;
			double exposureTimesRatio;

			int err = 0;
			err = ConfigureExposure(nodeMap, exposureTime);
			if (err < 0)
			{
				MessageBox(_T("Could not configure exposure"), _T("ERROR"), MB_ICONERROR);
			}

			StopwatchTimer timer((double)16);

			unsigned char *aryptr = new unsigned char[ImgHeight*ImgWidth];
			int *tempptr;
			time_t start = time(0);
			Population<int> population(popImageHeight, popImageWidth, 1, acceptedSimilarity); //initialize population
			double fitness;
			bool shortenExposureFlag = false;

			//////////////save file
			ofstream tfile("uGA_functionEvals_vs_fitness.txt", ios::app);
			ofstream timeVsFitnessFile("uGA_time_vs_fitness.txt", ios::app);
			TimeStampGenerator timestamp;
			//////////////

			//******************talk to boards?

			bool stopConditionsMetFlag = false;

			ImageScaleManager scaler = ImageScaleManager(ImgWidth, ImgHeight, 1);
			scaler.SetBinSize(binSizeX, binSizeY);
			scaler.SetLUT(NULL);
			scaler.SetUsedBins(numBinsX, numBinsY);

			CameraDisplay* camDisplay = new CameraDisplay(cameraImageHeight, cameraImageWidth, "Camera Display");
			bool displayCameraImage_flag = true;
			CameraDisplay* slmDisplay = new CameraDisplay(popImageHeight, popImageWidth * 3, "SLM Display");

			if (displayCameraImage_flag)
			{
				camDisplay->OpenDisplay();
				//slmDisplay->OpenDisplay();
			}

			int populationSize = population.GetSize();
			//int populationSize = 1;

			//APP PROFILING SETUP
			frequency = 0;
			recordedTimes;
			recordedTimes.push_back(TimeStamp(0, "UGA BEGAN"));

			LARGE_INTEGER li; //used as temporary variable for recording time at specific point
			QueryPerformanceFrequency(&li);
			frequency = double(li.QuadPart) / 1000.0;


			pCam->BeginAcquisition();

			for (int i = 0; i < gennum; i++)
			{//each generation

				for (int j = 0; j < populationSize; j++)
				{//each individual

					//Start timer for finding generation profiling
					QueryPerformanceCounter(&startTime);

					//Record Initial timestamp
					recordedTimes.push_back(RecordTime(startTime, frequency, "Generation #" + to_string(i) + " | Induvidual " + to_string(j) + " - Began"));
					QueryPerformanceCounter(&startTime);

					scaler.TranslateImage(population.GetImage(), aryptr);
					recordedTimes.push_back(RecordTime(startTime, frequency, "Image Translated"));
					QueryPerformanceCounter(&startTime);

					//write to slm
					for (int i = 1; i <= numBoards; i++)
					{
						blink_sdk->Write_image(i, aryptr, ImgHeight, false, false, 0.0);
					}
					recordedTimes.push_back(RecordTime(startTime, frequency, "Writing to the SLM Complete"));
					QueryPerformanceCounter(&startTime);
					/*blink_sdk->Write_image(1, aryptr, ImgHeight, false, false, 0.0);*/

					ImagePtr pImage = Image::Create();

					recordedTimes.push_back(RecordTime(startTime, frequency, "pImage Created"));
					QueryPerformanceCounter(&startTime);

					//Take the picture
					pImage = AcquireImages(pCam, nodeMap, nodeMapTLDevice);
					camImg = static_cast<unsigned char*>(pImage->GetData());
					exposureTimesRatio = initialExposureTime / exposureTime;

					recordedTimes.push_back(RecordTime(startTime, frequency, "Images Acquired"));
					QueryPerformanceCounter(&startTime);

					fitness = FindAverageValue(camImg, targetMatrix, pImage->GetWidth(), pImage->GetHeight());
					recordedTimes.push_back(RecordTime(startTime, frequency, "Average Value (Fitness) Calculated"));
					QueryPerformanceCounter(&startTime);

					//Sleep While camera takes a picture (1)
					std::chrono::milliseconds sleepDuration((int)ceil(FrameRateMs));
					std::this_thread::sleep_for(sleepDuration);
					recordedTimes.push_back(RecordTime(startTime, frequency, "Sleep Complete - But requested: " + to_string((int)ceil(FrameRateMs)) + " MS"));
					QueryPerformanceCounter(&startTime);

					if (j == 1)
					{
						camDisplay->UpdateDisplay(camImg);
					}

					recordedTimes.push_back(RecordTime(startTime, frequency, "Image Display Updated"));
					QueryPerformanceCounter(&startTime);

					//for the elite value of the last generation
					if (j == 0)
					{
						timeVsFitnessFile << timestamp.MS_SinceStart() << " " << fitness*exposureTimesRatio << endl;
						tfile << i * populationSize << " " << fitness*exposureTimesRatio << endl;

						if (saveImages)
						{
							//save to jpeg
							ostringstream filename;
							filename << i + 1 << "_" << j << ".jpg";
							pImage->Save(filename.str().c_str());
						}
					}
					recordedTimes.push_back(RecordTime(startTime, frequency, "Saved Elite"));
					QueryPerformanceCounter(&startTime);

					//check for stop conditions
					if (fitness*exposureTimesRatio > fitnessToStop && timestamp.S_SinceStart() > secondsToStop && (i * 5 + j) > functionEvalsToStop)
					{
						stopConditionsMetFlag = true;
					}

					recordedTimes.push_back(RecordTime(startTime, frequency, "Stop Conditions Checked"));
					QueryPerformanceCounter(&startTime);

					population.SetFitness(fitness);

					if (fitness > maxFitnessValue) shortenExposureFlag = true; //if fitness value is too  high

					//Print Profiler Results
					for (int i = 1; i < recordedTimes.size(); i++)
						Utility::printLine(recordedTimes[i].GetLabel());

					recordedTimes.push_back(RecordTime(startTime, frequency, "All Other things for induvidual "));
					QueryPerformanceCounter(&startTime);

					//Efficiently remove all but the last entry in recorded times
					TimeStamp lastTimeStamp(recordedTimes[recordedTimes.size() - 1]);
					recordedTimes.clear();
					recordedTimes.push_back(TimeStamp(lastTimeStamp.GetDurationMSec(), lastTimeStamp.GetLabel()));

					Utility::printLine();

				}// for each individual

				population.NextGeneration(i + 1);
				if (shortenExposureFlag)//half exposure time if fitness value is too high
				{
					exposureTime /= 2;
					int err = ConfigureExposure(nodeMap, exposureTime);
					if (err < 0)
					{
						MessageBox(_T("Could not configure exposure"), _T("ERROR"), MB_ICONERROR);
					}
					shortenExposureFlag = false;
				}

				if (stopConditionsMetFlag)
				{
					break;
				}

			}//for each generation
			delete camDisplay;
			//delete slmDisplay;

			timeVsFitnessFile << timestamp.MS_SinceStart() << " " << 0 << endl;
			timeVsFitnessFile.close();
			tfile.close();

			//Realease all captured images
			int countReleasedAtEnd = 0;
			while (pCam->GetNumImagesInUse() != 0)
			{
				pCam->GetNextImage()->Release();
				countReleasedAtEnd++;
			}
			pCam->EndAcquisition();
			Utility::printLine("Released " + to_string(countReleasedAtEnd) + " at the end");

			//release camera
			pCam->DeInit();

			scaler.TranslateImage(population.GetImage(), aryptr);
			//write phase bmp
			Mat m_ary = Mat(512, 512, CV_8UC1, aryptr);
			imwrite("uGA_phaseopt.bmp", m_ary);
			//*****************************send aryptr to slm

		}
		catch (Spinnaker::Exception &e)
		{
			Utility::printLine("ERROR: " + string(e.what()));
		}

		pCam = NULL;
		camList.Clear();
		system->ReleaseInstance();

	}

	//reset buttons
	m_StartStopButton.EnableWindow(true);

	m_ImageListBox.EnableWindow(true);
	m_SlmPwrButton.EnableWindow(true);
	m_uGAButton.EnableWindow(true);
	m_SGAButton.EnableWindow(true);
	m_OptButton.EnableWindow(true);
}

/////////////////////////////////////////////////////////
//
//   OnBnClickedSgaButton()
//
//   Inputs: none
//
//   Returns: none
//
//   Purpose: run the sequence for an SGA process
//
//   Modifications:
//
////////////////////////////////////////////////////////
void CBlinkPCIeSDKDlg::OnBnClickedSgaButton()
{
	//check if we are currently sequencing. If not, start; if so, stop.
	CString RunState;
	m_StartStopButton.GetWindowText(RunState);
	CStringA WindowText(RunState);
	if (strcmp((const char*)WindowText, "START") == 0)
	{
		//PARAMETERS
		int cameraImageHeight = 64;
		int cameraImageWidth = 64;

		int FPS = 300;
		//assign variables
		double initialExposureTime = 2000; //in microseconds

		try
		{
			CString path;
			m_ISTEdit.GetWindowTextW(path);
			if (path.IsEmpty()) throw new exception();
			initialExposureTime = _tstof(path);
		}
		catch (...)
		{
			MessageBox(_T("Error Parsing Initial Shutter Time"), _T("PARSING ERROR"), MB_ICONERROR);
			return;
		}
		double acceptedSimilarity = .97; //this is the percentage of same genome two individuals must share for the program to count them as the same
		//Values of 1 of greater for acceptedSimilarity will prevent detection of convergence
		double maxFitnessValue = 30; //if an individual reaches this fitness value, the exposure time will be halved after the generation is done
		int popImageWidth = 32;
		try
		{
			CString path;
			m_NBEdit.GetWindowTextW(path);
			if (path.IsEmpty()) throw new exception();
			popImageWidth = _tstof(path);
		}
		catch (...)
		{
			MessageBox(_T("Error Parsing Number of Bins"), _T("PARSING ERROR"), MB_ICONERROR);
			return;
		}
		int popImageHeight = popImageWidth;
		int popImageDensity = 1; //bin parameters
		//stop conditions
		double fitnessToStop = 0;
		try
		{
			CString path;
			m_MFEdit.GetWindowTextW(path);
			if (path.IsEmpty()) throw new exception();
			fitnessToStop = _tstof(path);
		}
		catch (...)
		{
			MessageBox(_T("Error Parsing Minimum Fitness"), _T("PARSING ERROR"), MB_ICONERROR);
			return;
		}
		double secondsToStop = 60;
		try
		{
			CString path;
			m_MSEEdit.GetWindowTextW(path);
			if (path.IsEmpty()) throw new exception();
			secondsToStop = _tstof(path);
		}
		catch (...)
		{
			MessageBox(_T("Error Parsing Minimum Seconds Elapsed"), _T("PARSING ERROR"), MB_ICONERROR);
			return;
		}
		int functionEvalsToStop = 0;
		try
		{
			CString path;
			m_MFEEdit.GetWindowTextW(path);
			if (path.IsEmpty()) throw new exception();
			functionEvalsToStop = _tstof(path);
		}
		catch (...)
		{
			MessageBox(_T("Error Parsing Minimum Function Evaluations"), _T("PARSING ERROR"), MB_ICONERROR);
			return;
		}
		int gennum = 3000; //max number of generations to run, assuming no other stop conditions are met (each gen is 5 func evals)
		bool saveImages = false; //set to true to save images of the fittest individual of each gen
		bool displayCameraImage = true; //set to true to open a window showing the camera image

		//bin parameters
		int binSizeX = 16;
		try
		{
			CString path;
			m_BSEdit.GetWindowTextW(path);
			if (path.IsEmpty()) throw new exception();
			binSizeX = _tstof(path);
		}
		catch (...)
		{
			MessageBox(_T("Error Parsing Bin Size"), _T("PARSING ERROR"), MB_ICONERROR);
			return;
		}
		int binSizeY = binSizeX;
		int numBinsX = popImageWidth;
		int numBinsY = popImageHeight;

		int targetRadius = 2;
		try
		{
			CString path;
			m_IREdit.GetWindowTextW(path);
			if (path.IsEmpty()) throw new exception();
			targetRadius = _tstof(path);
		}
		catch (...)
		{
			MessageBox(_T("Error Parsing Integration Radius"), _T("PARSING ERROR"), MB_ICONERROR);
			return;
		}
		//END PARAMETERS

		//indicate we are currently sequencing
		m_StartStopButton.EnableWindow(false);
		//disable some of the buttons in the dialog so that the user
		//cannot do anything while sequencing, other than stop sequencing
		m_ImageListBox.EnableWindow(false);
		m_SlmPwrButton.EnableWindow(false);
		m_uGAButton.EnableWindow(false);
		m_SGAButton.EnableWindow(false);
		m_OptButton.EnableWindow(false);

		rtime.open("SGA_rtime.txt", ios::app);

		//*****************!! NOTE - DON'T DAMAGE YOUR SLM !! ************************
		//if your frame rate is not the same frame rate that was used in OnInitDialog, and
		//if the LC type is FLC then it is VERY IMPORTANT that true frames be recalculated
		//prior to calling SetTimer such that the Frame Rate and True Frames are properly
		//related
		if (!is_LC_Nematic) //FLC
		{
			trueFrames = blink_sdk->Compute_TF(float(frameRate));
		}
		blink_sdk->Set_true_frames(trueFrames);

		targetMatrix = new int[cameraImageHeight*cameraImageWidth];
		GenerateTargetMatrix_SinglePoint(targetMatrix, cameraImageWidth, cameraImageHeight, targetRadius);

		float FrameRateMs = (1.0 / (float)frameRate) * 1000.0;

		//start camera
		int resultCam = 0;
		//retrieve singleton reference to system object
		SystemPtr system = System::GetInstance();
		//retrieve a list of cameras from the system
		CameraList camList = system->GetCameras();
		unsigned int numCameras = camList.GetSize();
		CameraPtr pCam = NULL;
		//ensure only one camera
		if (numCameras != 1)
		{
			//clear camera list before releasing system
			camList.Clear();
			system->ReleaseInstance();
			Utility::printLine("Only ONE camera, please!");
		}
		try
		{
			pCam = camList.GetByIndex(0);
			//retrieve TL device nodemap
			INodeMap &nodeMapTLDevice = pCam->GetTLDeviceNodeMap();
			//initialize camera
			pCam->Init();
			//retrieve GenICm nodemap
			INodeMap &nodeMap = pCam->GetNodeMap();

			//configure image properties
			int resultImage = 0;
			//resultImage = resultImage | ConfigureCustomImageSettings(pCam, FPS);
			ImagePtr pImage = Image::Create();
			//*************AOI
			unsigned char *camImg = new unsigned char;
			double exposureTime = initialExposureTime;
			double exposureTimesRatio;

			int err = 0;
			err = ConfigureExposure(nodeMap, exposureTime);
			if (err < 0)
			{
				MessageBox(_T("Could not configure exposure"), _T("ERROR"), MB_ICONERROR);
			}

			StopwatchTimer timer((double)16);

			unsigned char *aryptr = new unsigned char[ImgHeight*ImgWidth];
			int *tempptr;
			time_t start = time(0);

			int populationSize = 30;

			SGAPopulation<int> population(popImageHeight, popImageWidth, 1, acceptedSimilarity, populationSize); //initialize population
			double fitness;
			bool shortenExposureFlag = false;

			//////////////save file
			ofstream tfile("SGA_functionEvals_vs_fitness.txt", ios::app);
			ofstream timeVsFitnessFile("SGA_time_vs_fitness.txt", ios::app);
			TimeStampGenerator timestamp;
			//////////////

			//******************talk to boards?

			bool stopConditionsMetFlag = false;

			ImageScaleManager scaler = ImageScaleManager(ImgWidth, ImgHeight, 1);
			scaler.SetBinSize(binSizeX, binSizeY);
			scaler.SetLUT(NULL);
			scaler.SetUsedBins(numBinsX, numBinsY);

			CameraDisplay* camDisplay = new CameraDisplay(cameraImageHeight, cameraImageWidth, "Camera Display");
			bool displayCameraImage_flag = true;
			CameraDisplay* slmDisplay = new CameraDisplay(popImageHeight, popImageWidth * 3, "SLM Display");

			if (displayCameraImage_flag)
			{
				camDisplay->OpenDisplay();
				//slmDisplay->OpenDisplay();
			}

			pCam->BeginAcquisition();

			for (int i = 0; i < gennum; i++)
			{//each generation
				for (int j = 0; j < population.GetSize(); j++) //population.GetSize()
				{//each individual
					scaler.TranslateImage(population.GetImage(), aryptr);

					//write to slm 
					for (int i = 1; i <= numBoards; i++)
					{
						blink_sdk->Write_image(i, aryptr, ImgHeight, false, false, 0.0);
					}

					//take the picture
					pImage = AcquireImages(pCam, nodeMap, nodeMapTLDevice);
					camImg = static_cast<unsigned char*>(pImage->GetData());
					exposureTimesRatio = initialExposureTime / exposureTime;
					//find fitness
					auto start = chrono::high_resolution_clock::now();
					fitness = FindAverageValue(camImg, targetMatrix, pImage->GetWidth(), pImage->GetHeight());
					auto stop = chrono::high_resolution_clock::now();
					std::chrono::duration<double> duration = stop - start;

					Utility::printLine("Calcualtion of average value in UGA  took : " + to_string(std::chrono::duration_cast<std::chrono::milliseconds>(duration).count()) + " and Frame MS to sleep is " + to_string(FrameRateMs));
					std::chrono::milliseconds sleepDuration((int)ceil(FrameRateMs)); //NEEDED??
					std::this_thread::sleep_for(sleepDuration);

					if (j == 0)
					{
						camDisplay->UpdateDisplay(camImg);
					}

					//for the elite value of the last generation
					if (j == 29)
					{
						timeVsFitnessFile << timestamp.MS_SinceStart() << " " << fitness*exposureTimesRatio << endl;
						tfile << i * populationSize << " " << fitness*exposureTimesRatio << endl;

						if (saveImages)
						{
							ostringstream filename;
							filename << i + 1 << "_" << j << ".jpg";
							pImage->Save(filename.str().c_str());
						}
					}
					//check for stop conditions
					if (fitness*exposureTimesRatio > fitnessToStop && timestamp.S_SinceStart() > secondsToStop && (i * 5 + j) > functionEvalsToStop)
					{
						stopConditionsMetFlag = true;
					}

					population.SetFitness(fitness);

					if (fitness > maxFitnessValue) shortenExposureFlag = true; //if fitness value is too  high
				}// for each individual

				population.NextGeneration(i + 1);
				if (shortenExposureFlag)//half exposure time if fitness value is too high
				{
					exposureTime /= 2;
					int err = ConfigureExposure(nodeMap, exposureTime);
					if (err < 0)
					{
						MessageBox(_T("Could not configure exposure"), _T("ERROR"), MB_ICONERROR);
					}
					shortenExposureFlag = false;
				}

				if (stopConditionsMetFlag)
				{
					break;
				}
			}//for each generation
			//pImage->Release();
			delete camDisplay;
			//delete slmDisplay;

			timeVsFitnessFile << timestamp.MS_SinceStart() << " " << 0 << endl;
			timeVsFitnessFile.close();
			tfile.close();

			//Release Remaining Images
			while (pCam->GetNumImagesInUse() != 0)
				pCam->GetNextImage()->Release();
			pCam->EndAcquisition();

			//release camera
			pCam->DeInit();
			pCam = NULL;
			camList.Clear();
			system->ReleaseInstance();

			scaler.TranslateImage(population.GetImage(), aryptr);
			//write phase bmp
			Mat m_ary = Mat(512, 512, CV_8UC1, aryptr);
			imwrite("SGA_phaseopt.bmp", m_ary);
			//*****************************send aryptr to slm
		}
		catch (Spinnaker::Exception &e)
		{
			Utility::printLine("ERROR: " + string(e.what()));
		}
	}
	//reset buttons
	m_StartStopButton.EnableWindow(true);

	m_ImageListBox.EnableWindow(true);
	m_SlmPwrButton.EnableWindow(true);
	m_uGAButton.EnableWindow(true);
	m_SGAButton.EnableWindow(true);
	m_OptButton.EnableWindow(true);
}

/////////////////////////////////////////////////////////
//
//   OnBnClickedOptButton()
//
//   Inputs: none
//
//   Returns: none
//
//   Purpose: run the sequence for an optimization process
//
//   Modifications:
//
////////////////////////////////////////////////////////
void CBlinkPCIeSDKDlg::OnBnClickedOptButton()
{
	Utility::printLine("OPT 5 BUTTON CLICKED!");

	//check if we are currently sequencing. If not, start; if so, stop.
	CString RunState;
	m_StartStopButton.GetWindowText(RunState);
	CStringA WindowText(RunState);
	if (strcmp((const char*)WindowText, "START") == 0)
	{
		//disable buttons
		m_StartStopButton.EnableWindow(false);

		m_ImageListBox.EnableWindow(false);
		m_SlmPwrButton.EnableWindow(false);

		m_uGAButton.EnableWindow(false);
		m_SGAButton.EnableWindow(false);

		m_OptButton.EnableWindow(false);

		//Declare variables
		int ID;
		int cameraImageHeight, cameraImageWidth;// x0, y0; //------------ - defined in ConfigureCustomImageSettings()
		double exptime, PC, FPS, *newFPS, dexptime, ratio, crop;
		int ii, jj, nn, mm, iMax, jMax, ll, lMax, loopvalue, kk;
		int bin, dphi;
		double rMax;
		int targetRadius;

		//set variables
		iMax = 512;
		jMax = 512;
		bin = 16;
		unsigned char *aryptr = new unsigned char[iMax*jMax];

		try
		{
			CString path;
			m_BSEdit.GetWindowTextW(path);
			if (path.IsEmpty()) throw new exception();
			bin = _tstof(path);
		}
		catch (...)
		{
			MessageBox(_T("Error Parsing Bin Size"), _T("PARSNG ERROR"), MB_ICONERROR);
			return;
		}
		dphi = 16;
		exptime = 2000; //in microseconds
		try
		{
			CString path;
			m_ISTEdit.GetWindowTextW(path);
			if (path.IsEmpty()) throw new exception();
			exptime = _tstof(path);
		}
		catch (...)
		{
			MessageBox(_T("Error Parsing Initial Shutter Time"), _T("PARSING ERROR"), MB_ICONERROR);
			return;
		}
		//PC = 30; //37; ------------------------------can't find in Spinnaker SDK
		FPS = 100; //300; //160 or 670
		cameraImageHeight = 64; //300 or 50
		cameraImageWidth = 64; //300 or 52
		//x0 = 612; //488 or 612
		//y0 = 486; //362 or 486 ----------------------------defined in ConfigureCustomImageSettings(), as they are the same for all
		//newFPS = NULL; -------------------no longer needed for new SDK
		dexptime = exptime;
		crop = 0;
		targetRadius = 2;
		try
		{
			CString path;
			m_IREdit.GetWindowTextW(path);
			if (path.IsEmpty()) throw new exception();
			targetRadius = _tstof(path);
		}
		catch (...)
		{
			MessageBox(_T("Error Parsing Integration Radius"), _T("PARSING ERROR"), MB_ICONERROR);
			return;
		}

		targetMatrix = new int[height*width];
		GenerateTargetMatrix_SinglePoint(targetMatrix, height, width, targetRadius);

		////////////////////save file
		ofstream tfile("Opt_functionEvals_vs_fitness.txt", ios::app);
		ofstream timeVsFitnessFile("Opt_time_vs_fitness.txt", ios::app);
		TimeStampGenerator timestamp;
		/////////////////////

		//******************get a handle to the board?

		//start camera
		//int resultCam = 0;
		//retrieve singleton reference to system object
		SystemPtr system = System::GetInstance();
		//retrieve a list of cameras from the system
		CameraList camList = system->GetCameras();
		unsigned int numCameras = camList.GetSize();
		CameraPtr pCam = NULL;
		//ensure only one camera
		if (numCameras != 1)
		{
			//clear camera list before releasing system
			camList.Clear();
			system->ReleaseInstance();
			MessageBox(_T("Only one camera, please"), _T("ERROR"), MB_ICONERROR);
		}
		try
		{
			pCam = camList.GetByIndex(0);
			//retrieve TL device nodemap
			INodeMap &nodeMapTLDevice = pCam->GetTLDeviceNodeMap();
			//initialize camera
			pCam->Init();
			//retrieve GenICam nodemap
			INodeMap &nodeMap = pCam->GetNodeMap();

			Utility::printLine("Node Map gotten!");

			int err = ConfigureExposure(nodeMap, dexptime);
			if (err < 0)
			{
				MessageBox(_T("Could not configure exposure"), _T("ERROR"), MB_ICONERROR);
			}

			Utility::printLine("Exposure configured!");

			//configure image
			//int resultImage = 0;
			//resultImage = resultImage | ConfigureCustomImageSettings(pCam, FPS);
			ImagePtr pImage = Image::Create();
			//********************AOI stuff

			int RateMs = 10;
			int index = 0;

			//initialize array
			for (ii = 0; ii < iMax; ii++)
			{
				for (jj = 0; jj < jMax; jj++)
				{
					kk = ii * 512 + jj;
					aryptr[kk] = 0;
				}
			}

			//*****************!! NOTE - DON'T DAMAGE YOUR SLM !! ************************
			//if your frame rate is not the same frame rate that was used in OnInitDialog, and
			//if the LC type is FLC then it is VERY IMPORTANT that true frames be recalculated
			//prior to calling SetTimer such that the Frame Rate and True Frames are properly
			//related
			if (!is_LC_Nematic) //FLC
			{
				trueFrames = blink_sdk->Compute_TF(float(frameRate));
			}
			blink_sdk->Set_true_frames(trueFrames);

			//set timer to periodically load images fom the sequence list to the hardware.
			//start by converting frame rate to ms.
			float FrameRateMs = (1.0 / (float)frameRate) * 1000.0;
			//SetTimer(1, FrameRateMs, NULL)

			Utility::printLine("FPS Determiniede!");

			CameraDisplay* camDisplay = new CameraDisplay(cameraImageHeight, cameraImageWidth, "Camera Display");
			Utility::printLine("1");
			camDisplay->OpenDisplay();
			Utility::printLine("2");
			unsigned char* camImg = new unsigned char;

			Utility::printLine("Cam Display Setup!");


			//Optimization Code
			Utility::printLine("Began Acquisition!");
			time_t start = time(0);
			for (ii = 0; ii < iMax; ii += bin)
			{

				pCam->BeginAcquisition();

				for (jj = 0; jj < jMax; jj += bin)
				{
					lMax = 0;
					rMax = 0;
					const clock_t begin = clock();
					//find max phase for bin
					for (ll = 0; ll < 256; ll += dphi)
					{
						//bin phase
						for (nn = 0; nn < bin; nn++)
						{
							for (mm = 0; mm < bin; mm++)
							{
								kk = (ii + nn) * iMax + (mm + jj);
								ary[kk] = ll;
							}
						}

						ratio = exptime / dexptime;

						//set phase and analyze
						aryptr = ary;
						Sleep(FrameRateMs);
						//write aryptr to board
						for (int i = 1; i <= numBoards; i++)
						{
							blink_sdk->Write_image(i, aryptr, ImgHeight, false, false, 0.0);
						}

						Utility::printLine("About to aquire an image!");

						//take picture
						pImage = AcquireImages(pCam, nodeMap, nodeMapTLDevice);

						Utility::printLine("Image Aquisition complete!");

						camImg = static_cast<unsigned char*>(pImage->GetData());;
						width = pImage->GetWidth();
						height = pImage->GetHeight();
						camDisplay->UpdateDisplay(camImg);

						loopvalue = FindAverageValue(camImg, targetMatrix, width, height);

						timeVsFitnessFile << timestamp.MS_SinceStart() << " " << loopvalue * ratio << endl;
						tfile << index << " " << loopvalue * ratio << endl;

						if (loopvalue > 150)
						{
							dexptime /= 2;
							int err2 = ConfigureExposure(nodeMap, dexptime);
							if (err2 < 0)
							{
								MessageBox(_T("Could not configure exposure"), _T("ERROR"), MB_ICONERROR);
							}
						}

						if (loopvalue * ratio > rMax)
						{
							lMax = ll;
							rMax = loopvalue * ratio;
						}
					}
					//set maximal phase
					for (nn = 0; nn < bin; nn++)
					{
						for (mm = 0; mm < bin; mm++)
						{
							kk = (ii + nn) * iMax + (mm + jj);
							ary[kk] = lMax;
						}
					}

					//save image as jpeg
					/*	ostringstream filename;
					filename << index << ".jpg";
					pImage->Save(filename.str().c_str());*/
					ofstream rtime;
					rtime.open("Opt_rtime.txt", ios::app);
					rtime << float(clock() - begin) << "   " << lMax << "   " << dexptime << endl;
					rtime.close();
					//iterate index
					index += 1;
				}
				//Realease all captured images
				int countReleasedAtEnd = 0;
				while (pCam->GetNumImagesInUse() != 0)
				{
					pCam->GetNextImage()->Release();
					countReleasedAtEnd++;
				}
				pCam->EndAcquisition();
				Utility::printLine("Released " + to_string(countReleasedAtEnd) + " at the end");

				

				if (timestamp.S_SinceStart() > 60)
					break;
			}
			time_t end = time(0);

			//release camera
			pCam->DeInit();
			pCam = NULL;
			camList.Clear();
			system->ReleaseInstance();

			timeVsFitnessFile.close();
			tfile.close();

			camDisplay->CloseDisplay();

			double dt = difftime(end, start);

			ofstream tfile2("Opt_time.txt", ios::app);

			tfile2 << dt << endl;
			tfile2.close();

			//write phase bmp
			Mat m_ary = Mat(512, 512, CV_8UC1, aryptr);
			imwrite("Opt_phaseopt.bmp", m_ary);

		}
		catch (Spinnaker::Exception &e)
		{
			Utility::printLine("ERROR: " + string(e.what()));
		}
	}

	m_StartStopButton.EnableWindow(true);

	m_ImageListBox.EnableWindow(true);
	m_SlmPwrButton.EnableWindow(true);

	m_uGAButton.EnableWindow(true);
	m_SGAButton.EnableWindow(true);
	m_OptButton.EnableWindow(true);
}

#define MAX_CFileDialog_FILE_COUNT 99
#define FILE_LIST_BUFFER_SIZE ((MAX_CFileDialog_FILE_COUNT * (MAX_PATH + 1)) + 1)
void CBlinkPCIeSDKDlg::OnBnClickedSetlut()
{
	CString fileName;
	LPWSTR p = fileName.GetBuffer(FILE_LIST_BUFFER_SIZE);
	CFileDialog dlgFile(TRUE);
	OPENFILENAME& ofn = dlgFile.GetOFN();
	ofn.lpstrFile = p;
	ofn.nMaxFile = FILE_LIST_BUFFER_SIZE;

	if (dlgFile.DoModal() == IDOK)
	{
		fileName = dlgFile.GetPathName();
		fileName.ReleaseBuffer();
	}

	SLM_Board* pBoard = SLM_board_list[0];
	ReadLUTFile(pBoard->LUT, fileName);
}


void CBlinkPCIeSDKDlg::OnBnClickedSetwfc()
{
	CString fileName;
	LPWSTR p = fileName.GetBuffer(FILE_LIST_BUFFER_SIZE);
	CFileDialog dlgFile(TRUE);
	OPENFILENAME& ofn = dlgFile.GetOFN();
	ofn.lpstrFile = p;
	ofn.nMaxFile = FILE_LIST_BUFFER_SIZE;

	if (dlgFile.DoModal() == IDOK)
	{
		fileName = dlgFile.GetPathName();
		fileName.ReleaseBuffer();
	}

	SLM_Board * pBoard = SLM_board_list[0];
	pBoard->PhaseCompensationFileName = fileName;
	ReadAndScaleBitmap(pBoard->PhaseCompensationData, pBoard->PhaseCompensationFileName);
}


TimeStamp CBlinkPCIeSDKDlg::RecordTime(LARGE_INTEGER start, double frequency, std::string label)
{
	LARGE_INTEGER end;
	QueryPerformanceCounter(&end);

	//Calculate and round the time taken
	double interval = static_cast<double>((end.QuadPart - start.QuadPart)) / frequency;
	interval = round(interval * 10000) / 10000;

	TimeStamp curEntry(interval, label);

	return curEntry;
}