// Implementation file for PICam version of CameraController

#include "stdafx.h"				// Required in source
#include "CameraControllerPICam.h"	// Header file
#include "MainDialog.h"
#include "Utility.h"

CameraController::CameraController(MainDialog* dlg_) {
	this->dlg = dlg_;
	if (!Picam_IsLibraryInitialized()) {
		Picam_InitializeLibrary();
	}
}

CameraController::~CameraController() {
	if (Picam_IsLibraryInitialized()) {
		Picam_UninitializeLibrary();
	}
}
