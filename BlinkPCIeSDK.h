
// BlinkPCIeSDK.h : main header file for the ARO Project application 
// here we inherit from the base class CWinApp to begin with a 
// Windows App template

#include "BlinkPCIeSDKDlg.h"

class CBlinkPCIeSDKApp : public CWinApp
{
public:
	// [CONSTRUCTOR(S)]
	CBlinkPCIeSDKApp();

	// [INITIALIZER]
	virtual BOOL InitInstance();

	DECLARE_MESSAGE_MAP()
};

// Commit to singleton design
extern CBlinkPCIeSDKApp theApp;
