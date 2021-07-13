// CameraControlDialog.cpp : implementation file
//

#include "stdafx.h"
#include "afxdialogex.h"
#include "CameraControlDialog.h"


// CameraControlDialog dialog

IMPLEMENT_DYNAMIC(CameraControlDialog, CDialogEx)

CameraControlDialog::CameraControlDialog(CWnd* pParent /*=NULL*/)
	: CDialogEx(CameraControlDialog::IDD, pParent)
{

	this->m_mainToolTips = new CToolTipCtrl();
	this->m_mainToolTips->Create(this);
	// Currently no active tool tips used
	this->m_mainToolTips->Activate(true);
}

CameraControlDialog::~CameraControlDialog()
{
	int result = int();
	this->EndDialog(result);
	CDialog::OnClose();
}

void CameraControlDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EXPOSURE_TIME_INPUT, m_initialExposureTimeInput);
	DDX_Control(pDX, IDC_FPS_INPUT, m_FramesPerSecond);
	DDX_Control(pDX, IDC_GMMA_VALUE_INPUT, m_gammaValue);
}


BEGIN_MESSAGE_MAP(CameraControlDialog, CDialogEx)
END_MESSAGE_MAP()


BOOL CameraControlDialog::PreTranslateMessage(MSG* pMsg) {
	if (this->m_mainToolTips != NULL) {
		this->m_mainToolTips->RelayEvent(pMsg);
	}
	return CDialog::PreTranslateMessage(pMsg);
}
// CameraControlDialog message handlers
