#ifndef OPTIMIZATION_H_
#define OPTIMIZATION_H_

#include <string>
#include <fstream>				// used to export information to file for debugging
using std::ofstream;
#include <vector> // For managing ind_threads
using std::vector;

#include <thread> // For ind_threads used in runOptimization
#include <mutex>  // Mutexes to protect identified critical sections

#include "Spinnaker.h"
#include "SpinGenApi\SpinnakerGenApi.h"
using namespace Spinnaker;
using namespace Spinnaker::GenApi;
using namespace Spinnaker::GenICam;

class MainDialog;
class CameraController;
class SLMController;
class ImageScaler;
class CameraDisplay;
class TimeStampGenerator;

class Optimization {
protected:
	//Object references
	MainDialog& dlg;
	CameraController* cc;
	SLMController* sc;
	//Base algorithm parameters
	double acceptedSimilarity = .97; // images considered the same when reach this threshold (has to be less than 1)
	double maxFitnessValue = 200; // max allowed fitness value - when reached exposure is halved (TODO: check this feature)
	double maxGenenerations = 3000;

	//Base algorithm stop conditions
	double fitnessToStop = 0;
	double secondsToStop = 60;
	double genEvalToStop = 0;

	//Preference-type parameters
	bool saveImages = false;		// TRUE -> save images of the fittest individual of each gen
	bool displayCamImage = true;    // TRUE -> opens a window showing the camera image
	bool displaySLMImage = false;   // TODO: only first SLM right now - add functionality to display any or all boards

	//Instance variables (used during optimization process)
	// Values assigned within setupInstanceVariables(), then if needed cleared in shutdownOptimizationInstance()
	bool isWorking = false; // true if currently actively running the optimization algorithm
	bool usingHardware = false; // debug flag of using hardware currently in a given thread (to know if accidentally having two threads use hardware at once!)
	int populationSize; // Size of the population being used (number of individuals in a population class)
	int popCount;		// Number of populations working with (equal to sc->boards.size() if multi-SLM mode)
	int eliteSize;		// Number of elite individuals within the population (should be less than populationSize)
	bool shortenExposureFlag;   // Set to true by individual if fitness is too high
	bool stopConditionsMetFlag; // Set to true if a stop condition was reached by one of the individuals
	CameraDisplay * camDisplay; // Display for camera
	CameraDisplay * slmDisplay; // Display for SLM
	int curr_gen;				// Current generation being evaluated (start at 0)
	TimeStampGenerator * timestamp; // Timer to track and store elapsed time as the algorithm executes

	ImagePtr bestImage;
	std::vector<ImageScaler*> scalers;
	std::vector<unsigned char*> slmScaledImages;
	// Output debug streams // TODO at finished state may seek to change/remove these to improve performance
	ofstream tfile;				// Record elite individual progress over generations
	ofstream timeVsFitnessFile;	// Recoding general fitness progress
	ofstream efile;				// Expsoure file to record when exposure is shortened

	// Methods for use in runOptimization()
	bool prepareStopConditions();
	bool prepareSoftwareHardware();

	// Creates a scaler with given SLMController
	// Input: slmImg - array that will be storing scalled image to be initialized with 0's
	//		  slmNum - index for board that will be scaling to, 0 based (defaults to 0)
	// Output: returns scaler that will scale
	ImageScaler* setupScaler(unsigned char *slmImg, int slmNum);

	// Check to see if we have reached the end condition
	// Input:
	//		curFitness - the current fitness to compare against fitnessToStop
	//		curSecPassed - current passed time in seconds to compare against secondsToStop
	//		curGenerations - the current generation that has been evaluated to compare against genEvalsToStop
	// Output: returns true if either input is greater than compared against OR the MainDialog's stopFlag has been set to true
	bool stopConditionsReached(double curFitness, double curSecPassed, double curGenerations);

	// Save the various setting parameters used in this optimization
	// Stores the values with formatting in "/logs/[time]_[optType]_Optimization_Parameters.txt"
	// Input:
	//		time - the current time as a string label
	//		optType - a string for identifying the kind of optimization that has been performed
	void saveParameters(std::string time, std::string optType);

	// Methods relying on implementation from child classes
	virtual bool setupInstanceVariables() = 0;		 // Setting up properties used in runOptimization()
	virtual bool shutdownOptimizationInstance() = 0; // Cleaning up properties as well as final saving for runOptimization()
	virtual bool runIndividual(int indID) = 0;		 // Method for handling the execution of an individual

	// Vector hold threads of running individuals
	vector<std::thread> ind_threads;
	// Mutexes to protect critical sections
	std::mutex hardwareMutex; // Mutex to protect access to the hardware used in evaluating an individual (SLM, Camera, etc.)
	std::mutex consoleMutex, imageMutex, camDisplayMutex;	// Mutex to protect access to i/o and lastImage dimension values
	std::mutex tfileMutex, timeVsFitMutex, efileMutex;					// Mutex to protect file i/o
	std::mutex stopFlagMutex, exposureFlagMutex;						// Mutex to protect important flags 
public:
	// Constructor
	Optimization(MainDialog& dlg_, CameraController* cc, SLMController* sc);

	// Method for performing the optimization
	// Output: returns true if successful ran without error, false if error occurs
	virtual bool runOptimization() = 0;
};

#endif
