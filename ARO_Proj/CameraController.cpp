#include "stdafx.h"				// Required in source

#include "CameraController.h"	// Header file
#include "MainDialog.h"
#include "Utility.h"
#include <string>

using std::ostringstream;

// [CONSTRUCTOR(S)]
CameraController::CameraController(MainDialog& dlg_) : dlg(dlg_) {
	//Camera access
	UpdateConnectedCameraInfo();
}

//[DESTRUCTOR]
CameraController::~CameraController() {
	Utility::printLine("INFO: Beginning to shutdown camera!");
	if (isCamCreated) {
		stopCamera();
		shutdownCamera();
	}
	Utility::printLine("INFO: Finished shutting down camera!");
}

// [CAMERA CONTROL]
bool CameraController::setupCamera() {
	Utility::printLine();

	// Quit if don't have a reference to UI (latest parameters)
	if (!dlg)
		return false;
	Utility::printLine("INFO: dlg reference checked!");

	if (!UpdateImageParameters())
		return false;
	Utility::printLine("INFO: updated image parameters!");

	if (!cam->IsValid() || !cam->IsInitialized()) {
		shutdownCamera();
		if (!UpdateConnectedCameraInfo())
			return false;
		Utility::printLine("INFO: updated connected camera info!");
	}
	
	if (!ConfigureCustomImageSettings())
		return false;
	Utility::printLine("INFO: configured image settings on camera!");
	
	if (!ConfigureExposureTime())
		return false;
	Utility::printLine("INFO: configured expousre settting on camera!");

	isCamCreated = true;
	return true;
}

bool CameraController::startCamera() {
	try	{
		INodeMap &nodeMap = cam->GetNodeMap();
		INodeMap &TLnodeMap = cam->GetTLStreamNodeMap();
		// Set acquisition mode to singleframe
		// - retrieve enumerationg node to set
		CEnumerationPtr ptrAcquisitionMode = nodeMap.GetNode("AcquisitionMode"); 
		if (!IsAvailable(ptrAcquisitionMode) || !IsWritable(ptrAcquisitionMode)) {
			Utility::printLine("Unable to set acquisition mode (enum retrieval).");
			return false;
		}
		// - retrieve "continuous" entry node from enumeration node
		CEnumEntryPtr ptrAcquisitionModeType = ptrAcquisitionMode->GetEntryByName("Continuous");
		if (!IsAvailable(ptrAcquisitionModeType) || !IsReadable(ptrAcquisitionModeType)) {
			Utility::printLine("Unable to set acquisition mode (entry retrieval).");
			return false;
		}
		// - retrieve integer value from entry node "continuous"
		int64_t acquisitionModeTypeValue = ptrAcquisitionModeType->GetValue();
		// - set the retrieved integer as the correct node value
		ptrAcquisitionMode->SetIntValue(acquisitionModeTypeValue);

		// Setting buffer handler
		//		Spinnaker defaults to OldestFirst, changed to NewestOnly as we are only interested in current image for individual being run
		CEnumerationPtr ptrSBufferHandler = TLnodeMap.GetNode("StreamBufferHandlingMode");
		if (!IsAvailable(ptrSBufferHandler) || !IsWritable(ptrSBufferHandler)) {
			Utility::printLine("Unable to set acquisition mode (enum retrieval).");
			return false;
		}
		else {
			ptrSBufferHandler->SetIntValue(StreamBufferHandlingMode_NewestOnly);
			Utility::printLine("INFO: Camera buffer set to 'NewestOnly'!");
		}
		
		//Begin Aquisition
		cam->BeginAcquisition();
		Utility::printLine("INFO: Successfully began acquiring images!");
	}
	catch (Spinnaker::Exception &e)	{
		Utility::printLine("ERROR: Camera could not start - /n" + std::string(e.what()));
		return false;
	}

	return true;
}

bool CameraController::stopCamera() {
	//TODO: release any image(s) that are currently used

	cam->EndAcquisition();
	return true;
}

//Releases camera references - have to call setup camera again if need to use camer after this call
bool CameraController::shutdownCamera()
{
	//release camera
	cam->DeInit();

	//release system
	cam = NULL;
	camList.Clear();
	system->ReleaseInstance();
	isCamCreated = false;

	return true;
}

// [ACQUISITION]
/*getImage: high level wrapper that allows to take an image if camera was started
*/
bool CameraController::saveImage(ImagePtr curImage, int curGen) {
	if (!curImage.IsValid()) {
		Utility::printLine("ERROR: Camera Aquisition resulted in a NULL image!");
		return false;
	}

	ostringstream filename;
	filename << "logs/UGA_Gen_" << curGen << "_Elite.jpg";
	curImage->Save(filename.str().c_str());
	Utility::printLine("INFO: Saved elite of generation #" + std::to_string(curGen) + "!");

	Utility::printLine("INFO: saved an image for generation #" + std::to_string(curGen));

	return true;
}

//AcquireImages: get one image from the camera
void CameraController::AcquireImages(ImagePtr& curImage, ImagePtr& convertedImage) {
	Utility::printLine("Aquire Images _ 2", true);

	convertedImage = Image::Create();
	Utility::printLine("Aquire Images _ 3", true);

	try {
		Utility::printLine("Aquire Images _ 5", true);
		// Retrieve next received image
		curImage = cam->GetNextImage();

		// Ensure image completion
		if (curImage->IsIncomplete()) {
			//TODO: implement proper handling of inclomplete images (retake of image)
			Utility::printLine("ERROR: Image incomplete: " + std::string(Image::GetImageStatusDescription(curImage->GetImageStatus())));
		}
		//Print The dimensions of the image (should be XX by YY)
		//Utility::printLine("Current Image Dimensions : " + std::to_string(pResultImage->GetWidth()) + " by " + std::to_string(pResultImage->GetHeight()));

		Utility::printLine("Aquire Images _ 7", true);

		//copy image to pImage pointer
		convertedImage = curImage->Convert(PixelFormat_Mono8); // TODO try see if there is any performance gain if use -> , HQ_LINEAR);

		Utility::printLine("Aquire Images _ 8", true);
	}
	catch (Spinnaker::Exception &e) {
		Utility::printLine("ERROR: " + std::string(e.what()));
	}

	Utility::printLine("#####################################################", true);
}

// [CAMERA SETUP]
bool CameraController::UpdateImageParameters() {
	bool result = true;

	//Frames per second
	try	{
		CString path("");
		dlg.m_cameraControlDlg.m_FramesPerSecond.GetWindowTextW(path);
		if (path.IsEmpty()) throw new std::exception();
		fps = _tstof(path);
		frameRateMS = 1000 / fps;
	}
	catch (...)	{
		Utility::printLine("ERROR: Was unable to parse the frames per second time input feild!");
		result = false;
	}

	//Gamma value
	try	{
		CString path("");
		dlg.m_cameraControlDlg.m_gammaValue.GetWindowTextW(path);
		if (path.IsEmpty()) throw new std::exception();
		gamma = _tstof(path);
	}
	catch (...)	{
		Utility::printLine("ERROR: Was unable to parse the frames per second time input feild!");
		result = false;
	}

	//Initial exposure time
	try	{
		CString path("");
		dlg.m_cameraControlDlg.m_initialExposureTimeInput.GetWindowTextW(path);
		if (path.IsEmpty()) throw new std::exception();
		initialExposureTime = _tstof(path);
	}
	catch (...)	{
		Utility::printLine("ERROR: Was unnable to parse the initial exposure time input feild!");
		result = false;
	}

	//Get all AOI settings
	try	{
		CString path("");
		dlg.m_aoiControlDlg.m_leftInput.GetWindowTextW(path);
		if (path.IsEmpty()) throw new std::exception();
		x0 = _tstoi(path);
		path = L"";
		dlg.m_aoiControlDlg.m_rightInput.GetWindowTextW(path);
		if (path.IsEmpty()) throw new std::exception();
		y0 = _tstoi(path);
		path = L"";
		dlg.m_aoiControlDlg.m_widthInput.GetWindowTextW(path);
		if (path.IsEmpty()) throw new std::exception();
		cameraImageWidth = _tstoi(path);
		path = L"";
		dlg.m_aoiControlDlg.m_heightInput.GetWindowTextW(path);
		if (path.IsEmpty()) throw new std::exception();
		cameraImageHeight = _tstoi(path);
	}
	catch (...)	{
		Utility::printLine("ERROR: Was unnable to parse AOI settings!");
		result = false;
	}

	//Number of image bins X and Y (ASK: if actually need to be thesame)
	try	{
		CString path("");
		dlg.m_optimizationControlDlg.m_numberBins.GetWindowTextW(path);
		if (path.IsEmpty()) throw new std::exception();
		numberOfBinsX = _tstof(path);
	}
	catch (...)	{
		Utility::printLine("ERROR: Was unable to parse the number of bins input feild!");
		result = false;
	}
	numberOfBinsY = numberOfBinsX;

	//Size of bins X and Y (ASK: if actually thesame xy? and isn't stating the # of bins already determine size?)
	try	{
		CString path("");
		dlg.m_optimizationControlDlg.m_binSize.GetWindowTextW(path);
		if (path.IsEmpty()) throw new std::exception();
		binSizeX = _tstof(path);
	}
	catch (...)	{
		Utility::printLine("ERROR: Was unable to parse bin size input feild!");
		result = false;
	}
	binSizeY = binSizeX;

	//Integration/target radius (ASK: what is this for?)
	try	{
		CString path("");
		dlg.m_optimizationControlDlg.m_targetRadius.GetWindowTextW(path);
		if (path.IsEmpty()) throw new std::exception();
		targetRadius = _tstof(path);
	}
	catch (...)	{
		Utility::printLine("ERROR: Was unable to parse integration radius input feild!");
		result = false;
	}

	// Setup target matrix
	delete[] targetMatrix;
	targetMatrix = new int[cameraImageHeight*cameraImageWidth];
	Utility::GenerateTargetMatrix_SinglePoint(targetMatrix, cameraImageWidth, cameraImageHeight, targetRadius);

	return result;
}

//GetConnectedCameraInfo: used to proccess/store data about all currentlyconnected cameras
bool CameraController::UpdateConnectedCameraInfo() {
	try	{
		//Spinaker system object w/ camera list
		system = System::GetInstance();
		camList = system->GetCameras();

		if (system == NULL)	{
			Utility::printLine("ERROR: Camera system not avaliable!");
			return false;
		}
		if (camList.GetSize() == 0)	{
			Utility::printLine("ERROR: No cameras avaliable!");
			return false;
		}
		else
			Utility::printLine("INFO: There are " + std::to_string(camList.GetSize()) + " cameras avaliable!");

		//Check if only one camera
		int camAmount = camList.GetSize();
		if (camAmount != 1)	{
			//clear camera list before releasing system
			camList.Clear();
			system->ReleaseInstance();
			Utility::printLine("ERROR: only 1 camera has to be connected, but you have" + std::to_string(camAmount));
			return false;
		}

		//Get camera reference
		cam = camList.GetByIndex(0);
		if (!cam.IsValid())	{
			Utility::printLine("ERROR: Retrieved Camera not Valid!");
			return false;
		}
		else
			Utility::printLine("INFO: Retrieved Camera is Valid!");

		//Initialize camera
		cam->Init();
		if (!cam->IsInitialized())	{
			Utility::printLine("ERROR: Retrieved Camera could not be initialized!");
			return false;
		}
		else
			Utility::printLine("INFO: Retrieved Camera was initialized!");

		//TODO: determine if need a class reference to nodeMap of cam
		//nodeMap = cam->GetNodeMap();
		//nodeMapTLDevice = cam->GetTLDeviceNodeMap();
	}
	catch (Spinnaker::Exception &e)	{
		Utility::printLine("ERROR: " + std::string(e.what()));
		return false;
	}

	return true;
}

/* ConfigureCustomImageSettings()
 * @return - TRUE if succesful, FALSE if failed */
bool CameraController::ConfigureCustomImageSettings() {
	// REFERENCES: try to look through these if current implementation one malfunctions:
	// 1) http://perk-software.cs.queensu.ca/plus/doc/nightly/dev/vtkPlusSpinnakerVideoSource_8cxx_source.html
	// 2) Useful for getting node info: https://www.flir.com/support-center/iis/machine-vision/application-note/spinnaker-nodes/
	// 3) Use SpinView program to find correct node names and value types 
	
	//Check current camera is valid
	if (!cam->IsValid() || !cam->IsInitialized()) {
		Utility::printLine("ERROR: trying to configure invalid or nonexistant camera");
		return false;
	}

	//Default centering of image AOI
	//int maxHeight = (int)cam->HeightMax.GetValue();
	//int maxWidth = (int)cam->WidthMax.GetValue();
	//// - X axis center
	//if (x0 = -1 && ((maxWidth / 2) - (cameraImageWidth / 2)) < 0)
	//	x0 = 0;
	//else
	//	x0 = (maxWidth / 2) - (cameraImageWidth / 2);
	//// - Y axis center
	//if (y0 == -1 && ((maxHeight / 2) - (cameraImageHeight / 2)) < 0)
	//	y0 = 0;
	//else
	//	y0 = (maxHeight / 2) - (cameraImageHeight / 2);
	
	//XY factor check (axes offsets need to be certain factor to be valid see above)
	if (x0 % 4 != 0)
		x0 -= x0 % 4;
	if (y0 % 2 != 0)
		y0 -= y0 % 2;

	try	{
		//Apply mono 8 pixel format
		if (cam->PixelFormat != NULL && cam->PixelFormat.GetAccessMode() == RW)
			cam->PixelFormat.SetValue(PixelFormat_Mono8);
		else
			Utility::printLine("ERROR: Pixel format not available...");

		//Apply initial zero offset in x direction (needed to minimize AOI errors)
		if (cam->OffsetX != NULL && cam->OffsetX.GetAccessMode() == RW) {
			cam->OffsetX.SetValue(0);
		}
		else {
			Utility::printLine("ERROR: OffsetX not available for initial setup");
		}
		//Apply initial zero offset in y direction (needed to minimize AOI errors)
		if (cam->OffsetY != NULL && cam->OffsetY.GetAccessMode() == RW) {
			cam->OffsetY.SetValue(0);
		}
		else {
			Utility::printLine("ERROR: OffsetY not available for initial setup");
		}
		//Apply target image width
		if (cam->Width != NULL && cam->Width.GetAccessMode() == RW && cam->Width.GetInc() != 0 && cam->Width.GetMax() != 0) {
			cam->Width.SetValue(cameraImageWidth);
		}
		else {
			Utility::printLine("ERROR: Width not available to be set");
		}
		//Apply target image height
		if (cam->Height != NULL && cam->Height.GetAccessMode() == RW && cam->Height.GetInc() != 0 && cam->Height.GetMax() != 0) {
			cam->Height.SetValue(cameraImageHeight);
		}
		else {
			Utility::printLine("ERROR: Height not available");
		}
		//Apply final offset in x direction
		if (cam->OffsetX != NULL && cam->OffsetX.GetAccessMode() == RW) {
			cam->OffsetX.SetValue(x0);
		}
		else {
			Utility::printLine("ERROR: Final OffsetX not available");
		}
		//Apply final offset in y direction
		if (cam->OffsetY != NULL && cam->OffsetY.GetAccessMode() == RW) {
			cam->OffsetY.SetValue(y0);
		}
		else {
			Utility::printLine("ERROR: OffsetY not available");
		}
		INodeMap & nodeMap = cam->GetNodeMap();
		INodeMap & nodeMapTLDevice = cam->GetTLDeviceNodeMap();
		if (PrintDeviceInfo(nodeMapTLDevice) == -1) {
			Utility::printLine("WARNING: Couldn't display camera information!");
		}
		//Enable Manual Frame Rate Setting
		bool setFrameRate = true;
		Spinnaker::GenApi::CBooleanPtr FrameRateEnablePtr = cam->GetNodeMap().GetNode("AcquisitionFrameRateEnabled");
		if (Spinnaker::GenApi::IsAvailable(FrameRateEnablePtr) && Spinnaker::GenApi::IsWritable(FrameRateEnablePtr)) {
			FrameRateEnablePtr->SetValue(true);
			Utility::printLine("INFO: Set Framerate Manual Enble to True!");
		}
		else {
			Utility::printLine("ERROR: Unable to set Frame Rate Enable to True!");
			setFrameRate = false;
		}

		// Disable Auto Frame Rate Control
		if (setFrameRate) {
			// Enumeration node
			CEnumerationPtr ptrFrameAuto = cam->GetNodeMap().GetNode("AcquisitionFrameRateAuto");
			if (Spinnaker::GenApi::IsAvailable(ptrFrameAuto) && Spinnaker::GenApi::IsWritable(ptrFrameAuto))
			{
				//EnumEntry node (always associated with an Enumeration node)
				CEnumEntryPtr ptrFrameAutoOff = ptrFrameAuto->GetEntryByName("Off");
				//Turn off Auto Gain
				ptrFrameAuto->SetIntValue(ptrFrameAutoOff->GetValue());
				if (ptrFrameAuto->GetIntValue() == ptrFrameAutoOff->GetValue())	{
					Utility::printLine("INFO: Set auto acquisition framerate mode to off!");
				}
				else {
					Utility::printLine("WARNING: Auto acquistion framerate mode was not set to 'off'!");
					setFrameRate = false;
				}
			}
			else {
				Utility::printLine("ERROR: Unable to set auto acquisition framerate mode to 'off'!");
				setFrameRate = false;
			}
		}

		//Set the correct framerate given by the user thrugh the UI
		if (setFrameRate) {
			CFloatPtr ptrFrameRateSetting = cam->GetNodeMap().GetNode("AcquisitionFrameRate");
			if (Spinnaker::GenApi::IsAvailable(ptrFrameRateSetting) && Spinnaker::GenApi::IsWritable(ptrFrameRateSetting))	{
				ptrFrameRateSetting->SetValue(fps);
				if (ptrFrameRateSetting->GetValue() == fps)
					Utility::printLine("INFO: Set FPS of camera to " + std::to_string(fps) + "!");
				else
					Utility::printLine("WARNING: Was unable to set the FPS of camera to " + std::to_string(fps) + " it is actually " + std::to_string(ptrFrameRateSetting->GetValue()) + "!");
			}
			else {
				Utility::printLine("ERROR: Unable to set fps node not avaliable!");
			}
		}

		//Enable manual gamma adjustment:
		Spinnaker::GenApi::CBooleanPtr GammaEnablePtr = cam->GetNodeMap().GetNode("GammaEnabled");
		if (Spinnaker::GenApi::IsAvailable(GammaEnablePtr) && Spinnaker::GenApi::IsWritable(GammaEnablePtr)) {
			GammaEnablePtr->SetValue(true);
			Utility::printLine("INFO: Set manual gamma enable to true");

			CFloatPtr ptrGammaSetting = cam->GetNodeMap().GetNode("Gamma");
			if (Spinnaker::GenApi::IsAvailable(ptrGammaSetting) && Spinnaker::GenApi::IsWritable(ptrGammaSetting))	{
				ptrGammaSetting->SetValue(gamma);
				if (ptrGammaSetting->GetValue() == gamma)
					Utility::printLine("INFO: Set Gamma of camera to " + std::to_string(gamma) + "!");
				else
					Utility::printLine("WARNING: Was unable to set the Gamma of camera to " + std::to_string(gamma) + " it is actually " + std::to_string(ptrGammaSetting->GetValue()) + "!");
			}
		}
		else {
			Utility::printLine("ERROR: Unable to set manual gamma enable to true");
		}
	}
	catch (Spinnaker::Exception &e)	{
		Utility::printLine("ERROR: Exception while setting camera options \n" + std::string(e.what()));
		return false;
	}

	return true;
}

//REFERENCE: From Spinaker Documentation Aqciusition.cpp Example
// This function prints the device information of the camera from the transport
// layer; please see NodeMapInfo example for more in-depth comments on printing
// device information from the nodemap.
int CameraController::PrintDeviceInfo(INodeMap & nodeMap) {
	Utility::printLine();
	Utility::printLine("*** CAMERA INFORMATION ***");
	try	{
		FeatureList_t features;
		CCategoryPtr category = nodeMap.GetNode("DeviceInformation");
		if (IsAvailable(category) && IsReadable(category)) {
			category->GetFeatures(features);
			FeatureList_t::const_iterator it;
			for (it = features.begin(); it != features.end(); ++it)	{
				CNodePtr pfeatureNode = *it;
				std::string name(pfeatureNode->GetName().c_str());
				Utility::printLine(name + " : ");

				CValuePtr pValue = (CValuePtr)pfeatureNode;
				if (IsReadable(pValue))	{
					std::string value(pValue->ToString().c_str());
					Utility::print(value);
				}
				else
					Utility::print("Node not readable");
			}
		}
		else {
			Utility::printLine("Device control information not available.");
		}
	}
	catch (Spinnaker::Exception &e) {
		Utility::printLine("ERROR: " + std::string(e.what()));
		Utility::printLine();
		return -1;
	}

	Utility::printLine();
	return 0;
}

/* ConfigureExposureTime: sets the cameras exposure time
 * @return - TRUE if success, FALSE if failed */
bool CameraController::ConfigureExposureTime() {
	finalExposureTime = initialExposureTime;
		
	return SetExposure(finalExposureTime);
}

// [UTILITY]
// GetExposureRatio: calculates how many times exposure was cut in half
// @returns - the ratio of starting and final exosure time 
double CameraController::GetExposureRatio() {
	return initialExposureTime / finalExposureTime;
}

/* SetExposure: configure a custom exposure time. Automatic exposure is turned off, then the custom setting is applied.
* @param exposureTimeToSet - self explanatory (in microseconds = 10^-6 seconds)
* @return FALSE if failed, TRUE if succeded */
bool CameraController::SetExposure(double exposureTimeToSet) {
	//Constraint exposure time from going lower than camera limit
	//TODO: determine this lower bound for the camera we are using

	try {
		INodeMap &nodeMap = cam->GetNodeMap();

		// Turn off automatic exposure mode
		CEnumerationPtr ptrExposureAuto = nodeMap.GetNode("ExposureAuto");
		if (!IsAvailable(ptrExposureAuto) || !IsWritable(ptrExposureAuto)) {
			Utility::printLine("Unable to disable automatic exposure (node retrieval)");
			return false;
		}
		CEnumEntryPtr ptrExposureAutoOff = ptrExposureAuto->GetEntryByName("Off");
		if (!IsAvailable(ptrExposureAutoOff) || !IsReadable(ptrExposureAutoOff)) {
			Utility::printLine("Unable to disable automatic exposure (enum entry retrieval)");
			return false;
		}
		ptrExposureAuto->SetIntValue(ptrExposureAutoOff->GetValue());

		// Set exposure manually
		CFloatPtr ptrExposureTime = nodeMap.GetNode("ExposureTime");
		if (!IsAvailable(ptrExposureTime) || !IsWritable(ptrExposureTime)) {
			Utility::printLine("ERROR: Unable to set exposure time.");
			return false;
		}
		// Ensure new time does not exceed max set to max if does
		const double exposureTimeMax = ptrExposureTime->GetMax();
		if (exposureTimeToSet > exposureTimeMax) {
			exposureTimeToSet = exposureTimeMax;
			Utility::printLine("WARNING: Exposure time of " + std::to_string(exposureTimeToSet) + " is to big. Exposure set too max of " + std::to_string(exposureTimeMax));
		}
		ptrExposureTime->SetValue(exposureTimeToSet);
	}
	catch (Spinnaker::Exception &e)	{
		Utility::printLine("ERROR: Cannot Set Exposure Time:\n" + std::string(e.what()));
		return false;
	}

	return true;
}

void CameraController::HalfExposureTime() {
	finalExposureTime /= 2;
	if (!SetExposure(finalExposureTime))
		Utility::printLine("ERROR: wasn't able to half the exposure time!");
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
	//TODO: implement check if camera is currently running an optimization
	//if (isCamStarted)
	//{
	//	Utility::printLine("WARNING: cannot retrive camera info because camera controller is currently in use!");
	//	return false;
	//}

	if (cam->IsValid() && cam->IsInitialized()) {
		CIntegerPtr ptrWidth = cam->GetNodeMap().GetNode("WidthMax");
		x = ptrWidth->GetValue();

		CIntegerPtr ptrHeight = cam->GetNodeMap().GetNode("HeightMax");
		y = ptrHeight->GetValue();
	}
	else {
		Utility::printLine("ERROR: camera that was retrived for gathering info is not valid!");
		return false;
	}

	return true;
}
