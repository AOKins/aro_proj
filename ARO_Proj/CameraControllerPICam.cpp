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
	// Deallocate resources from library
	
	if (this->libraryInitialized) {
		Picam_UninitializeLibrary();
	}
	delete this->libraryInitialized;
	
}


// TODO

bool CameraController::setupCamera() { return false; }
bool CameraController::startCamera() { return false; }
bool CameraController::saveImage(ImageController * curImage, std::string path) { return false; }
ImageController* CameraController::AcquireImage() { return NULL; }
bool CameraController::stopCamera() { return false; }
bool CameraController::shutdownCamera() { return false; }

// [SETUP]
bool CameraController::UpdateImageParameters() { return false; }

bool CameraController::UpdateConnectedCameraInfo() {
	// Initialize library if needed
	Picam_IsLibraryInitialized(this->libraryInitialized);
	if (this->libraryInitialized) {
		Picam_InitializeLibrary();
	}
	// Connect this camera to the first available
	Picam_OpenFirstCamera(this->camera_);

	// Check to make sure connected
	pibln connected;
	Picam_IsCameraConnected(this->camera_, &connected);

	if (this->camera_ == NULL || !connected) {
		Utility::printLine("ERROR: Failed to open camera!");
		return false;
	}

	return true; // No errors!
}



bool CameraController::ConfigureCustomImageSettings() { return false; }
bool CameraController::ConfigureExposureTime() { return false; }

// [UTILITY]
int CameraController::PrintDeviceInfo() { return 0; }

// [ACCESSOR(S)/MUTATOR(S)]
bool CameraController::GetCenter(int &x, int &y) { return false; }
bool CameraController::GetFullImage(int &x, int &y) { return false; }
// Return true if this controller has access to at least one camera
bool CameraController::hasCameras() { return false; }
// Setter for exposure setting
bool CameraController::SetExposure(double exposureTimeToSet) { return 0; }
// Get the multiplier for exposure having been halved
double CameraController::GetExposureRatio() { return 0; }
// half the current exposure time setting
void CameraController::HalfExposureTime() { return; }






#endif // End of PICam implementation of CameraController
