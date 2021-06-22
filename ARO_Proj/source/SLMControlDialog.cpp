// SLMControlDialog.cpp : implementation file
//

#include "../headers/stdafx.h"
#include "../headers/SLMController.h"
#include "../headers/SLM_Board.h"
#include "../headers/SLMControlDialog.h"
#include "../headers/Utility.h"
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
}

SLMControlDialog::~SLMControlDialog()
{
	delete this->slmCtrl;
}

void SLMControlDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_SLM_PWR_BUTTON, m_SlmPwrButton);
	DDX_Control(pDX, ID_SLM_SELECT, slmSelection_);
	DDX_Control(pDX, IDC_SLM_DUAL, dualEnable);
	DDX_Control(pDX, IDC_SLM_ALLSAME, SLM_SetALLSame_);
}

BEGIN_MESSAGE_MAP(SLMControlDialog, CDialogEx)
	ON_BN_CLICKED(IDC_SLM_PWR_BUTTON, &SLMControlDialog::OnBnClickedSlmPwrButton)
	ON_BN_CLICKED(IDC_SETLUT, &SLMControlDialog::OnBnClickedSetlut)
	ON_BN_CLICKED(IDC_SETWFC, &SLMControlDialog::OnBnClickedSetwfc)
	ON_BN_CLICKED(IDC_SLM_DUAL, &SLMControlDialog::OnBnClickedDualSLM)
	ON_CBN_SELCHANGE(IDC_SLM_ALLSAME, &SLMControlDialog::OnCbnChangeSlmAll)
	ON_CBN_SELCHANGE(ID_SLM_SELECT, &SLMControlDialog::OnCbnSelchangeSlmSelect)
END_MESSAGE_MAP()

// SLMControlDialog message handlers

void SLMControlDialog::OnBnClickedSlmPwrButton() {
	CString PowerState;
	m_SlmPwrButton.GetWindowTextW(PowerState);
	CStringA pState(PowerState);

	if (strcmp(pState, "Turn power ON") == 0) //the strings are equal, power is off
	{
		this->slmCtrl->setBoardPower(true); //turn the SLM on
		m_SlmPwrButton.SetWindowTextW(_T("Turn power OFF")); //update button
		Utility::printLine("INFO: All SLMs were turned ON");
	}
	else {
		this->slmCtrl->setBoardPower(false); //turn the SLM off
		m_SlmPwrButton.SetWindowTextW(_T("Turn power ON")); //update button
		Utility::printLine("INFO: All SLMs were turned OFF");
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
		noErrors = false;
		MessageBox(
			(LPCWSTR)L"ERROR in SLM selection!",
			(LPCWSTR)L"Failed to load WFC file as SLM selected is not in boards list.",
			MB_ICONERROR | MB_OK
		);
		return noErrors;
	}

	// Assigning LUT file with given file path, and if error give message box to try again
	if (!this->slmCtrl->AssignLUTFile(slmNum, filePath)) {
		// Notify user of error in LUT file loading and get response action
		Utility::printLine("ERROR: Failed to assign given file " + filePath + " to board " + std::to_string(slmNum) + "!");
		// Resource: https://docs.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-messagebox
		int err_response = MessageBox(
			(LPCWSTR)L"ERROR in file load!",
			(LPCWSTR)L"Failed to load LUT file\nTry Again? Canceling may leave with undefined issues with SLM",
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
		CFileDialog dlgFile(TRUE, NULL, L"./slm3986_at532_P8.LUT", OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, filterFiles);

		OPENFILENAME& ofn = dlgFile.GetOFN();
		ofn.lpstrFile = p;
		ofn.nMaxFile = FILE_LIST_BUFFER_SIZE;

		if (dlgFile.DoModal() == IDOK) {
			fileName = dlgFile.GetPathName();
			fileName.ReleaseBuffer();
		}
		filePath = CT2A(fileName);

		if (this->SLM_SetALLSame_.GetCheck() == BST_UNCHECKED) {
			// Get the SLM being assinged the LUT file, asssumes the index poistion of the selection is consistent with board selection
			//		for example if the user selects 1 (out of 2) the value should be 0.  This is done currently (June 15th as a shortcut instead of parsing text of selection)
			tryAgain = !attemptLUTload(this->slmSelectionID_, filePath);
		}
		else {
			// SLM set all setting is checked, so assign the same LUT file to all the boards
			for (int  slmNum = 0; slmNum < this->slmCtrl->boards.size() && !tryAgain; slmNum++) {
				tryAgain = !attemptLUTload(slmNum, filePath);
			}
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
		noErrors = false;
		MessageBox(
			(LPCWSTR)L"ERROR in SLM selection!",
			(LPCWSTR)L"Failed to load WFC file as SLM selected is not in boards list.",
			MB_ICONERROR | MB_OK
			);
		return noErrors;
	}

	SLM_Board * board = slmCtrl->boards[slmNum];
	board->PhaseCompensationFileName = filePath;
	if (!slmCtrl->ReadAndScaleBitmap(board, board->PhaseCompensationData, board->PhaseCompensationFileName)) {
		// Notify user of error in LUT file loading and get response action
		Utility::printLine("ERROR: Failed to assign given WFC file " + filePath + " to board " + std::to_string(slmNum) + "!");
		// Resource: https://docs.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-messagebox
		int err_response = MessageBox(
			(LPCWSTR)L"ERROR in file load!",
			(LPCWSTR)L"Failed to load WFC file\nTry Again? Canceling may leave with undefined issues with SLM",
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
		CFileDialog dlgFile(TRUE);
		OPENFILENAME& ofn = dlgFile.GetOFN();
		ofn.lpstrFile = p;
		ofn.nMaxFile = FILE_LIST_BUFFER_SIZE;

		if (dlgFile.DoModal() == IDOK)	{
			fileName = dlgFile.GetPathName();
			fileName.ReleaseBuffer();
		}
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
	} while (tryAgain);
}

SLMController* SLMControlDialog::getSLMCtrl() { return this->slmCtrl; }

void SLMControlDialog::OnBnClickedDualSLM() {
	// When attempting to enable Dual SLM setup, will confirm that there are enough boards
	if (this->dualEnable.GetCheck() == BST_CHECKED) {
		if (this->slmCtrl->boards.size() < 2) {
			// If not possible, will give warning in console and window along with undoing the selection
			Utility::printLine("WARNING: Multi-SLM was enabled but there are 1 or fewer boards! Disabling check.");
			MessageBox(
				(LPCWSTR)L"Multi SLM ERROR",
				(LPCWSTR)L"You have attempted to enable Multi SLM but not enough boards were found (1 or fewer)! Disabling selection.",
				MB_ICONWARNING | MB_OK
			);
			this->dualEnable.SetCheck(BST_UNCHECKED);
		}
		else {
			Utility::printLine("INFO: Multi-SLM has been enabled");

		}
	}
	else {
		Utility::printLine("INFO: Multi-SLM has been disabled");
	}
}

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

// Update selection ID value
void SLMControlDialog::OnCbnSelchangeSlmSelect() { this->slmSelectionID_ = this->slmSelection_.GetCurSel(); }
