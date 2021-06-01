
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
#include <fstream>				// used to export information to file for debugging
#include <chrono>
#include <thread>
#include <memory>
using std::ostringstream;
using std::ofstream;
using namespace cv;

////
// TODOs:	Refactor the code, breaking runOptimization() content down into submethods in preparation for multithreading and improved readability
//			Add support for multiple boards
//			Address how to handle if image acquisition failed (currently just moves on to next individual without assigning a default fitness value)
//			send final scaled image to the SLM //ASK needed?
//			Remove unneeded file i/o once debugging is complete

bool SGA_Optimization::runOptimization() {
	Utility::printLine("SGA BUTTON PRESSED!");

    //Setup before optimization (see base class for implementation)
	if (!prepareSoftwareHardware())
		Utility::printLine("ERROR: Failed to prepare software or/and hardware for SGA Optimization");

	try {// Begin Camera Exception Handling
        //Create initial population of images
		int populationSize = 30;
		int eliteSize = 5;
		SGAPopulation<int> population(cc->numberOfBinsY * cc->numberOfBinsX * cc->populationDensity, populationSize, eliteSize, acceptedSimilarity); // current population being used

		//Find genome length
		int genomeLength = sc->getBoardWidth(0) * sc->getBoardHeight(0) * 1;
		if (genomeLength == -1) {
			Utility::printLine("ERROR: Genome Length Cannot Be -1");
			return false;
		}

		int imageLength = cc->cameraImageHeight * cc->cameraImageWidth;
		if (imageLength == -1) {
			Utility::printLine("ERROR: Image Length Cannot Be -1");
			return false;
		}

		unsigned char *aryptr = new unsigned char[genomeLength];
		unsigned char *camImg = new unsigned char;
		std::shared_ptr<std::vector<int>> tempptr;
		ImagePtr curImage = Image::Create();
		ImagePtr convImage = Image::Create();
		double exposureTimesRatio = 1.0;

        // Generation monitoring variables
		double fitness = 0;
		bool shortenExposureFlag = false;
		bool stopConditionsMetFlag = false;

        //Open up files to which progress would be logged
        ofstream tfile("logs/SGA_functionEvals_vs_fitness.txt", std::ios::app);
		ofstream timeVsFitnessFile("logs/SGA_time_vs_fitness.txt", std::ios::app);
		TimeStampGenerator timestamp;

        // Setup image displays for camera and SLM
		bool displayCamImage = true;
		bool displaySLMImage = false; //TODO: only first SLM right now - add functionality to display any or all boards
		CameraDisplay* camDisplay = new CameraDisplay(cc->cameraImageHeight, cc->cameraImageWidth, "Camera Display");
		CameraDisplay* slmDisplay = new CameraDisplay(sc->getBoardHeight(0), sc->getBoardWidth(0), "SLM Display");
		if (displayCamImage)
			camDisplay->OpenDisplay();
		if (displaySLMImage)
			slmDisplay->OpenDisplay();

		// Scaler Setup (using base class)
		ImageScaler* scaler = setupScaler(aryptr);
		int lastImgWidth, lastImgHeight;

		cc->startCamera();
		
		for (int curr_gen = 0; curr_gen < maxGenenerations; curr_gen++)
        {
			for (int indID = 0; indID < population.getSize(); indID++) {
                //Apply LUT/Binning to randomly the generated individual's image
				tempptr = std::make_shared<int>(population.getImage(indID)); // Get the image for the individual
				scaler->TranslateImage(tempptr, aryptr);

				//Write translated image to SLM boards //TODO: modify as it assumes boards get the same image and have same size
				for (int i = 1; i <= sc->boards.size(); i++)
					sc->blink_sdk->Write_image(i, aryptr, sc->getBoardHeight(i-1), false, false, 0.0);

				//Wait for camera's next image to be the SLM's projection (wait atleast one frame)
				//Sleep(cc->frameRateMS * 2);
				//Utility::printLine("INFO: waited " + std::to_string(cc->frameRateMS) + " to ensure correct image is taken!");

                //Acquire images // - take image and determine fitness
				cc->AcquireImages(curImage, convImage);
				lastImgWidth = convImage->GetWidth();
				lastImgHeight = convImage->GetHeight();
				camImg = static_cast<unsigned char*>(convImage->GetData());
				if (camImg == NULL) { // TODO: I think this is dangerous as Individual does not have default fitness if left unevaluated
					Utility::printLine("ERROR: Image Acquisition has failed!");
					continue;
				}
				exposureTimesRatio = cc->GetExposureRatio();	//needed for proper fitness value logging
				fitness = Utility::FindAverageValue(camImg, lastImgWidth, lastImgHeight, cc->targetRadius);

				if (indID == 0 && displayCamImage)
                    camDisplay->UpdateDisplay(camImg);

				// Record values for the top six individuals in each generation
				if (indID > 23)
					timeVsFitnessFile << timestamp.MS_SinceStart() << " " << fitness*exposureTimesRatio << " " << cc->finalExposureTime << " " << exposureTimesRatio << std::endl;

                //Save elite info of last generation
				if (indID == 29) {
					tfile << "SGA GENERATION:" << curr_gen << " " << fitness*exposureTimesRatio << std::endl;
					if (saveImages)
						cc->saveImage(convImage, (curr_gen + 1));
                }
				try {
					curImage->Release(); //important to not fill camera buffer
				}
				catch (Spinnaker::Exception &e) {
					Utility::printLine("WARNING: no need to release image at this point");
				}

                // Check stop conditions
				stopConditionsMetFlag = stopConditionsReached((fitness*exposureTimesRatio), timestamp.S_SinceStart(), curr_gen + 1);

				// Update fitness for this individual
				population.setFitness(indID, fitness * exposureTimesRatio);
				// If the fitness value is too high, flag that the exposure needs to be shortened
				if (fitness > maxFitnessValue) shortenExposureFlag = true;
            }

            //Create a more fit generation
			population.StartNextGeneration();

			// Half exposure time if fitness value is too high
			if (shortenExposureFlag) {
				cc->HalfExposureTime();
				shortenExposureFlag = false;
				ofstream efile("logs/exposure.txt", std::ios::app);
				efile << "Exposure shortened after gen: " << curr_gen + 1 << std::endl;
				efile.close();
			}

			if (stopConditionsMetFlag)
				break; // End optimization loop
        }

		// Save how final optimization looks through camera
		std::string curTime = Utility::getCurTime();
		Mat Opt_ary = Mat(lastImgHeight, lastImgWidth, CV_8UC1, camImg);
		cv::imwrite("logs/" + curTime + "_SGA_Optimized.bmp", Opt_ary);

		// Save final (most fit SLM image)
		tempptr = std::make_shared<int>(population.getImage(population.getSize() - 1)); // Get the image for the individual (most fit)
		scaler->TranslateImage(tempptr, aryptr);
		Mat m_ary = Mat(512, 512, CV_8UC1, aryptr);
		cv::imwrite("logs/" + curTime + "_SGA_phaseopt.bmp", m_ary);

		// Cleanup
		// - image displays
        camDisplay->CloseDisplay();
        slmDisplay->CloseDisplay();
		delete camDisplay;
		delete slmDisplay;
		delete scaler;
		delete[] aryptr;
		// - fitness logging files
		timeVsFitnessFile << timestamp.MS_SinceStart() << " " << 0 << std::endl;
		timeVsFitnessFile.close();
		tfile.close();
		// - camera
		cc->stopCamera();
		cc->shutdownCamera();

		// Generic file renaming to have time stamps
		std::rename("logs/SGA_functionEvals_vs_fitness.txt", ("logs/" + curTime + "_SGA_functionEvals_vs_fitness.txt").c_str());
		std::rename("logs/SGA_time_vs_fitness.txt", ("logs/" + curTime + "_SGA_time_vs_fitness.txt").c_str());
		std::rename("logs/exposure.txt", ("logs/" + curTime + "_SGA_exposure.txt").c_str());
		saveParameters(curTime, "SGA");
    }
    catch (Spinnaker::Exception &e) {
		Utility::printLine("ERROR: " + string(e.what()));
	}
    //Reset UI State
	isWorking = false;
	dlg.disableMainUI(!isWorking);
	return true;
}
