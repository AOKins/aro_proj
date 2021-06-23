////////////////////
// Define various methods for accessible & repeatable use in a Utility namespace
// Last edited: 06/23/2021 by Andrew O'Kins
////////////////////
#ifndef UTILITY_H_
#define UTILITY_H_

#include <string>	// output format
#include <vector>
#include <thread>

namespace Utility {
	// [CONSOLE FEATURES]
	static void printLine(std::string msg = "", bool isDebug = false);
	static void print(std::string msg);

	// [LOGIC]
	// Returns true if the two strings are equal
	static bool areEqual(std::string a, std::string b);
	// Check if button's text is equal to provided string
	// Input: 
	//	btn - button that contains a window text
	//	text - string to check against button window text
	// Output: true if btn label is same as text, false if not equal
	static bool IsButtonText(CButton& btn, std::string text);

	// [TIMING FEATURES]
	// Return a string of formatted time label with current local time
	static std::string getCurTime();

	// [IMAGE PROCCESSING]
	/* FindAverageValue:  Calculate an average intensity from an image taken by the
	* camera which can be used as a fitness value
	* @param Image -> the camera image to find intensity from
	* @param target -> weight multiplier to apply to raw image (2D array with same dimensions as Image)
	* @param width -> the width of the image in pixels
	* @param height -> the height of the image in pixels
	* @return -> the average intensity within weight from target */
	static double FindAverageValue(unsigned char *Image, int* target, size_t width, size_t height);

	//////////////////////////////////////////////////
	//
	//   FindAverageValue()
	//
	//   Inputs: Image -> the image to use
	//			 r	   -> radius of area (centered in middle of image) to find average within
	//			 width -> the width of the camera image in pixels
	//			 height -> the height of the camera image in pixels
	//
	//   Returns: The average intensity within the calculated area
	//
	//   Purpose: Calculate an average intensity from an image taken by the camera
	//			  which can be used as a fitness value
	//
	//   Modifications:
	//
	//////////////////////////////////////////////////
	static double FindAverageValue(unsigned char *Image, size_t width, size_t height, int r);

	/* GenerateTargetMatrix_SinglePoint: create an image of a single centered dot
	* @param target - where it's saved
	* @param width  - the width of the camera image in pixels
	* @param height - the height of the camera image in pixels
	* @param radius - the radius of the single point */
	static void GenerateTargetMatrix_SinglePoint(int* targetMatrix, int cameraImageWidth, int cameraImageHeight, int targetRadius);
	static void GenerateTargetMatrix_LoadFromFile(int *target, int width, int height);

	// [STRING PROCCESSING]
	// GenerateTargetMatrix_LoadFromFile: Reads a file to create a target matrix.
	// @param: target -> pointer to where we save the matrix
	// @param: width -> width of camera image in pixels
	// @param: height -> height of camera image in pixels
	static std::vector<std::string> seperateByDelim(std::string fullString, char delim);

	// [MULTITHREADING]
	// Method to rejoin all the threads in given vector and clean up vector
	// WARNING: If a thread is indefinitely running, this will get locked up
	// Input: myThreads - vector to threads that need to be rejoined
	// Output: threads is myThreads are rejoined and vector is cleared
	static void rejoinClear(std::vector<std::thread>& myThreads);
};

#endif
