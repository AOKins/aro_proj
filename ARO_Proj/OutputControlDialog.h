#pragma once
#include "afxwin.h"


// OutputControlDialog dialog

class OutputControlDialog : public CDialogEx
{
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
	// If checked, the optimization should save the images
	CButton m_SaveImagesCheck;
	// If checked, then when running the optimization produce a live camera display
	CButton m_displayCameraCheck;
	// If checked, set to display SLMs
	CButton m_displaySLM;
	// If checked, output various logging files to record performance of the optimization
	CButton m_logFilesCheck;
};
