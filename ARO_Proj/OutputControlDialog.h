#pragma once

#include "afxwin.h"
#include "resource.h"

// OutputControlDialog dialog
class OutputControlDialog : public CDialogEx {
	DECLARE_DYNAMIC(OutputControlDialog)
public:
	OutputControlDialog(CWnd* pParent = NULL);   // standard constructor
	virtual ~OutputControlDialog();

// Dialog Data
	enum { IDD = IDD_OUTPUT_CONTROL };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	// If checked, then when running the optimization produce a live camera display
	CButton m_displayCameraCheck;
	// If checked, set to display SLMs
	CButton m_displaySLM;
	// If checked, output various logging files to record performance of the optimization
	CButton m_logAllFilesCheck;
	// If checked, the optimization should save the images of elite individual during optimization
	CButton m_SaveEliteImagesCheck;
	// If checked, will output various timing info in timeVSfitness file
	CButton m_SaveTimeVFitnessCheck;
	// If checked, outputs to a file whenever exposure is shortened during optimization
	CButton m_SaveExposureShortCheck;
	// If checked, save the resulting optimized image (pretty much should always be true)
	CButton m_SaveFinalImagesCheck;
	// Gives where the user wants to store the output contents, expected to be a path to a usable folder (not a file!)
	CEdit m_OutputLocationField;

	afx_msg void OnBnClickedOutputLocationButton();
	afx_msg void OnBnClickedLogallFiles();
};
