// OptimizationControlDialog.cpp : implementation file
//

#include "../headers/stdafx.h"
#include "../headers/OptimizationControlDialog.h"
#include "../headers/afxdialogex.h"


// OptimizationControlDialog dialog

IMPLEMENT_DYNAMIC(OptimizationControlDialog, CDialogEx)

OptimizationControlDialog::OptimizationControlDialog(CWnd* pParent /*=NULL*/)
	: CDialogEx(OptimizationControlDialog::IDD, pParent)
{

}

OptimizationControlDialog::~OptimizationControlDialog()
{
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
	//DDX_Control(pDX, IDC_SAMPLE_CHECKMARK, m_SampleCheckmark);
}


BEGIN_MESSAGE_MAP(OptimizationControlDialog, CDialogEx)
END_MESSAGE_MAP()

// OptimizationControlDialog message handlers
