// Implementation file for PICam version of CameraController

#include "stdafx.h"				// Required in source
#include "CameraControllerPICam.h"	// Header file
#include "MainDialog.h"
#include "Utility.h"

CameraController::CameraController(MainDialog* dlg_) {
	this->dlg = dlg_;

	// Initialize library
	Picam_IsLibraryInitialized(this->libraryInitialized);
	if (this->libraryInitialized) {
		Picam_InitializeLibrary();
	}
}

CameraController::~CameraController() {
	// Deallocate resources from library
	if (this->libraryInitialized) {
		Picam_UninitializeLibrary();
	}
	delete this->libraryInitialized;
}
