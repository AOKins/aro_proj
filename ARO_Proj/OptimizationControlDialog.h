#pragma once

#include "resource.h"
#include "afxwin.h"

// OptimizationControlDialog dialog
class OptimizationControlDialog : public CDialogEx {
	DECLARE_DYNAMIC(OptimizationControlDialog)
public:
	OptimizationControlDialog(CWnd* pParent = NULL);   // standard constructor
	virtual ~OptimizationControlDialog();

// Dialog Data
	enum { IDD = IDD_OPTIMIZATION_CONTROL };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()
public:
	CEdit m_minFitness;
	CEdit m_minSeconds;
	CEdit m_minGenerations;
	CEdit m_binSize;
	CEdit m_numberBins;
	CEdit m_targetRadius;
	// GUI input for maximum number of generations
	CEdit m_maxGenerations;
	// The maximum amount of time (in seconds) to run the optimization algorithm, set to 0 for indefinite time
	CEdit m_maxSeconds;

	// Tool tips to help inform the user about a control
	CToolTipCtrl * m_mainToolTips;
	BOOL virtual PreTranslateMessage(MSG* pMsg);
	// If toggled, will skip the elite individuals that were copied over
	CButton m_skipEliteReevaluation;
};
