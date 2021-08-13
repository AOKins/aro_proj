////////////////////
// CameraControllerPICam.cpp - implementation of CameraController using PICam SDK
// Last edited: 08/13/2021 by Andrew O'Kins
////////////////////

#include "stdafx.h"				// Required in source
#include "CameraController.h"	// Header file (also will define if using PICam or Spinnaker)

#include "MainDialog.h"
#include "Utility.h"

CameraController::CameraController(MainDialog* dlg_) {
	this->dlg = dlg_;
	
	this->UpdateConnectedCameraInfo();
}

CameraController::~CameraController() {
	this->shutdownCamera();
}

// Call all configuration and setups to make sure it is ready before starting
bool CameraController::setupCamera() {

	// Quit if don't have a reference to UI (latest parameters)
	if (!dlg) {
		return false;
	}
	// Update image parameters according to dialog inputs
	if (!UpdateImageParameters()) {
		return false;
	}

	if (!ConfigureCustomImageSettings()) {
		return false;
	}

	if (!ConfigureExposureTime()) {
		return false;
	}
	Utility::printLine("INFO: Camera has been setup!");
	return true;
}

// Start acquisition process
bool CameraController::startCamera() {
	return true;
}

// Get most recent image
ImageController* CameraController::AcquireImage() {

	return new ImageController(nullptr, this->cameraImageWidth*this->cameraImageHeight, this->cameraImageWidth, this->cameraImageHeight);
}

// Stop acquisition process (but still holds camera instance and other resources)
bool CameraController::stopCamera() {
	return true;
}


// [UTILITY]
int CameraController::PrintDeviceInfo() {

	return 0;
}


bool CameraController::shutdownCamera() {
	
	return true;
}

// [SETUP]
// Update parameters according to GUI
bool CameraController::UpdateImageParameters() {

	return true;
}

// Reconnect camera
bool CameraController::UpdateConnectedCameraInfo() {
	
	return true; // No errors and now connected to camera!
}

// Set ROI parameters, image format, etc.
bool CameraController::ConfigureCustomImageSettings() {
	
	return true;
}

// Method of saving an image to a given file path
bool CameraController::saveImage(ImageController * curImage, std::string path) {
	
	return true;
}

// [ACCESSOR(S)/MUTATOR(S)]
bool CameraController::GetCenter(int &x, int &y) {
	x = 64;
	y = 64;
	return true;
}

bool CameraController::GetFullImage(int &x, int &y) {
	x = 128;
		y = 128;
	return true;
}

// Return true if this controller has access to at least one camera
bool CameraController::hasCameras() {

	return true;
}

// Exposure settings //

// Setter for exposure setting
// Input: exposureTimeToSet - time to set in microseconds
bool CameraController::SetExposure(double exposureTimeToSet) {
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

