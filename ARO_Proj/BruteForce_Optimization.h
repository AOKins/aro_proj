#ifndef BRUTE_FORCE_OPTIMIZATION_H_
#define BRUTE_FORCE_OPTIMIZATION_H_

#include "Optimization.h"

class BruteForce_Optimization : public Optimization {
	std::ofstream lmaxfile;
	std::ofstream rtime;

	std::vector<int*> finalImages_; // Once finished, contains the resulting optimized SLM images for all the boards used
	bool multiEnable_;
	bool dualEnable_;
	double allTimeBestFitness;
public:
	// Constructor - inherits from base class
	BruteForce_Optimization(MainDialog& dlg, CameraController* cc, SLMController* sc) : Optimization(dlg, cc, sc){};

	// Method for executing the optimization
	// Output: returns true if successful ran without error, false if error occurs
	bool runOptimization();

	bool setupInstanceVariables();
	bool shutdownOptimizationInstance();

	// Run individual for BF refers to the board being used
	// Input: boardID - index of SLM board being used (0 based)
	// Output: Result stored in slmImg
	bool runIndividual(int boardID);

	// Initialize slmImg with 0's
	void setBlankSlmImg(int* slmImg);
};

#endif
