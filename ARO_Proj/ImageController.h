#ifndef IMAGE_CONTROLLER_H_
#define IMAGE_CONTROLLER_H_

#include "Spinnaker.h"
#include "SpinGenApi\SpinnakerGenApi.h"
using namespace Spinnaker;
using namespace Spinnaker::GenApi;

// Class to encaspsulate interactions required to accessing image data and current SDK
//		(this is so that optimization classes aren't relying on an SDK's specific behaviors)
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
	// Desturctor - release the image information from buffer
	~ImageController() {
		this->image_->Release();
	}

	// Returns pointer to data associated with the image
	template <typename T>
	T * getRawData() {
		return static_cast<T>(this->~ImageController->GetData());
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
