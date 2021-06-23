////////////////////
// Optimization handler methods implementation for brute force algorithm
// Last edited: 06/18/2021 by Andrew O'Kins
////////////////////
#include "stdafx.h"						// Required in source
#include "BruteForce_Optimization.h"	// Header file

#include "Utility.h"
#include "Timing.h"
#include "CamDisplay.h"

#include "MainDialog.h"
#include "SLMController.h"
#include "CameraController.h"
#include "Blink_SDK.h"
#include "SLM_Board.h"

#include <string>
#include <chrono>
#include <string>
using std::ofstream;
using namespace cv;

// TODO: Consider how to go about using multi-SLM approach with this algorithm

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
	//Declare variables
	// TODO: which of these can be combined which can be deleted or implified ALSO make them understandable!
	double dblN;
	int binCol, binRow, nn, mm, slmWidth, slmHeight, ll, lMax, kk;
	int bin, dphi;
	double allTimeBestFitness = 0;
	double rMax;
	double exposureTimesRatio;
	// set values
	slmWidth = this->sc->getBoardWidth(0);
	slmHeight = this->sc->getBoardHeight(0);
	bin = 8;
	dblN = (slmWidth / bin) * (slmWidth / bin);
	dphi = 16;

	//Initialize array with 0
	for (binCol = 0; binCol < slmWidth; binCol++) {
		for (binRow = 0; binRow < slmHeight; binRow++) {
			kk = binCol * 512 + binRow;
			this->slmImg[kk] = 0;
		}
	}

	// Generation monitoring variables
	int index = 0;
	double fitness = 0;

	// Instance variables we need
	ImagePtr convImage, curImage;
	// Creating displays if desired
	if (displayCamImage) {
		this->camDisplay->OpenDisplay();
	}
	if (displaySLMImage) {
		this->slmDisplay->OpenDisplay();
	}
	try	{
		this->timestamp = new TimeStampGenerator();
		bool endOpt = false; // Initialy assume we are not going to end the optimziation
		// Iterate over bins
		for (binCol = 0; binCol < slmWidth && endOpt != true; binCol += bin) { // row
			for (binRow = 0; binRow < slmHeight && endOpt != true; binRow += bin) { // column
				lMax = 0;
				rMax = 0;
				//find max phase for this bin
				for (ll = 0; ll < 256 && endOpt != true; ll += dphi) {
					//bin phase
					for (nn = 0; nn < bin; nn++) {
						for (mm = 0; mm < bin; mm++) {
							kk = (binCol + nn) * slmWidth + (mm + binRow);
							this->slmImg[kk] = ll;
						}
					}
					this->usingHardware = true;
					// Update board with new images

					for (int i = 1; i <= this->sc->boards.size(); i++) {
						this->sc->blink_sdk->Write_image(i, this->slmImg, this->sc->getBoardHeight(i - 1), false, false, 0);
					}
					//Acquire and display camera image
					this->cc->AcquireImages(curImage, convImage);

					unsigned char* camImg = static_cast<unsigned char*>(convImage->GetData());
					if (camImg == NULL)	{
						Utility::printLine("ERROR: Image Acquisition has failed!");
						continue;
					}
					if (this->displayCamImage) {
						this->camDisplay->UpdateDisplay(camImg);
					}
					this->usingHardware = false;

					exposureTimesRatio = this->cc->GetExposureRatio();
					fitness = Utility::FindAverageValue(camImg, convImage->GetWidth(), curImage->GetHeight(), this->cc->targetRadius);

					//Record current performance to file //Ask what kind of calcualtion is this?
					double ms = index*dphi + ll / dphi;
					this->timeVsFitnessFile << this->timestamp->MS_SinceStart() << " " << fitness * this->cc->GetExposureRatio() << " " << this->cc->GetExposureRatio() << std::endl;
					this->tfile << ms << " " << fitness * this->cc->GetExposureRatio() << " " << this->cc->GetExposureRatio() << std::endl;

					// Keep record of the best fitness value and image
					if (fitness * exposureTimesRatio > rMax) {
						lMax = ll;
						rMax = fitness * exposureTimesRatio;
						this->bestImage->DeepCopy(convImage);
					}
					curImage->Release(); //important to not fill camera buffer

					// Halve the exposure time if over max fitness allowed
					if (fitness > maxFitnessValue) {
						this->cc->HalfExposureTime();
					}
					endOpt = dlg.stopFlag;
				}

				//Print current optimization progress to conosle
				if (rMax > allTimeBestFitness)	{
					allTimeBestFitness = rMax;
					Utility::printLine("INFO: Current best fitness value updated to - " + std::to_string(allTimeBestFitness));
				}

				//set maximal phase
				for (nn = 0; nn < bin; nn++) {
					for (mm = 0; mm < bin; mm++) {
						kk = (binCol + nn) * slmWidth + (mm + binRow);
						this->slmImg[kk] = lMax;
					}
				}
				lmaxfile << lMax << " " << rMax << std::endl;

				// Save progress data
				rtime << this->timestamp->MS_SinceStart() << " ms  " << lMax << "   " << this->cc->finalExposureTime << std::endl;
				index += 1;
			}
		}
	}
	catch (Spinnaker::Exception &e) {
		Utility::printLine("ERROR: " + string(e.what()));
	}
	// Cleanup
	return shutdownOptimizationInstance();;
}

bool BruteForce_Optimization::setupInstanceVariables() {
	//this->dlg.m_slmControlDlg.dualEnable.GetCheck() == CHECKED;

	this->slmImg = new unsigned char[this->sc->getBoardWidth(0) * this->sc->getBoardHeight(0) * 1];// Initialize array for storing slm images

	this->cc->startCamera(); // setup camera

	this->tfile.open("logs/Opt_functionEvals_vs_fitness.txt", std::ios::app);
	this->timeVsFitnessFile.open("logs/Opt_time_vs_fitness.txt", std::ios::app);

	// Setup displays
	this->camDisplay = new CameraDisplay(this->cc->cameraImageHeight, this->cc->cameraImageWidth, "Camera Display");
	this->slmDisplay = new CameraDisplay(this->sc->getBoardHeight(0), this->sc->getBoardWidth(0), "SLM Display");
	// Setup container for best image
	this->bestImage = Image::Create();

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

	//Record the final (most fit) slm image
	Mat m_ary = Mat(512, 512, CV_8UC1, this->slmImg);
	cv::imwrite("logs/" + curTime + "_OPT5_phaseopt.bmp", m_ary);

	this->lmaxfile.close();
	this->rtime.close();

	delete[] this->slmImg;
	// - camera shutdown
	this->cc->stopCamera();
	this->cc->shutdownCamera();

	// - memory deallocation
	this->camDisplay->CloseDisplay();
	this->slmDisplay->CloseDisplay();
	delete this->camDisplay;
	delete this->slmDisplay;
	delete this->timestamp;

	//Reset UI State
	this->isWorking = false;
	this->dlg.disableMainUI(!isWorking);

	return true;
}
