#ifndef C_ABOUT_DIALOG_
#define C_ABOUT_DIALOG_

#include "stdafx.h"				// MFC/VS Specific (should be on top)
#include "afxdialogex.h"
#include <stdlib.h>
#include <stdio.h>


// CAboutDlg dialog used for App About
class CAboutDialog : public CDialog
{
public:
	CAboutDialog();

	// Dialog Data
	//#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
	//#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDialog::CAboutDialog() : CDialog(IDD_ABOUTBOX)
{
}

void CAboutDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDialog, CDialog)
END_MESSAGE_MAP()

#endif