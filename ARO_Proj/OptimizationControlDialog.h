#pragma once

#include "resource.h"
#include "afxwin.h"

// OptimizationControlDialog dialog

class OptimizationControlDialog : public CDialogEx
{
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
	CButton m_SampleCheckmark; // Appears to currently (June 9th 2021) to be unused, getCheck() commented out in initDialog of mainDialog()
};
