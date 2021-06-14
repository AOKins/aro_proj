
#include "stdafx.h"				// Required in source
#include <string>
using std::string;
#include "Optimization.h"		// Header file

#include "MainDialog.h"			// used for UI reference
#include "CameraController.h"
#include "SLMController.h"
#include "Utility.h"			// used for debug statements
#include "Timing.h"				// contains time keeping functions
#include "ImageScaler.h"		// changes size of image to fit slm //ASK: is this correct? 

#include <fstream>				// used to export information to file 
#include <chrono>
#include <thread>

Optimization::Optimization(MainDialog& dlg_, CameraController* cc, SLMController* sc) : dlg(dlg_) {
	if (cc == nullptr)
		Utility::printLine("WARNING: invalid camera controller passed to optimization!");
	if (sc == nullptr)
		Utility::printLine("WARNING: invalid camera controller passed to optimization!");
	this->cc = cc;
	this->sc = sc;
	this->ind_threads.clear();
}

// [SETUP]
bool Optimization::prepareStopConditions() {
	bool result = true;

	// Fitness to stop at
	try	{
		CString path;
		this->dlg.m_optimizationControlDlg.m_minFitness.GetWindowTextW(path);
		if (path.IsEmpty()){
			throw new std::exception();
		}
		this->fitnessToStop = _tstof(path);
	}
	catch (...)	{
		Utility::printLine("ERROR: Can't Parse Minimum Fitness");
		result = false;
	}

	// Time (in sec) to stop at
	try	{
		CString path;
		this->dlg.m_optimizationControlDlg.m_minSeconds.GetWindowTextW(path);
		if (path.IsEmpty()) throw new std::exception();
		secondsToStop = _tstof(path);
	}
	catch (...)	{
		Utility::printLine("ERROR: Can't Parse Minimum Seconds Elapsed");
		result = false;
	}

	// Generations evaluations to stop at
	try	{
		CString path;
		this->dlg.m_optimizationControlDlg.m_minGenerations.GetWindowTextW(path);
		if (path.IsEmpty()) throw new std::exception();
		genEvalToStop = _tstof(path);
	}
	catch (...)	{
		Utility::printLine("ERROR: Can't Parse Minimum Function Evaluations");
		result = false;
	}

	return result;
}

//[SETUP]
bool Optimization::prepareSoftwareHardware() {
	Utility::printLine("INFO: Preparing equipment and software for optimization!");

	//Can't start operation if an optimization is already running 
	if (this->isWorking) {
		Utility::printLine("WARNING: cannot prepare hardware the second time!");
		return false;
	}
	Utility::printLine("INFO: No optimization running, able to perform setup!");


	// - configure equipment
	if (!this->cc->setupCamera())	{
		Utility::printLine("ERROR: Camera setup has failed!");
		return false;
	}
	Utility::printLine("INFO: Camera setup complete!");

	if (!this->sc->slmCtrlReady()) {
		Utility::printLine("ERROR: SLM setup has failed!");
		return false;
	}
	Utility::printLine("INFO: SLM setup complete!");

	// - configure algorithm parameters
	if (!prepareStopConditions()) {
		Utility::printLine("ERROR: Preparing stop conditions has failed!");
		return false;
	}
	Utility::printLine("INFO: Stop conditions updated!");
	
	// - configure proper UI states
	this->isWorking = true;
	this->dlg.disableMainUI(!isWorking);
	return true;
}

ImageScaler* Optimization::setupScaler(unsigned char *aryptr) {
	ImageScaler* scaler = new ImageScaler(sc->getBoardWidth(0), sc->getBoardHeight(0), 1, NULL);
	scaler->SetBinSize(cc->binSizeX, cc->binSizeY);
	scaler->SetLUT(NULL);
	scaler->SetUsedBins(cc->numberOfBinsX, cc->numberOfBinsY);
	scaler->ZeroOutputImage(aryptr);

	return scaler;
}

// [SAVE/LOAD FEATURES]
void Optimization::saveParameters(std::string time, std::string optType) {
	std::ofstream paramFile("logs/" + time + "_" + optType + "_Optimization_Parameters.txt", std::ios::app);
	paramFile << "----------------------------------------------------------------" << std::endl;
	paramFile << "OPTIMIZATION SETTINGS:" << std::endl;
	paramFile << "Type - " << optType << std::endl;
	if (optType != "OPT5") {
		paramFile << "Stop Fitness - " << std::to_string(fitnessToStop) << std::endl;
		paramFile << "Stop Time - " << std::to_string(secondsToStop) << std::endl;
		paramFile << "Stop Generation - " << std::to_string(genEvalToStop) << std::endl;
	}
	paramFile << "----------------------------------------------------------------" << std::endl;
	paramFile << "CAMERA SETTINGS:" << std::endl;
	paramFile << "AOI x0 - " << std::to_string(cc->x0) << std::endl;
	paramFile << "AOI y0 - " << std::to_string(cc->y0) << std::endl;
	paramFile << "AOI Image Width - " << std::to_string(cc->cameraImageWidth) << std::endl;
	paramFile << "AOI Image Height - " << std::to_string(cc->cameraImageHeight) << std::endl;
	paramFile << "Acquisition Gamma - " << std::to_string(cc->gamma) << std::endl;
	paramFile << "Acquisition FPS - " << std::to_string(cc->fps) << std::endl;
	paramFile << "Acquisition Initial Exposure Time - " << std::to_string(cc->initialExposureTime) << std::endl;
	paramFile << "Number of Bins X - " << std::to_string(cc->numberOfBinsX) << std::endl;
	paramFile << "Number of Bins Y - " << std::to_string(cc->numberOfBinsY) << std::endl;
	paramFile << "Bins Size X - " << std::to_string(cc->numberOfBinsX) << std::endl;
	paramFile << "Bins Size Y - " << std::to_string(cc->numberOfBinsY) << std::endl;
	paramFile << "Target Radius - " << std::to_string(cc->targetRadius) << std::endl;
	paramFile << "----------------------------------------------------------------" << std::endl;
	paramFile << "SLM SETTINGS:" << std::endl;
	paramFile << "Board Amount - " << std::to_string(sc->numBoards) << std::endl;
	paramFile.close();
}

//[CHECKS]
bool Optimization::stopConditionsReached(double curFitness, double curSecPassed, double curGenerations) {
	if (curFitness > this->fitnessToStop && curSecPassed > this->secondsToStop && curGenerations > this->genEvalToStop) {
		return true;
	}
	return false;
}

void Optimization::rejoinClear() {
	// Rejoin all the threads and clear ind_threads vector for future use
	// Note: if a thread is stuck in an indefinite duration, this will lock out
	for (int i = 0; i < this->ind_threads.size(); i++) {
		this->ind_threads[i].join();
	}
	this->ind_threads.clear();
}
