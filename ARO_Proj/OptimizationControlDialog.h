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
};
