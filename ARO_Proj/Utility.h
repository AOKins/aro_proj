////////////////////
// Utility.h - Define various methods for accessible & repeatable use in a Utility namespace
// Last edited: 08/11/2021 by Andrew O'Kins
////////////////////

#ifndef UTILITY_H_
#define UTILITY_H_

#include <string>	// output format of getCurDateTime and getCurLocalTime
#include <vector>	// for seperateByDelim and rejoinClear
#include <thread>	// for rejoinClear

// Utility namespace to encapsulate the various isolated methods that aren't associated with a particular class
namespace Utility {
	// [CONSOLE FEATURES]
	// On a new line print a message formatted as <[LOCAL TIME]> - [MESSAGE]
	// if isDebug and this is release build, message ignored
	void printLine(std::string msg = "", bool isDebug = false);

	// [TIMING FEATURES]
	// Return a string of formatted time label with current local date and time
	std::string getCurDateTime();

	// Return a string of formatted time label with current local time (no date)
	std::string getCurLocalTime();

	// [IMAGE PROCCESSING]
	// Calculate an average intensity from an image taken by the camera  which can be used as a fitness value
	// Input: image - pointer to the image data
	//		  width - the width of the camera image in pixels
	//		 height - the height of the camera image in pixels
	//			  r - radius of area (centered in middle of image) to find average within
	// Output: The average intensity within the calculated area
	double FindAverageValue(void *image, int width, int height, int r);

	// [STRING PROCCESSING]
	// Separate a string into a vector array, breaks in given character
	// Input: fullString - string to seperate into parts
	//		       delim - character that when found will be fulcrum to seperate the full string by
	// Output: a vector of strings that are substrings of the inputted string broken up by the delim input
	std::vector<std::string> seperateByDelim(std::string fullString, char delim);

	// [MULTITHREADING]
	// Method to rejoin all the threads in given vector and clean up vector
	// WARNING: If a thread is indefinitely running (such as running in an infinite loop), this will lock up
	// Input: myThreads - vector to threads that need to be rejoined
	// Output: threads is myThreads are rejoined and vector is cleared
	void rejoinClear(std::vector<std::thread>& myThreads);
};

#endif
