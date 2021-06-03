#ifndef MAIN_DIALOG_
#define MAIN_DIALOG_

// [DESCRIPTION]
// MainDialog.h : header file for the main dialog of the program


// [FORWARD DEFINITIONS]
class SLMController;
class CameraController;

// [INCUDE FILES]
//	- Dialog and and IDE
#include "afxwin.h"
#include "afxdialogex.h"
#include <stdlib.h>
#include <stdio.h>
#include "OptimizationControlDialog.h"
#include "SLMControlDialog.h"
#include "CameraControlDialog.h"
#include "AOIControlDialog.h"
#include "afxcmn.h"


class MainDialog : public CDialog
{
public:
	// [GLOBAL PARAMETERS]
	int frameRate = 200; //200 FPS or 200 HZ (valid range 1 - 1000)
	SLMController* slmCtrl;
	CameraController* camCtrl;
	FILE* fp;

	// [CONSTRUCTOR(S)]
	// Standard Constructor
	MainDialog(CWnd* pParent = nullptr);

	// [UI UPDATES]
	void disableMainUI(bool isMainEnabled);

// DIALOG DATA
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_BLINKPCIESDK_DIALOG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support

protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	BOOL m_CompensatePhase;
	CButton m_CompensatePhaseCheckbox;
	CListBox m_ImageListBox;
	CButton m_uGAButton;
	CButton m_SGAButton;
	CButton m_OptButton;
	CEdit m_BSEdit;
	CEdit m_NBEdit;
	CEdit m_IREdit;
	CEdit m_MFEdit;
	CEdit m_MSEEdit;
	CEdit m_MFEEdit;

protected:
	afx_msg void OnSlmPwrButton();
	afx_msg void OnSelchangeImageListbox();
	afx_msg void OnStartStopButton();
	virtual void OnOK();
	//afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnClose();
	afx_msg void OnCompensatePhaseCheckbox();
public:
	afx_msg void OnBnClickedUgaButton();
	afx_msg void OnBnClickedSgaButton();
	afx_msg void OnBnClickedOptButton();
private:
	CString m_ReadyRunning;
public:
	OptimizationControlDialog m_optimizationControlDlg;
	SLMControlDialog m_slmControlDlg;
	CameraControlDialog m_cameraControlDlg;
	AOIControlDialog m_aoiControlDlg;
	CWnd* m_pwndShow;
	CTabCtrl m_TabControl;
	afx_msg void OnTcnSelchangeTab1(NMHDR *pNMHDR, LRESULT *pResult);
};


#endif