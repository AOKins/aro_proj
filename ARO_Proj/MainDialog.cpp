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
#include "resource.h"
#include "MainDialog.h"			// Header file for dialog functions

// - Aglogrithm Related
#include "Optimization.h"
#include "uGA_Optimization.h"
#include "SGA_Optimization.h"
#include "BruteForce_Optimization.h"

//	- Helper
#include "Utility.h"			// Collection of static helper functions
#include "CamDisplay.h"
#include "SLMController.h"
#include "CameraController.h"
#include "Blink_SDK.h"			// Camera functions
#include "SLM_Board.h"


//	- System
#include <fstream>				// Used to export information to file 
#include <string>				// Used as the primary "letters" datatype
#include <vector>				// Used for image value storage

//[NAMESPACES]
using namespace std;
using namespace cv;


#define MAX_CFileDialog_FILE_COUNT 99
#define FILE_LIST_BUFFER_SIZE ((MAX_CFileDialog_FILE_COUNT * (MAX_PATH + 1)) + 1)

// [CONSTRUCTOR/COMPONENT EVENTS]
// Constructor for dialog
MainDialog::MainDialog(CWnd* pParent) : CDialog(IDD_BLINKPCIESDK_DIALOG, pParent), m_CompensatePhase(FALSE), m_ReadyRunning(_T(""))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}


// Reference UI components
void MainDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Check(pDX, IDC_COMPENSATE_PHASE_CHECKBOX, m_CompensatePhase);
	DDX_Control(pDX, IDC_COMPENSATE_PHASE_CHECKBOX, m_CompensatePhaseCheckbox);
	DDX_Control(pDX, IDC_IMAGE_LISTBOX, m_ImageListBox);
	DDX_Control(pDX, IDC_UGA_BUTTON, m_uGAButton);
	DDX_Control(pDX, IDC_SGA_BUTTON, m_SGAButton);
	DDX_Control(pDX, IDC_OPT_BUTTON, m_OptButton);
	DDX_Control(pDX, IDC_TAB1, m_TabControl);
}

//Set functions that respond to user action
BEGIN_MESSAGE_MAP(MainDialog, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_LBN_SELCHANGE(IDC_IMAGE_LISTBOX, OnSelchangeImageListbox)
	ON_WM_TIMER()
	ON_WM_CLOSE()
	ON_BN_CLICKED(IDC_COMPENSATE_PHASE_CHECKBOX, OnCompensatePhaseCheckbox)
	ON_BN_CLICKED(IDC_UGA_BUTTON, &MainDialog::OnBnClickedUgaButton)
	ON_BN_CLICKED(IDC_SGA_BUTTON, &MainDialog::OnBnClickedSgaButton)
	ON_BN_CLICKED(IDC_OPT_BUTTON, &MainDialog::OnBnClickedOptButton)
	ON_NOTIFY(TCN_SELCHANGE, IDC_TAB1, &MainDialog::OnTcnSelchangeTab1)
END_MESSAGE_MAP()



//////////////////////////////////////////////////////////////
//
//   OnInitDialog()
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
BOOL MainDialog::OnInitDialog()
{
	//[CONSOLE OUTPUT]
	//Enable console output
	//#ifdef _DEBUG

	if (!AllocConsole())
		AfxMessageBox(L"Console will not be shown!");

	freopen_s(&fp, "CONOUT$", "w", stdout);
	std::cout.clear();

	/*if (!AllocConsole())
	{
		AfxMessageBox(L"Output will not be shown!");
	}
	else
	{
		freopen("CONOUT$", "w", stdout);
		AfxMessageBox(L"Output WILL be shown!");
		std::cout << "!!!!!" << std::endl;
	}*/
	//#endif

	//[UI SETUP]
	CDialog::OnInitDialog();
	SetIcon(m_hIcon, TRUE);

	//[SET UI DEFAULTS]
	// - image names to the listbox and select the first image
	// TODO: check if this the correct image setup 
	m_ImageListBox.AddString(_T("ImageOne.bmp"));
	m_ImageListBox.AddString(_T("ImageTwo.zrn"));
	m_ImageListBox.SetCurSel(0);

	//Setup the tab control that hosts all of the app's settings
	//1) https://stackoverflow.com/questions/1044315/setwindowpos-function-not-moving-window
	//2) https://www.youtube.com/watch?v=WHPNzx4E5rM
	// - set tab headings
	LPCTSTR headings[] = {L"Optimization Settings", L"SLM Settings", L"Camera Settings", L"AOI Settings" };
	for (int i = 0; i < 4; i++)
		m_TabControl.InsertItem(i, headings[i]);

	// - set all tab components (Create Dialogs from Templates)
	CRect rect;
	m_TabControl.GetClientRect(&rect); //Gets dimensions of the tab control to fit in dialogs inside

	m_optimizationControlDlg.Create(IDD_OPTIMIZATION_CONTROL, &m_TabControl);
	m_optimizationControlDlg.SetWindowPos(NULL, rect.top + 5, rect.left + 30, rect.Width() - 10, rect.Height() - 35, SWP_SHOWWINDOW | SWP_NOZORDER);

	m_slmControlDlg.Create(IDD_SLM_CONTROL, &m_TabControl);
	m_slmControlDlg.SetWindowPos(NULL, rect.top + 5, rect.left + 30, rect.Width() - 10, rect.Height() - 35, SWP_HIDEWINDOW | SWP_NOZORDER);

	m_cameraControlDlg.Create(IDD_CAMERA_CONTROL, &m_TabControl);
	m_cameraControlDlg.SetWindowPos(NULL, rect.top + 5, rect.left + 30, rect.Width() - 10, rect.Height() - 35, SWP_HIDEWINDOW | SWP_NOZORDER);

	m_aoiControlDlg.Create(IDD_AOI_CONTROL, &m_TabControl);
	m_aoiControlDlg.SetWindowPos(NULL, rect.top + 5, rect.left + 30, rect.Width() - 10, rect.Height() - 35, SWP_HIDEWINDOW | SWP_NOZORDER);

	// - set all default settings
	m_cameraControlDlg.m_FramesPerSecond.SetWindowTextW(_T("200"));
	m_cameraControlDlg.m_initialExposureTimeInput.SetWindowTextW(_T("2000"));
	m_cameraControlDlg.m_gammaValue.SetWindowTextW(_T("1.25"));
	m_optimizationControlDlg.m_binSize.SetWindowTextW(_T("16"));
	m_optimizationControlDlg.m_numberBins.SetWindowTextW(_T("32"));
	m_optimizationControlDlg.m_targetRadius.SetWindowTextW(_T("2"));
	m_optimizationControlDlg.m_minFitness.SetWindowTextW(_T("0"));
	m_optimizationControlDlg.m_minSeconds.SetWindowTextW(_T("60"));
	m_optimizationControlDlg.m_minGenerations.SetWindowTextW(_T("0"));
	m_aoiControlDlg.m_leftInput.SetWindowTextW(_T("896"));
	m_aoiControlDlg.m_rightInput.SetWindowTextW(_T("568"));
	m_aoiControlDlg.m_widthInput.SetWindowTextW(_T("64"));
	m_aoiControlDlg.m_heightInput.SetWindowTextW(_T("64"));
	m_slmControlDlg.m_SlmPwrButton.SetWindowTextW(_T("Turn power ON")); // - power button (TODO: determine if SLM is actually off at start)
	
	//GetCheck() Retrieves BST_CHECKED if checked & BST_UNCHECKED if not checked
	m_optimizationControlDlg.m_SampleCheckmark.GetCheck(); 

	// - get reference to slm controller
	slmCtrl = m_slmControlDlg.getSLMCtrl();
	slmCtrl->SetMainDlg(this);
	Utility::printLine("INFO: Accessed slm controller!");

	//Show the default tab (optimizations settings)
	m_pwndShow = &m_optimizationControlDlg;

	//Use slm control reference to set additional settings
	// - if the liquid crystal type is nematic, then allow the user the option to
	//compensate for phase imperfections by applying a phase compensation image
	if (slmCtrl != nullptr)
	{
		Utility::printLine("INFO: SLM Control not NULL");

		if (!slmCtrl->IsAnyNematic())
			m_CompensatePhaseCheckbox.ShowWindow(false);
		else
			m_CompensatePhaseCheckbox.ShowWindow(true);
	}
	else
		Utility::printLine("INFO: SLM Control NULL");

	camCtrl = new CameraController((*this));
	if (camCtrl != nullptr)
	{
		m_aoiControlDlg.SetCameraController(camCtrl);
		Utility::printLine("INFO: Camera Control not NULL");
	}
	else
		Utility::printLine("INFO: Camera Control NULL");

	// Send the currently selected image from the PCIe card memory board
	// to the SLM. User will immediately see first image in sequence. 
	OnSelchangeImageListbox();

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
void MainDialog::OnSysCommand(UINT nID, LPARAM lParam)
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
void MainDialog::OnPaint()
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
HCURSOR MainDialog::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
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
void MainDialog::OnSelchangeImageListbox()
{
	//Figure out which image in the list was just selected
	int sel = m_ImageListBox.GetCurSel();
	if (sel == LB_ERR) //nothing selected
		return;

	slmCtrl->ImageListBoxUpdate(sel);
}

//OnStartStopButton: performs stop button action
void MainDialog::OnStartStopButton()
{
	//LARGE TODO: allow to stop execution by pressing stop HERE
	return;
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
void MainDialog::OnClose()
{
	Utility::printLine("INFO: System begiinng to close closing!");

	delete camCtrl;

	Utility::printLine("INFO: Camera was shut down!");

	//Finish console output
	fclose(fp);
	if (!FreeConsole())
		AfxMessageBox(L"Could not free the console!");
	
	Utility::printLine("INFO: Console realeased (why are you seeing this?)");

	CDialog::OnClose();
}

/* OnOK: perfomrs enter key functions - used to prevent accidental exit of program and equipment damag e*/
void MainDialog::OnOK()
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
void MainDialog::OnCompensatePhaseCheckbox()
{
	UpdateData(true);

	//Re-load the sequence if apropriate checkbox pressed
	slmCtrl->LoadSequence();
	//Load the currently selected image to the SLM
	OnSelchangeImageListbox();
}


//OnBnClickedUgaButton: Performs the uGA Algorithm Button Press Action
void MainDialog::OnBnClickedUgaButton()
{
	Utility::printLine("INFO: uGA optimization started!");
	uGA_Optimization opt((*this), camCtrl, slmCtrl);
	if (!opt.runOptimization())
		Utility::printLine("ERROR: uGA optimization failed!");
}

//OnBnClickedSgaButton: Performs the SGA Algorithm Button Press Action
void MainDialog::OnBnClickedSgaButton()
{
	Utility::printLine("INFO: SGA optimization started!");
	SGA_Optimization opt((*this), camCtrl, slmCtrl);
	if(!opt.runOptimization())
		Utility::printLine("ERROR: SGA optimization failed!");
}

//OnBnClickedOptButton: Performs the OPT5 Algorithm Button Press Action
void MainDialog::OnBnClickedOptButton()
{
	Utility::printLine("INFO: OPT5 optimization started!");
	BruteForce_Optimization opt((*this), camCtrl, slmCtrl);
	if(!opt.runOptimization())
		Utility::printLine("ERROR: OPT 5 optimization failed!");
}

//OnTcnSelchangeTab1: changes the shown dialog when a new tab is selected
void MainDialog::OnTcnSelchangeTab1(NMHDR *pNMHDR, LRESULT *pResult)
{
	//Hide the tab shown currently
	if (m_pwndShow != NULL)
	{
		m_pwndShow->ShowWindow(SW_HIDE);
		m_pwndShow = NULL;
	}
	int tabIndex = m_TabControl.GetCurSel();
	switch (tabIndex)
	{
	case 0:
		m_optimizationControlDlg.ShowWindow(SW_SHOW);
		m_pwndShow = &m_optimizationControlDlg;
		break;
	case 1:
		m_slmControlDlg.ShowWindow(SW_SHOW);
		m_pwndShow = &m_slmControlDlg;
		break;
	case 2:
		m_cameraControlDlg.ShowWindow(SW_SHOW);
		m_pwndShow = &m_cameraControlDlg;
		break;
	case 3:
		m_aoiControlDlg.ShowWindow(SW_SHOW);
		m_pwndShow = &m_aoiControlDlg;
		break;
	default:
		Utility::printLine("WARNING: Requested to show a tab that shouldn't exist!");
	}
	*pResult = 0;
}



// [UI UPDATE]
void MainDialog::disableMainUI(bool isMainEnabled)
{
	//All controls have thesame state
	// - enabled: when nothing is running
	// - disabled when the algorithm is running

	m_ImageListBox.EnableWindow(isMainEnabled);
	m_uGAButton.EnableWindow(isMainEnabled);
	m_SGAButton.EnableWindow(isMainEnabled);
	m_OptButton.EnableWindow(isMainEnabled);

	//Stop button always enabled when everything is disabled
	//TODO: add and implement a stop button
}


///////////////////////////////////////////////////////////////////////////////////////
//FUNCTIONS THAT MIGHT BE NEEDED IN THE FUTURE
///* OnTimer: This function is called periodically after the user has started the
// * software sequencing through the images in the image list. This is
// * used to update the dialog with the current state of the hardware.
// * If the hardware is displaying image 1 on the SLM, that image will be
// * selected in the listbox. */
//void MainDialog::OnTimer(UINT_PTR nIDEvent)
//{
//	//take picture
//
//	//retrieve singleton reference to system object
//	SystemPtr system = System::GetInstance();
//	//retrieve list of cameras from the system
//	CameraList camList = system->GetCameras();
//	unsigned int numCameras = camList.GetSize();
//	//make an image pointer
//	ImagePtr pImage = Image::Create();
//	//finish if there are no cameras
//	if (numCameras == 0)
//	{
//		//clear camera list before releasing system
//		camList.Clear();
//		system->ReleaseInstance();
//		Utility::printLine("Only ONE camera, please!");
//	}
//	int result = 0;
//	int err = 0;
//	try
//	{
//		//create a shared pointer to the camera--the pointer will automatically clean up after the try
//		pCam = camList.GetByIndex(0);
//		INodeMap &nodeMapTLDevice = pCam->GetTLDeviceNodeMap();
//		pCam->Init();
//		INodeMap &nodeMap = pCam->GetNodeMap();
//
//		err = ConfigureExposure(nodeMap, exposureTime);
//		if (err < 0)
//		{
//			MessageBox(_T("Could not configure exposure"), _T("ERROR"), MB_ICONERROR);
//		}
//
//		pImage = AcquireImages(pCam, nodeMap, nodeMapTLDevice);
//		pCam->DeInit();
//
//	}
//	catch (Spinnaker::Exception &e)
//	{
//		Utility::printLine("ERROR: " + string(e.what()));
//	}
//	//save image to char vector
//	width = pImage->GetWidth();
//	height = pImage->GetHeight();
//	camImage = static_cast<unsigned char*>(pImage->GetData());
//	//clear the cam list, and release the memory and system
//	pCam = NULL;
//	camList.Clear();
//	pImage->Release();
//	system->ReleaseInstance();
//
//	//find fitness
//	double fitness = FindAverageValue(camImage, targetMatrix, width, height);
//	populationPointer->SetFitness(fitness);
//	rtime << fitness << endl;
//	camDisplay->UpdateDisplay(camImage);
//
//	if (populationCounter < 5) populationCounter++;
//	else
//	{
//		populationCounter = 1;
//		populationPointer->NextGeneration(0);
//	}
//
//	//send the data of the currently selected image in the sequence list out to the SLM
//	ImgScaleMngr->TranslateImage(populationPointer->GetImage(), ary);
//	for (int blist = 0; blist < numBoards; blist++)
//	{
//		int board = blist + 1; //sdk uses 1-based index for boards
//		blink_sdk->Write_image(board, ary, 512, false, false, 0.0);
//	}
//
//	CDialog::OnTimer(nIDEvent);
//}
