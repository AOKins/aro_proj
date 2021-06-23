#ifndef BRUTE_FORCE_OPTIMIZATION_H_
#define BRUTE_FORCE_OPTIMIZATION_H_

#include "Optimization.h"

class MainDialog;
class CameraController;
class SLMController;

class BruteForce_Optimization : public Optimization {
	std::ofstream lmaxfile;
	std::ofstream rtime;
	unsigned char* slmImg;
	bool multiEnable_;
public:
	// Constructor - inherits from base class
	BruteForce_Optimization(MainDialog& dlg, CameraController* cc, SLMController* sc) : Optimization(dlg, cc, sc){};

	// Method for executing the optimization
	// Output: returns true if successful ran without error, false if error occurs
	bool runOptimization();

	bool setupInstanceVariables();
	bool shutdownOptimizationInstance();
	// This method returns false as this isn't/shoudn't be used by this optimization
	bool runIndividual(int indID) { return false; };
};

#endif
