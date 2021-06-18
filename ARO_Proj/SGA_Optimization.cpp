////////////////////
// Optimization handler methods implementation for simple genetic algorithm
// Last edited: 06/18/2021 by Andrew O'Kins
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

using namespace cv;

////
// TODOs:	Debug multithreading as needed
//			Add support for multi-SLM configuration
//			Properly Address how to handle if image acquisition failed (currently just moves on to next individual without assigning a default fitness value)
//			send final scaled image to the SLM //ASK needed?
//			Remove undesired file i/o once debugging is complete

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
	try {	// Begin general camera exception handling while optimization loop is going
		this->timestamp = new TimeStampGenerator();		// Starting time stamp to track elapsed time
		// Optimization loop for each generation
		for (this->curr_gen = 0; this->curr_gen < maxGenenerations && !stopConditionsMetFlag; this->curr_gen++) {
			// Run each individual, giving them all fitness values as a result of their genome
			for (int indID = 0; indID < population->getSize(); indID++) {
				//this->runIndividual(indID); // Serial
				this->ind_threads.push_back(std::thread(this->runIndividual, indID)); // Parallel
			}
			Utility::rejoinClear(this->ind_threads);
		
			// Create a more fit generation
			population->nextGeneration();
			// Half exposure time if fitness value is too high
			if (this->shortenExposureFlag) {
				this->cc->HalfExposureTime();
				this->shortenExposureFlag = false;
				this->efile << "Exposure shortened after gen: " << this->curr_gen + 1 << " with new ratio " << this->cc->GetExposureRatio() << std::endl;
			}
		}
		// Cleanup & Save resulting instance
		if (shutdownOptimizationInstance()) {
			Utility::printLine("INFO: Successfully ended optimization instance and saved results");
		}
    }
    catch (Spinnaker::Exception &e) {
		Utility::printLine("ERROR: " + string(e.what()));
		return false;
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
bool SGA_Optimization::runIndividual(int indID) {
	//Apply LUT/Binning to randomly the generated individual's image
	std::vector<int> * tempptr = (population->getGenome(indID)); // Get the SLM image for the individual
	scaler->TranslateImage(tempptr, this->slmImg); // Scale the individual's image to SLM size

	// Local scoped ImagePtr's as we are only concerned with this individual
		// Image pointers to the image from the Camera's buffer
	ImagePtr convImage, curImage;
		// Image pointer to copy from the camera buffer and seperate before release lock from hardware
	ImagePtr thisImage = Image::Create();
	ImagePtr thisImageConvert = Image::Create();

	std::unique_lock<std::mutex>  consoleLock(consoleMutex, std::defer_lock);
	std::unique_lock<std::mutex> hardwareLock(hardwareMutex, std::defer_lock);

	hardwareLock.lock();
	// Write translated image to SLM boards //TODO: modify as it assumes boards get the same image and have same size
	for (int i = 1; i <= sc->boards.size(); i++) {
		this->sc->blink_sdk->Write_image(i, this->slmImg, sc->getBoardHeight(i - 1), false, false, 0.0);
	}
	// Acquire images // - take image
	this->cc->AcquireImages(curImage, convImage);
	// DeepCopy before removing from camera buffer
	thisImage->DeepCopy(curImage);
	thisImageConvert->DeepCopy(convImage);

	try {
		curImage->Release(); //important to not fill camera buffer
	}
	catch (Spinnaker::Exception &e) {
		consoleLock.lock();
		Utility::printLine("WARNING: current image release failed. no need to release image at this point");
		consoleLock.unlock();
	}

	hardwareLock.unlock(); // Now done with the hardware

	// Getting the image dimensions and data from resulting image
	int imgWidth = thisImageConvert->GetWidth();
	int imgHeight = thisImageConvert->GetHeight();
	unsigned char * camImg = static_cast<unsigned char*>(thisImageConvert->GetData());
	
	// Giving error and ends early if there is no data
	if (camImg == NULL) { // TODO: I think this is dangerous as Individual does not have default fitness if left unevaluated
		consoleLock.lock();
		Utility::printLine("ERROR: Image Acquisition has failed!");
		consoleLock.unlock();
		return false;
	}

	// Get current exposure setting of camera (relative to initial)
	double exposureTimesRatio = this->cc->GetExposureRatio();	// needed for proper fitness value across changing exposure time
	// Determine the fitness by intensity of the image within circle of target radius
	double fitness = Utility::FindAverageValue(camImg, imgWidth, imgHeight, cc->targetRadius);

	// Display first individual
	if (indID == 0 && displayCamImage) {
		std::unique_lock<std::mutex> camLock(camDisplayMutex, std::defer_lock);
		camLock.lock();
		camDisplay->UpdateDisplay(camImg);
		camLock.unlock();
	}
	// Record values for the top six individuals in each generation
	if (indID > 23) {
		std::unique_lock<std::mutex> tVfLock(timeVsFitMutex, std::defer_lock);
		tVfLock.lock();
		this->timeVsFitnessFile << timestamp->MS_SinceStart() << "," << fitness*exposureTimesRatio << "," << cc->finalExposureTime << "," << exposureTimesRatio << std::endl;
		tVfLock.unlock();
	}
	//Save elite info of last generation
	if (indID == (population->getSize() - 1)) {
		std::unique_lock<std::mutex> tFileLock(tfileMutex, std::defer_lock);
		tFileLock.lock();
		this->tfile << "SGA GENERATION," << curr_gen << "," << fitness*exposureTimesRatio << std::endl;
		tFileLock.unlock();
		if (saveImages) {
			cc->saveImage(thisImageConvert, (curr_gen + 1));
		}
		// Also save the image
		std::unique_lock<std::mutex> imageLock(imageMutex, std::defer_lock);
		imageLock.lock();
		this->bestImage->DeepCopy(thisImageConvert);
		imageLock.unlock();
	}

	// Check stop conditions
	stopConditionsMetFlag = stopConditionsReached((fitness*exposureTimesRatio), timestamp->S_SinceStart(), curr_gen + 1);

	// Update fitness for this individual
	population->setFitness(indID, fitness * exposureTimesRatio);

	// If the fitness value is too high, flag that the exposure needs to be shortened
	if (fitness > maxFitnessValue) {
		std::unique_lock<std::mutex> expsureFlagLock(exposureFlagMutex, std::defer_lock);
		expsureFlagLock.lock();
		this->shortenExposureFlag = true;
		expsureFlagLock.unlock();
	}

	return true; // No errors!
}

// Method to setup specific properties runOptimziation() instance
bool SGA_Optimization::setupInstanceVariables() {
	// Setting population size as well as number of elite individuals kept in the genetic repopulation
	this->populationSize = 30;
	this->eliteSize = 5;
	this->bestImage = Image::Create();
	
	this->ind_threads.clear();
	
	// Find length for SLM images
	this->slmLength = this->sc->getBoardWidth(0) * this->sc->getBoardHeight(0) * 1;
	if (slmLength <= -1) {
		Utility::printLine("ERROR: SLM Length cannot be less than 0!");
		return false;
	}
	// Find length of camera images
	this->imageLength = this->cc->cameraImageHeight * this->cc->cameraImageWidth;
	if (this->imageLength <= -1) {
		Utility::printLine("ERROR: Image Length cannot be less than 0!");
		return false;
	}
	// Setting population
	this->population = new SGAPopulation<int>(this->cc->numberOfBinsY * this->cc->numberOfBinsX * this->cc->populationDensity, this->populationSize, this->eliteSize, this->acceptedSimilarity);

	this->shortenExposureFlag = false;		// Set to true by individual if fitness is too high
	this->stopConditionsMetFlag = false;	// Set to true if a stop condition was reached by one of the individuals

	// Setup image displays for camera and SLM
	this->camDisplay = new CameraDisplay(this->cc->cameraImageHeight, this->cc->cameraImageWidth, "Camera Display");
	this->slmDisplay = new CameraDisplay(this->sc->getBoardHeight(0), this->sc->getBoardWidth(0), "SLM Display");
	// Open displays if preference is set
	if (this->displayCamImage) {
		this->camDisplay->OpenDisplay();
	}
	if (this->displaySLMImage) {
		this->slmDisplay->OpenDisplay();
	}
	this->slmImg = new unsigned char[slmLength]; // Char array for writing SLM images
	// Scaler Setup (using base class)
	this->scaler = setupScaler(this->slmImg);

	// Start up the camera
	this->cc->startCamera();
	
	//Open up files to which progress will be logged
	this->tfile.open("logs/SGA_functionEvals_vs_fitness.txt", std::ios::app);
	this->timeVsFitnessFile.open("logs/SGA_time_vs_fitness.txt", std::ios::app);
	this->efile.open("logs/exposure.txt", std::ios::app);

	return true; // Returning true if no issues met
}

// Method to clean up & save resulting runOptimziation() instance
bool SGA_Optimization::shutdownOptimizationInstance() {
	// - fitness logging files
	this->timeVsFitnessFile << this->timestamp->MS_SinceStart() << " " << 0 << std::endl;
	this->timeVsFitnessFile.close();
	this->tfile.close();
	this->efile.close();
	
	// Get elite info
	unsigned char* eliteImage = static_cast<unsigned char*>(this->bestImage->GetData());
	int imgHeight = this->bestImage->GetHeight();
	int imgWidth = this->bestImage->GetWidth();

	// Save how final optimization looks through camera
	std::string curTime = Utility::getCurTime();
	Mat Opt_ary = Mat(imgHeight, imgWidth, CV_8UC1, eliteImage);
	cv::imwrite("logs/" + curTime + "_SGA_Optimized.bmp", Opt_ary);

	// Save final (most fit SLM image)
	std::vector<int> * tempptr = this->population->getGenome(this->population->getSize() - 1); // Get the image for the individual (most fit)

	scaler->TranslateImage(tempptr, this->slmImg);
	Mat m_ary = Mat(512, 512, CV_8UC1, this->slmImg);
	cv::imwrite("logs/" + curTime + "_SGA_phaseopt.bmp", m_ary);

	// Generic file renaming to have time stamps of run
	std::rename("logs/SGA_functionEvals_vs_fitness.txt", ("logs/" + curTime + "_SGA_functionEvals_vs_fitness.txt").c_str());
	std::rename("logs/SGA_time_vs_fitness.txt", ("logs/" + curTime + "_SGA_time_vs_fitness.txt").c_str());
	std::rename("logs/exposure.txt", ("logs/" + curTime + "_SGA_exposure.txt").c_str());
	saveParameters(curTime, "SGA");

	// - image displays
	this->camDisplay->CloseDisplay();
	this->slmDisplay->CloseDisplay();
	// - camera
	this->cc->stopCamera();
	this->cc->shutdownCamera();
	// - pointers
	delete this->camDisplay;
	delete this->slmDisplay;
	delete this->timestamp;
	delete this->population;
	delete[] this->slmImg;
	delete this->scaler;

	return true;
}
