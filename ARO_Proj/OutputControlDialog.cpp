// OutputControlDialog.cpp : implementation file
//

#include "stdafx.h"
#include "OutputControlDialog.h"
#include "afxdialogex.h"
#include "Utility.h"

// OutputControlDialog dialog

#define MAX_CFileDialog_FILE_COUNT 99
#define FILE_LIST_BUFFER_SIZE ((MAX_CFileDialog_FILE_COUNT * (MAX_PATH + 1)) + 1)

IMPLEMENT_DYNAMIC(OutputControlDialog, CDialogEx)

OutputControlDialog::OutputControlDialog(CWnd* pParent /*=NULL*/)
	: CDialogEx(OutputControlDialog::IDD, pParent)
{

}

OutputControlDialog::~OutputControlDialog()
{
}

void OutputControlDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_DISPLAY_CAM, m_displayCameraCheck);
	DDX_Control(pDX, IDC_DISPLAY_SLM, m_displaySLM);
	DDX_Control(pDX, IDC_LOGALL_FILES, m_logAllFilesCheck);
	DDX_Control(pDX, IDC_SAVE_ELITEIMAGE, m_SaveEliteImagesCheck);
	DDX_Control(pDX, IDC_SAVE_TIMEVFIT, m_SaveTimeVFitnessCheck);
	DDX_Control(pDX, IDC_SAVE_FINALIMAGE, m_SaveFinalImagesCheck);
	DDX_Control(pDX, IDC_EXPOSURE_FILE, m_SaveExposureShortCheck);
	DDX_Control(pDX, IDC_OUTPUT_LOCATION, m_OutputLocationField);
}


BEGIN_MESSAGE_MAP(OutputControlDialog, CDialogEx)
	ON_BN_CLICKED(IDC_OUTPUT_LOCATION_BUTTON, &OutputControlDialog::OnBnClickedOutputLocationButton)
	ON_BN_CLICKED(IDC_LOGALL_FILES, &OutputControlDialog::OnBnClickedLogallFiles)
END_MESSAGE_MAP()


// OutputControlDialog message handlers

void OutputControlDialog::OnBnClickedOutputLocationButton()
{
	bool tryAgain;
	CString folderInput;
	do {
		tryAgain = false;
		LPWSTR p = folderInput.GetBuffer(FILE_LIST_BUFFER_SIZE);
		// https://www.codeproject.com/Tips/993640/Using-MFC-CFolderPickerDialog
		CString current;
		this->m_OutputLocationField.GetWindowTextW(current);
		CFolderPickerDialog dlgFolder(current);
		if (dlgFolder.DoModal() == IDOK) {
			folderInput = dlgFolder.GetPathName();

			folderInput += _T("\\");
			this->m_OutputLocationField.SetWindowTextW(folderInput);

			Utility::printLine("INFO: Updated output path to '" + std::string(CT2A(folderInput)) + "'!");
		}

	} while (tryAgain == true);
}

// If this is enabled, disable all of the other toggles or vice versa
void OutputControlDialog::OnBnClickedLogallFiles() {
	bool enable = !(this->m_logAllFilesCheck.GetCheck() == BST_CHECKED);
	// If enable all is checked, then the sub-options are disabled
	this->m_SaveEliteImagesCheck.EnableWindow(enable);
	this->m_SaveFinalImagesCheck.EnableWindow(enable);
	this->m_SaveTimeVFitnessCheck.EnableWindow(enable);
	this->m_SaveExposureShortCheck.EnableWindow(enable);

}
