////////////////////
// Optimization handler methods implementation for simple genetic algorithm
// Last edited: 07/14/2021 by Andrew O'Kins
////////////////////
#include "stdafx.h"				// Required in source
#include "SGA_Optimization.h"	// Header file

////
// TODOs:	Properly Address how to handle if image acquisition failed (currently just moves on to next individual without assigning a default fitness value)
//			send final scaled image to the SLM //ASK needed?

// Method for executing the optimization
// Output: returns true if successful ran without error, false if error occurs
bool SGA_Optimization::runOptimization() {
	Utility::printLine("INFO: Starting SGA Optimization!");

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
	double start = 0;
	double end = 0;
	try {	// Begin general camera exception handling while optimization loop is going
		this->timestamp = new TimeStampGenerator();		// Starting time stamp to track elapsed time
		// Optimization loop for each generation
		for (this->curr_gen = 0; this->curr_gen < this->maxGenenerations && !this->stopConditionsMetFlag; this->curr_gen++) {
			// Run each individual, giving them all fitness values as a result of their genome
			for (int indID = 0; indID < population[0]->getSize(); indID++) {
				if (this->multithreadEnable == true) {
					// Lambda function to access this instance of Optimization to perform runIndividual
					// Input: indID - index location to run individual from in population
					// Captures: this - pointer to current SGA_Optimization instance
					this->ind_threads.push_back(std::thread([this](int indID) {	this->runIndividual(indID); }, indID)); // Parallel
				}
				else {
					this->runIndividual(indID); // Serial
				}
			}
			Utility::rejoinClear(this->ind_threads);

			// Update displays with best individual now that they are done
			if (this->displayCamImage) {
				this->camDisplay->UpdateDisplay(this->bestImage->getRawData<unsigned char>());
			}
			if (this->displaySLMImage) {
				for (int slmID = 0; slmID < this->popCount; slmID++) {
					this->scalers[slmID]->TranslateImage(this->population[slmID]->getGenome(this->populationSize-1), this->slmScaledImages[slmID]);
					this->slmDisplayVector[slmID]->UpdateDisplay(this->slmScaledImages[slmID]);
				}
			}

			// Perform GA crossover/breeding to produce next generation
			for (int popID = 0; popID < population.size(); popID++) {
				if (this->multithreadEnable == true) {
					this->ind_threads.push_back(std::thread([this](int popID) { this->population[popID]->nextGeneration(); }, popID)); // Parallel
				}
				else {
					this->population[popID]->nextGeneration(); // Serial
				}
			}
			Utility::rejoinClear(this->ind_threads);

			// Half exposure time if fitness value is too high
			if (this->shortenExposureFlag) {
				this->cc->HalfExposureTime();
				this->shortenExposureFlag = false;
				if (this->saveExposureShorten || this->logAllFiles) {
					this->efile << "Exposure shortened after gen: " << this->curr_gen + 1 << " with new ratio " << this->cc->GetExposureRatio() << std::endl;
				}
			}
			// Output to the terminal progress to help show progress
			if (this->curr_gen % 10 == 0) {
				Utility::printLine("INFO: Finished generation #" + std::to_string(this->curr_gen) + " with a fitness of " + std::to_string(this->population[0]->getFitness(this->populationSize - 1)));
			}
			if (this->saveTimeVSFitness) {
				end = this->timestamp->MS_SinceStart();
				this->timePerGenFile << this->curr_gen << "," << end - start << std::endl;
				start = end;
			}
		}
		// Cleanup & Save resulting instance
		if (shutdownOptimizationInstance()) {
			Utility::printLine("INFO: Successfully ended optimization instance and saved results");
		}
		else {
			Utility::printLine("WARNING: Failure to properly end optimization instance!");
		}
	}
	catch (std::exception &e) {
		Utility::printLine("ERROR: " + std::string(e.what()));
		return false;
	}
	//Reset UI State
	this->isWorking = false;
	this->dlg.disableMainUI(!isWorking);
	return true;
}

// Method for handling the execution of an individual
// Input: indID - index value for individual in each population being run to determine fitness (for multithreading will be the thread id as well)
// Output: returns false if a critical error occurs, true otherwise
//	individual in population index indID will have assigned fitness according to result from cc
//	lastImgWidth,lastImgHeight updated according to result from cc
//     shortenExposureFlag is set to true if fitness value is high enough
//     stopConditionsMetFlag is set to true if conditions met
bool SGA_Optimization::runIndividual(int indID) {
	try {
		ImageController * curImage = NULL;

		std::unique_lock<std::mutex>  consoleLock(consoleMutex, std::defer_lock);
		std::unique_lock<std::mutex> hardwareLock(hardwareMutex, std::defer_lock);
		std::unique_lock<std::mutex> scalerLock(this->slmScalersMutex, std::defer_lock);

		hardwareLock.lock();
		// Pre end the result for the individual if the stop flag has been raised while waiting
		if (this->dlg.stopFlag == true) {
			hardwareLock.unlock();
			return true;
		}
		// If the hardware boolean is already true, then that means another thread is using this hardware and we have an issue
		if (this->usingHardware) {
			hardwareLock.unlock();
			consoleLock.lock();
			Utility::printLine("ERROR: HARDWARE BEING USED BY OTHER THREAD(S)! Exiting out of this individual");
			consoleLock.unlock();
			return false;
		}
		this->usingHardware = true;

		// Write translated image to SLM boards, assumes there are at least as many boards as populations
		// Multi SLM engaged -> should write to every board (popCount = # of boards)
		// Single SLM -> should only write to board 0 (popCount = 1)
		scalerLock.lock();
		for (int i = 0; i < this->popCount; i++) {
			// Scale the individual genome to fit SLM
			this->scalers[i]->TranslateImage(this->population[i]->getGenome(indID), this->slmScaledImages[i]); // Translate the vector genome into char array image
			// Write to SLM
			this->sc->writeImageToBoard(i, this->slmScaledImages[i]);
		}
		scalerLock.unlock();

		// Acquire image // - take image
		curImage = this->cc->AcquireImage();

		this->usingHardware = false;
		hardwareLock.unlock(); // Now done with the hardware

		// Giving error and ends early if there is no data
		if (curImage == NULL) { // TODO: I think this is dangerous as Individual does not have default fitness if left unevaluated
			consoleLock.lock();
			Utility::printLine("ERROR: Image Acquisition has failed!");
			consoleLock.unlock();
			return false;
		}

		// Getting the image dimensions and data from resulting image
		int imgWidth = curImage->getWidth();
		int imgHeight = curImage->getHeight();
		unsigned char * camImg = curImage->getRawData<unsigned char>();

		// Get current exposure setting of camera (relative to initial)
		double exposureTimesRatio = this->cc->GetExposureRatio();	// needed for proper fitness value across changing exposure time
		// Determine the fitness by intensity of the image within circle of target radius
		double fitness = Utility::FindAverageValue(camImg, imgWidth, imgHeight, this->cc->targetRadius);

		// Record files
		if (this->logAllFiles || this->saveTimeVSFitness) {
			std::unique_lock<std::mutex> tVfLock(this->timeVsFitMutex, std::defer_lock);
			tVfLock.lock();
			this->timeVsFitnessFile << this->timestamp->MS_SinceStart() << "," << fitness*exposureTimesRatio << "," << this->cc->finalExposureTime << "," << exposureTimesRatio << std::endl;
			tVfLock.unlock();
		}
		//Save elite info of last generation
		if (indID == (population[0]->getSize() - 1)) {
			if ((this->saveEliteImages) && (this->curr_gen % this->saveEliteFrequency == 0)) {
				// Save Info
				std::unique_lock<std::mutex> tFileLock(tfileMutex, std::defer_lock);
				tFileLock.lock();
				this->tfile << "SGA GENERATION," << this->curr_gen << "," << fitness*exposureTimesRatio << std::endl;
				tFileLock.unlock();
				// Save image
				std::string curTime = Utility::getCurTime(); // Get current time to use as timeStamp
				this->cc->saveImage(curImage, std::string(this->outputFolder + curTime + "_SGA_Gen_" + std::to_string(this->curr_gen + 1) + "_Elite_Camera" + ".jpg"));
				scalerLock.lock();
				for (int popID = 0; popID < this->population.size(); popID++) {
					scalers[popID]->TranslateImage(this->population[popID]->getGenome(this->population[popID]->getSize() - 1), this->slmScaledImages[popID]);
					cv::Mat m_ary = cv::Mat(512, 512, CV_8UC1, this->slmScaledImages[popID]);
					cv::imwrite(this->outputFolder + curTime + "_SGA_Gen_"+std::to_string(this->curr_gen + 1) +"_Elite_SLM_" + std::to_string(popID) + ".bmp", m_ary);
				}
				scalerLock.unlock();
			}

			// Also save the image as current best regardless (not an output until end of optimization)
			std::unique_lock<std::mutex> imageLock(this->imageMutex, std::defer_lock);
			imageLock.lock();
			this->bestImage = curImage;
			imageLock.unlock();
		}

		// Check stop conditions, only assign true if we reached the condition (race condition if done directly)
		if (stopConditionsReached((fitness*exposureTimesRatio), this->timestamp->S_SinceStart(), this->curr_gen + 1) == true) {
			std::unique_lock<std::mutex> stopLock(this->stopFlagMutex, std::defer_lock);
			stopLock.lock();
			this->stopConditionsMetFlag = true;
			stopLock.unlock();
		}

		// Update fitness for the individuals
		for (int popID = 0; popID < this->population.size(); popID++) {
			this->population[popID]->setFitness(indID, fitness * exposureTimesRatio);
		}
		// If the fitness value is too high, flag that the exposure needs to be shortened
		if (fitness > this->maxFitnessValue) {
			std::unique_lock<std::mutex> expsureFlagLock(exposureFlagMutex, std::defer_lock);
			expsureFlagLock.lock();
			this->shortenExposureFlag = true;
			expsureFlagLock.unlock();
		}
		// If the pointer to current image does not also point to the best image we are safe to delete
		if (curImage != bestImage) {
			delete curImage;
		}
	}
	catch (std::exception &e) {
		Utility::printLine("ERROR: Unidentified error occured while running individual #" + std::to_string(indID));
		Utility::printLine(e.what());
	}
	return true; // No errors!
}

// Method to setup specific properties runOptimziation() instance
bool SGA_Optimization::setupInstanceVariables() {
	// Setting population size as well as number of elite individuals kept in the genetic repopulation
	this->populationSize = 30;
	this->eliteSize = 5;
	this->ind_threads.clear();

	// Set things up accordingly if doing single or multi-SLM
	if (dlg.m_slmControlDlg.multiEnable.GetCheck() == BST_CHECKED) {
		this->popCount = int(sc->boards.size());
	}
	else if (dlg.m_slmControlDlg.dualEnable.GetCheck() == BST_CHECKED) {
		// Will asume that if this is enabled that there are indeed 2 boards (not checking here right now)
		this->popCount = 2;
	}
	else {
		this->popCount = 1;
	}

	// Setting population vector
	this->population.clear();
	for (int i = 0; i < this->popCount; i++) {
		this->population.push_back(new SGAPopulation<int>(this->cc->numberOfBinsY * this->cc->numberOfBinsX * this->cc->populationDensity, this->populationSize, this->eliteSize, this->acceptedSimilarity, this->multithreadEnable));
	}

	this->shortenExposureFlag = false;		// Set to true by individual if fitness is too high
	this->stopConditionsMetFlag = false;	// Set to true if a stop condition was reached by one of the individuals
	this->bestImage = NULL;
	// Setup image displays for camera and SLM
	// Open displays if preference is set
	if (this->displayCamImage) {
		this->camDisplay = new CameraDisplay(this->cc->cameraImageHeight, this->cc->cameraImageWidth, "Camera Display");
		this->camDisplay->OpenDisplay();
	}
	this->slmDisplayVector.clear();
	if (this->displaySLMImage) {
		for (int displayNum = 0; displayNum < this->popCount; displayNum++) {
			this->slmDisplayVector.push_back(new CameraDisplay(this->sc->getBoardHeight(displayNum), this->sc->getBoardWidth(displayNum), ("SLM Display " + std::to_string(displayNum + 1)).c_str()));
		}
		for (int i = 0; i < this->popCount; i++) {
			this->slmDisplayVector[i]->OpenDisplay();
		}
	}
	// Scaler Setup (using base class)
	this->slmScaledImages.clear();
	// Setup the scaled images vector
	this->slmScaledImages = std::vector<unsigned char*>(this->sc->boards.size());
	this->scalers.clear();
	// Setup a vector for every board
	for (int i = 0; i < sc->boards.size(); i++) {
		this->slmScaledImages[i] = new unsigned char[this->sc->boards[i]->GetArea()];
		this->scalers.push_back(setupScaler(this->slmScaledImages[i], i));
	}

	// Start up the camera
	this->cc->startCamera();

	//Open up files to which progress will be logged
	if (this->logAllFiles || this->saveTimeVSFitness) {
		this->timePerGenFile.open(this->outputFolder + "SGA_timePerformance.txt");
		timePerGenFile << "Generation,Time\n";
		this->timeVsFitnessFile.open(this->outputFolder + "SGA_time_vs_fitness.txt");
	}
	if (this->logAllFiles || this->saveExposureShorten) {
		this->efile.open(this->outputFolder + "SGA_exposure.txt");
	}
	if (this->logAllFiles || this->saveEliteImages) {
		this->tfile.open(this->outputFolder + "SGA_functionEvals_vs_fitness.txt");
	}

	return true; // Returning true if no issues met
}

// Method to clean up & save resulting runOptimziation() instance
bool SGA_Optimization::shutdownOptimizationInstance() {

	std::string curTime = Utility::getCurTime();

	// Only save images if not aborting (successful results)
	if (dlg.stopFlag == false && this->saveResultImages) {
		// Get elite info
		unsigned char* eliteImage = this->bestImage->getRawData<unsigned char>();
		int imgHeight = this->bestImage->getHeight();
		int imgWidth = this->bestImage->getWidth();

		// Save how final optimization looks through camera
		this->cc->saveImage(this->bestImage, this->outputFolder + curTime + "_SGA_Optimized.bmp");

		// Save final (most fit SLM images)
		for (int popID = 0; popID < this->population.size(); popID++) {
			scalers[popID]->TranslateImage(this->population[popID]->getGenome(this->population[popID]->getSize() - 1), this->slmScaledImages[popID]);
			cv::Mat m_ary = cv::Mat(512, 512, CV_8UC1, this->slmScaledImages[popID]);
			cv::imwrite(this->outputFolder + curTime + "_SGA_phaseopt_SLM_" + std::to_string(popID) + ".bmp", m_ary);
		}
	}

	// Generic file renaming to have time stamps of run
	if (this->logAllFiles || this->saveTimeVSFitness) {
		this->timeVsFitnessFile << this->timestamp->MS_SinceStart() << " " << 0 << std::endl;
		this->timeVsFitnessFile.close();
		std::rename((this->outputFolder + "SGA_timePerformance.txt").c_str(), (this->outputFolder + curTime + "_SGA_timePerformance.txt").c_str());
		std::rename((this->outputFolder + "SGA_time_vs_fitness.txt").c_str(), (this->outputFolder + curTime + "_SGA_time_vs_fitness.txt").c_str());
	}
	if (this->logAllFiles || this->saveEliteImages) {
		this->tfile.close();
		std::rename((this->outputFolder + "SGA_functionEvals_vs_fitness.txt").c_str(), (this->outputFolder + curTime + "_SGA_functionEvals_vs_fitness.txt").c_str());
	}
	if (this->logAllFiles || this->saveExposureShorten) {
		this->efile.close();
		std::rename((this->outputFolder + "SGA_exposure.txt").c_str(), (this->outputFolder + curTime + "_SGA_exposure.txt").c_str());
	}
	if (this->logAllFiles || this->saveParametersPref) {
		saveParameters(curTime, "SGA");
	}

	// - image displays
	if (this->camDisplay != NULL) {
		this->camDisplay->CloseDisplay();
		delete this->camDisplay;
	}
	for (int i = 0; i < this->slmDisplayVector.size(); i++) {
		this->slmDisplayVector[i]->CloseDisplay();
		delete this->slmDisplayVector[i];
	}
	this->slmDisplayVector.clear();

	// - camera
	this->cc->stopCamera();
	//this->cc->shutdownCamera();
	// - pointers
	delete this->bestImage;
	for (int i = 0; i < this->population.size(); i++) {
		delete this->population[i];
	}
	this->population.clear();
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
	return true;
}
