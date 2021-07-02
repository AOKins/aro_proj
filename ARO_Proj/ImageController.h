#ifndef IMAGE_CONTROLLER_H_
#define IMAGE_CONTROLLER_H_

#include "Spinnaker.h"
#include "SpinGenApi\SpinnakerGenApi.h"
using namespace Spinnaker;
using namespace Spinnaker::GenApi;
using namespace Spinnaker::GenICam;

// Class to encaspsulate interactions with Image data and SDK
//		This is so that Optimization classes don't aren't relying on SDK specific behaviors 
class ImageController {
private:
	ImagePtr image_; // Pointer to Image in Spinnaker SDK
public:
	// Constructor with set image to assign
	// Performs deep copy, original should be safe to release
	ImageController(ImagePtr& setImage) {
		this->image_ = Image::Create();
		this->image_->DeepCopy(setImage);
	}

	~ImageController() {
		this->image_->Release();
	}

	// Returns data associated with the image
	void * getRawData() {
		return this->~ImageController->GetData();
	}

	// Return width of the Image
	int getWidth() {
		return int(this->image_->GetWidth());
	}

	// Return height of the image
	int getHeight() {
		return int(this->image_->GetHeight());
	}
};

#endif
