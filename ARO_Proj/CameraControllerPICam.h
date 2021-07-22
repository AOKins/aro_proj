// TODO: Implement PICam version
#ifndef CAMERA_CONTROLLER_PICAM_H_
#define CAMERA_CONTROLLER_PICAM_H_

#ifdef USE_PICAM

#include <string>

#include "picam.h" // core include for PICam SDK

#include "ImageControllerPICam.h" // Image wrapper


class MainDialog;

class CameraController {
public:
	//Image parameters (with defaults set)
	// ROI offset
	int x0 = 896;				//  Must be a factor of 4 (like 752)
	int y0 = 568;				//	Must be a factor of 2 (like 752)
	// ROI dimension
	int cameraImageWidth = 64;	//	Must be a factor of 32 (like 64)
	int cameraImageHeight = 64;	//	Must be a factor of 2  (like 64)
	int populationDensity = 1;

	double gamma = 1.25;
	int fps = 200;
	double initialExposureTime = 2000;
	double finalExposureTime = 2000;

	int numberOfBinsX = 128;
	int numberOfBinsY = 128;
	int binSizeX = 4;
	int binSizeY = 4;

	//Image target settings
	int targetRadius = 5;
private:
	MainDialog* dlg;

	PicamHandle* camera_;

	//Logic control
	pibln * libraryInitialized; // Boolean to give if the library has been initialized or not
public:

	CameraController(MainDialog* dlg_);
	~CameraController();

	bool setupCamera();
	bool startCamera();
	bool saveImage(ImageController * curImage, std::string path);
	ImageController* AcquireImage();
	bool stopCamera();
	bool shutdownCamera();

	// [SETUP]
	bool UpdateImageParameters();
	bool UpdateConnectedCameraInfo();
	bool ConfigureCustomImageSettings();
	bool ConfigureExposureTime();

	// [UTILITY]
	int PrintDeviceInfo();

	// [ACCESSOR(S)/MUTATOR(S)]
	bool GetCenter(int &x, int &y);
	bool GetFullImage(int &x, int &y);
	// Return true if this controller has access to at least one camera
	bool hasCameras();
	// Setter for exposure setting
	bool SetExposure(double exposureTimeToSet);
	// Get the multiplier for exposure having been halved
	double GetExposureRatio();
	// half the current exposure time setting
	void HalfExposureTime();
};

#endif

#endif