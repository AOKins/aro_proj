#pragma once

#include "resource.h"
#include "afxwin.h"

class CameraController;

// AOIControlDialog dialog
class AOIControlDialog : public CDialogEx {
	DECLARE_DYNAMIC(AOIControlDialog)

public:
	AOIControlDialog(CWnd* pParent = NULL);   // standard constructor
	virtual ~AOIControlDialog();
	CameraController* cc;

// Dialog Data
	enum { IDD = IDD_AOI_CONTROL };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	CEdit m_leftInput;
	CEdit m_rightInput;
	CEdit m_widthInput;
	CEdit m_heightInput;

	afx_msg void OnBnClickedCenterAoiButton();
	CButton m_maxImageSizeBtn;
	CButton m_centerAOIBtn;

	void SetCameraController(CameraController* cc);

	void SetAOIFeilds(int x, int y, int width, int height);
	afx_msg void OnBnClickedMaxImageSizeButton();

	// Tool tips to help inform the user about a control
	CToolTipCtrl * m_mainToolTips;
	BOOL virtual PreTranslateMessage(MSG* pMsg);
};
