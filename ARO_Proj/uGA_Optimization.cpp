////////////////////
// Optimization handler methods implementation for micro-genetic algorithm
// Last edited: 06/10/2021 by Andrew O'Kins
////////////////////
#include "stdafx.h"				// Required in source
#include "uGA_Optimization.h"	// Header file
#include "uGA_Population.h"

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
#include <memory>

using namespace cv;

////
// TODOs:	Verify currrent (refactorization and multithreading) implementation
//			Add support for multi-SLM configuration

// Method for executing the optimization
// Output: returns true if successful ran without error, false if error occurs
bool uGA_Optimization::runOptimization() {
	Utility::printLine("uGA BUTTON CLICKED!");

	//Setup before optimization (see base class for implementation)
	if (!prepareSoftwareHardware()) {
		Utility::printLine("ERROR: Failed to prepare software or/and hardware for UGA Optimization");
		return false;
	}
	if (!setupInstanceVariables()) {
		Utility::printLine("ERROR: Failed to prepare values and files for SGA Optimization");
		return false;
	}
	try {	// Begin camera exception handling while optimization loop is going
	
		this->timestamp = new TimeStampGenerator();		// Starting time stamp to track elapsed time
		// Optimization loop for each generation
		for (this->curr_gen = 0; this->curr_gen < this->maxGenenerations && !this->stopConditionsMetFlag; this->curr_gen++) {
			// Run each individual, giving them all fitness values as a result of their genome
			for (int indID = 0; indID < this->population->getSize(); indID++) {
				runIndividual(indID);
			}
			// Only create new geneartion and change exposure setting if the stop condition hasn't been reached yet
			if (!this->stopConditionsMetFlag) {
				// Create a more fit generation
				this->population->nextGeneration();
				// Half exposure time if fitness value is too high
				if (this->shortenExposureFlag) {
					this->cc->HalfExposureTime();
					this->shortenExposureFlag = false;
					this->efile << "Exposure shortened after gen: " << this->curr_gen + 1 << " with new ratio " << this->cc->GetExposureRatio() << std::endl;
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
	this->isWorking = false;
	this->dlg.disableMainUI(!isWorking);
	return true;
}

// Method for handling the execution of an individual
// Input: indID - index value for individual being run to determine fitness (for multithreading will be the thread id as well)
// Output: returns false if a critical error occurs, true otherwise
//	individual in population index indID will have assigned fitness according to result from cc
//	lastImgWidth,lastImgHeight updated according to result from cc
//     shortenExposureFlag is set to true if fitness value is high enough
//     stopConditionsMetFlag is set to true if conditions met
bool uGA_Optimization::runIndividual(int indID) {
	//Apply LUT/Binning to randomly the generated individual's image
	std::shared_ptr<std::vector<int>> tempptr(this->population->getImage(indID)); // Get the image for the individual
	this->scaler->TranslateImage(tempptr, aryptr);

	// Write translated image to SLM boards //TODO: modify as it assumes boards get the same image and have same size
	for (int i = 1; i <= this->sc->boards.size(); i++) {
		this->sc->blink_sdk->Write_image(i, aryptr, sc->getBoardHeight(i - 1), false, false, 0.0);
	}
	// Acquire images // - take image and determine fitness
	this->cc->AcquireImages(this->curImage, this->convImage);
	int imgWidth = this->convImage->GetWidth();
	int imgHeight = this->convImage->GetHeight();
	this->camImg = static_cast<unsigned char*>(convImage->GetData());

	if (this->camImg == NULL) { // TODO: I think this is dangerous as Individual does not have default fitness if left unevaluated
		Utility::printLine("ERROR: Image Acquisition has failed!");
		return false;
	}
	double exposureTimesRatio = this->cc->GetExposureRatio();	// needed for proper fitness value across changing exposure time
	double fitness = Utility::FindAverageValue(this->camImg, imgWidth, imgHeight, this->cc->targetRadius);

	// Setting last height and width info to current
	this->lastImgHeight = imgWidth;
	this->lastImgWidth = imgHeight;

	// Display first individual
	if (indID == 0 && this->displayCamImage) {
		this->camDisplay->UpdateDisplay(camImg);
	}
	// Record values for the top six individuals in each generation
	if (indID > 23) {
		this->timeVsFitnessFile << this->timestamp->MS_SinceStart() << "," << fitness*exposureTimesRatio << "," << this->cc->finalExposureTime << "," << exposureTimesRatio << std::endl;
	}
	//Save elite info of last generation
	if (indID == (population->getSize() - 1)) {
		this->tfile << "uGA GENERATION," << this->curr_gen << "," << fitness*exposureTimesRatio << std::endl;
		if (saveImages) {
			cc->saveImage(convImage, (this->curr_gen + 1));
		}
	}
	try {
		this->curImage->Release(); //important to not fill camera buffer
	}
	catch (Spinnaker::Exception &e) {
		Utility::printLine("WARNING: current image release failed. no need to release image at this point");
	}

	// Check stop conditions
	this->stopConditionsMetFlag = stopConditionsReached((fitness*exposureTimesRatio), this->timestamp->S_SinceStart(), this->curr_gen + 1);

	// Update fitness for this individual
	this->population->setFitness(indID, fitness * exposureTimesRatio);
	// If the fitness value is too high, flag that the exposure needs to be shortened
	if (fitness > maxFitnessValue) {
		this->shortenExposureFlag = true;
	}
	return true;
}

// Method to setup specific properties runOptimziation() instance
bool uGA_Optimization::setupInstanceVariables() {
	// Setting population size as well as number of elite individuals kept in the genetic repopulation
	this->populationSize = 5;
	this->eliteSize = 1;

	// Find length for SLM images
	this->slmLength = this->sc->getBoardWidth(0) * this->sc->getBoardHeight(0) * 1;
	if (this->slmLength <= -1) {
		Utility::printLine("ERROR: SLM Length cannot be less than 0!");
		return false;
	}
	// Find length of camera images
	this->imageLength = this->cc->cameraImageHeight * this->cc->cameraImageWidth;
	if (imageLength <= -1) {
		Utility::printLine("ERROR: Image Length cannot be less than 0!");
		return false;
	}
	// Setting population
	this->population = new uGAPopulation<int>(this->cc->numberOfBinsY * this->cc->numberOfBinsX * this->cc->populationDensity, this->populationSize, this->eliteSize, this->acceptedSimilarity);

	this->aryptr = new unsigned char[slmLength]; // Char array for writing SLM images
	this->camImg = new unsigned char; // Char array to store resulting camera image

	this->shortenExposureFlag = false; // Set to true by individual if fitness is too high
	this->stopConditionsMetFlag = false; // Set to true if a stop condition was reached by one of the individuals

	// Setup image displays for camera and SLM
	this->camDisplay = new CameraDisplay(this->cc->cameraImageHeight, this->cc->cameraImageWidth, "Camera Display");
	this->slmDisplay = new CameraDisplay(this->sc->getBoardHeight(0), this->sc->getBoardWidth(0), "SLM Display");
	// Open displays if preference is set
	if (this->displayCamImage)
		this->camDisplay->OpenDisplay();
	if (this->displaySLMImage)
		this->slmDisplay->OpenDisplay();
	// Scaler Setup (using base class)
	this->scaler = setupScaler(aryptr);

	// Start up the camera
	this->cc->startCamera();

	// Setup image pointers
	this->curImage = Image::Create();
	this->convImage = Image::Create();

	//Open up files to which progress will be logged
	this->tfile.open("logs/uGA_functionEvals_vs_fitness.txt", std::ios::app);
	this->timeVsFitnessFile.open("logs/uGA_time_vs_fitness.txt", std::ios::app);
	this->efile.open("logs/exposure.txt", std::ios::app);

	return true; // Returning true if no issues met
}

// Method to clean up & save resulting runOptimziation() instance
bool uGA_Optimization::shutdownOptimizationInstance() {
	// - fitness logging files
	this->timeVsFitnessFile << timestamp->MS_SinceStart() << " " << 0 << std::endl;
	this->timeVsFitnessFile.close();
	this->tfile.close();
	this->efile.close();

	// Save how final optimization looks through camera
	std::string curTime = Utility::getCurTime();
	Mat Opt_ary = Mat(lastImgHeight, lastImgWidth, CV_8UC1, camImg);
	imwrite("logs/" + curTime + "_uGA_Optimized.bmp", Opt_ary);

	// Save final (most fit SLM image)
	std::shared_ptr<std::vector<int>> tempptr(population->getImage(population->getSize() - 1)); // Get the image for the individual (most fit)
	scaler->TranslateImage(tempptr, this->aryptr);
	Mat m_ary = Mat(512, 512, CV_8UC1, this->aryptr);
	imwrite("logs/" + curTime + "_uGA_phaseopt.bmp", m_ary);

	// Generic file renaming to have time stamps of run
	std::rename("logs/uGA_functionEvals_vs_fitness.txt", ("logs/" + curTime + "_uGA_functionEvals_vs_fitness.txt").c_str());
	std::rename("logs/uGA_time_vs_fitness.txt", ("logs/" + curTime + "_uGA_time_vs_fitness.txt").c_str());
	std::rename("logs/exposure.txt", ("logs/" + curTime + "_uGA_exposure.txt").c_str());
	saveParameters(curTime, "uGA");

	// - image displays
	this->camDisplay->CloseDisplay();
	this->slmDisplay->CloseDisplay();
	// - camera
	this->cc->stopCamera();
	this->cc->shutdownCamera();
	// - pointers
	delete this->camDisplay;
	delete this->slmDisplay;
	delete this->population;
	delete this->timestamp;
	delete[] this->aryptr;
	delete this->scaler;

	return true;
}
