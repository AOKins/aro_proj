// CameraController.h - file to determine which CameraController wrapper to include
// When using camera controller, this header file should be what you include rather than from a specific SDK!

// Spinnaker version
#ifdef SPINNAKER_VERSION
	#include "CameraControllerSpinnaker.h"
	#include "ImageControllerSpinnaker.h"
	#ifdef PICAM_VERSION // Performing additional check to see if other version is also being includes (which would be bad!)
		#error "Cannot have Spinnaker and PICam versions in same build!"
	#endif
#endif
// PICam version
#ifdef PICAM_VERSION
	#include "CameraControllerPICam.h"
	#include "ImageControllerPICam.h"
	#ifdef SPINNAKER_VERSION // Performing additional check to see if other version is also being includes (which would be bad!)
		#error "Cannot have Spinnaker and PICam versions in same build!"
	#endif
#endif
