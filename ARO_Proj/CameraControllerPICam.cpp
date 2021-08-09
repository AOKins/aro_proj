////////////////////
// CameraControllerPICam.cpp - implementation of CameraController using PICam SDK
// Last edited: 07/21/2021 by Andrew O'Kins
////////////////////

#include "stdafx.h"				// Required in source
#include "CameraController.h"	// Header file (also will define if using PICam or Spinnaker)

#ifdef USE_PICAM // Only include this implementation content if using PICam

#include "MainDialog.h"
#include "Utility.h"

CameraController::CameraController(MainDialog* dlg_) {
	this->dlg = dlg_;
	this->libraryInitialized = false;
	this->buffer = NULL;

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
	return true; 
}

// Start acquisition process
bool CameraController::startCamera() {
	
	// Begin acquisition management when camera has been configured and is now ready to being acquiring images.

	/*	// Setting readout to 0 for indefinite acquisition
	if (Picam_SetParameterIntegerValue(this->camera_, PicamParameter_ReadoutCount, 0) != PicamError_None) {
		Utility::printLine("ERROR: Failed to set readout to continuous!");
		return false;
	}
	*/


	/*
	Picam_StartAcquisition(this->camera_);
	*/

	return true;
}

// Get most recent image
ImageController* CameraController::AcquireImage() {
	
	// Get most recent image and return within ImageController class

	PicamAvailableData curImageData;
	PicamAcquisitionErrorsMask errors;
	PicamError result;
	// Get the frame (size of image data itself in bytes) and readout (image and meta data)
	piint frame_size = 0;
	Picam_GetParameterIntegerValue(this->camera_, PicamParameter_FrameSize, &frame_size);
	piint readout_size = 0;
	Picam_GetParameterIntegerValue(this->camera_, PicamParameter_ReadoutStride, &readout_size);

	int num_pixels = frame_size / 2; // Number of pixels is half the size in bytes (2-byte depth for each pixel)

	// Grab an image
		// Simple, none-advanced way that I think is also slow
	result = Picam_Acquire(this->camera_, 1, -1, &curImageData, &errors);

		// Attempt for acquisiton that is using (hopefully faster) asynchronous
	// PicamAcquisitionStatus curr_status;
	// result = Picam_WaitForAcquisitionUpdate(this->camera_, -1, &curImageData, &curr_status);

	if (result != PicamError_None) {
		Utility::printLine("ERROR: Failed to acquire data from camera!");
		return NULL;
	}

	// Getting a pointer to the most recent frame (our most recent image data) by skipping older readouts (simple method should have readout_count == 1)
	void * curr_frame = curImageData.initial_readout + readout_size*(curImageData.readout_count - 1);

	// Copy data into ImageController, but be sure to convert from 2 byte elements to 1 byte
	// Casting pointer as type unsigned short (2 byte elements)
	return new ImageController((unsigned short*)curr_frame, num_pixels, this->cameraImageWidth, this->cameraImageHeight);
}

// Stop acquisition process (but still holds camera instance and other resources)
bool CameraController::stopCamera() {

	// End acquisition management but still need to have camera online for another run if needed
//	Picam_StopAcquisition(this->camera_);
	
	/*
	Have to iterate through wait acquisition update until the running bool is false!


	*/

	return true;
}


// [UTILITY]
int CameraController::PrintDeviceInfo() {
	// TODO
	Utility::printLine();
	Utility::printLine("*** CAMERA INFORMATION ***");

	
	// Display Exposure time and calculated Framerate
	Utility::printLine("Exposure time: " + std::to_string(getFloatParameterValue(PicamParameter_ExposureTime)) + " milliseconds");
	Utility::printLine("Calculated framerate: " + std::to_string(getFloatParameterValue(PicamParameter_FrameRateCalculation)) + " frames per second");

	return 0;
}

piint CameraController::getIntParameterValue(PicamParameter parameter) {
	piint value;
	PicamError errmsg = Picam_GetParameterIntegerValue(this->camera_, parameter, &value);
	if (errmsg == PicamError_None) {
		return value;
	}
	else {
		Utility::printLine("WARNING: Failed to get an integer parameter! Error code: " + errmsg);
		return 0;
	}
}

piflt CameraController::getFloatParameterValue(PicamParameter parameter) {
	piflt value;
	PicamError errmsg = Picam_GetParameterFloatingPointValue(this->camera_, parameter, &value);
	if (errmsg == PicamError_None) {
		return value;
	}
	else {
		Utility::printLine("WARNING: Failed to get an integer parameter! Error code: " + errmsg);
		return 0;
	}
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
	PicamError err;
	// Initialize library if needed
	Picam_IsLibraryInitialized(this->libraryInitialized);
	if (!this->libraryInitialized) {
		Picam_InitializeLibrary();
	}

	// Release if already connected to a camera
	pibln connected;
	err = Picam_IsCameraConnected(&this->camera_, &connected);
	if (connected) {
		Picam_CloseCamera(this->camera_);
	}

	// Connect this camera to the first available
	if (Picam_OpenFirstCamera(&this->camera_) == PicamError_None) {
		Utility::printLine("INFO: Connected to first PICam camera");
	}
	else {
		// If no connected camera, will create a demo camera for the purposes of demonstrating the program and debugging
		// Demo setup is based on provided PICam example within its acquire.cpp source file
		Utility::printLine("WARNING: Failed to connect to a camera, creating demo camera to demonstrate program");
		
		PicamCameraID id;
		PicamError demoConnectErr = Picam_ConnectDemoCamera(PicamModel_Pixis100B, "12345", &id);
		if (demoConnectErr == PicamError_None) {
			Picam_OpenCamera(&id, &this->camera_); // connecting to demo camera
			Utility::printLine("INFO: Current demo using model Pixis100B, serial number 12345");
		}
		else {
			std::string errMsg = "ERROR: " + std::to_string(demoConnectErr);
			Utility::printLine(errMsg);
		}
	}

	// Check to make sure connected
	Picam_IsCameraConnected(this->camera_, &connected);

	if (!connected) {
		Utility::printLine("ERROR: Failed to open camera!");
		return false;
	}
	return true; // No errors and now connected to camera!
}

// Set ROI parameters, image format, etc.
bool CameraController::ConfigureCustomImageSettings() {
	// Set Pixel format to smallest available size (which is 2 bytes in size, will need to descale when getting images to 1 byte)
	if (Picam_SetParameterIntegerValue(this->camera_, PicamParameter_PixelFormat, PicamPixelFormat_Monochrome16Bit) != PicamError_None) {
		Utility::printLine("ERROR: Failed to set pixel format to mono 16 bit!");
		return false;
	}
	// Read one full frame at a time
	if (Picam_SetParameterIntegerValue(this->camera_, PicamParameter_ReadoutControlMode, PicamReadoutControlMode_FullFrame) != PicamError_None) {
		Utility::printLine("ERROR: Failed to set readout control mode to full frame!");
		return false;
	}



	// ROI according to GUI, implementation approach based on PICam example rois.cpp //

	//XY factor check (axes offsets need to be certain factor to be valid see above)
	if (this->x0 % 4 != 0) {
		this->x0 -= this->x0 % 4;
	}
	if (this->y0 % 2 != 0) {
		this->y0 -= this->y0 % 2;
	}

	// Checking if the GUI setup is invalid by checking against max dimensions and if our ROI is going out of bounds
	int x_max, y_max;
	this->GetFullImage(x_max, y_max);
	if (this->x0 + this->cameraImageWidth > x_max || this->y0 + this->cameraImageHeight > y_max) {
		Utility::printLine("ERROR: Set ROI exceeds the camera constraints!  Defaulting to entire camera image");
		this->x0 = 0;
		this->y0 = 0;
		this->cameraImageWidth = x_max;
		this->cameraImageHeight = y_max;
	}

	/* Get the orinal ROI */
	const PicamRois* region;
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

	PicamError errMsg = Picam_SetParameterRoisValue(this->camera_, PicamParameter_Rois, region);

	if (errMsg != PicamError_None) {
		Utility::printLine("ERROR: Failed to set ROI");
		return false;
	}
	
	// Setting initial exposure //
	this->ConfigureExposureTime();

	// TODO: Figure out these two parameters
	// Set gamma
		// Unsure of comparablse setting for PICam


	// Set FPS
		// Unsure if there is a comparable setting for PICam

	const PicamParameter* failed_parameters;
	piint failed_parameters_count;
	errMsg = Picam_CommitParameters(this->camera_,	&failed_parameters,	&failed_parameters_count);

	// - print any invalid parameters
	if (failed_parameters_count > 0) {
		Utility::printLine("ERROR: invalid following parameters to commit!");
	}

	// - free picam-allocated resources
	Picam_DestroyParameters(failed_parameters);

	// Print resulting Device info
	if (PrintDeviceInfo() == -1) {
		Utility::printLine("WARNING: Couldn't display camera information!");
	}

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
// Input: exposureTimeToSet - time to set in microseconds
bool CameraController::SetExposure(double exposureTimeToSet) {
	// PICam deals with exposure time in milliseconds, so need to divide the input by 1000
	if (Picam_SetParameterFloatingPointValue(this->camera_, PicamParameter_ExposureTime, exposureTimeToSet / 1000) != PicamError_None) {
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
