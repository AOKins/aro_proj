
#include "stdafx.h"				// Required in source
#include "uGA_Optimization.h"	// Header file
#include "Population.h"

#include "Utility.h"
#include "Timing.h"
#include "CamDisplay.h"

#include "MainDialog.h"
#include "SLMController.h"
#include "SLM_Board.h"
#include "CameraController.h"
#include "Blink_SDK.h"

#include <fstream>				// used to export information to file 
#include <chrono>
#include <thread>
#include <string>
using std::string;
using std::ofstream;
using std::ostringstream;
using namespace cv;

bool uGA_Optimization::runOptimization()
{
	//Utility::printLine("UGA BUTTON CLICKED!");

	////Setup before optimization (see base class for implementation)
	//if (!prepareSoftwareHardware())
	//	Utility::printLine("ERROR: Failed to prepare software or/and hardware for UGA Optimization");

	//try // Begin Camera Exception Handling
	//{
	//	// Create initial "individual" images that form the small population
	//	Population<int> population(cc->numberOfBinsX, cc->numberOfBinsY, 1, acceptedSimilarity);
	//	
	//	unsigned char *camImg = new unsigned char;
	//	unsigned char *aryptr = new unsigned char[cc->cameraImageWidth * cc->cameraImageHeight];
	//	double exposureTimesRatio = 1.0;

	//	// Generation monitoring variables
	//	double fitness = 0;
	//	bool shortenExposureFlag = false;
	//	bool stopConditionsMetFlag = false;

	//	// Open files to for recording progress info 
	//	ofstream tfile("uGA_generation_vs_fitness.txt", std::ios::app);
	//	ofstream timeVsFitnessFile("uGA_time_vs_fitness.txt", std::ios::app);
	//	TimeStampGenerator timestamp;

	//	// Setup image displays for camera and SLM
	//	bool displayCamImage = true;
	//	bool displaySLMImage = false; //TODO: only first SLM right now - add functionality to display any or all boards
	//	CameraDisplay* camDisplay = new CameraDisplay(cc->cameraImageHeight, cc->cameraImageWidth, "Camera Display");
	//	//CameraDisplay* slmDisplay = new CameraDisplay(sc->boards[0]->imageHeight, sc->boards[0]->imageWidth, "SLM Display");
	//	ASSERT(camDisplay != NULL);	//TODO: convert to normal validations with messages for user
	//	//ASSERT(slmDisplay != NULL);
	//	if (displayCamImage)
	//		camDisplay->OpenDisplay();
	//	//if (displaySLMImage)
	//	//	slmDisplay->OpenDisplay();

	//	//Begin recording
	//	if (!cc->startCamera())
	//	{
	//		Utility::printLine("ERROR: was unable to start the camera!");
	//		return false;
	//	}

	//	///HUGE TODO: make this a part of slm controller
	//	for (int i = 0; i < maxGenenerations; i++)
	//	{//each generation
	//		for (int j = 0; j < population.GetSize(); j++)
	//		{//each individual

	//			if (sc->scaler == NULL)
	//			{
	//				return false;
	//				Utility::printLine("ERROR: Image Scaler in slm controller Not Avaliable!");
	//			}

	//			Utility::printLine("UGA _ 0");

	//			//Apply LUT/Binning to randomly the generated individual's image
	//			sc->scaler->TranslateImage(population.GetImage(), aryptr);



	//			Utility::printLine("UGA _ 1");

	//			//Write translated image to SLM boards
	//			for (int i = 1; i <= sc->boards.size(); i++)
	//				sc->blink_sdk->Write_image(i, aryptr, sc->boards[i - 1]->imageHeight, false, false, 0.0);

	//			Utility::printLine("UGA _ 2");

	//			//Acquire images
	//			// - determine if currently testing last generation's elite
	//			bool saveImage = false;
	//			if (j == 0 && saveImages) // if current elite and image saving is enabled
	//				saveImage = true;

	//			Utility::printLine("UGA _ 3");

	//			// - take image and determine fitness
	//			ImagePtr curImage = Image::Create();
	//			curImage = cc->AcquireImages();
	//			camImg = static_cast<unsigned char*>(curImage->GetData());
	//			if (camImg == NULL)
	//			{
	//				Utility::printLine("ERROR: Image Acquisition has failed!");
	//				continue;
	//			}

	//			exposureTimesRatio = cc->GetExposureRatio();	//needed for proper fitness value logging
	//			fitness = Utility::FindAverageValue(camImg, cc->targetMatrix, cc->cameraImageWidth, cc->cameraImageHeight);

	//			Utility::printLine("UGA _ 4");

	//			Utility::printLine("UGA _ 5");

	//			if (j == 1 && displayCamImage)
	//				camDisplay->UpdateDisplay(camImg);

	//			Utility::printLine("UGA _ 6");

	//			// Save the elite information of last generation
	//			if (j == 0)
	//			{
	//				timeVsFitnessFile << timestamp.MS_SinceStart() << " " << fitness*exposureTimesRatio << std::endl;
	//				tfile << "UGA GENERATION:" << i  << " " << fitness*exposureTimesRatio << std::endl;

	//				if (saveImages)
	//					cc->saveImage(curImage, (i+1));
	//			}

	//			// Check stop conditions
	//			stopConditionsMetFlag = stopConditionsReached((fitness*exposureTimesRatio), timestamp.S_SinceStart(), i+1);

	//			// Update population fitness
	//			population.SetFitness(fitness);
	//			if (fitness > maxFitnessValue) shortenExposureFlag = true;

	//		}// for each individual

	//		//Create a more fit generation
	//		population.NextGeneration(i + 1);

	//		// Half exposure time if fitness value is too high
	//		if (shortenExposureFlag)
	//		{
	//			cc->HalfExposureTime();
	//			shortenExposureFlag = false;
	//		}

	//		if (stopConditionsMetFlag)
	//			break;

	//	}//for each generation

	//	// Cleanup
	//	// - image displays
	//	delete camDisplay;
	//	//delete slmDisplay;
	//	delete[] camImg;
	//	delete[] aryptr;
	//	// - fitness logging files
	//	timeVsFitnessFile.close();
	//	tfile.close();
	//	// - camera
	//	cc->endCamera();

	//	// Save final (most fit) SLM image
	//	sc->scaler->TranslateImage(population.GetImage(), aryptr);
	//	Mat m_ary = Mat(512, 512, CV_8UC1, aryptr);
	//	cv::imwrite("uGA_phaseopt.bmp", m_ary);

	//	//TODO: send final scaled image to the SLM
	//}
	//catch (Spinnaker::Exception &e)
	//{
	//	Utility::printLine("ERROR: " + string(e.what()));
	//}

	////Reset UI State
	//isWorking = false;
	//dlg.disableMainUI(!isWorking);

	return true;
}