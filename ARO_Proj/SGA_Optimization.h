////////////////////
// SGA_Optimization.h - handler for simple genetic algorithm that inherits from base Optimization class
// Last edited: 06/17/2021 by Andrew O'Kins
////////////////////

#ifndef SGA_OPTIMIZATION_H_
#define SGA_OPTIMIZATION_H_

#include "Optimization.h"
#include "SGA_Population.h"

class SGA_Optimization : public Optimization {
	// Method to setup specific properties runOptimziation() instance
	bool setupInstanceVariables();

	// Method to clean up & save resulting runOptimziation() instance
	bool shutdownOptimizationInstance();

	// Method for handling the execution of an individual
	// Input:
	//		indID - index value for individual being run to determine fitness (for multithreading will be the thread id as well)
	//		popID - index value for population being run to determine fitness
	// Output: returns false if a critical error occurs, true otherwise
	//		individual in population index indID will have assigned fitness according to result from cc
	//		lastImgWidth,lastImgHeight updated according to result from cc
	//		shortenExposureFlag is set to true if fitness value is high enough
	//		stopConditionsMetFlag is set to true if conditions met
	bool runIndividual(int indID);

	// Population that this optimization class uses
	// Set here to prevent possible slicing if were instead using a base class pointer in base class of Optimization
	std::vector<SGAPopulation<int>*> population;
public:
	// Constructor - inherits from base class
	SGA_Optimization(MainDialog* dlg, CameraController* cc, SLMController* sc) : Optimization(dlg, cc, sc){};

	// Method for executing the optimization
	// Output: returns true if successful ran without error, false if error occurs
	bool runOptimization();
};

#endif
