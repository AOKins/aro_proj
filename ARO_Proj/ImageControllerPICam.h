//TODO: Implement PICam version of ImageController
#ifndef IMAGE_CONTROLLER_PICAM_H_
#define IMAGE_CONTROLLER_PICAM_H_

#include "picam.h"
#include <fstream> // For save image

class ImageController {
private:
	unsigned char * data_;
	int width_;
	int height_;
	int size_;
public:
	ImageController() {
		this->data_ = nullptr;
		this->size_ = 0;
	}

	// Constructor with set image to assign
	// Performs deep copy, original should be safe to release
	// Input:	rawData - pointer to image data
	//			size - number of elements in rawData
	ImageController(int * rawData, int size, int width, int height) {
		this->size_;

		this->width_ = width;
		this->height_ = height;

		this->data_ = new unsigned char[size];
		for (int index = 0; index < size; index++) {
			this->data_[index] = rawData[index];
		}
	}

	ImageController(ImageController & other) {
		this->size_ = other.getSize();
		unsigned char * otherData = other.getRawData<unsigned char>();

		this->data_ = new unsigned char[this->size_];
		for (int index = 0; index < this->size_; index++) {
			this->data_[index] = otherData[index];
		}
	}

	// Desturctor - checks if need to call Release()
	~ImageController() {
		delete[] this->data_;
	}


	// Getter for release (used in copy constructor)
	const int getSize() {
		return this->size_;
	}

	// Returns pointer to data associated with the image
	template <typename T>
	T * getRawData() {
		return static_cast<T>(this->data_);
	}

	// Return width of the Image
	const int getWidth() {
		return this->width_;
	}

	// Return height of the image
	const int getHeight() {
		return this->height_;
	}

	// Use the SDK's method of saving the image
	void saveImage(std::string path) {
		// No provided method in PICam, so attempting to implement personal bitmap image output
		
		// Open file
		std::ofstream outputFile;
		outputFile.open(path, std::ios::binary);

		unsigned char tempValue;// Byte size variable for temp value to give (making sure it is only 1 byte in size)
		// Resource: https://en.wikipedia.org/wiki/BMP_file_format & https://web.archive.org/web/20080912171714/http://www.fortunecity.com/skyscraper/windows/364/bmpffrmt.html
		// File Header info // 
		tempValue = 66; // B
		outputFile << tempValue;
		tempValue = 109; // M
		outputFile << tempValue; 
		// Size
		tempValue = this->size_ * 3 + 55;
		outputFile << tempValue;
		// Reserved (can be 0)
		tempValue = 0;
		outputFile << tempValue << tempValue << tempValue << tempValue;

		// Offset to start (starting address of the image data) and should take up 10 bytes
		tempValue = 1078;
		outputFile << tempValue;

		// Bitmap Info Header
		tempValue = 40;
		outputFile << tempValue;
		tempValue = 100;
		outputFile << tempValue << tempValue;
		tempValue = 1;
		outputFile << tempValue;
		tempValue = 8;
		outputFile << tempValue;
		tempValue = 0;
		outputFile << tempValue << tempValue << tempValue << tempValue << tempValue << tempValue;
		
		// Data itself!
		for (int i = 0; i < this->size_; i++) {
			outputFile << this->data_[i]; // R
			outputFile << this->data_[i]; // G
			outputFile << this->data_[i]; // B
		}

		// Closing file, all done
		outputFile.close();
	}
};

#endif
