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
	this->shutdownCamera();
}



// Call all configuration and setups to make sure it is ready before starting
bool CameraController::setupCamera() {
	if (this->camera_ == NULL) {
		UpdateConnectedCameraInfo();
	}
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
	return false; 
}

// Start acquisition
bool CameraController::startCamera() {
	
	// Begin acquisition management when camera has been configured and is now ready to being acquiring images.
	
	return true;
}

// Get most recent image
ImageController* CameraController::AcquireImage() {
	
	// Get most recent image and return within ImageController class

	PicamAvailableData curImageData;
	PicamAcquisitionErrorsMask errors;

	// Get the frame size (size of image data itself in bytes)
	piint frame_size = 0;
	Picam_GetParameterIntegerValue(this->camera_, PicamParameter_FrameSize, &frame_size);
	int num_pixels = frame_size / 2; // Number of pixels is half the size in bytes (16 bit depth for each pixel)
	// Grab an image
	if (Picam_Acquire(this->camera_, 1, -1, &curImageData, &errors) != PicamError_None) {
		Utility::printLine("ERROR: Failed to acquire data from camera!");
		return NULL;
	}

	// Copy data into ImageController, but be sure to convert from 2 byte elements to 1 byte
					// Casting pointer as type short (2 byte elements)
	return new ImageController((short*)curImageData.initial_readout, num_pixels, this->cameraImageWidth, this->cameraImageHeight);
}


bool CameraController::stopCamera() {

	// End acquisition management but still need to have camera online for another run if needed

	return true;
}


// [UTILITY]
int CameraController::PrintDeviceInfo() {
	// TODO



	return 0;
}


bool CameraController::shutdownCamera() {
	Utility::printLine("INFO: Beginning to shutdown camera!");
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
	Utility::printLine("INFO: Finished shutting down camera!");
	
	return true;
}

// [SETUP]
// Update parameters according to GUI
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

// Set ROI parameters, image format, etc.
bool CameraController::ConfigureCustomImageSettings() {
	//XY factor check (axes offsets need to be certain factor to be valid see above)
	if (this->x0 % 4 != 0) {
		this->x0 -= this->x0 % 4;
	}
	if (this->y0 % 2 != 0) {
		this->y0 -= this->y0 % 2;
	}
	
	// Set Pixel format to smallest available size (which is 2 bytes in size, will need to descale when getting images to 1 byte)
	if (Picam_SetParameterIntegerValue(this->camera_, PicamParameter_PixelFormat, PicamPixelFormat_Monochrome16Bit) != PicamError_None) {
		Utility::printLine("ERROR: Failed to set pixel format to mono 16 bit!");
		return false;
	}
	// ROI according to GUI
	const PicamRois* region;
	/* Get the orinal ROI */
	if (Picam_GetParameterRoisValue(this->camera_, PicamParameter_Rois, &region) != PicamError_None) {
		Utility::printLine("ERROR: Failed to receive region");
		return false;
	}
	if (region->roi_count == 1) {
		region->roi_array[0].height = this->cameraImageHeight;
		region->roi_array[0].width = this->cameraImageWidth;
		region->roi_array[0].x = this->x0;
		region->roi_array[0].y = this->y0;

		region->roi_array[0].x_binning = 1;
		region->roi_array[0].y_binning = 1;
	}

	if (Picam_SetParameterRoisValue(this->camera_, PicamParameter_Rois, region) != PicamError_None) {
		Utility::printLine("ERROR: Failed to set ROI");
		return false;
	}
	

	if (PrintDeviceInfo() == -1) {
		Utility::printLine("WARNING: Couldn't display camera information!");
	}

	// TODO: Figure out these two parameters
	// Set gamma
		// It may appear that the equivalent to Gamma


	// Set FPS
		// Unsure if there is a comparable setting for PICam

	return true;
}

// Method of saving an image to a given file path
bool CameraController::saveImage(ImageController * curImage, std::string path) {
	if (curImage == NULL || curImage == nullptr) {
		Utility::printLine("ERROR: Attempted to save image with null pointer!");
		return false;
	}
	curImage->saveImage(path);
	return true;

}

// [ACCESSOR(S)/MUTATOR(S)]
bool CameraController::GetCenter(int &x, int &y) {
	int fullWidth = -1;
	int fullHeight = -1;

	if (!GetFullImage(fullWidth, fullHeight))
	{
		Utility::printLine("ERROR: failed to get max dimensions for getting center values");
		return false;
	}

	x = fullWidth / 2;
	y = fullHeight / 2;
	return true;
}

bool CameraController::GetFullImage(int &x, int &y) {
	// Get the ROI constraints
	const PicamRoisConstraint * constraint;

	if (Picam_GetParameterRoisConstraint(this->camera_, PicamParameter_Rois, PicamConstraintCategory_Required, &constraint) != PicamError_None) {
		Utility::printLine("ERROR: Failed to get ROI constraints!");
		return false;
	}
	// Set what the max dimensions are
	x = int(constraint->width_constraint.maximum);
	y = int(constraint->height_constraint.maximum);

	/* Clean up constraints after using them */
	Picam_DestroyRoisConstraints(constraint);
	return true;
}

// Return true if this controller has access to at least one camera
bool CameraController::hasCameras() {
	pibln connected;
	// Get if the camera is connected
	Picam_IsCameraConnected(this->camera_, &connected);
	return connected != 0;
}

// Exposure settings //

// Setter for exposure setting
bool CameraController::SetExposure(double exposureTimeToSet) {
	if (Picam_SetParameterFloatingPointValue(this->camera_, PicamParameter_ExposureTime, exposureTimeToSet) != PicamError_None) {
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
