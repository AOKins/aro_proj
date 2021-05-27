#ifndef SGA_OPTIMIZATION_H_
#define SGA_OPTIMIZATION_H_

#include "Optimization.h"

class MainDialog;
class CameraController;
class SLMController;

class SGA_Optimization : public Optimization
{

public:
	SGA_Optimization(MainDialog& dlg, CameraController* cc, SLMController* sc) : Optimization(dlg, cc, sc){};

	bool runOptimization();
};

#endif