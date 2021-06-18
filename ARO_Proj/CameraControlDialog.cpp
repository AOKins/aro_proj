// CameraControlDialog.cpp : implementation file
//

#include "stdafx.h"
#include "CameraControlDialog.h"
#include "afxdialogex.h"


// CameraControlDialog dialog

IMPLEMENT_DYNAMIC(CameraControlDialog, CDialogEx)

CameraControlDialog::CameraControlDialog(CWnd* pParent /*=NULL*/)
	: CDialogEx(CameraControlDialog::IDD, pParent) { }

CameraControlDialog::~CameraControlDialog() {}

void CameraControlDialog::DoDataExchange(CDataExchange* pDX) {
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EXPOSURE_TIME_INPUT, m_initialExposureTimeInput);
	DDX_Control(pDX, IDC_FPS_INPUT, m_FramesPerSecond);
	DDX_Control(pDX, IDC_GMMA_VALUE_INPUT, m_gammaValue);
}

BEGIN_MESSAGE_MAP(CameraControlDialog, CDialogEx)
END_MESSAGE_MAP()

// CameraControlDialog message handlers
