#pragma once

#include "resource.h"
#include "afxwin.h"

// OutputControlDialog dialog

class OutputControlDialog : public CDialogEx
{
	DECLARE_DYNAMIC(AOIControlDialog)

public:
	OutputControlDialog(CWnd* pParent = NULL);   // standard constructor
	virtual ~OutputControlDialog();

	// Dialog Data
	enum { IDD = IDD_OUTPUT_CONTROL };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:


};
