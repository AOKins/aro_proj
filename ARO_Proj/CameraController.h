#ifndef CAMERA_CONTROLLER_H_
#define CAMERA_CONTROLLER_H_

#include <string>

#include "Spinnaker.h"
#include "SpinGenApi\SpinnakerGenApi.h"
using namespace Spinnaker;
using namespace Spinnaker::GenApi;
using namespace Spinnaker::GenICam;

#include "MainDialog.h"

class CameraController {
public:
	//Image parameters (with defaults set)
	int x0 = 896;				//  Must be a factor of 4 (like 752)
	int y0 = 568;				//	Must be a factor of 2 (like 752)
	int cameraImageWidth = 64;	//	Must be a factor of 32 (like 64)
	int cameraImageHeight = 64;	//	Must be a factor of 2  (like 64)
	int populationDensity = 1;

	double gamma = 1.25;
	int fps = 200;
	int frameRateMS = 5;
	double initialExposureTime = 2000;
	double finalExposureTime = 2000;
	float msPerFrame;

	int numberOfBinsX = 128;
	int numberOfBinsY = 128;
	int binSizeX = 4;
	int binSizeY = 4;

	//Image target matrix settings
	int* targetMatrix;
	int targetRadius = 5;

private:
	//UI/Equipment reference
	MainDialog& dlg;
	//Camera access
	SystemPtr system;
	CameraList camList;

	//Logic control
	bool isCamCreated = false;

public:
	CameraPtr cam;

	CameraController(MainDialog& dlg_);
	~CameraController();

	bool setupCamera();
	bool startCamera();
	bool saveImage(ImagePtr& curImage, std::string curGen);
	void AcquireImages(ImagePtr& curImage, ImagePtr& convertedImage);
	bool stopCamera();
	bool shutdownCamera();

	// [SETUP]
	bool UpdateImageParameters();
	bool UpdateConnectedCameraInfo();
	bool ConfigureCustomImageSettings();
	bool ConfigureExposureTime();

	// [UTILITY]
	int PrintDeviceInfo(INodeMap & nodeMap);
	bool hasCameras(); // Return true if at least one camera in camera list

	bool SetExposure(double exposureTimeToSet);
	double GetExposureRatio();
	void HalfExposureTime();

	// [ACCESSOR(S)/MUTATOR(S)]
	bool GetCenter(int &x, int &y);
	bool GetFullImage(int &x, int &y);

};

#endif
