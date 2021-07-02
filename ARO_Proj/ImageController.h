#ifndef IMAGE_CONTROLLER_H_
#define IMAGE_CONTROLLER_H_

#include "Spinnaker.h"
#include "SpinGenApi\SpinnakerGenApi.h"
using namespace Spinnaker;
using namespace Spinnaker::GenApi;
using namespace Spinnaker::GenICam;

// Class to encaspsulate interactions with Image data and current SDK
//		(this is so that optimization classes aren't relying on SDK specific behaviors)
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

	void saveImage(std::string path) {
		this->image_->saveImage(path.c_str());
	}
};

#endif
