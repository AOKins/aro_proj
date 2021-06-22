// [DESCRIPTION]
// BlinkPCIeSDKDlg.cpp: implementation file for the main dialog of the program
// Authors: Benjamin Richardson, Rebecca Tucker, Kostiantyn Makrasnov and More @ ISP ASL

// [DEFINITIONS/ABRIVIATIONS]
// MFC - Microsoft Foundation Class library - Used to design UI in C++
// SLM - Spatial Light Modulator - the device that contains parameters which we want to optimize (takes in image an reflects light based on that image)
// LC  - Liquid Crystal - the type of diplay the SLM has

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
#include "CamDisplay.h"			//
#include "SLMController.h"		// Wrapper for SLM control
#include "CameraController.h"	// Spinnaker Camera interface wrapper
#include "Blink_SDK.h"			// SLM SDK functions
#include "SLM_Board.h"

//	- System
#include <fstream>				// Used to export information to file 
#include <string>				// Used as the primary "letters" datatype
#include <vector>				// Used for image value storage
#include <iostream>				// Used for console output

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
	DDX_Control(pDX, IDC_START_STOP_BUTTON, m_StartStopButton);
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
	ON_BN_CLICKED(IDC_START_STOP_BUTTON, &MainDialog::OnBnClickedStartStopButton)
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

	if (!AllocConsole()) {
		AfxMessageBox(L"Console will not be shown!");
	}
	freopen_s(&fp, "CONOUT$", "w", stdout);
	std::cout.clear();

	/*
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
	for (int i = 0; i < 4; i++) {
		m_TabControl.InsertItem(i, headings[i]);
	}
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

	// - get reference to slm controller
	slmCtrl = m_slmControlDlg.getSLMCtrl();
	slmCtrl->SetMainDlg(this);
	m_slmControlDlg.populateSLMlist(); // Simple method to setup the list of selections

	// Give a warning message if no boards have been detected
	if (m_slmControlDlg.slmCtrl->boards.size() < 1) {
		MessageBox((LPCWSTR)L"No SLM detected!",
			(LPCWSTR)L"No SLM has been detected to be connected!",
			MB_ICONWARNING | MB_OK);
	}
	else { // This else case is here just to debug
		MessageBox((LPCWSTR)L"SLM(s) detected!",
			(LPCWSTR)L"SLMs have been detected to be connected!",
			MB_ICONWARNING | MB_OK);
	}

	//Show the default tab (optimizations settings)
	m_pwndShow = &m_optimizationControlDlg;

	//Use slm control reference to set additional settings
	// - if the liquid crystal type is nematic, then allow the user the option to
	//compensate for phase imperfections by applying a phase compensation image
	if (slmCtrl != nullptr)	{
		if (!slmCtrl->IsAnyNematic())
			m_CompensatePhaseCheckbox.ShowWindow(false);
		else
			m_CompensatePhaseCheckbox.ShowWindow(true);
	}
	else
		Utility::printLine("WARNING: SLM Control NULL");

	camCtrl = new CameraController((*this));
	if (camCtrl != nullptr)	{
		m_aoiControlDlg.SetCameraController(camCtrl);
	}
	else
		Utility::printLine("WARNING: Camera Control NULL");

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
void MainDialog::OnSelchangeImageListbox() {
	//Figure out which image in the list was just selected
	int sel = this->m_ImageListBox.GetCurSel();
	if (sel == LB_ERR){ //nothing selected
		return;
	}
	slmCtrl->ImageListBoxUpdate(sel);
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
void MainDialog::OnClose() {
	// If optimization is running, give warning and prevent closing of application
	if (this->running_optimization_) {
		Utility::printLine("WARNING: Optimization still running!");
		MessageBox(
			(LPCWSTR)L"Still running optimization!",
			(LPCWSTR)L"The optimization is still running! Must be stopped first.",
			MB_ICONWARNING | MB_OK
			);
	}
	else {
		Utility::printLine("INFO: System beginning to close closing!");

		delete this->camCtrl;
		// SLM controller is destructed by the SLM Dialog Controller

		//Finish console output
		fclose(fp);
		if (!FreeConsole()) {
			AfxMessageBox(L"Could not free the console!");
		}
		Utility::printLine("INFO: Console realeased (why are you seeing this?)");
		CDialog::OnClose();
	}
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

//OnBnClickedUgaButton: Select the uGA Algorithm Button
void MainDialog::OnBnClickedUgaButton() {
	this->opt_selection_ = OptType::uGA;
	Utility::printLine("INFO: uGA optimization selected");

	this->m_uGAButton.EnableWindow(false);
	this->m_SGAButton.EnableWindow(true);
	this->m_OptButton.EnableWindow(true);
	this->m_StartStopButton.EnableWindow(true);
}

//OnBnClickedSgaButton: Select the SGA Algorithm Button
void MainDialog::OnBnClickedSgaButton() {
	Utility::printLine("INFO: SGA optimization selected");
	this->opt_selection_ = OptType::SGA;

	this->m_SGAButton.EnableWindow(false);
	this->m_uGAButton.EnableWindow(true);
	this->m_OptButton.EnableWindow(true);
	this->m_StartStopButton.EnableWindow(true);
}

//OnBnClickedOptButton: Select the OPT5 Algorithm Button
void MainDialog::OnBnClickedOptButton() {
	Utility::printLine("INFO: OPT5 optimization selected");
	this->opt_selection_ = OptType::OPT5;

	this->m_OptButton.EnableWindow(false);
	this->m_SGAButton.EnableWindow(true);
	this->m_uGAButton.EnableWindow(true);
	this->m_StartStopButton.EnableWindow(true);
}

//OnTcnSelchangeTab1: changes the shown dialog when a new tab is selected
void MainDialog::OnTcnSelchangeTab1(NMHDR *pNMHDR, LRESULT *pResult) {
	//Hide the tab shown currently
	if (m_pwndShow != NULL)	{
		m_pwndShow->ShowWindow(SW_HIDE);
		m_pwndShow = NULL;
	}
	int tabIndex = m_TabControl.GetCurSel();
	switch (tabIndex) {
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
void MainDialog::disableMainUI(bool isMainEnabled) {
	//All controls have thesame state
	// - enabled: when nothing is running
	// - disabled when the algorithm is running

	m_ImageListBox.EnableWindow(isMainEnabled);
	m_uGAButton.EnableWindow(isMainEnabled);
	m_SGAButton.EnableWindow(isMainEnabled);
	m_OptButton.EnableWindow(isMainEnabled);
}

// Worker thread process for running optimization while MainDialog continues listening for other input
UINT __cdecl optThreadMethod(LPVOID instance) {
	MainDialog * dlg = (MainDialog*)instance;
	dlg->disableMainUI(false); // Disable main UI (except for Start/Stop Button)

	// Setting that we are now runnign an optimization
	dlg->running_optimization_ = true;	// Change label of this button to START now that the optimization is over
	dlg->m_StartStopButton.SetWindowTextW(L"STOP");

	// Perform the optimzation operation depending on selection
	if (dlg->opt_selection_ == dlg->OptType::OPT5) {
		BruteForce_Optimization opt((*dlg), dlg->camCtrl, dlg->slmCtrl);
		dlg->opt_success = opt.runOptimization();
	}
	else if (dlg->opt_selection_ == dlg->OptType::SGA) {
		SGA_Optimization opt((*dlg), dlg->camCtrl, dlg->slmCtrl);
		dlg->opt_success = opt.runOptimization();
	}
	else if (dlg->opt_selection_ == dlg->OptType::uGA) {
		uGA_Optimization opt((*dlg), dlg->camCtrl, dlg->slmCtrl);
		dlg->opt_success = opt.runOptimization();
	}
	else {
		Utility::printLine("ERROR: No optimization method selected!");
		dlg->opt_success = false;
	}
	// Output if error/failure in Optimization
	if (!dlg->opt_success) {
		Utility::printLine("ERROR: Optimization failed!");
		MessageBox(NULL, (LPCWSTR)L"Error!", (LPCWSTR)L"An error has occurred while running the optimization.", MB_ICONERROR | MB_OK);
	}
	// Setting that we are no longer running an optimization
	dlg->running_optimization_ = false;
	// Change label of this button to START now that the optimization is over
	dlg->m_StartStopButton.SetWindowTextW(L"Start Optimization");

	// Update UI
	dlg->disableMainUI(true);
	Utility::printLine("INFO: End of worker optimization thread!");
	return 0;
}

void MainDialog::OnBnClickedStartStopButton() {
	if (this->running_optimization_ == true) {
		Utility::printLine("INFO: Optimization currently running, attempting to stop safely");
		this->stopFlag = true;
	}
	else {
		Utility::printLine("INFO: No optimization currently running, attempting to start depending on selection");
		this->stopFlag = false;
		if (this->runOptThread != NULL) {
			Utility::printLine("WARNING: Optimization worker thread is not null but creating another thread");
		}
		this->runOptThread = AfxBeginThread(optThreadMethod, LPVOID(this));
		this->runOptThread->m_bAutoDelete = true; // Explicit setting for auto delete
	}
}
