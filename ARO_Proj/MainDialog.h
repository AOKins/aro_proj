#ifndef MAIN_DIALOG_
#define MAIN_DIALOG_

// [DESCRIPTION]
// MainDialog.h : header file for the main dialog of the program

// [INCUDE FILES]
#include "afxwin.h"
#include "afxdialogex.h"
#include <stdlib.h>
#include <stdio.h>

#include "OptimizationControlDialog.h"
#include "SLMControlDialog.h"
#include "CameraControlDialog.h"
#include "AOIControlDialog.h"
#include "afxcmn.h"

#include <string>

class MainDialog : public CDialog {
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
	// Set the UI to default values
	void setDefaultUI();
	// Set the UI according to given file path
	bool setUIfromFile(std::string filePath);
	// Set a value in the UI according to a given name and value as string (used by setUIfromFile)
	bool setValueByName(std::string varName, std::string varValue);

	// Write current UI values to given file location
	bool saveUItoFile(std::string filePath);

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
	CButton m_uGAButton; // Select uGA button
	CButton m_SGAButton; // Select SGA button
	CButton m_OptButton; // Select OPT5 (BruteForce) button
	CButton m_StartStopButton; // Start selected optimization button (or if opt is running will stop)
	CEdit m_BSEdit;
	CEdit m_NBEdit;
	CEdit m_IREdit;
	CEdit m_MFEdit;
	CEdit m_MSEEdit;
	CEdit m_MFEEdit;
	// Thread property to run the optimization algorithm through once committed to running
	CWinThread* runOptThread;

protected:
	afx_msg void OnSlmPwrButton();
	virtual void OnOK();
	//afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnClose();
public:
	afx_msg void OnBnClickedUgaButton();
	afx_msg void OnBnClickedSgaButton();
	afx_msg void OnBnClickedOptButton();
private:
	CString m_ReadyRunning;
public:
	// Sub dialogs
	OptimizationControlDialog m_optimizationControlDlg;
	SLMControlDialog m_slmControlDlg;
	CameraControlDialog m_cameraControlDlg;
	AOIControlDialog m_aoiControlDlg;

	CWnd* m_pwndShow;
	CTabCtrl m_TabControl;
	afx_msg void OnTcnSelchangeTab1(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBnClickedStartStopButton();
	
	bool stopFlag = false;
	bool running_optimization_;
	bool opt_success;
	// Enumeration for type of optimizations to select
	enum OptType {
		OPT5,
		SGA,
		uGA
	};
	OptType opt_selection_; // Current selected optimization algorithm
	// Handle process of loading settings, requesting file to select and attempt load
	afx_msg void OnBnClickedLoadSettings();
	// Handle process of saving settings, requesting file path/name to save to
	afx_msg void OnBnClickedSaveSettings();
};

// Worker thread process for running optimization while MainDialog continues listening for other input
// Input: instance - pointer to MainDialog instance that called this method (will be cast to MainDialgo*)
// Output: optimization according to dlg.opt_selection_ is performed
UINT __cdecl optThreadMethod(LPVOID instance);

#endif
