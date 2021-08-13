////////////////////
// CameraController.h - file to determine which CameraController wrapper to include for building
// When using camera controller, this header file should be what you include rather than from a specific SDK!
// Last edited: 08/02/2021 by Andrew O'Kins
////////////////////

// Forward declarations of the classes to be defined
class CameraController;
class ImageController;

// Define which version to build with (Spinnaker or PICam?) cannot be both
//#define USE_PICAM
//#define USE_SPINNAKER
#define TEST_ONLY


#ifdef TEST_ONLY
	#include "CameraControllerEmpty.h"
	#include "ImageControllerEmpty.h"
#endif