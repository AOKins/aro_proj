// OutputControlDialog.cpp : implementation file
//

#include "stdafx.h"
#include "OutputControlDialog.h"
#include "afxdialogex.h"


// OutputControlDialog dialog

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
	DDX_Control(pDX, IDC_SAVE_IMAGES, m_SaveImagesCheck);
	DDX_Control(pDX, IDC_DISPLAY_CAM, m_displayCameraCheck);
	DDX_Control(pDX, IDC_DISPLAY_SLM, m_displaySLM);
	DDX_Control(pDX, IDC_LOG_FILES, m_logFilesCheck);
}


BEGIN_MESSAGE_MAP(OutputControlDialog, CDialogEx)
END_MESSAGE_MAP()


// OutputControlDialog message handlers
