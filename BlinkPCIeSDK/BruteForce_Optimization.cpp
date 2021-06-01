
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
#include <thread>
#include <string>
using std::ofstream;
using std::ostringstream;
using namespace cv;


bool BruteForce_Optimization::runOptimization() {
	Utility::printLine("OPT 5 BUTTON CLICKED!");

	//Setup before optimization (see base class for implementation)
	if (!prepareSoftwareHardware())
		Utility::printLine("ERROR: Failed to prepare software or/and hardware for Brute Force Optimization");

	//Declare variables
	//TODO: which of these can be combined which can be deeleted or implified ALSO make them understandable!
	double exptime, PC, crop, dexptime, dblN;
	int ii, jj, nn, mm, iMax, jMax, ll, lMax, kk;
	int bin, dphi;
	double allTimeBestFitness = 0;
	double rMax;

	//set variables TODO: determine wich are needed and whch are in controllers
	iMax = 512;
	jMax = 512;
	bin = 8;
	dblN = (iMax / bin) * (iMax / bin);
	dphi = 16;

	// Open files for logging algorithm progress 
	ofstream tfile("logs/Opt_functionEvals_vs_fitness.txt", std::ios::app);
	ofstream timeVsFitnessFile("logs/Opt_time_vs_fitness.txt", std::ios::app);
	ofstream lmaxfile("logs/lmax.txt", std::ios::app);
	TimeStampGenerator timestamp;

	//Find genome length
	int genomeLength = sc->getBoardWidth(0) * sc->getBoardHeight(0) * 1;
	if (genomeLength == -1)	{
		Utility::printLine("ERRROR: Genome Length Cannot Be -1");
		return false;
	}

	// Allocate memory to store used images
	unsigned char *aryptr = new unsigned char[genomeLength];
	unsigned char *camImg = new unsigned char;

	ImagePtr curImage = Image::Create();
	ImagePtr convImage = Image::Create();
	double exposureTimesRatio = 1.0;

	//Initialize "2D" array
	for (ii = 0; ii < iMax; ii++) {
		for (jj = 0; jj < jMax; jj++) {
			kk = ii * 512 + jj;
			aryptr[kk] = 0;
		}
	}

	// Generation monitoring variables
	int index = 0;
	double fitness = 0;
	bool shortenExposureFlag = false;
	bool stopConditionsMetFlag = false;

	// Setup image displays for camera and SLM
	bool displayCamImage = true;
	bool displaySLMImage = false; //TODO: only first SLM right now - add functionality to display any or all boards
	CameraDisplay* camDisplay = new CameraDisplay(cc->cameraImageHeight, cc->cameraImageWidth, "Camera Display");
	CameraDisplay* slmDisplay = new CameraDisplay(sc->getBoardHeight(0), sc->getBoardWidth(0), "SLM Display");
	if (displayCamImage)
		camDisplay->OpenDisplay();
	if (displaySLMImage)
		slmDisplay->OpenDisplay();
	
	try	{
		time_t start = time(0);
		cc->startCamera();

		// Iterate over bins
		for (ii = 0; ii < iMax; ii += bin) {
			for (jj = 0; jj < jMax; jj += bin)	{
				lMax = 0;
				rMax = 0;
				const clock_t begin = clock();
				//find max phase for bin
				for (ll = 0; ll < 256; ll += dphi)	{
					//bin phase
					for (nn = 0; nn < bin; nn++) {
						for (mm = 0; mm < bin; mm++) {
							kk = (ii + nn) * iMax + (mm + jj);
							aryptr[kk] = ll;
						}
					}

					//Update board with new image
					for (int i = 1; i <= sc->boards.size(); i++)
						sc->blink_sdk->Write_image(i, aryptr, sc->getBoardHeight(i-1), false, false, 0.0);

					//Acquire and display camera image
					cc->AcquireImages(curImage, convImage);
					camImg = static_cast<unsigned char*>(convImage->GetData());
					if (camImg == NULL)	{
						Utility::printLine("ERROR: Image Acquisition has failed!");
						continue;
					}
					if (displayCameraImage)
						camDisplay->UpdateDisplay(camImg);
					exposureTimesRatio = cc->GetExposureRatio();
					fitness = Utility::FindAverageValue(camImg, convImage->GetWidth(), curImage->GetHeight(), cc->targetRadius);
					curImage->Release(); //important to not fill camera buffer

					//Record current performance to file //Ask what kind of calcualtion is this?
					double ms = index*dphi + ll / dphi;
					timeVsFitnessFile << timestamp.MS_SinceStart() << " " << fitness * cc->GetExposureRatio() << " " << cc->GetExposureRatio() << std::endl;
					tfile << ms << " " << fitness * cc->GetExposureRatio() << " " << cc->GetExposureRatio() << std::endl;

					// Keep record of the best fitness value and 
					if (fitness * exposureTimesRatio > rMax) {
						lMax = ll;
						rMax = fitness * exposureTimesRatio;	
					}

					// Halve the exposure time if over max fitness allowed
					if (fitness > maxFitnessValue)
						cc->HalfExposureTime();

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
				rtime << float(clock() - begin) << "   " << lMax << "   " << cc->finalExposureTime << std::endl;
				rtime.close();

				index += 1;
			}

			//See if this is supposed to be here 
			if (timestamp.S_SinceStart() > secondsToStop)
				break;
		}
		time_t end = time(0);

		//Record total time taken for optimization
		std::string curTime = Utility::getCurTime();
		double dt = difftime(end, start);
		ofstream tfile2("logs/" + curTime + "_OPT5_time.txt", std::ios::app);
		tfile2 << dt << std::endl;
		tfile2.close();

		// Save how final optimization looks through camera
		Mat Opt_ary = Mat(curImage->GetHeight(), curImage->GetWidth(), CV_8UC1, camImg);
		cv::imwrite("logs/" + curTime + "_OPT5_Optimized.bmp", Opt_ary);

		//Record the final (most fit) slm image
		Mat m_ary = Mat(512, 512, CV_8UC1, aryptr);
		imwrite("logs/" + curTime + "_OPT5_phaseopt.bmp", m_ary);

		//Cleanup
		// - camera shutdown
		cc->stopCamera();
		cc->shutdownCamera();
		// - log files close
		timeVsFitnessFile.close();
		tfile.close();
		lmaxfile.close();
		// - memory deallocation
		camDisplay->CloseDisplay();
		slmDisplay->CloseDisplay();
		delete camDisplay;
		delete slmDisplay;
		delete[] aryptr;

		// Generic file renaming to include time stamps
		std::rename("logs/Opt_functionEvals_vs_fitness.txt", ("logs/" + curTime + "_OPT5_functionEvals_vs_fitness.txt").c_str());
		std::rename("logs/Opt_time_vs_fitness.txt", ("logs/" + curTime + "_OPT5_time_vs_fitness.txt").c_str());
		std::rename("logs/lmax.txt", ("logs/" + curTime + "_OPT5_lmax.txt").c_str());
		std::rename("logs/Opt_rtime.txt", ("logs/" + curTime + "_OPT5_rtime.txt").c_str());
		saveParameters(curTime, "OPT5");
	}
	catch (Spinnaker::Exception &e) {
		Utility::printLine("ERROR: " + string(e.what()));
	}

	//Reset UI State
	isWorking = false;
	dlg.disableMainUI(!isWorking);

	return true;
}
