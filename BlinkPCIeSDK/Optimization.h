#ifndef OPTIMIZATION_H_
#define OPTIMIZATION_H_

class MainDialog;
class CameraController;
class SLMController;
class ImageScaler;
class CameraDisplay;
class TimeStampGenerator;

#include <string>
#include <fstream>				// used to export information to file for debugging
using std::ofstream;

#include "Spinnaker.h"
#include "SpinGenApi\SpinnakerGenApi.h"
using namespace Spinnaker;
using namespace Spinnaker::GenApi;
using namespace Spinnaker::GenICam;

class Optimization {
protected:
	//Object references
	MainDialog& dlg;
	CameraController* cc;
	SLMController* sc;
	//Base algorithm parameters
	double acceptedSimilarity = .97; // images considered the same when reach this threshold (has to be less than 1)
	double maxFitnessValue    = 200; // max allowed fitness value - when reached exposure is halved (TODO: check this feature)
	double maxGenenerations   = 3000;

	//Base algorithm stop conditions
	double fitnessToStop =  0;
	double secondsToStop = 60;
	double genEvalToStop =  0;

	//Preference-type parameters
	bool saveImages = false;		//TRUE -> save images of the fittest individual of each gen
	bool displayCamImage = true; //TRUE -> opens a window showing the camera image
	bool displaySLMImage = false; //TODO: only first SLM right now - add functionality to display any or all boards

	//Instance variables
	bool isWorking = false; // true if currently actively running the optimization algorithm
	int populationSize; // Size of the population being used
	int eliteSize;		// Number of elite individuals within the population (should be less than populationSize)
	int slmLength;		// Size of images for sc
	int imageLength;	// Size of images from cc
	unsigned char *aryptr; // Char array for writing SLM images
	unsigned char *camImg; // Char array to store resulting camera image
	bool shortenExposureFlag;   // Set to true by individual if fitness is too high
	bool stopConditionsMetFlag; // Set to true if a stop condition was reached by one of the individuals
	CameraDisplay * camDisplay; // Display for camera
	CameraDisplay * slmDisplay; // Display for SLM
	ImageScaler * scaler;		// Image scaler
	int curr_gen;				// Current generation being evaluated (start at 0)
	ImagePtr curImage, convImage;	// pointers to current image
	TimeStampGenerator * timestamp; // Timer to track and store elapsed time as the algorithm executes
	int lastImgHeight, lastImgWidth;// dimensions of last image taken from camera

	// Output debug streams // TODO at finished state may seek to change/remove these to improve performance
	ofstream tfile;				// Record elite individual progress over generations
	ofstream timeVsFitnessFile;	// Recoding general fitness progress
	ofstream efile;				// Expsoure file to record when exposure is shortened

	// Methods for use in runOptimization()
	bool prepareStopConditions();
	bool prepareSoftwareHardware();
	ImageScaler* setupScaler(unsigned char *aryptr);
	bool stopConditionsReached(double curFitness, double curSecPassed, double curGenerations);
	void saveParameters(std::string time, std::string optType);

	virtual bool setupInstanceVariables() = 0;		 // Setting up properties used in runOptimization()
	virtual bool shutdownOptimizationInstance() = 0; // Cleaning up properties as well as final saving for runOptimization()
	virtual bool runIndividual(int indID) = 0;		 // Method for handling the execution of an individual
public:
	// Constructor
	Optimization(MainDialog& dlg_, CameraController* cc, SLMController* sc);

	// Method for performing the optimization
	// Output: returns true if successful ran without error, false if error occurs
	virtual bool runOptimization() = 0;
};

#endif
