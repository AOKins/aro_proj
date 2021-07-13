// OptimizationControlDialog.cpp : implementation file
//

#include "stdafx.h"
#include "OptimizationControlDialog.h"
#include "afxdialogex.h"


// OptimizationControlDialog dialog

IMPLEMENT_DYNAMIC(OptimizationControlDialog, CDialogEx)

OptimizationControlDialog::OptimizationControlDialog(CWnd* pParent /*=NULL*/)
	: CDialogEx(OptimizationControlDialog::IDD, pParent)
{
	this->m_mainToolTips = new CToolTipCtrl();
}

BOOL OptimizationControlDialog::OnInitDialog() {
	this->m_mainToolTips->Create(this);
	// Currently no active tool tips used
	this->m_mainToolTips->Activate(true);
	return CDialogEx::OnInitDialog();
}

OptimizationControlDialog::~OptimizationControlDialog()
{
	delete this->m_mainToolTips;
}

void OptimizationControlDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_MIN_FITNESS_INPUT, m_minFitness);
	DDX_Control(pDX, IDC_MIN_SECONDS_INPUT, m_minSeconds);
	DDX_Control(pDX, IDC_MIN_GENERTIONS_INPUT, m_minGenerations);
	DDX_Control(pDX, IDC_EDIT_BIN_SIZE, m_binSize);
	DDX_Control(pDX, IDC_EDIT_NUMBER_BINS, m_numberBins);
	DDX_Control(pDX, IDC_EDIT_TARGET_RADIUS, m_targetRadius);
	DDX_Control(pDX, IDC_MAX_GENERATIONS, m_maxGenerations);
	DDX_Control(pDX, IDC_MAX_SEC_INPUT, m_maxSeconds);
}


BOOL OptimizationControlDialog::PreTranslateMessage(MSG* pMsg) {
	if (this->m_mainToolTips != NULL) {
		this->m_mainToolTips->RelayEvent(pMsg);
	}
	return CDialog::PreTranslateMessage(pMsg);
}


BEGIN_MESSAGE_MAP(OptimizationControlDialog, CDialogEx)
END_MESSAGE_MAP()

// OptimizationControlDialog message handlers
