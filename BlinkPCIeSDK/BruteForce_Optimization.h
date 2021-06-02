#ifndef BRUTE_FORCE_OPTIMIZATION_H_
#define BRUTE_FORCE_OPTIMIZATION_H_

#include "Optimization.h"

class MainDialog;
class CameraController;
class SLMController;

class BruteForce_Optimization : public Optimization {
public:
	// Constructor - inherits from base class
	BruteForce_Optimization(MainDialog& dlg, CameraController* cc, SLMController* sc) : Optimization(dlg, cc, sc){};

	// Method for executing the optimization
	// Output: returns true if successful ran without error, false if error occurs
	bool runOptimization();
};

#endif
