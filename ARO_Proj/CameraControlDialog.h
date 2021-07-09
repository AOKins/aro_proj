#pragma once

#include "afxwin.h"
#include "resource.h"

// CameraControlDialog dialog
class CameraControlDialog : public CDialogEx {
	DECLARE_DYNAMIC(CameraControlDialog)

public:
	CameraControlDialog(CWnd* pParent = NULL);   // standard constructor
	virtual ~CameraControlDialog();

// Dialog Data
	enum { IDD = IDD_CAMERA_CONTROL };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	CEdit m_initialExposureTimeInput;
	CEdit m_FramesPerSecond;
	CEdit m_gammaValue;
};
