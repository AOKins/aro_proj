#ifndef UTILITY_H_
#define UTILITY_H_

#include <string>
#include <vector>
#include <thread>

class Utility {
public:
	// [CONSOLE FEATURES]
	static void printLine(std::string msg = "", bool isDebug = false);
	static void print(std::string msg);

	// [LOGIC]
	static void areEqual(std::string a, std::string b);
	static bool IsButtonText(CButton& btn, std::string text);

	// [TIMING FEATURES]
	static void beginTimer();
	static void recordTimeStamp(std::string msg);
	static void pauseTimer();
	static void endTimer(); //TODO: Also clears the times and stops the timer
	static std::string getCurTime();

	// [IMAGE PROCCESSING]
	static double FindAverageValue(unsigned char *Image, int* target, size_t width, size_t height);
	static double FindAverageValue(unsigned char *Image, size_t width, size_t height, int r);

	static void GenerateTargetMatrix_SinglePoint(int* targetMatrix, int cameraImageWidth, int cameraImageHeight, int targetRadius);
	static void GenerateTargetMatrix_LoadFromFile(int *target, int width, int height);

	// [STRING PROCCESSING]
	static std::vector<std::string> seperateByDelim(std::string fullString, char delim);

	// [MULTITHREADING]
	static void rejoinClear(std::vector<std::thread>& myThreads);
};

#endif
