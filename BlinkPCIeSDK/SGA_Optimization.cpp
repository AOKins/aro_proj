////////////////////
// Optimization handler methods implementation for standard genetic algorithm
// Last edited: 06/02/2021 by Andrew O'Kins
////////////////////
#include "stdafx.h"				// Required in source
#include "SGA_Optimization.h"	// Header file
#include "SGA_Population.h"

#include "Utility.h"
#include "Timing.h"
#include "CamDisplay.h"

#include "MainDialog.h"
#include "SLMController.h"
#include "CameraController.h"
#include "ImageScaler.h"
#include "Blink_SDK.h"
#include "SLM_Board.h"

#include <cstdlib>
#include <chrono>
#include <thread>
#include <memory>
using std::ostringstream;
using namespace cv;

////
// TODOs:	Support multithreading where appriopriate to improve speed performance
//			Add support for multiple boards
//			Address how to handle if image acquisition failed (currently just moves on to next individual without assigning a default fitness value)
//			send final scaled image to the SLM //ASK needed?
//			Remove unneeded file i/o once debugging is complete

// Method for executing the optimization
// Output: returns true if successful ran without error, false if error occurs
bool SGA_Optimization::runOptimization() {
	Utility::printLine("SGA BUTTON PRESSED!");
    //Setup before optimization (see base class for implementation)
	if (!prepareSoftwareHardware()) {
		Utility::printLine("ERROR: Failed to prepare software or/and hardware for SGA Optimization");
		return false;
	}
	// Setup variables that are of instance and depend on this specific optimization method (such as pop size)
	if (!setupInstanceVariables()) {
		Utility::printLine("ERROR: Failed to prepare values and files for SGA Optimization");
		return false;
	}
	try {	// Begin camera exception handling while optimization loop is going
		this->timestamp = new TimeStampGenerator();		// Starting time stamp to track elapsed time
		// Optimization loop for each generation
		for (this->curr_gen = 0; this->curr_gen < maxGenenerations && !stopConditionsMetFlag; this->curr_gen++) {
			// Run each individual, giving them all fitness values as a result of their genome
			for (int indID = 0; indID < population->getSize(); indID++) {
				runIndividual(indID);
            }
			// Only create new geneartion and change exposure setting if the stop condition hasn't been reached yet
			if (!stopConditionsMetFlag) {
				// Create a more fit generation
				population->StartNextGeneration();
				// Half exposure time if fitness value is too high
				if (this->shortenExposureFlag) {
					cc->HalfExposureTime();
					shortenExposureFlag = false;
					efile << "Exposure shortened after gen: " << this->curr_gen + 1 << " with new ratio " << cc->GetExposureRatio() << std::endl;
				}
			}
        }
		// Cleanup & Save resulting instance
		if (shutdownOptimizationInstance()) {
			Utility::printLine("INFO: Successfully ended optimization instance and saved results");
		}
    }
    catch (Spinnaker::Exception &e) {
		Utility::printLine("ERROR: " + string(e.what()));
	}
    //Reset UI State
	isWorking = false;
	dlg.disableMainUI(!isWorking);
	return true;
}

// Method for handling the execution of an individual
// Input: indID - index value for individual being run to determine fitness (for multithreading will be the thread id as well)
// Output: returns false if a critical error occurs, true otherwise
//	individual in population index indID will have assigned fitness according to result from cc
//	lastImgWidth,lastImgHeight updated according to result from cc
//     shortenExposureFlag is set to true if fitness value is high enough
//     stopConditionsMetFlag is set to true if conditions met
bool SGA_Optimization::runIndividual(int indID) {
	//Apply LUT/Binning to randomly the generated individual's image
	std::shared_ptr<std::vector<int>> tempptr;
	tempptr = std::make_shared<int>(population->getImage(indID)); // Get the image for the individual
	scaler->TranslateImage(tempptr, aryptr);

	// Write translated image to SLM boards //TODO: modify as it assumes boards get the same image and have same size
	for (int i = 1; i <= sc->boards.size(); i++) {
		sc->blink_sdk->Write_image(i, aryptr, sc->getBoardHeight(i-1), false, false, 0.0);
	}
	// Acquire images // - take image and determine fitness
	cc->AcquireImages(curImage, convImage);
	int imgWidth = convImage->GetWidth();
	int imgHeight = convImage->GetHeight();
	camImg = static_cast<unsigned char*>(convImage->GetData());

	if (camImg == NULL) { // TODO: I think this is dangerous as Individual does not have default fitness if left unevaluated
		Utility::printLine("ERROR: Image Acquisition has failed!");
		return false;
	}
	double exposureTimesRatio = cc->GetExposureRatio();	// needed for proper fitness value across changing exposure time
	double fitness = Utility::FindAverageValue(camImg, imgWidth, imgHeight, cc->targetRadius);

	// Setting last height and width info to current
	this->lastImgHeight = imgWidth;
	this->lastImgWidth  = imgHeight;

	// Display first individual
	if (indID == 0 && displayCamImage) {
		camDisplay->UpdateDisplay(camImg);
	}
	// Record values for the top six individuals in each generation
	if (indID > 23) {
		this->timeVsFitnessFile << timestamp->MS_SinceStart() << "," << fitness*exposureTimesRatio << "," << cc->finalExposureTime << "," << exposureTimesRatio << std::endl;
	}
	//Save elite info of last generation
	if (indID == (population->getSize()-1)) {
		this->tfile << "SGA GENERATION," << curr_gen << "," << fitness*exposureTimesRatio << std::endl;
		if (saveImages) {
			cc->saveImage(convImage, (curr_gen + 1));
		}
	}
	try {
		curImage->Release(); //important to not fill camera buffer
	}
	catch (Spinnaker::Exception &e) {
		Utility::printLine("WARNING: current image release failed. no need to release image at this point");
	}

	// Check stop conditions
	stopConditionsMetFlag = stopConditionsReached((fitness*exposureTimesRatio), timestamp->S_SinceStart(), curr_gen + 1);

	// Update fitness for this individual
	population->setFitness(indID, fitness * exposureTimesRatio);
	// If the fitness value is too high, flag that the exposure needs to be shortened
	if (fitness > maxFitnessValue) {
		this->shortenExposureFlag = true;
	}
	return true;
}

// Method to setup specific properties runOptimziation() instance
bool SGA_Optimization::setupInstanceVariables() {
	// Setting population size as well as number of elite individuals kept in the genetic repopulation
	this->populationSize = 30;
	this->eliteSize = 5;

	// Find length for SLM images
	this->slmLength = sc->getBoardWidth(0) * sc->getBoardHeight(0) * 1;
	if (slmLength <= -1) {
		Utility::printLine("ERROR: SLM Length cannot be less than 0!");
		return false;
	}
	// Find length of camera images
	this->imageLength = cc->cameraImageHeight * cc->cameraImageWidth;
	if (imageLength <= -1) {
		Utility::printLine("ERROR: Image Length cannot be less than 0!");
		return false;
	}
	// Setting population
	this->population = new SGAPopulation<int>(cc->numberOfBinsY * cc->numberOfBinsX * cc->populationDensity, populationSize, eliteSize, acceptedSimilarity);

	this->aryptr = new unsigned char[slmLength]; // Char array for writing SLM images
	this->camImg = new unsigned char; // Char array to store resulting camera image

	this->shortenExposureFlag = false; // Set to true by individual if fitness is too high
	this->stopConditionsMetFlag = false; // Set to true if a stop condition was reached by one of the individuals

	// Setup image displays for camera and SLM
	camDisplay = new CameraDisplay(cc->cameraImageHeight, cc->cameraImageWidth, "Camera Display");
	slmDisplay = new CameraDisplay(sc->getBoardHeight(0), sc->getBoardWidth(0), "SLM Display");
	// Open displays if preference is set
	if (displayCamImage)
		camDisplay->OpenDisplay();
	if (displaySLMImage)
		slmDisplay->OpenDisplay();
	// Scaler Setup (using base class)
	scaler = setupScaler(aryptr);

	// Start up the camera
	cc->startCamera();

	// Setup image pointers
	this->curImage = Image::Create();
	this->convImage = Image::Create();

	//Open up files to which progress will be logged
	tfile.open("logs/SGA_functionEvals_vs_fitness.txt", std::ios::app);
	timeVsFitnessFile.open("logs/SGA_time_vs_fitness.txt", std::ios::app);
	efile.open("logs/exposure.txt", std::ios::app);

	return true; // Returning true if no issues met
}

// Method to clean up & save resulting runOptimziation() instance
bool SGA_Optimization::shutdownOptimizationInstance() {
	// - image displays
	this->camDisplay->CloseDisplay();
	this->slmDisplay->CloseDisplay();
	// - camera
	cc->stopCamera();
	cc->shutdownCamera();
	// - pointers
	delete this->camDisplay;
	delete this->slmDisplay;
	delete this->scaler;
	delete[] this->aryptr;
	delete this->population;
	delete timestamp;

	// - fitness logging files
	timeVsFitnessFile << timestamp->MS_SinceStart() << " " << 0 << std::endl;
	timeVsFitnessFile.close();
	tfile.close();
	efile.close();

	// Save how final optimization looks through camera
	std::string curTime = Utility::getCurTime();
	Mat Opt_ary = Mat(lastImgHeight, lastImgWidth, CV_8UC1, camImg);
	cv::imwrite("logs/" + curTime + "_SGA_Optimized.bmp", Opt_ary);

	// Save final (most fit SLM image)
	std::shared_ptr<std::vector<int>> tempptr;
	tempptr = std::make_shared<int>(population->getImage(population->getSize() - 1)); // Get the image for the individual (most fit)
	scaler->TranslateImage(tempptr, aryptr);
	Mat m_ary = Mat(512, 512, CV_8UC1, aryptr);
	cv::imwrite("logs/" + curTime + "_SGA_phaseopt.bmp", m_ary);

	// Generic file renaming to have time stamps of run
	std::rename("logs/SGA_functionEvals_vs_fitness.txt", ("logs/" + curTime + "_SGA_functionEvals_vs_fitness.txt").c_str());
	std::rename("logs/SGA_time_vs_fitness.txt", ("logs/" + curTime + "_SGA_time_vs_fitness.txt").c_str());
	std::rename("logs/exposure.txt", ("logs/" + curTime + "_SGA_exposure.txt").c_str());
	saveParameters(curTime, "SGA");
	return true;
}
