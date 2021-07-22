// SLMControlDialog.cpp : implementation file
//

#include "stdafx.h"
#include "SLMController.h"
#include "SLM_Board.h"
#include "SLMControlDialog.h"
#include "Utility.h"
#include "afxdialogex.h"

#include <string>

#define MAX_CFileDialog_FILE_COUNT 99
#define FILE_LIST_BUFFER_SIZE ((MAX_CFileDialog_FILE_COUNT * (MAX_PATH + 1)) + 1)

// SLMControlDialog dialog

IMPLEMENT_DYNAMIC(SLMControlDialog, CDialogEx)

SLMControlDialog::SLMControlDialog(CWnd* pParent /*=NULL*/)
: CDialogEx(SLMControlDialog::IDD, pParent)
{
	this->slmCtrl = new SLMController();
	this->slmSelectionID_ = 0;
	this->m_mainToolTips = new CToolTipCtrl();
}

BOOL SLMControlDialog::OnInitDialog() {
	// Setup tool tips for SLM dialog window
	
	this->m_mainToolTips->Create(this);
	this->m_mainToolTips->AddTool(this->GetDlgItem(IDC_SLM_PWR_BUTTON), L"Set currently selected SLM's power on or off");
	this->m_mainToolTips->AddTool(this->GetDlgItem(ID_SLM_SELECT), L"Select SLM to assign LUT and wavefront compensation files to and turn on/off");
	this->m_mainToolTips->AddTool(this->GetDlgItem(IDC_SLM_MULTI), L"Optimize all connected boards at the same time");
	this->m_mainToolTips->AddTool(this->GetDlgItem(IDC_SLM_ALLSAME), L"Set to ignore select SLM and apply changes in settings to all connected boards");
	this->m_mainToolTips->AddTool(this->GetDlgItem(IDC_SLM_DUAL), L"Optimize first two connected boards at the same time");
	this->m_mainToolTips->AddTool(this->GetDlgItem(IDC_CURRENT_WFC_OUT), L"The current wave compensation file being assigned to this SLM");
	this->m_mainToolTips->AddTool(this->GetDlgItem(IDC_CURR_LUT_OUT), L"The current LUT file being assigned to this SLM");
	this->m_mainToolTips->AddTool(this->GetDlgItem(IDC_SETLUT), L"Set LUT file for the selected board(s)");
	this->m_mainToolTips->AddTool(this->GetDlgItem(IDC_SETWFC), L"Set wavefront compensation file for the selected board(s)");
	this->m_mainToolTips->Activate(true);

	return CDialogEx::OnInitDialog();
}

SLMControlDialog::~SLMControlDialog()
{
	delete this->m_mainToolTips;
}

void SLMControlDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Check(pDX, IDC_COMPENSATE_PHASE_CHECKBOX, m_CompensatePhase);
	DDX_Control(pDX, IDC_COMPENSATE_PHASE_CHECKBOX, m_CompensatePhaseCheckbox);
	DDX_Control(pDX, IDC_SLM_PWR_BUTTON, m_SlmPwrButton);
	DDX_Control(pDX, ID_SLM_SELECT, slmSelection_);
	DDX_Control(pDX, IDC_SLM_MULTI, multiEnable);
	DDX_Control(pDX, IDC_SLM_ALLSAME, SLM_SetALLSame_);
	DDX_Control(pDX, IDC_SLM_DUAL, dualEnable);
	DDX_Control(pDX, IDC_CURRENT_WFC_OUT, m_WFC_pathDisplay);
	DDX_Control(pDX, IDC_CURR_LUT_OUT, m_LUT_pathDisplay);
}

BEGIN_MESSAGE_MAP(SLMControlDialog, CDialogEx)
	ON_BN_CLICKED(IDC_SLM_PWR_BUTTON, &SLMControlDialog::OnBnClickedSlmPwrButton)
	ON_BN_CLICKED(IDC_SETLUT, &SLMControlDialog::OnBnClickedSetlut)
	ON_BN_CLICKED(IDC_SETWFC, &SLMControlDialog::OnBnClickedSetwfc)
	ON_BN_CLICKED(IDC_SLM_MULTI, &SLMControlDialog::OnBnClickedMultiSLM)
	ON_CBN_SELCHANGE(IDC_SLM_ALLSAME, &SLMControlDialog::OnCbnChangeSlmAll)
	ON_CBN_SELCHANGE(ID_SLM_SELECT, &SLMControlDialog::OnCbnSelchangeSlmSelect)
	ON_BN_CLICKED(IDC_COMPENSATE_PHASE_CHECKBOX, &SLMControlDialog::OnCompensatePhaseCheckbox)
	ON_BN_CLICKED(IDC_SLM_DUAL, &SLMControlDialog::OnBnClickedSlmDual)
END_MESSAGE_MAP()

BOOL SLMControlDialog::PreTranslateMessage(MSG* pMsg) {
	if (this->m_mainToolTips != NULL) {
		this->m_mainToolTips->RelayEvent(pMsg);
	}
	return CDialog::PreTranslateMessage(pMsg);
}

// Update selection ID value and rest of GUI
void SLMControlDialog::OnCbnSelchangeSlmSelect() { 
	// Update current SLM selection
	this->slmSelectionID_ = this->slmSelection_.GetCurSel();
	// Update Phase Compensation File path
	CString WFCpath(this->slmCtrl->boards[this->slmSelectionID_]->PhaseCompensationFileName.c_str());
	this->m_WFC_pathDisplay.SetWindowTextW(WFCpath);
	// Update LUT file name
	CString LUTpath(this->slmCtrl->boards[this->slmSelectionID_]->LUTFileName.c_str());
	this->m_LUT_pathDisplay.SetWindowTextW(LUTpath);
	// Update power button to reflect on current power state
	CString pwrMsg;
	if (this->slmCtrl->boards[this->slmSelectionID_]->isPoweredOn()) {
		pwrMsg = "Turn power OFF";
	}
	else {
		pwrMsg = "Turn power ON";
	}
	this->m_SlmPwrButton.SetWindowTextW(pwrMsg);
}


void SLMControlDialog::OnBnClickedSlmPwrButton() {
	CString PowerState;
	m_SlmPwrButton.GetWindowTextW(PowerState);
	CStringA pState(PowerState);

	// Update SLM power
	if (this->SLM_SetALLSame_.GetCheck() == BST_CHECKED) {
		if (!this->slmCtrl->boards[this->slmSelectionID_]->isPoweredOn()) {// if this board is not powered on then power on all boards
			this->slmCtrl->setBoardPowerALL(true); //turn the SLMs on
			Utility::printLine("INFO: All SLMs were turned ON");
		}
		else {
			this->slmCtrl->setBoardPowerALL(false); //turn the SLMs off
			Utility::printLine("INFO: All SLMs were turned OFF");
		}
	}
	else {
		if (!this->slmCtrl->boards[this->slmSelectionID_]->isPoweredOn()) {// if not powered on, then turn on
			this->slmCtrl->setBoardPower(this->slmSelectionID_,true); //turn the SLM on
			Utility::printLine("INFO: SLM #"+std::to_string(this->slmSelectionID_+1) + " was turned ON");
		}
		else {
			this->slmCtrl->setBoardPower(this->slmSelectionID_, false); //turn the SLM off
			Utility::printLine("INFO: SLM #" + std::to_string(this->slmSelectionID_ + 1) + " was turned OFF");
		}
	}
	// Update power button text
	if (this->slmCtrl->boards[this->slmSelectionID_]->isPoweredOn()) {
		m_SlmPwrButton.SetWindowTextW(_T("Turn power OFF")); //update button
	}
	else {
		this->m_SlmPwrButton.SetWindowTextW(_T("Turn power OFF")); //update button
	}
}

// Method for encapsulating the UX of setting LUT file for given board and filepath
// Input:
//		slmNum - the board being assigned (0 based index)
//		filePath - string to path of the LUT file being loaded
// Output: returns true if error occurs and user selects retry, false otherwise
//		When an error occurs in loading LUT file, gives message box with option to retry or cancel
bool SLMControlDialog::attemptLUTload(int slmNum, std::string filePath) {
	bool noErrors = true;
	if (slmNum < 0 || slmNum >= this->slmCtrl->boards.size()) {
		std::string errMsg = "Failed to assign given LUT file" + filePath + " to board " + std::to_string(slmNum) + " as there is none at this position!";
		noErrors = false;
		MessageBox(
			(LPCWSTR)(errMsg.c_str()),
			(LPCWSTR)L"ERROR in SLM selection!",
			MB_ICONERROR | MB_OK
		);
		return noErrors;
	}
	// Assigning LUT file with given file path, and if error give message box to try again
	if (!this->slmCtrl->AssignLUTFile(slmNum, filePath)) {
		std::string errMsg = "Failed to assign given LUT file" + filePath + " to board " + std::to_string(slmNum) + "!";
		// Notify user of error in LUT file loading and get response action
		Utility::printLine("ERROR: " + errMsg);
		// Resource: https://docs.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-messagebox
		int err_response = MessageBox(
			(LPCWSTR)(errMsg.c_str()),
			(LPCWSTR)L"ERROR in file load!",
			MB_ICONERROR | MB_RETRYCANCEL
		);
		// Respond to decision
		switch (err_response) {
			case IDRETRY:
				noErrors = false;
				break;
			default: // Cancel or other unknown response will not have try again to make sure not stuck in undesired loop
				noErrors = true;
		}
	}
	else {
		Utility::printLine("INFO: Assigned the following file to board " + std::to_string(slmNum) + ": " + filePath);
	}
	return noErrors;
}

void SLMControlDialog::OnBnClickedSetlut() {
	bool tryAgain;
	CString fileName;
	std::string filePath;
	do {
		tryAgain = false;
		LPWSTR p = fileName.GetBuffer(FILE_LIST_BUFFER_SIZE);

		// Initially only show LUT files (end in .LUT extension) but also provide option to show all files
		static TCHAR BASED_CODE filterFiles[] = _T("LUT Files (*.LUT)|*.LUT|ALL Files (*.*)|*.*||");
		// Construct and open standard Windows file dialog box with default filename being "./slm3986_at532_P8.LUT"
		CFileDialog dlgFile(TRUE, NULL, L"./slm3986_at532_P8.LUT", OFN_FILEMUSTEXIST, filterFiles);

		OPENFILENAME& ofn = dlgFile.GetOFN();
		ofn.lpstrFile = p;
		ofn.nMaxFile = FILE_LIST_BUFFER_SIZE;

		if (dlgFile.DoModal() == IDOK) {
			fileName = dlgFile.GetPathName();
			fileName.ReleaseBuffer();

			filePath = CT2A(fileName);

			if (this->SLM_SetALLSame_.GetCheck() == BST_UNCHECKED) {
				// Get the SLM being assinged the LUT file, asssumes the index poistion of the selection is consistent with board selection
				//		for example if the user selects 1 (out of 2) the value should be 0.  This is done currently (June 15th as a shortcut instead of parsing text of selection)
				tryAgain = !attemptLUTload(this->slmSelectionID_, filePath);
			}
			else {
				// SLM set all setting is checked, so assign the same LUT file to all the boards
				for (int slmNum = 0; slmNum < this->slmCtrl->boards.size() && !tryAgain; slmNum++) {
					tryAgain = !attemptLUTload(slmNum, filePath);
				}
			}
			CString fileCS(this->slmCtrl->boards[this->slmSelectionID_]->LUTFileName.c_str());
			this->m_LUT_pathDisplay.SetWindowTextW(fileCS);
		}
		else {
			Utility::printLine("INFO: Cancelled setting LUT file, no change made.");
			tryAgain = false;
		}
	} while (tryAgain);
}

// Method for encapsulating the UX of setting WFC file for given board and filepath
// Input:
//		slmNum - the board being assigned (0 based index)
//		filePath - string to path of the LUT file being loaded
// Output: returns true if error occurs and user does not select retry, false otherwise
//		When an error occurs in loading LUT file, gives message box with option to retry or cancel
bool SLMControlDialog::attemptWFCload(int slmNum, std::string filePath) {
	bool noErrors = true;
	if (slmNum >= this->slmCtrl->boards.size() || slmNum < 0) {
		std::string errMsg = "Failed to assign given WFC file " + filePath + " to board " + std::to_string(slmNum) + " as there is none at this position!";
		noErrors = false;
		MessageBox(
			(LPCWSTR)(errMsg.c_str()),
			(LPCWSTR)L"ERROR in SLM selection!",
			MB_ICONERROR | MB_OK
			);
		return noErrors;
	}
	SLM_Board * board = slmCtrl->boards[slmNum];
	// Read and assign the image file to PhaseCompensationData
	if (!slmCtrl->LoadPhaseCompensationData(board, filePath)) {
		std::string errMsg = "Failed to assign given WFC file '" + filePath + "' to board " + std::to_string(slmNum) + "!";
		// Notify user of error in LUT file loading and get response action
		Utility::printLine("ERROR: " + errMsg);
		// Resource: https://docs.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-messagebox
		int err_response = MessageBox(
			(LPCWSTR)(errMsg.c_str()),
			(LPCWSTR)L"ERROR in file load!",
			MB_ICONERROR | MB_RETRYCANCEL
			);
		// Respond to decision
		switch (err_response) {
			case IDRETRY:
				noErrors = false;
				break;
			default: // Cancel or other unknown response will not have try again to make sure not stuck in undesired loop
				noErrors = true;
		}
	}
	else { // If no issue update the file name
		board->PhaseCompensationFileName = filePath;
	}
	// Update GUI
	CString fileCS(board->PhaseCompensationFileName.c_str());
	this->m_LUT_pathDisplay.SetWindowTextW(fileCS);
	return noErrors;
}

void SLMControlDialog::OnBnClickedSetwfc() {
	bool tryAgain;
	CString fileName;
	std::string filePath;
	do {
		// Attempt to select WFC file
		tryAgain = false;
		LPWSTR p = fileName.GetBuffer(FILE_LIST_BUFFER_SIZE);
		static TCHAR BASED_CODE filterFiles[] = _T("BMP Files (*.bmp)|*.bmp|ALL Files (*.*)|*.*||");
		// Construct and open standard Windows file dialog box with default filename being "./slm3986_at532_P8.LUT"
		CFileDialog dlgFile(TRUE, NULL, L"./Blank.bmp", OFN_FILEMUSTEXIST, filterFiles);
		OPENFILENAME& ofn = dlgFile.GetOFN();
		ofn.lpstrFile = p;
		ofn.nMaxFile = FILE_LIST_BUFFER_SIZE;

		if (dlgFile.DoModal() == IDOK)	{
			fileName = dlgFile.GetPathName();
			fileName.ReleaseBuffer();
			filePath = CT2A(fileName);
			int slmNum;
			// If not set to set all SLMs, then get current selection and set only that one
			if (this->SLM_SetALLSame_.GetCheck() == BST_UNCHECKED) {
				slmNum = this->slmSelection_.GetCurSel(); // NOTE: GetCurSel is index of selection and doesn't refer to "face value" of the selection
				tryAgain = !attemptWFCload(slmNum, filePath);
			}
			else {
				for (slmNum = 0; slmNum < this->slmCtrl->boards.size() && !tryAgain; slmNum++) {
					tryAgain = !attemptWFCload(slmNum, filePath);
				}
			}
		}
		else {
			Utility::printLine("INFO: Cancelled setting WFC file, no change made.");
			tryAgain = false;
		}
	} while (tryAgain);
}

SLMController* SLMControlDialog::getSLMCtrl() { return this->slmCtrl; }

void SLMControlDialog::OnCbnChangeSlmAll() {
	// If ALLSame is enabled, then disable the SLM selection box
	if (this->SLM_SetALLSame_.GetCheck() == BST_CHECKED) {
		Utility::printLine("INFO: Set to all SLM being the same! Now when setting LUT/WFC it will apply to all the SLMs!");
		this->slmSelection_.EnableWindow(false);
	}
	// If ALLSame is disabled, then enable the SLM selection box
	else {
		Utility::printLine("INFO: SLM set to NOT all same!");
		this->slmSelection_.EnableWindow(true);
	}
}

// Simple method for populating the selection list depending on number of boards connected
void SLMControlDialog::populateSLMlist() {
	this->slmSelection_.Clear();
	int numBoards = int(this->slmCtrl->boards.size());
	// Populate drop down menu with numbers for each SLM
	for (int i = 0; i < numBoards; i++) {
		CString strI(std::to_string(i + 1).c_str());
		LPCTSTR lpstrI(strI);
		this->slmSelection_.AddString(lpstrI);
	}
	// If there is at least one board, default to board 0 for selection
	if (numBoards > 0) {
		this->slmSelection_.SetCurSel(0);
	}
}

void SLMControlDialog::OnBnClickedMultiSLM() {
	// When attempting to enable Multi SLM setup, will confirm that there are enough boards
	if (this->multiEnable.GetCheck() == BST_CHECKED) {
		if (this->slmCtrl->boards.size() < 2) {
			// If not possible, will give warning in console and window along with undoing the selection
			Utility::printLine("WARNING: Multi-SLM was enabled but there are 1 or fewer boards! Disabling check.");
			MessageBox(
				(LPCWSTR)L"Multi SLM ERROR",
				(LPCWSTR)L"You have attempted to enable Multi SLM but not enough boards were found (1 or fewer)! Disabling selection.",
				MB_ICONWARNING | MB_OK
				);
			this->multiEnable.SetCheck(BST_UNCHECKED);
		}
		else {
			Utility::printLine("INFO: Multi-SLM has been enabled");
			this->dualEnable.SetCheck(BST_UNCHECKED);
		}
	}
	else {
		Utility::printLine("INFO: Multi-SLM has been disabled");
	}
}

void SLMControlDialog::OnBnClickedSlmDual() {
	// When attempting to enable Multi SLM setup, will confirm that there are enough boards
	if (this->dualEnable.GetCheck() == BST_CHECKED) {
		if (this->slmCtrl->boards.size() < 2) {
			// If not possible, will give warning in console and window along with undoing the selection
			Utility::printLine("WARNING: Dual-SLM was enabled but there are 1 or fewer boards! Disabling check.");
			MessageBox(
				(LPCWSTR)L"Dual SLM ERROR",
				(LPCWSTR)L"You have attempted to enable Dual SLM but not enough boards were found (1 or fewer)! Disabling selection.",
				MB_ICONWARNING | MB_OK
				);
			this->dualEnable.SetCheck(BST_UNCHECKED);
		}
		else {
			Utility::printLine("INFO: Dual-SLM has been enabled");
			this->multiEnable.SetCheck(BST_UNCHECKED);
		}
	}
	else {
		Utility::printLine("INFO: Dual-SLM has been disabled");
	}
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
//			  This will enable the usage of the PhaseCompensationData on writing images to the SLMs
//
//   Modifications:
//
//////////////////////////////////////////////////
void SLMControlDialog::OnCompensatePhaseCheckbox() {
	UpdateData(true);
}
