#pragma once

#include "resource.h"
#include "afxwin.h"

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
};