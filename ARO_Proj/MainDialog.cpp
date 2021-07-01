// [DESCRIPTION]
// MainDialog.cpp: implementation file for the main dialog of the program
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
using namespace cv;

#define MAX_CFileDialog_FILE_COUNT 99
#define FILE_LIST_BUFFER_SIZE ((MAX_CFileDialog_FILE_COUNT * (MAX_PATH + 1)) + 1)

// [CONSTRUCTOR/COMPONENT EVENTS]
// Constructor for dialog
MainDialog::MainDialog(CWnd* pParent) : CDialog(IDD_BLINKPCIESDK_DIALOG, pParent), m_ReadyRunning(_T(""))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

// Reference UI components
void MainDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
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
	ON_WM_TIMER()
	ON_WM_CLOSE()
	ON_BN_CLICKED(IDC_UGA_BUTTON, &MainDialog::OnBnClickedUgaButton)
	ON_BN_CLICKED(IDC_SGA_BUTTON, &MainDialog::OnBnClickedSgaButton)
	ON_BN_CLICKED(IDC_OPT_BUTTON, &MainDialog::OnBnClickedOptButton)
	ON_NOTIFY(TCN_SELCHANGE, IDC_TAB1, &MainDialog::OnTcnSelchangeTab1)
	ON_BN_CLICKED(IDC_START_STOP_BUTTON, &MainDialog::OnBnClickedStartStopButton)
	ON_BN_CLICKED(IDC_LOAD_SETTINGS, &MainDialog::OnBnClickedLoadSettings)
	ON_BN_CLICKED(IDC_SAVE_SETTINGS, &MainDialog::OnBnClickedSaveSettings)
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
BOOL MainDialog::OnInitDialog() {
	//[CONSOLE OUTPUT]
	if (!AllocConsole()) {
		AfxMessageBox(L"Console will not be shown!");
	}
	freopen_s(&fp, "CONOUT$", "w", stdout);
	std::cout.clear();
	
	//[UI SETUP]
	CDialog::OnInitDialog();
	SetIcon(m_hIcon, TRUE);

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


	//[SET UI DEFAULTS]
	// - get reference to slm controller
	this->slmCtrl = m_slmControlDlg.getSLMCtrl();
	this->slmCtrl->SetMainDlg(this);

	// - set all default settings
	setDefaultUI();
	this->m_slmControlDlg.m_SlmPwrButton.SetWindowTextW(_T("Turn power ON")); // - power button (TODO: determine if SLM is actually off at start)

	// Give a warning message if no boards have been detected
	if (m_slmControlDlg.slmCtrl->boards.size() < 1) {
		MessageBox((LPCWSTR)L"No SLM detected!",
				   (LPCWSTR)L"No SLM has been detected to be connected!",
					MB_ICONWARNING | MB_OK);
	}
	//Show the default tab (optimizations settings)
	m_pwndShow = &m_optimizationControlDlg;

	this->camCtrl = new CameraController((*this));
	if (this->camCtrl != nullptr)	{
		m_aoiControlDlg.SetCameraController(this->camCtrl);
	}
	else {
		Utility::printLine("WARNING: Camera Control NULL");
	}

	// Send the currently selected image from the PCIe card memory board
	// to the SLM. User will immediately see first image in sequence. 
	this->m_slmControlDlg.OnSelchangeImageListbox();

	return TRUE;  // return TRUE  unless you set the focus to a control
}

// Set the UI to default values
void MainDialog::setDefaultUI() {
	// - image names to the listbox and select the first image
	// TODO: check if this is the correct image list box setup 
	this->m_slmControlDlg.m_ImageListBox.ResetContent();
	this->m_slmControlDlg.m_ImageListBox.AddString(_T("ImageOne.bmp"));
	this->m_slmControlDlg.m_ImageListBox.AddString(_T("ImageTwo.zrn"));
	this->m_slmControlDlg.m_ImageListBox.SetCurSel(0);

	this->m_cameraControlDlg.m_FramesPerSecond.SetWindowTextW(_T("200"));
	this->m_cameraControlDlg.m_initialExposureTimeInput.SetWindowTextW(_T("2000"));
	this->m_cameraControlDlg.m_gammaValue.SetWindowTextW(_T("1.25"));
	
	this->m_optimizationControlDlg.m_binSize.SetWindowTextW(_T("16"));
	this->m_optimizationControlDlg.m_numberBins.SetWindowTextW(_T("32"));
	this->m_optimizationControlDlg.m_targetRadius.SetWindowTextW(_T("2"));
	this->m_optimizationControlDlg.m_minFitness.SetWindowTextW(_T("0"));
	this->m_optimizationControlDlg.m_minSeconds.SetWindowTextW(_T("60"));
	this->m_optimizationControlDlg.m_maxSeconds.SetWindowTextW(_T("0")); // 0 or less indicates indefinite
	this->m_optimizationControlDlg.m_minGenerations.SetWindowTextW(_T("0"));
	this->m_optimizationControlDlg.m_maxGenerations.SetWindowTextW(_T("3000"));

	this->m_aoiControlDlg.m_leftInput.SetWindowTextW(_T("896"));
	this->m_aoiControlDlg.m_rightInput.SetWindowTextW(_T("568"));
	this->m_aoiControlDlg.m_widthInput.SetWindowTextW(_T("64"));
	this->m_aoiControlDlg.m_heightInput.SetWindowTextW(_T("64"));

	//Use slm control reference to set additional settings
	// - if the liquid crystal type is nematic, then allow the user the option to
	//compensate for phase imperfections by applying a phase compensation image
	if (this->slmCtrl != nullptr)	{
		if (!this->slmCtrl->IsAnyNematic())
			this->m_slmControlDlg.m_CompensatePhaseCheckbox.ShowWindow(false);
		else
			this->m_slmControlDlg.m_CompensatePhaseCheckbox.ShowWindow(true);
	}
	else {
		Utility::printLine("WARNING: SLM Control NULL");
	}

	this->m_slmControlDlg.populateSLMlist(); // Simple method to setup the list of selections

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
void MainDialog::OnPaint() {
	if (IsIconic()) 	{
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
	else {
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
HCURSOR MainDialog::OnQueryDragIcon() {
	return static_cast<HCURSOR>(m_hIcon);
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
//
//   Modifications:
//				Will halt closing if the optimization is still running, giving 
//				MessageBox to user as well as console to inform them
//
///////////////////////////////////////////////////
void MainDialog::OnClose() {
	// If optimization is running, give warning and prevent closing of application
	if (this->running_optimization_) {
		Utility::printLine("WARNING: Optimization still running!");
		MessageBox(
			(LPCWSTR)L"Still running optimization!",
			(LPCWSTR)L"The optimization is still running! Must be stopped first.",
			MB_ICONWARNING | MB_OK);
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
void MainDialog::OnOK() {}


//OnBnClickedUgaButton: Select the uGA Algorithm Button
void MainDialog::OnBnClickedUgaButton() {
	this->opt_selection_ = OptType::uGA;
	Utility::printLine("INFO: uGA optimization selected");

	// Disabling uGA (now that it's selected) and enabling other options and start button
	this->m_uGAButton.EnableWindow(false);
	this->m_SGAButton.EnableWindow(true);
	this->m_OptButton.EnableWindow(true);
	this->m_StartStopButton.EnableWindow(true);
}

//OnBnClickedSgaButton: Select the SGA Algorithm Button
void MainDialog::OnBnClickedSgaButton() {
	Utility::printLine("INFO: SGA optimization selected");
	this->opt_selection_ = OptType::SGA;

	// Disabling SGA (now that it's selected) and enabling other options and start button
	this->m_SGAButton.EnableWindow(false);
	this->m_uGAButton.EnableWindow(true);
	this->m_OptButton.EnableWindow(true);
	this->m_StartStopButton.EnableWindow(true);
}

//OnBnClickedOptButton: Select the OPT5 Algorithm Button
void MainDialog::OnBnClickedOptButton() {
	Utility::printLine("INFO: OPT5 optimization selected");
	this->opt_selection_ = OptType::OPT5;

	// Disabling BF (now that it's selected) and enabling other options and start button
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

	this->m_slmControlDlg.m_ImageListBox.EnableWindow(isMainEnabled);
	this->m_uGAButton.EnableWindow(isMainEnabled);
	this->m_SGAButton.EnableWindow(isMainEnabled);
	this->m_OptButton.EnableWindow(isMainEnabled);
}

// Start the selected optimization if haven't started, or attempt to stop if already running
void MainDialog::OnBnClickedStartStopButton() {
	if (this->running_optimization_ == true) {
		Utility::printLine("INFO: Optimization currently running, attempting to stop safely.");
		this->stopFlag = true;
	}
	else {
		Utility::printLine("INFO: No optimization currently running, attempting to start depending on selection.");
		this->stopFlag = false;

		// Give an error message if no boards were detected to optimize
		if (this->slmCtrl->boards.size() < 1) {
			MessageBox((LPCWSTR)L"No SLM detected!",
				(LPCWSTR)L"No SLM has been detected to be optimize! Cancelling action.",
				MB_ICONERROR| MB_OK);
			return;
		}
		// Give an error message if no camera
		/*if (this->camCtrl->hasCameras()) {
			MessageBox((LPCWSTR)L"No camera detected!",
				(LPCWSTR)L"No camera has been detected to possibly use! Cancelling action.",
				MB_ICONERROR | MB_OK);
			return;
		}*/
		this->runOptThread = AfxBeginThread(optThreadMethod, LPVOID(this));
		this->runOptThread->m_bAutoDelete = true; // Explicit setting for auto delete
	}
}

// Worker thread process for running optimization while MainDialog continues listening for other input
// Input: instance - pointer to MainDialog instance that called this method (will be cast to MainDialgo*)
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

void MainDialog::OnBnClickedLoadSettings() {
	// TODO: Add your control notification handler code here
	bool tryAgain;
	CString fileName;
	LPWSTR p = fileName.GetBuffer(FILE_LIST_BUFFER_SIZE);
	std::string filePath;
	do {
		tryAgain = false;
		
		// Default to .cfg file extension (this program uses that in it's generation of saving settings)
		static TCHAR BASED_CODE filterFiles[] = _T("Config Files (*.cfg)|*.cfg|ALL Files (*.*)|*.*||");

		CFileDialog dlgFile(TRUE, NULL, L"./", OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, filterFiles);

		OPENFILENAME& ofn = dlgFile.GetOFN();
		ofn.lpstrFile = p;
		ofn.nMaxFile = FILE_LIST_BUFFER_SIZE;

		if (dlgFile.DoModal() == IDOK) {
			fileName = dlgFile.GetPathName();
			fileName.ReleaseBuffer();

			filePath = CT2A(fileName);

			if (!setUIfromFile(filePath)) {
				int err_response = MessageBox(
					(LPCWSTR)L"An error occurred while trying load the file\nTry Again?",
					(LPCWSTR)L"ERROR in file load!",
					MB_ICONERROR | MB_RETRYCANCEL);
				// Respond to decision
				switch (err_response) {
				case IDRETRY:
					tryAgain = true;
					break;
				default: // Cancel or other unknown response will not have try again to make sure not stuck in undesired loop
					tryAgain = false;
				}
			}
		}
		else {
			tryAgain = false;
		}

	} while (tryAgain);
}

bool MainDialog::setUIfromFile(std::string filePath) {
	std::ifstream inputFile(filePath);
	std::string lineBuffer;
	try {
		while (std::getline(inputFile, lineBuffer)) {
			if (lineBuffer != "" && (lineBuffer.find("#") != 0)) {
				// Locate the "=" and use as pivot where prior is the variable name and after is variable value
				int equals_pivot = int(lineBuffer.find("="));
				// Assumption made that there are no spaces from variable name to variable value, rather only occurring after a variable is assigned a value and afterwards may be an in-line comment
				int end_point = int(lineBuffer.find_first_of(" "));

				if (equals_pivot != end_point) {
					// With the two positions acquired, capture the varable's name and the value it is being assigned
					std::string variableName = lineBuffer.substr(0, equals_pivot);
					std::string variableValue = lineBuffer.substr(equals_pivot + 1, end_point - equals_pivot - 1);

					if (!setValueByName(variableName, variableValue)) {
						Utility::printLine("WARNING: Failure to interpret variable '" + variableName + "' with value '" + variableValue + "'!");
					}
				}
			}
		}
	}
	catch (std::exception &e) {
		Utility::printLine("ERROR: " + std::string(e.what()));
		return false;
	}
	return true;
}

bool MainDialog::setValueByName(std::string name, std::string value) {
	CString valueStr(value.c_str());
	// Do a pre-emptive check if either name or value are empty
	if (name == "" || value == "") {
		return false;
	}
	// Camera Dialog
	else if (name == "framesPerSecond")
		this->m_cameraControlDlg.m_FramesPerSecond.SetWindowTextW(valueStr);
	else if (name == "initialExposureTime")
		this->m_cameraControlDlg.m_initialExposureTimeInput.SetWindowTextW(valueStr);
	else if (name == "gamma")
		this->m_cameraControlDlg.m_gammaValue.SetWindowTextW(valueStr);
	// AOI Dialog
	else if (name == "leftAOI")
		this->m_aoiControlDlg.m_leftInput.SetWindowTextW(valueStr);
	else if (name == "rightAOI")
		this->m_aoiControlDlg.m_rightInput.SetWindowTextW(valueStr);
	else if (name == "widthAOI")
		this->m_aoiControlDlg.m_widthInput.SetWindowTextW(valueStr);
	else if (name == "heightAOI")
		this->m_aoiControlDlg.m_heightInput.SetWindowTextW(valueStr);
	// Optimization Dialog
	else if (name == "binSize")
		this->m_optimizationControlDlg.m_binSize.SetWindowTextW(valueStr);
	else if (name == "binNumber")
		this->m_optimizationControlDlg.m_numberBins.SetWindowTextW(valueStr);
	else if (name == "targetRadius")
		this->m_optimizationControlDlg.m_targetRadius.SetWindowTextW(valueStr);
	else if (name == "minFitness")
		this->m_optimizationControlDlg.m_minFitness.SetWindowTextW(valueStr);
	else if (name == "minSeconds")
		this->m_optimizationControlDlg.m_minSeconds.SetWindowTextW(valueStr);
	else if (name == "maxSeconds")
		this->m_optimizationControlDlg.m_maxSeconds.SetWindowTextW(valueStr);
	else if (name == "minGenerations")
		this->m_optimizationControlDlg.m_minGenerations.SetWindowTextW(valueStr);
	else if (name == "maxGenerations")
		this->m_optimizationControlDlg.m_maxGenerations.SetWindowTextW(valueStr);
	// SLM Dialog
	else if (name == "slmConfigMode") {
		if (valueStr == "multi") {
			this->m_slmControlDlg.multiEnable.SetCheck(BST_CHECKED);
			this->m_slmControlDlg.dualEnable.SetCheck(BST_UNCHECKED);
		}
		else if (valueStr == "dual") {
			this->m_slmControlDlg.multiEnable.SetCheck(BST_UNCHECKED);
			this->m_slmControlDlg.dualEnable.SetCheck(BST_CHECKED);
		}
		else {
			this->m_slmControlDlg.multiEnable.SetCheck(BST_UNCHECKED);
			this->m_slmControlDlg.dualEnable.SetCheck(BST_UNCHECKED);
		}
	}
	// TODO: More error handling and inform if there are issues or discrepencies (such as not setting all the SLMs or trying to write to one that is not connected)
	//		Also may need to add more robust form of getting boardID (rn assumes that there are at most 9 boards, anymore will lead to issues)
	else if (name.substr(0, 14) == "slmLutFilePath") {
		// We are dealing with LUT file setting
		int boardID = std::stoi(name.substr(14, 1)) - 1;
		Utility::printLine("DEBUG: LUT boardID->" + std::to_string(boardID));
		if (this->slmCtrl != NULL) {
			if (boardID < this->slmCtrl->boards.size() && boardID >= 0) {
				CT2CA converString(valueStr); // Resource for conversion https://stackoverflow.com/questions/258050/how-do-you-convert-cstring-and-stdstring-stdwstring-to-each-other
				this->m_slmControlDlg.attemptLUTload(boardID, std::string(converString));
			}
			else {
				Utility::printLine("WARNING: Load setting attempted to assign LUT file out of bounds!");
				return false;
			}
		}
		else {
			return false;
		}
	}
	else if (name.substr(0, 14) == "slmWfcFilePath") {
		// We are dealing with WFC file setting
		int boardID = std::stoi(name.substr(14, 1))-1;
		Utility::printLine("DEBUG: WFC boardID->" + std::to_string(boardID));
		if (this->slmCtrl != NULL) {
			if (boardID < this->slmCtrl->boards.size() && boardID >= 0) {
				CT2CA converString(valueStr); // Resource for conversion https://stackoverflow.com/questions/258050/how-do-you-convert-cstring-and-stdstring-stdwstring-to-each-other
				this->m_slmControlDlg.attemptWFCload(boardID, std::string(converString));
			}
			else {
				Utility::printLine("WARNING: Load setting attempted to assign WFC file out of bounds!");
			}
		}
	}

	if (name == "phaseCompensation") {
		if (value == "true") {
			this->m_slmControlDlg.m_CompensatePhaseCheckbox.SetCheck(BST_CHECKED);
		}
		else {
			this->m_slmControlDlg.m_CompensatePhaseCheckbox.SetCheck(BST_UNCHECKED);
		}
	}

	else {	// Unidentfied variable name, return false
		return false;
	}
	return true;
}

void MainDialog::OnBnClickedSaveSettings() {
	bool tryAgain;
	CString fileName;
	LPWSTR p = fileName.GetBuffer(FILE_LIST_BUFFER_SIZE);
	std::string filePath;
	do {
		tryAgain = false;

		// Default Save As dialog box with extension as .cfg
		CFileDialog dlgFile(FALSE, L"cfg");

		OPENFILENAME& ofn = dlgFile.GetOFN();
		ofn.lpstrFile = p;
		ofn.nMaxFile = FILE_LIST_BUFFER_SIZE;

		// If action is OK
		if (dlgFile.DoModal() == IDOK) {
			fileName = dlgFile.GetPathName();
			fileName.ReleaseBuffer();

			filePath = CT2A(fileName);

			if (!saveUItoFile(filePath)) {
				int err_response = MessageBox(
					(LPCWSTR)L"An error occurred while trying save settings\nTry Again?",
					(LPCWSTR)L"ERROR in saving!",
					MB_ICONERROR | MB_RETRYCANCEL);
				// Respond to decision
				switch (err_response) {
				case IDRETRY:
					tryAgain = true;
					break;
				default: // Cancel or other unknown response will not have try again to make sure not stuck in undesired loop
					tryAgain = false;
				}
			}
			else {// Give success message when no issues
				MessageBox((LPCWSTR)L"Successfully saved settings.",
					(LPCWSTR)L"Success!",
					MB_ICONINFORMATION | MB_OK);
				tryAgain = false;
			}
		}
		// If action is cancel
		else {
			tryAgain = false;
		}
		
	} while (tryAgain);
}

bool MainDialog::saveUItoFile(std::string filePath) {
	std::ofstream outFile; // output stream
	CString tempBuff; // Temporary hold of what's in dialog window
	outFile.open(filePath);
	
	outFile << "# ARO PROJECT CONFIGURATION FILE" << std::endl;
	// Camera Dialog
	outFile << "# Camera Settings" << std::endl;
	this->m_cameraControlDlg.m_FramesPerSecond.GetWindowTextW(tempBuff);
	outFile << "framesPerSecond=" << _tstof(tempBuff) << std::endl;
	this->m_cameraControlDlg.m_initialExposureTimeInput.GetWindowTextW(tempBuff);
	outFile << "initialExposureTime=" << _tstof(tempBuff) << std::endl;
	this->m_cameraControlDlg.m_gammaValue.GetWindowTextW(tempBuff);
	outFile << "gamma=" << _tstof(tempBuff) << std::endl;
	// AOI Dialog
	outFile << "# AOI Settings" << std::endl;
	this->m_aoiControlDlg.m_leftInput.GetWindowTextW(tempBuff);
	outFile << "leftAOI=" << _tstof(tempBuff) << std::endl;
	this->m_aoiControlDlg.m_rightInput.GetWindowTextW(tempBuff);
	outFile << "rightAOI=" << _tstof(tempBuff) << std::endl;
	this->m_aoiControlDlg.m_widthInput.GetWindowTextW(tempBuff);
	outFile << "widthAOI=" << _tstof(tempBuff) << std::endl;
	this->m_aoiControlDlg.m_heightInput.GetWindowTextW(tempBuff);
	outFile << "heightAOI=" << _tstof(tempBuff) << std::endl;
	// Optimization Dialog
	outFile << "# Optimization Settings" << std::endl;
	this->m_optimizationControlDlg.m_binSize.GetWindowTextW(tempBuff);
	outFile << "binSize=" << _tstof(tempBuff) << std::endl;
	this->m_optimizationControlDlg.m_numberBins.GetWindowTextW(tempBuff);
	outFile << "binNumber=" << _tstof(tempBuff) << std::endl;
	this->m_optimizationControlDlg.m_targetRadius.GetWindowTextW(tempBuff);
	outFile << "targetRadius=" << _tstof(tempBuff) << std::endl;
	this->m_optimizationControlDlg.m_minFitness.GetWindowTextW(tempBuff);
	outFile << "minFitness=" << _tstof(tempBuff) << std::endl;
	this->m_optimizationControlDlg.m_minSeconds.GetWindowTextW(tempBuff);
	outFile << "minSeconds=" << _tstof(tempBuff) << std::endl;
	this->m_optimizationControlDlg.m_maxSeconds.GetWindowTextW(tempBuff);
	outFile << "maxSeconds=" << _tstof(tempBuff) << std::endl;
	this->m_optimizationControlDlg.m_minGenerations.GetWindowTextW(tempBuff);
	outFile << "minGenerations=" << _tstof(tempBuff) << std::endl;
	this->m_optimizationControlDlg.m_maxGenerations.GetWindowTextW(tempBuff);
	outFile << "maxGenerations=" << _tstof(tempBuff) << std::endl;
	// SLM Dialog Settings
	outFile << "# SLM Configuratoin Settings" << std::endl;
	outFile << "slmConfigMode=";
	if (this->m_slmControlDlg.dualEnable.GetCheck() == BST_CHECKED) { outFile << "dual\n"; }
	else if (this->m_slmControlDlg.multiEnable.GetCheck() == BST_CHECKED) {	outFile << "multi\n"; }
	else { outFile << "single\n"; }

	if (this->slmCtrl != NULL) {
		// Output the LUT file being used for every board and PhaseCompensationFile
		for (int i = 0; i < this->slmCtrl->boards.size(); i++) {
			outFile << "slmLutFilePath" << std::to_string(i + 1) << "=" << this->slmCtrl->boards[i]->LUTFileName << "\n";
			outFile << "slmWfcFilePath" << std::to_string(i + 1) << "=" << this->slmCtrl->boards[i]->PhaseCompensationFileName << "\n";
		}
	}

	outFile << "phaseCompensation=";
	if (this->m_slmControlDlg.m_CompensatePhaseCheckbox.GetCheck() == BST_CHECKED) {
		outFile << "true\n";
	}
	else {
		outFile << "false\n";
	}

	return true;
}
