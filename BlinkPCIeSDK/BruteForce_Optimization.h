#ifndef BRUTE_FORCE_OPTIMIZATION_H_
#define BRUTE_FORCE_OPTIMIZATION_H_

#include "Optimization.h"

class MainDialog;
class CameraController;
class SLMController;

class BruteForce_Optimization : public Optimization
{

public:
	BruteForce_Optimization(MainDialog& dlg, CameraController* cc, SLMController* sc) : Optimization(dlg, cc, sc){};

	bool runOptimization();
};

#endif