////////////////////
// CameraControllerPICam.cpp - Implementation of CameraController using PICam SDK
// Last edited: 07/21/2021 by Andrew O'Kins
////////////////////

#include "stdafx.h"				// Required in source
#include "CameraController.h"	// Header file (also will define if using PICam or Spinnaker)
#ifdef USE_PICAM // Only include this implementation content if using PICam

#include "MainDialog.h"
#include "Utility.h"

CameraController::CameraController(MainDialog* dlg_) {
	this->dlg = dlg_;
	
	this->UpdateConnectedCameraInfo();
}

CameraController::~CameraController() {
	// Clear out camera
	pibln connected;

	// Release if already connected to a camera
	Picam_IsCameraConnected(this->camera_, &connected);
	if (connected) {
		Picam_CloseCamera(this->camera_);
	}

	// Deallocate resources from library
	if (this->libraryInitialized) {
		Picam_UninitializeLibrary();
	}

	delete this->libraryInitialized;
}



// Call all configuration and setups to make sure it is ready before starting
bool CameraController::setupCamera() {
	

	return false; 
}

// Start acquisition
bool CameraController::startCamera() { return false; }

// Get most recent image
ImageController* CameraController::AcquireImage() {
	

	return NULL; 
}


bool CameraController::stopCamera() { return false; }
bool CameraController::shutdownCamera() { return false; }

// [SETUP]
bool CameraController::UpdateImageParameters() {
	bool result = true;
	// Frames per second
	try	{
		CString path("");
		dlg->m_cameraControlDlg.m_FramesPerSecond.GetWindowTextW(path);
		if (path.IsEmpty()) throw new std::exception();
		fps = _tstoi(path);
	}
	catch (...)	{
		Utility::printLine("ERROR: Was unable to parse the frames per second time input field!");
		result = false;
	}
	// Gamma value
	try	{
		CString path("");
		dlg->m_cameraControlDlg.m_gammaValue.GetWindowTextW(path);
		if (path.IsEmpty()) throw new std::exception();
		gamma = _tstof(path);
	}
	catch (...)	{
		Utility::printLine("ERROR: Was unable to parse the gamma input field!");
		result = false;
	}
	// Initial exposure time
	try	{
		CString path("");
		dlg->m_cameraControlDlg.m_initialExposureTimeInput.GetWindowTextW(path);
		if (path.IsEmpty()) throw new std::exception();
		initialExposureTime = _tstof(path);
	}
	catch (...)	{
		Utility::printLine("ERROR: Was unable to parse the initial exposure time input field!");
		result = false;
	}
	// Get all AOI settings
	try	{
		CString path("");
		// Left offset
		dlg->m_aoiControlDlg.m_leftInput.GetWindowTextW(path);
		if (path.IsEmpty()) throw new std::exception();
		x0 = _tstoi(path);
		path = L"";
		// Top Offset
		dlg->m_aoiControlDlg.m_rightInput.GetWindowTextW(path);
		if (path.IsEmpty()) throw new std::exception();
		y0 = _tstoi(path);
		path = L"";
		// Width of AOI
		dlg->m_aoiControlDlg.m_widthInput.GetWindowTextW(path);
		if (path.IsEmpty()) throw new std::exception();
		cameraImageWidth = _tstoi(path);
		path = L"";
		// Hieght of AOI
		dlg->m_aoiControlDlg.m_heightInput.GetWindowTextW(path);
		if (path.IsEmpty()) throw new std::exception();
		cameraImageHeight = _tstoi(path);
	}
	catch (...)	{
		Utility::printLine("ERROR: Was unable to parse AOI settings!");
		result = false;
	}
	// Number of image bins X and Y (ASK: if actually need to be thesame)
	try	{
		CString path("");
		dlg->m_optimizationControlDlg.m_numberBins.GetWindowTextW(path);
		if (path.IsEmpty()) throw new std::exception();
		numberOfBinsX = _tstoi(path);
		numberOfBinsY = numberOfBinsX; // Number of bins in Y direction is equal to in X direction (square)
	}
	catch (...)	{
		Utility::printLine("ERROR: Was unable to parse the number of bins input field!");
		result = false;
	}
	//Size of bins X and Y (ASK: if actually thesame xy? and isn't stating the # of bins already determine size?)
	try	{
		CString path("");
		dlg->m_optimizationControlDlg.m_binSize.GetWindowTextW(path);
		if (path.IsEmpty()) throw new std::exception();
		binSizeX = _tstoi(path);
		binSizeY = binSizeX; // Square shape in size
	}
	catch (...)	{
		Utility::printLine("ERROR: Was unable to parse bin size input feild!");
		result = false;
	}
	// Integration/target radius
	try	{
		CString path("");
		dlg->m_optimizationControlDlg.m_targetRadius.GetWindowTextW(path);
		if (path.IsEmpty()) throw new std::exception();
		targetRadius = _tstoi(path);
	}
	catch (...)	{
		Utility::printLine("ERROR: Was unable to parse integration radius input feild!");
		result = false;
	}

	return result;
}

// Reconnect camera
bool CameraController::UpdateConnectedCameraInfo() {
	// Initialize library if needed
	Picam_IsLibraryInitialized(this->libraryInitialized);
	if (this->libraryInitialized) {
		Picam_InitializeLibrary();
	}
	pibln connected;
	
	// Release if already connected to a camera
	Picam_IsCameraConnected(this->camera_, &connected);
	if (connected) {
		Picam_CloseCamera(this->camera_);
	}

	// Connect this camera to the first available
	Picam_OpenFirstCamera(this->camera_);

	// Check to make sure connected
	Picam_IsCameraConnected(this->camera_, &connected);

	if (this->camera_ == NULL || !connected) {
		Utility::printLine("ERROR: Failed to open camera!");
		return false;
	}

	return true; // No errors!
}

bool CameraController::ConfigureCustomImageSettings() { return false; }


// [UTILITY]
int CameraController::PrintDeviceInfo() { return 0; }

// Method of saving an image to a given file path
bool CameraController::saveImage(ImageController * curImage, std::string path) {
	return false;

}

// [ACCESSOR(S)/MUTATOR(S)]
bool CameraController::GetCenter(int &x, int &y) { return false; }
bool CameraController::GetFullImage(int &x, int &y) { return false; }

// Return true if this controller has access to at least one camera
bool CameraController::hasCameras() {
	pibln connected;
	// Get if the camera is connected
	Picam_IsCameraConnected(this->camera_, &connected);
	return connected; 
}



// Exposure settings //

// Setter for exposure setting
bool CameraController::SetExposure(double exposureTimeToSet) {
	PicamError error;
	error = Picam_SetParameterFloatingPointValue(this->camera_, PicamParameter_ExposureTime, exposureTimeToSet);

	if (error != PicamError_None) {
		Utility::printLine("ERROR: Failed to set exposure parameter!");
		return false;
	}
	return true;
}

// Reset the exposure time back to initial
bool CameraController::ConfigureExposureTime() {
	finalExposureTime = initialExposureTime;

	return SetExposure(finalExposureTime);
}

// Get the multiplier for exposure having been halved
double CameraController::GetExposureRatio() {
	return initialExposureTime / finalExposureTime;
}

// half the current exposure time setting
void CameraController::HalfExposureTime() {
	finalExposureTime /= 2;
	if (!SetExposure(finalExposureTime)) {
		Utility::printLine("ERROR: wasn't able to half the exposure time!");
	}
}

#endif // End of PICam implementation of CameraController
