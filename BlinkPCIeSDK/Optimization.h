#ifndef OPTIMIZATION_H_
#define OPTIMIZATION_H_

class MainDialog;
class CameraController;
class SLMController;
class ImageScaler;

#include <string>

class Optimization
{
protected:
	//Object references
	MainDialog& dlg;
	CameraController* cc;
	SLMController* sc;

	//Base algorithm parameters
	double acceptedSimilarity = .97; //images considered the same when reach this threshold (has to be less than 1)
	double maxFitnessValue = 200;	 //max allowed fitness value - when reached exposure is halved (TODO: check this feature)
	double maxGenenerations = 3000;

	//Base algorithm stop conditions
	double fitnessToStop = 0;
	double secondsToStop = 60;
	double genEvalToStop = 0;

	//Preference-type parameters
	bool saveImages = false;		//TRUE -> save images of the fittest individual of each gen
	bool displayCameraImage = true; //TRUE -> opens a window showing the camera image

	//Instance variables
	bool isWorking = false;

	bool prepareStopConditions();
	bool prepareSoftwareHardware();
	ImageScaler* setupScaler(unsigned char *aryptr);
	bool stopConditionsReached(double curFitness, double curSecPassed, double curGenerations);
	void saveParameters(std::string time, std::string optType);

public:
	Optimization(MainDialog& dlg_, CameraController* cc, SLMController* sc);

	virtual bool runOptimization() = 0;
};


#endif
