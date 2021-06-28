////////////////////
// Optimization handler methods implementation for brute force algorithm
// Last edited: 06/28/2021 by Andrew O'Kins
////////////////////
#include "stdafx.h"						// Required in source
#include "BruteForce_Optimization.h"	// Header file

#include "Utility.h"
#include "Timing.h"
#include "CamDisplay.h"

#include "MainDialog.h"
#include "SLMController.h"
#include "ImageScaler.h"
#include "CameraController.h"
#include "Blink_SDK.h"
#include "SLM_Board.h"

#include <string>
#include <chrono>
#include <string>
using std::ofstream;
using namespace cv;

// TODO: Debug refactored/multi-SLM setup

bool BruteForce_Optimization::runOptimization() {
	Utility::printLine("OPT 5 BUTTON CLICKED!");
	//Setup before optimization (see base class for implementation)
	if (!prepareSoftwareHardware()) {
		Utility::printLine("ERROR: Failed to prepare software or/and hardware for Brute Force Optimization");
		return false;
	}
	// Setup variables that are of instance and depend on this specific optimization method (such as pop size)
	if (!setupInstanceVariables()) {
		Utility::printLine("ERROR: Failed to prepare values and files for OPT 5 Optimization");
		return false;
	}
	// Creating displays if desired
	if (displayCamImage) {
		this->camDisplay->OpenDisplay();
	}
	if (displaySLMImage) {
		this->slmDisplay->OpenDisplay();
	}
	try	{
		this->timestamp = new TimeStampGenerator();
		// By default run the first SLM (since with any option will be running with this
		//	TODO: If having "choose SLM to optimize" option, this will be here
		runIndividual(0);

		// If dual or multi, run second SLM
		if ( (this->dualEnable_ || this->multiEnable_) && this->dlg.stopFlag != true) {
			runIndividual(1);
		}
		
		// If multi, run rest of boards
		if (this->multiEnable_  && this->dlg.stopFlag != true) {
			for (int boardID = 2; boardID < this->sc->boards.size() && this->dlg.stopFlag != true; boardID++) {
				runIndividual(boardID);
			}
		}
	}
	catch (Spinnaker::Exception &e) {
		Utility::printLine("ERROR: " + string(e.what()));
	}
	// Cleanup
	return shutdownOptimizationInstance();;
}


// Run individual for BF refers to the board being used
// Input: indID - index of SLM board being used (0 based)
// Output: Result stored in
bool BruteForce_Optimization::runIndividual(int boardID) {
	if (boardID < 0 || boardID >= this->sc->boards.size()) {
		return false;
	}
	int slmWidth = this->sc->getBoardWidth(boardID);
	int slmHeight = this->sc->getBoardHeight(boardID);

	// Initialize array for storing slm images
	int * slmImg = new int[this->cc->numberOfBinsY * this->cc->numberOfBinsX * this->cc->populationDensity];

	int bin = 8; // TODO: Figure out these hardcoded value
	int dphi = 16;

	bool endOpt;
	//Initialize array of SLM image with 0s (note that the size of slmImg is dependent on the camera and not SLM!)
	setBlankSlmImg();

	ImagePtr curImage, convImage;
	try {
		// Iterate through columns
		for (int binCol = 0; binCol < slmWidth; binCol++) {
			// Iterate through rows
			for (int binRow = 0; binRow < slmHeight; binRow += bin) {
				int binValMax = 0;
				int	fitValMax = 0;
				// Current bin
				int binIndex = (binCol + binRow*this->cc->numberOfBinsX)*this->cc->populationDensity;

				// Find max phase for this bin
				for (int curBinVal = 0; curBinVal< 256; curBinVal += dphi) {
					// Assign at current bin the new value to test
					slmImg[binIndex] = curBinVal;

					// Scale and Write to board
					this->scalers[boardID]->TranslateImage(slmImg, this->slmScaledImages[boardID]);

					this->usingHardware = true;

					this->sc->blink_sdk->Write_image(boardID + 1, this->slmScaledImages[boardID], this->sc->getBoardHeight(boardID), false, false, 0);

					//Acquire camera image
					this->cc->AcquireImages(curImage, convImage);

					unsigned char* camImg = static_cast<unsigned char*>(convImage->GetData());
					if (camImg == NULL)	{
						Utility::printLine("ERROR: Image Acquisition has failed!");
						continue;
					}
					// Display cam image
					if (this->displayCamImage) {
						this->camDisplay->UpdateDisplay(camImg);
					}
					this->usingHardware = false;
					// Determine fitness

					double exposureTimesRatio = this->cc->GetExposureRatio();
					double fitness = Utility::FindAverageValue(camImg, convImage->GetWidth(), curImage->GetHeight(), this->cc->targetRadius);

					//Record current performance to file //Ask what kind of calcualtion is this?
					double ms = boardID*dphi + curBinVal / dphi;
					this->timeVsFitnessFile << this->timestamp->MS_SinceStart() << " " << fitness * this->cc->GetExposureRatio() << " " << this->cc->GetExposureRatio() << std::endl;
					this->tfile << ms << " " << fitness * this->cc->GetExposureRatio() << " " << this->cc->GetExposureRatio() << std::endl;

					// Keep record of the best fitness value and image
					if (fitness * exposureTimesRatio > fitValMax) {
						binValMax = curBinVal;
						fitValMax = fitness * exposureTimesRatio;
						this->bestImage->DeepCopy(convImage);
					}
					curImage->Release(); //important to not fill camera buffer

					// Halve the exposure time if over max fitness allowed
					if (fitness > maxFitnessValue) {
						this->cc->HalfExposureTime();
					}
					endOpt = dlg.stopFlag;
				}  // ... curBinVal loop

				if (fitValMax > this->allTimeBestFitness) {
					this->allTimeBestFitness = fitValMax;
					Utility::printLine("INFO: Current best fitness value updated to - " + std::to_string(allTimeBestFitness));
					this->bestImage->DeepCopy(convImage);
				}

				slmImg[binIndex] = binValMax;

				// Save progress data
				lmaxfile << binValMax << " " << fitValMax << std::endl;
				rtime << this->timestamp->MS_SinceStart() << " ms  " << fitValMax << "   " << this->cc->finalExposureTime << std::endl;
			} // ... binRow loop
		} // ... binCol loop

		this->finalImages_.push_back(slmImg);
		slmImg = NULL;
	}
	catch (Spinnaker::Exception &e) {
		Utility::printLine("ERROR: OPT5 ran into issue with board #" + std::to_string(boardID));
		Utility::printLine(std::string(e.what()));
		return false;
	}
	// With no errors, slmImg should now contain optimal image
	return true;
}

bool BruteForce_Optimization::setupInstanceVariables() {
	this->multiEnable_ = false;
	this->dualEnable_ = false;
	if (dlg.m_slmControlDlg.multiEnable.GetCheck() == BST_CHECKED) {
		this->multiEnable_ = true;
	}
	else if (dlg.m_slmControlDlg.dualEnable.GetCheck() == BST_CHECKED) {
		this->dualEnable_ = true;
	}


	this->cc->startCamera(); // setup camera

	this->tfile.open("logs/Opt_functionEvals_vs_fitness.txt", std::ios::app);
	this->timeVsFitnessFile.open("logs/Opt_time_vs_fitness.txt", std::ios::app);

	// Setup displays
	this->camDisplay = new CameraDisplay(this->cc->cameraImageHeight, this->cc->cameraImageWidth, "Camera Display");
	this->slmDisplay = new CameraDisplay(this->sc->getBoardHeight(0), this->sc->getBoardWidth(0), "SLM Display");
	// Setup container for best image
	this->bestImage = Image::Create();

	// Scaler Setup (using base class)
	this->slmScaledImages.clear();
	// Setup the scaled images vector
	this->slmScaledImages = std::vector<unsigned char*>(this->sc->boards.size());
	this->scalers.clear();
	// Setup a vector for every board and initializing all slmScaledImages to 0s
	for (int i = 0; i < sc->boards.size(); i++) {
		this->slmScaledImages[i] = new unsigned char[sc->boards[i]->GetArea()];
		this->scalers.push_back(setupScaler(this->slmScaledImages[i], i));
	}

	this->allTimeBestFitness = 0;

	// Open files for logging algorithm progress 
	this->lmaxfile.open("logs/lmax.txt", std::ios::app);
	this->rtime.open("logs/Opt_rtime.txt", std::ios::app);

	return true;
}

bool BruteForce_Optimization::shutdownOptimizationInstance() {
	// - log files close
	this->timeVsFitnessFile.close();
	this->tfile.close();
	// Generic file renaming to include time stamps
	std::string curTime = Utility::getCurTime();
	std::rename("logs/Opt_functionEvals_vs_fitness.txt", ("logs/" + curTime + "_OPT5_functionEvals_vs_fitness.txt").c_str());
	std::rename("logs/Opt_time_vs_fitness.txt", ("logs/" + curTime + "_OPT5_time_vs_fitness.txt").c_str());
	std::rename("logs/lmax.txt", ("logs/" + curTime + "_OPT5_lmax.txt").c_str());
	std::rename("logs/Opt_rtime.txt", ("logs/" + curTime + "_OPT5_rtime.txt").c_str());
	saveParameters(curTime, "OPT5");

	//Record total time taken for optimization
	curTime = Utility::getCurTime();
	std::ofstream tfile2("logs/" + curTime + "_OPT5_time.txt", std::ios::app);
	tfile2 << this->timestamp->MS_SinceStart() << std::endl;
	tfile2.close();

	// Save how final optimization looks through camera
	unsigned char* camImg = static_cast<unsigned char*>(this->bestImage->GetData());
	Mat Opt_ary = Mat(this->bestImage->GetHeight(), this->bestImage->GetWidth(), CV_8UC1, camImg);
	cv::imwrite("logs/" + curTime + "_OPT5_Optimized.bmp", Opt_ary);

	this->lmaxfile.close();
	this->rtime.close();

	// - camera shutdown
	this->cc->stopCamera();
	this->cc->shutdownCamera();

	// - memory deallocation
	this->camDisplay->CloseDisplay();
	this->slmDisplay->CloseDisplay();
	delete this->camDisplay;
	delete this->slmDisplay;
	delete this->timestamp;

	// Delete all the scalers in the vector
	for (int i = 0; i < this->scalers.size(); i++) {
		delete this->scalers[i];
	}
	this->scalers.clear();
	// Delete all the scaled image pointers in the vector
	for (int i = 0; i < this->slmScaledImages.size(); i++) {
		delete[] this->slmScaledImages[i];
	}
	this->slmScaledImages.clear();

	//Record the final (most fit) slm images followed by deleting them
	for (int i = this->finalImages_.size(); i >= 0; i--) {
		// Save image
		Mat m_ary = Mat(512, 512, CV_8UC1, this->finalImages_[i]);
		cv::imwrite("logs/" + curTime + "_OPT5_phaseopt.bmp", m_ary);

		delete[] this->finalImages_[i]; // deallocate
		this->finalImages_.pop_back();  // remove from vector
	}
	this->finalImages_.clear();
	
	//Reset UI State
	this->isWorking = false;
	this->dlg.disableMainUI(!isWorking);

	return true;
}

void BruteForce_Optimization::setBlankSlmImg() {
	for (int y = 0; y < this->cc->numberOfBinsY; y++) {
		for (int x = 0; x < this->cc->numberOfBinsX; x++) {
			int index = (x + y*this->cc->numberOfBinsX)*this->cc->populationDensity;
			this->slmImg_[index] = 0;
		}
	}
}
