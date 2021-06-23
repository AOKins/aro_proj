#pragma once

#include "resource.h"
#include "afxwin.h"

#include <string>
// SLMControlDialog dialog

class SLMControlDialog : public CDialogEx
{
	DECLARE_DYNAMIC(SLMControlDialog)

public:
	SLMControlDialog(CWnd* pParent = NULL);   // standard constructor
	virtual ~SLMControlDialog();
	SLMController* slmCtrl;

// Dialog Data
	enum { IDD = IDD_SLM_CONTROL };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedSlmPwrButton();
	afx_msg void OnBnClickedSetlut();
	afx_msg void OnBnClickedSetwfc();
	CButton m_SlmPwrButton;
	SLMController* getSLMCtrl();
	afx_msg void OnBnClickedMultiSLM();
	afx_msg void OnCbnChangeSlmAll();
	void populateSLMlist();
	bool attemptLUTload(int i, std::string filePath);
	bool attemptWFCload(int i, std::string filePath);
	// Selection List for current SLM to set LUT and WFC files
	// DON'T USE GetCurSel() with this!  Use this->slmSelectionID_
	CComboBox slmSelection_;
	// Current selection according to combo box, use this if you want current selection rather than GetCurSel
	// Default in construction to value 0 (first item)
	int slmSelectionID_;
	// If true, then running optimization with dual SLM configuration
	CButton multiEnable;
	// If TRUE then treat all the SLMs as the same rather than distinct
	CButton SLM_SetALLSame_;
	afx_msg void OnCbnSelchangeSlmSelect();
	afx_msg void OnBnClickedCompensatePhaseCheckbox();
	CButton dualEnable;
	afx_msg void OnBnClickedSlmDual();
};
