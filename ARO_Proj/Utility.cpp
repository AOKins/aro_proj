#include "stdafx.h"

#include <fstream>	// used to export information to file 
#include <conio.h>	// console operations
#include <ctime>
#include <iostream>
#include <algorithm>
#include "CameraController.h"
#include "SLMController.h"
#include "Utility.h"

#include <opencv2\highgui\highgui.hpp>	//image processing
#include <opencv2\imgproc\imgproc.hpp>

// [CONSOLE FEATURES]
void Utility::printLine(std::string msg, bool isDebug) {
	// Comment out only when need to see debug type line pinting
	if (isDebug) {
		return;
	}
	std::cout << "\n" << msg;

	std::string curMsg = "\n" + msg;
	const char * dimMsgC = curMsg.c_str();
	_cprintf(dimMsgC);
}

void Utility::print(std::string msg) {
	std::cout << msg;

	//const char * dimMsgC = msg.c_str();
	//_cprintf(dimMsgC);
}

// [LOGIC]
bool Utility::areEqual(std::string a, std::string b) {
	if (a == b) {
		return true;
	}
	else {
		return false;
	}
}

bool Utility::IsButtonText(CButton& btn, std::string text) {
	CString PowerState;
	btn.GetWindowTextW(PowerState);
	CStringA pState(PowerState);

	if (strcmp(pState, text.c_str()) == 0) //the strings are equal, power is off
		return true;
	else
		return false;
}


// [TIMING FEATURES]
//getCurTime: This function returns the current time and date in Month-Day-Year Year-Minute-Second forat
std::string Utility::getCurTime() {
	time_t tt;
	struct tm * curTime;
	time(&tt);
	curTime = localtime(&tt);
	
	std::vector<std::string> timeParts = seperateByDelim(asctime(curTime), ' ');
	std::vector<std::string> hourMinuteSecondParts = seperateByDelim(timeParts[3], ':');

	std::string finalTimeString = "";
	finalTimeString += timeParts[1] + "-"; // month
	finalTimeString += timeParts[2] + "-"; // date (day)
	finalTimeString += timeParts[4] + "_"; // year
	finalTimeString += std::to_string(std::stoi((std::string(hourMinuteSecondParts[0])))%12) + "-";	//hour
	finalTimeString += hourMinuteSecondParts[1] + "-";  //minute
	finalTimeString += hourMinuteSecondParts[2];		//seconds

	finalTimeString.erase(std::remove(finalTimeString.begin(), finalTimeString.end(), '\n'), finalTimeString.end());

	return finalTimeString;
}

//[IMAGE PROCCESSING]
/* FindAverageValue:  Calculate an average intensity from an image taken by the
* camera which can be used as a fitness value
* @param Image -> the camera image to use
* @param target -> area to find average within, I believe
* @param width -> the width of the camera image in pixels
* @param height -> the height of the camera image in pixels
* @return -> the average intensity within the calculated area */
double Utility::FindAverageValue(unsigned char *Image, int* target, size_t width, size_t height) {
	cv::Mat m_ary(width, height, CV_8UC1, Image);
	double sum = 0;
	double area = 0;
	int x0, y0, r;
	x0 = 32;
	y0 = 32;
	r = 2;

	for (int i = x0 - r; i < x0 + r; i++) {
		for (int j = y0 - r; j < y0 + r; j++) {
			sum += m_ary.at<unsigned char>(i, j) * target[j*width + i];
			area += 1;
		}
	}

	// DEBUG
	// ofstream efile("target.txt", ios::app);
	// efile << sum << "  " << area << endl;
	// efile.close();
	return sum / area;
}

//////////////////////////////////////////////////
//
//   FindAverageValue()
//
//   Inputs: Image -> the camera image to use
//			 target -> area to find average within, I believe
//			 width -> the width of the camera image in pixels
//			 height -> the height of the camera image in pixels
//
//   Returns: The average intensity within the calculated area
//
//   Purpose: Calculate an average intensity from an image taken by the camera
//			  which can be used as a fitness value
//
//   Modifications: Changed from "peakvalue()," most of the stuff was cut out
//
//////////////////////////////////////////////////
double Utility::FindAverageValue(unsigned char *Image, size_t width, size_t height, int r) {
	int value, ll, kk, cx, cy, ymin, ymax;
	double rdbl, sdbl, rloop, sloop, xmin, xmax, area;
	cv::Mat m_ary = cv::Mat(height, width, CV_8UC1, Image);
	rdbl = 0;
	rloop = 0;
	sloop = 0;
	cx = width / 2;
	cy = height / 2;
	area = 3.14*pow(r, 2);
	ymin = cy - r;
	ymax = cy + r;

	//calculate average
	for (ll = ymin; ll < ymax; ll++) {
		xmin = cx - sqrt(pow(r, 2) - pow(ll - cy, 2));
		xmax = cx + sqrt(pow(r, 2) - pow(ll - cy, 2));

		for (kk = xmin; kk < xmax; kk++){
			value = m_ary.at<unsigned char>(ll, kk);
			rloop += value;
		}
	}

	rdbl = rloop / area;

	//calculate std. dev.
	for (ll = ymin; ll < ymax; ll++) {
		for (kk = xmin; kk < xmax; kk++) {
			value = m_ary.at<unsigned char>(ll, kk);
			sloop += pow((value - rdbl), 2);
		}
	}

	sdbl = sqrt(sloop / 2500) / 50;

	return rdbl;
}


/* GenerateTargetMatrix_SinglePoint: create an image of a single centered dot
* @param target - where it's saved
* @param width  - the width of the camera image in pixels
* @param height - the height of the camera image in pixels
* @param radius - the radius of the single point */
void Utility::GenerateTargetMatrix_SinglePoint(int* targetMatrix, int cameraImageWidth, int cameraImageHeight, int targetRadius) {
	int centerX = cameraImageWidth / 2;
	int centerY = cameraImageHeight / 2;
	int ymin = centerY - targetRadius;
	int ymax = centerY + targetRadius;
	double xmin, xmax;
	for (int i = 0; i < cameraImageWidth * cameraImageHeight; i++) {
		targetMatrix[i] = -1;
	}
	for (int i = ymin; i < ymax; i++) {
		// calculate rough dimensions of a circle
		xmin = centerX - sqrt(pow(targetRadius, 2) - pow(i - centerY, 2));
		xmax = centerX + sqrt(pow(targetRadius, 2) - pow(i - centerY, 2));
		for (int j = xmin; j < xmax; j++) {
			targetMatrix[i*cameraImageWidth + j] = 500;
		}
	}

	std::ofstream efile("targetmat.txt");
	for (int i = 0; i < cameraImageWidth; i++) {
		for (int j = 0; j < cameraImageHeight; j++)	{
			efile << targetMatrix[j*cameraImageWidth + i] << ' ';
		}
		efile << std::endl;
	}
	efile.close();
}

/* GenerateTargetMatrix_LoadFromFile: Reads a file to create a target matrix.
 * @param: target -> pointer to where we save the matrix
 * @param: width -> width of camera image in pixels
 * @param: height -> height of camera image in pixels */
void Utility::GenerateTargetMatrix_LoadFromFile(int *target, int width, int height) {
	//Load file
	std::string targfilename = "DRAWN_TARGET.TXT";
	std::ifstream targfile(targfilename);
	if (targfile.fail()) {
		throw new std::exception("Failed Target Matrix Load: Could Not Open File");
	}
	std::vector<std::vector<int>> targvec;
	int targetwidth = 0;
	while (!targfile.eof())	{
		std::string line;
		std::getline(targfile, line);
		if (line.length() > targetwidth) { //find widest point
			targetwidth = line.length();
		}

		std::vector<int> targline;
		for (int i = 0; i < line.length(); i++)	{
			if (line[i] == ' ')	{
				targline.push_back(0);
			}
			else {
				targline.push_back(1);
			}
		}
		targvec.push_back(targline);
	}
	//calculate size and middle
	int targetheight = targvec.size();
	int xStart = (width / 2) - (targetwidth / 2);
	int yStart = (height / 2) - (targetheight / 2);

	if (xStart < 0 || yStart < 0) {
		throw new std::exception("Failed Target Matrix Load: Target Matrix Too Big");
	}

	//apply loaded mat to target

	for (int i = 0; i < width * height; i++) {
		target[i] = 0;
	}

	for (int i = 0; i < targetheight; i++) {
		for (int j = 0; j < targetwidth; j++) {
			target[((yStart + i) * width) + (xStart + j)] = targvec[i][j];
		}
	}

	std::ofstream efile("targetmat.txt", std::ios::app);
	for (int i = 0; i < height; i++) {
		for (int j = 0; j < width; j++)	{
			efile << target[i*width + j] << ' ';
		}
		efile << std::endl;
	}
	efile.close();
}

//[STRING PROCCESING]
std::vector<std::string> Utility::seperateByDelim(std::string fullString, char delim) {
	//TODO: make into a seperate UTILITY function
	std::vector<std::string> parts;
	std::string curPart = "";
	for (int i = 0; i < fullString.size(); i++)	{
		if (fullString[i] == delim && curPart != "") {
			parts.push_back(curPart);
			curPart = "";
		}
		else {
			curPart += fullString[i];
		}
	}
	if (curPart != "")
		parts.push_back(curPart);

	return parts;
}

// [MULTITHREADING]
// Rejoin all the threads and clear ind_threads vector for future use
// Note: if a thread is stuck in an indefinite duration, this will lock up
void Utility::rejoinClear(std::vector<std::thread> & myThreads) {
	for (int i = 0; i < myThreads.size(); i++) {
		myThreads[i].join();
	}
	myThreads.clear();
}
