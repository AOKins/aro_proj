// AOIControlDialog.cpp : implementation file
//

#include "stdafx.h"
#include "afxdialogex.h"
#include "CameraController.h"
#include "AOIControlDialog.h"
#include "Utility.h"

#include <string>

// AOIControlDialog dialog
IMPLEMENT_DYNAMIC(AOIControlDialog, CDialogEx)

AOIControlDialog::AOIControlDialog(CWnd* pParent /*=NULL*/)
	: CDialogEx(AOIControlDialog::IDD, pParent)
{
}

AOIControlDialog::~AOIControlDialog()
{
}

void AOIControlDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_AOI_LEFT_INPUT, m_leftInput);
	DDX_Control(pDX, IDC_AOI_RIGHT_INPUT, m_rightInput);
	DDX_Control(pDX, IDC_AOI_WIDTH_INPUT, m_widthInput);
	DDX_Control(pDX, IDC_AOI_HEIGHT_INPUT, m_heightInput);
	DDX_Control(pDX, IDC_MAX_IMAGE_SIZE_BUTTON, m_maxImageSizeBtn);
	DDX_Control(pDX, IDC_CENTER_AOI_BUTTON, m_centerAOIBtn);
}


BEGIN_MESSAGE_MAP(AOIControlDialog, CDialogEx)
//	ON_BN_CLICKED(IDC_MAX_IMAGE_SIZE_BUTTON, &AOIControlDialog::OnBnClickedMaxImageSizeButton)
	ON_BN_CLICKED(IDC_CENTER_AOI_BUTTON, &AOIControlDialog::OnBnClickedCenterAoiButton)
//	ON_BN_DOUBLECLICKED(IDC_MAX_IMAGE_SIZE_BUTTON, &AOIControlDialog::OnBnDoubleclickedMaxImageSizeButton)
//	ON_BN_CLICKED(IDC_MAX_IMAGE_SIZE_BUTTON, &AOIControlDialog::OnBnClickedMaxImageSizeButton)
ON_BN_CLICKED(IDC_MAX_IMAGE_SIZE_BUTTON, &AOIControlDialog::OnBnClickedMaxImageSizeButton)
END_MESSAGE_MAP()


// AOIControlDialog message handlers
void AOIControlDialog::OnBnClickedCenterAoiButton()
{
	Utility::printLine("INFO: starting to set AOI to center;");

	int centerX;
	int centerY;

	if (this->cc != nullptr) {
		if (!cc->GetCenter(centerX, centerY))
			Utility::printLine("ERROR: Cannot retrieve the center information for camera controller!");
	}
	else
		Utility::printLine("ERROR: Cannot find the camera controller to center the AOI settings!");

	int maxWidth;
	int maxHeight;
	if (!cc->GetFullImage(maxWidth, maxHeight))
		Utility::printLine("ERROR: Cannot retrieve the max image information from camera controller!");

	//Get Current width and height from the input feilds
	int curWidth;
	int curHeight;
	try	{
		CString path("");
		m_widthInput.GetWindowTextW(path);
		if (path.IsEmpty()) throw new std::exception();
		curWidth = _tstoi(path);

		m_heightInput.GetWindowTextW(path);
		if (path.IsEmpty()) throw new std::exception();
		curHeight = _tstoi(path);
	}
	catch (...)	{
		Utility::printLine("ERROR: Cannot retrieve current width and height when centering AOI");
	}

	//Calculate the offset x and y based on current width and height
	int finalX = 0;
	int finalY = 0;
	int finalWidth = maxWidth;
	int finalHeight = maxHeight;
	if (curWidth < maxWidth && curWidth >= 0) {
		int halfWidth = curWidth / 2;
		finalX = centerX - curWidth;
		finalWidth = curWidth;
	}
	if (curHeight < maxHeight && curHeight >= 0) {
		int halfHeight = curHeight / 2;
		finalY = centerY - halfHeight;
		finalHeight = curHeight;
	}

	//Set all of the final AOI values to respective feilds
	SetAOIFeilds(finalX, finalY, finalWidth, finalHeight);
}

//[ACCESSOR(S)/MUTATORS]
void AOIControlDialog::SetCameraController(CameraController* cc)
{
	this->cc = cc;
}

void AOIControlDialog::SetAOIFeilds(int x, int y, int width, int height)
{
	std::string xStr = std::to_string(x);
	std::string yStr = std::to_string(y);
	std::string wStr = std::to_string(width);
	std::string hStr = std::to_string(height);

	std::wstring xStrW(xStr.begin(), xStr.end());
	std::wstring yStrW(yStr.begin(), yStr.end());
	std::wstring wStrW(wStr.begin(), wStr.end());
	std::wstring hStrW(hStr.begin(), hStr.end());

	m_leftInput.SetWindowTextW(xStrW.c_str());
	m_rightInput.SetWindowTextW(yStrW.c_str());
	m_widthInput.SetWindowTextW(wStrW.c_str());
	m_heightInput.SetWindowTextW(hStrW.c_str());
}

void AOIControlDialog::OnBnClickedMaxImageSizeButton()
{
	int finalWidth;
	int finalHeight;

	if (cc != nullptr) {
		if (!cc->GetFullImage(finalWidth, finalHeight)) {
			Utility::printLine("ERROR: Cannot retrieve the max image information from camera controller!");
		}
	}
	else {
		Utility::printLine("ERROR: Cannot find the camera controller to center the AOI settings!");
	}

	SetAOIFeilds(0, 0, finalWidth, finalHeight);
}
