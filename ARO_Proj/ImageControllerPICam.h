//TODO: Implement PICam version of ImageController
#ifndef IMAGE_CONTROLLER_PICAM_H_
#define IMAGE_CONTROLLER_PICAM_H_

#include "picam.h"

class ImageController {
private:

	bool needRelease;
public:
	ImageController() {
		this->needRelease = false;
	}

	// Constructor with set image to assign
	// Performs deep copy, original should be safe to release
	// Input:	setImage - imageptr to copy from
	//			release - set to true if this image when done with will need to be released from buffer with Release() call
	ImageController() {
	}

	ImageController(ImageController & other) {
	}

	// Desturctor - checks if need to call Release()
	~ImageController() {
	}


	// Getter for release (used in copy constructor)
	const bool getReleaseBool() {
	}

	// Returns pointer to data associated with the image
	template <typename T>
	T * getRawData() {
		return nullptr;
	}

	// Return width of the Image
	const int getWidth() {
		return 0;
	}

	// Return height of the image
	const int getHeight() {
		return 0;
	}

	// Use the SDK's method of saving the image
	void saveImage(std::string path) {
		
	}
};

#endif
