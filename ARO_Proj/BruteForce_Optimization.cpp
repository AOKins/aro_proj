
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

#include <fstream>				// used to export information to file 
#include <chrono>
#include <string>
using std::ofstream;
using namespace cv;

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
	int ii, jj, nn, mm, iMax, jMax, ll, lMax, kk;
	int bin, dphi;
	double allTimeBestFitness = 0;
	double rMax;
	double exposureTimesRatio;

	//set variables TODO: determine wich are needed and whch are in controllers
	iMax = 512;
	jMax = 512;
	bin = 8;
	dblN = (iMax / bin) * (iMax / bin);
	dphi = 16;

	// Open files for logging algorithm progress 
	ofstream lmaxfile("logs/lmax.txt", std::ios::app);

	//Find genome length
	int genomeLength = sc->getBoardWidth(0) * sc->getBoardHeight(0) * 1;
	if (genomeLength == -1)	{
		Utility::printLine("ERRROR: Genome Length Cannot Be -1");
		return false;
	}

	//Initialize array
	for (ii = 0; ii < iMax; ii++) {
		for (jj = 0; jj < jMax; jj++) {
			kk = ii * 512 + jj;
			this->aryptr[kk] = 0;
		}
	}

	// Generation monitoring variables
	int index = 0;
	double fitness = 0;

	// Creating displays if desired
	if (displayCamImage) {
		this->camDisplay->OpenDisplay();
	}
	if (displaySLMImage) {
		this->slmDisplay->OpenDisplay();
	}
	try	{
		this->timestamp = new TimeStampGenerator();
		// Iterate over bins
		for (ii = 0; ii < iMax; ii += bin) {
			for (jj = 0; jj < jMax; jj += bin)	{
				lMax = 0;
				rMax = 0;
				//find max phase for bin
				for (ll = 0; ll < 256; ll += dphi)	{
					//bin phase
					for (nn = 0; nn < bin; nn++) {
						for (mm = 0; mm < bin; mm++) {
							kk = (ii + nn) * iMax + (mm + jj);
							this->aryptr[kk] = ll;
						}
					}
					//Update board with new image
					for (int i = 1; i <= this->sc->boards.size(); i++) {
						this->sc->blink_sdk->Write_image(i, this->aryptr, this->sc->getBoardHeight(i - 1), false, false, 0.0);
					}
					//Acquire and display camera image
					this->cc->AcquireImages(this->curImage, this->convImage);
					this->camImg = static_cast<unsigned char*>(this->convImage->GetData());
					if (this->camImg == NULL)	{
						Utility::printLine("ERROR: Image Acquisition has failed!");
						continue;
					}
					if (this->displayCamImage) {
						this->camDisplay->UpdateDisplay(camImg);
					}
					exposureTimesRatio = this->cc->GetExposureRatio();
					fitness = Utility::FindAverageValue(this->camImg, this->convImage->GetWidth(), this->curImage->GetHeight(), this->cc->targetRadius);
					this->curImage->Release(); //important to not fill camera buffer

					//Record current performance to file //Ask what kind of calcualtion is this?
					double ms = index*dphi + ll / dphi;
					this->timeVsFitnessFile << this->timestamp->MS_SinceStart() << " " << fitness * this->cc->GetExposureRatio() << " " << this->cc->GetExposureRatio() << std::endl;
					this->tfile << ms << " " << fitness * this->cc->GetExposureRatio() << " " << this->cc->GetExposureRatio() << std::endl;

					// Keep record of the best fitness value and 
					if (fitness * exposureTimesRatio > rMax) {
						lMax = ll;
						rMax = fitness * exposureTimesRatio;
					}

					// Halve the exposure time if over max fitness allowed
					if (fitness > maxFitnessValue) {
						this->cc->HalfExposureTime();
					}

				}

				//Print current optimization progress to conosle
				if (rMax > allTimeBestFitness)	{
					allTimeBestFitness = rMax;
					Utility::printLine("INFO: Current best fitness value updated to - " + std::to_string(allTimeBestFitness));
				}

				//set maximal phase
				for (nn = 0; nn < bin; nn++) {
					for (mm = 0; mm < bin; mm++) {
						kk = (ii + nn) * iMax + (mm + jj);
						aryptr[kk] = lMax;
					}
				}

				lmaxfile << lMax << " " << rMax << std::endl;

				/// Save image as jpeg
				///	ostringstream filename;
				/// filename << index << ".jpg";
				/// pImage->Save(filename.str().c_str());*/

				// Save progress data
				ofstream rtime;
				rtime.open("logs/Opt_rtime.txt", std::ios::app);
				rtime << this->timestamp->MS_SinceStart() << " ms  " << lMax << "   " << this->cc->finalExposureTime << std::endl;
				rtime.close();

				index += 1;
			}

			//See if this is supposed to be here 
			if (this->timestamp->S_SinceStart() > this->secondsToStop){
				break;
			}
		}

		//Record total time taken for optimization
		std::string curTime = Utility::getCurTime();
		ofstream tfile2("logs/" + curTime + "_OPT5_time.txt", std::ios::app);
		tfile2 << this->timestamp->MS_SinceStart() << std::endl;
		tfile2.close();

		// Save how final optimization looks through camera
		Mat Opt_ary = Mat(this->curImage->GetHeight(), this->curImage->GetWidth(), CV_8UC1, this->camImg);
		cv::imwrite("logs/" + curTime + "_OPT5_Optimized.bmp", Opt_ary);

		//Record the final (most fit) slm image
		Mat m_ary = Mat(512, 512, CV_8UC1, this->aryptr);
		imwrite("logs/" + curTime + "_OPT5_phaseopt.bmp", m_ary);

	}
	catch (Spinnaker::Exception &e) {
		Utility::printLine("ERROR: " + string(e.what()));
	}

	//Cleanup
	lmaxfile.close();
	shutdownOptimizationInstance();
	
	//Reset UI State
	this->isWorking = false;
	this->dlg.disableMainUI(!isWorking);

	return true;
}

bool BruteForce_Optimization::setupInstanceVariables() {
	this->slmLength = this->sc->getBoardWidth(0) * this->sc->getBoardHeight(0) * 1;
	// Allocate memory to store used images
	this->aryptr = new unsigned char[slmLength];
	this->camImg = new unsigned char;

	this->cc->startCamera();

	this->tfile.open("logs/Opt_functionEvals_vs_fitness.txt", std::ios::app);
	this->timeVsFitnessFile.open("logs/Opt_time_vs_fitness.txt", std::ios::app);

	this->camDisplay = new CameraDisplay(this->cc->cameraImageHeight, this->cc->cameraImageWidth, "Camera Display");
	this->slmDisplay = new CameraDisplay(this->sc->getBoardHeight(0), this->sc->getBoardWidth(0), "SLM Display");

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

	// - camera shutdown
	this->cc->stopCamera();
	this->cc->shutdownCamera();
	this->curImage = Image::Create();
	this->convImage = Image::Create();

	// - memory deallocation
	this->camDisplay->CloseDisplay();
	this->slmDisplay->CloseDisplay();
	delete this->camDisplay;
	delete this->slmDisplay;
	delete[] this->aryptr;
	delete this->timestamp;

	return true;
}