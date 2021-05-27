#ifndef UGA_OPTIMIZATION_H_
#define UGA_OPTIMIZATION_H_


#include "Optimization.h"


class MainDialog;
class CameraController;
class SLMController;

class uGA_Optimization : public Optimization
{
public:
	uGA_Optimization(MainDialog& dlg, CameraController* cc, SLMController* sc) : Optimization(dlg, cc, sc){};

	bool runOptimization();
};

#endif