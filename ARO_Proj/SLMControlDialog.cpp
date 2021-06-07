// SLMControlDialog.cpp : implementation file
//

#include "stdafx.h"
#include "SLMController.h"
#include "SLM_Board.h"
#include "SLMControlDialog.h"
#include "Utility.h"
#include "afxdialogex.h"

#define MAX_CFileDialog_FILE_COUNT 99
#define FILE_LIST_BUFFER_SIZE ((MAX_CFileDialog_FILE_COUNT * (MAX_PATH + 1)) + 1)

// SLMControlDialog dialog

IMPLEMENT_DYNAMIC(SLMControlDialog, CDialogEx)

SLMControlDialog::SLMControlDialog(CWnd* pParent /*=NULL*/)
	: CDialogEx(SLMControlDialog::IDD, pParent)
{
	slmCtrl = new SLMController();
}

SLMControlDialog::~SLMControlDialog()
{
	delete slmCtrl;
}

void SLMControlDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_SLM_PWR_BUTTON, m_SlmPwrButton);
}


BEGIN_MESSAGE_MAP(SLMControlDialog, CDialogEx)
	ON_BN_CLICKED(IDC_SLM_PWR_BUTTON, &SLMControlDialog::OnBnClickedSlmPwrButton)
	ON_BN_CLICKED(IDC_SETLUT, &SLMControlDialog::OnBnClickedSetlut)
	ON_BN_CLICKED(IDC_SETWFC, &SLMControlDialog::OnBnClickedSetwfc)
END_MESSAGE_MAP()


// SLMControlDialog message handlers


void SLMControlDialog::OnBnClickedSlmPwrButton()
{
	CString PowerState;
	m_SlmPwrButton.GetWindowTextW(PowerState);
	CStringA pState(PowerState);

	if (strcmp(pState, "Turn power ON") == 0) //the strings are equal, power is off
	{
		slmCtrl->setBoardPower(true); //turn the SLM on
		m_SlmPwrButton.SetWindowTextW(_T("Turn power OFF")); //update button
		Utility::printLine("INFO: All SLMs were turned ON");
	}
	else
	{
		slmCtrl->setBoardPower(false); //turn the SLM off
		m_SlmPwrButton.SetWindowTextW(_T("Turn power ON")); //update button
		Utility::printLine("INFO: All SLMs were turned OFF");
	}
}


void SLMControlDialog::OnBnClickedSetlut()
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

	std::string file = CT2A(fileName);
	slmCtrl->AssignLUTFile(0, file);
}


void SLMControlDialog::OnBnClickedSetwfc()
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

	SLM_Board * board = slmCtrl->boards[0];
	std::string file = CT2A(fileName);
	board->PhaseCompensationFileName = file;
	slmCtrl->ReadAndScaleBitmap(board, board->PhaseCompensationData, board->PhaseCompensationFileName);
}


SLMController* SLMControlDialog::getSLMCtrl() { return slmCtrl; }
