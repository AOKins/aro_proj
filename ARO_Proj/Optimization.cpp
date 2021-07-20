#include "stdafx.h"				// Required in source
#include "Optimization.h"		// Header file

Optimization::Optimization(MainDialog* dlg, CameraController* cc, SLMController* sc) {
	if (cc == nullptr)
		Utility::printLine("WARNING: invalid camera controller passed to optimization!");
	if (sc == nullptr)
		Utility::printLine("WARNING: invalid SLM controller passed to optimization!");
	this->cc = cc;
	this->sc = sc;
	this->dlg = dlg;
	this->ind_threads.clear();
	
	// Read from the output dialog for output parameters
	prepareOutputSettings();

	if (dlg->m_MultiThreadEnable.GetCheck() == BST_CHECKED) {
		this->multithreadEnable = true;
	}
	else {
		this->multithreadEnable = false;
	}
}

// [SETUP]
// Draw from GUI the optimization stop conditions
bool Optimization::prepareStopConditions() {
	bool result = true;

	// Fitness to stop at
	try	{
		CString path;
		this->dlg->m_optimizationControlDlg.m_minFitness.GetWindowTextW(path);
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
		this->dlg->m_optimizationControlDlg.m_minSeconds.GetWindowTextW(path);
		if (path.IsEmpty()) {
			throw new std::exception();
		}
		this->minSecondsToStop = _tstof(path);
	}
	catch (...)	{
		Utility::printLine("ERROR: Can't Parse Minimum Seconds Elapsed");
		result = false;
	}
	try	{
		CString path;
		this->dlg->m_optimizationControlDlg.m_maxSeconds.GetWindowTextW(path);
		if (path.IsEmpty()) {
			throw new std::exception();
		}
		this->maxSecondsToStop= _tstof(path);
	}
	catch (...)	{
		Utility::printLine("ERROR: Can't Parse Minimum Seconds Elapsed");
		result = false;
	}
	// Generations evaluations to at least do (minimum)
	try	{
		CString path;
		this->dlg->m_optimizationControlDlg.m_minGenerations.GetWindowTextW(path);
		if (path.IsEmpty()) {
			throw new std::exception();
		}
		this->genEvalToStop = _tstof(path);
	}
	catch (...)	{
		Utility::printLine("ERROR: Can't Parse Minimum Function Evaluations");
		result = false;
	}
	// Generations evaluations to stop at (maximum)
	try	{
		CString path;
		this->dlg->m_optimizationControlDlg.m_maxGenerations.GetWindowTextW(path);
		if (path.IsEmpty()) {
			throw new std::exception();
		}
		this->maxGenenerations = _tstof(path);
	}
	catch (...)	{
		Utility::printLine("ERROR: Can't Parse Minimum Function Evaluations");
		result = false;
	}
	return result;
}

// Draw from GUI the output settings
bool Optimization::prepareOutputSettings() {
	// Display Camera
	if (this->dlg->m_outputControlDlg.m_displayCameraCheck.GetCheck() == BST_CHECKED) {
		this->displayCamImage = true;
	}
	else {
		this->displayCamImage = false;
	}
	// Display SLMs
	if (this->dlg->m_outputControlDlg.m_displaySLM.GetCheck() == BST_CHECKED) {
		this->displaySLMImage = true;
	}
	else {
		this->displaySLMImage = false;
	}
	// A check all Enable all option
	if (this->dlg->m_outputControlDlg.m_logAllFilesCheck.GetCheck() == BST_CHECKED) {
		this->logAllFiles = true;
		this->saveEliteImages = true;
	}
	else {
		this->logAllFiles = false;
	}
	if (this->dlg->m_outputControlDlg.m_SaveParameters.GetCheck() == BST_CHECKED) {
		this->saveParametersPref = true;
	}
	else {
		this->saveParametersPref = false;
	}
	// If this enable all checkbox isn't enabled, then we must check the more specific ones
	if (this->logAllFiles == false) {
		if (this->dlg->m_outputControlDlg.m_SaveFinalImagesCheck.GetCheck() == BST_CHECKED) {
			this->saveResultImages = true;
		}
		else {
			this->saveResultImages = false;
		}
		if (this->dlg->m_outputControlDlg.m_SaveEliteImagesCheck.GetCheck() == BST_CHECKED) {
			this->saveEliteImages = true;
		}
		else {
			this->saveEliteImages = false;
		}
		if (this->dlg->m_outputControlDlg.m_SaveExposureShortCheck.GetCheck() == BST_CHECKED) {
			this->saveExposureShorten = true;
		}
		else {
			this->saveExposureShorten = false;
		}
		if (this->dlg->m_outputControlDlg.m_SaveTimeVFitnessCheck.GetCheck() == BST_CHECKED) {
			this->saveTimeVSFitness = true;
		}
		else {
			this->saveTimeVSFitness = false;
		}
	}
	// Get where to store the outputs
	CString buff;
	this->dlg->m_outputControlDlg.m_OutputLocationField.GetWindowTextW(buff);
	this->outputFolder = CT2A(buff);
	// Get freqency of saving elite images (getting value regardless of if it is enabled or not)
	try	{
		this->dlg->m_outputControlDlg.m_eliteSaveFreq.GetWindowTextW(buff);
		if (buff.IsEmpty()) {
			throw new std::exception();
		}
		this->saveEliteFrequency = _tstoi(buff);
	}
	catch (...)	{
		Utility::printLine("ERROR: Failure to read save elite frequency! Disabling save elite");
		this->saveEliteImages = false;
	}

	return true;
}

//[SETUP]
// Setup camera, verify SLM is ready and prepare stop conditions
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

	if (!this->sc->slmCtrlReady() && this->sc->updateFramerateFromGUI()) {
		Utility::printLine("ERROR: SLM setup has failed!");
		return false;
	}
	Utility::printLine("INFO: SLM setup complete!");

	// - configure algorithm parameters
	if (!prepareStopConditions()) {
		Utility::printLine("ERROR: Preparing stop conditions has failed!");
		return false;
	}
	Utility::printLine("INFO: Hardware ready!");

	// - configure proper UI states
	this->isWorking = true;
	this->dlg->disableMainUI(!isWorking);

	return true;
}

// For a given board setup and return a scaler
// Input: slmNum (default 0 and 0 based) - index of board to set scaler with
//        slmImg - char pointer to array with size equal to total area of board
// Output: - returns pointer to a new ImageScaler based on width & height of SLM at slmNum
//		   - slmImg is filled with zeros
ImageScaler* Optimization::setupScaler(unsigned char *slmImg, int slmNum = 0) {
	int width = int(sc->getBoardWidth(slmNum));
	int height = int(sc->getBoardHeight(slmNum));

	ImageScaler* scaler = new ImageScaler(width, height, 1, NULL);
	scaler->SetBinSize(cc->binSizeX, cc->binSizeY);
	scaler->SetLUT(NULL);
	scaler->SetUsedBins(cc->numberOfBinsX, cc->numberOfBinsY);
	scaler->ZeroOutputImage(slmImg); // Initialize the slm image array to be all zeros

	return scaler;
}

// [SAVE/LOAD FEATURES]
// Output information of the parameters used in the optimization in to logs
void Optimization::saveParameters(std::string time, std::string optType) {
	std::ofstream paramFile(this->outputFolder + time + "_" + optType + "_Optimization_Parameters.txt");
	paramFile << "----------------------------------------------------------------" << std::endl;
	paramFile << "OPTIMIZATION SETTINGS:" << std::endl;
	paramFile << "Type - " << optType << std::endl;
	if (optType != "OPT5") {
		paramFile << "Stop Fitness - " << std::to_string(this->fitnessToStop) << std::endl;
		paramFile << "Min Stop Time - " << std::to_string(this->minSecondsToStop) << std::endl;
		paramFile << "Max Stop Time - " << std::to_string(this->maxSecondsToStop) << std::endl;
		paramFile << "Min Generation - " << std::to_string(this->genEvalToStop) << std::endl;
		paramFile << "Max Generation - " << std::to_string(this->maxGenenerations) << std::endl;;
	}
	paramFile << "----------------------------------------------------------------" << std::endl;
	paramFile << "CAMERA SETTINGS:" << std::endl;
	paramFile << "AOI x0 - " << std::to_string(this->cc->x0) << std::endl;
	paramFile << "AOI y0 - " << std::to_string(this->cc->y0) << std::endl;
	paramFile << "AOI Image Width - " << std::to_string(this->cc->cameraImageWidth) << std::endl;
	paramFile << "AOI Image Height - " << std::to_string(this->cc->cameraImageHeight) << std::endl;
	paramFile << "Acquisition Gamma - " << std::to_string(this->cc->gamma) << std::endl;
	paramFile << "Acquisition FPS - " << std::to_string(this->cc->fps) << std::endl;
	paramFile << "Acquisition Initial Exposure Time - " << std::to_string(this->cc->initialExposureTime) << std::endl;
	paramFile << "Number of Bins X - " << std::to_string(this->cc->numberOfBinsX) << std::endl;
	paramFile << "Number of Bins Y - " << std::to_string(this->cc->numberOfBinsY) << std::endl;
	paramFile << "Bins Size X - " << std::to_string(this->cc->numberOfBinsX) << std::endl;
	paramFile << "Bins Size Y - " << std::to_string(this->cc->numberOfBinsY) << std::endl;
	paramFile << "Target Radius - " << std::to_string(this->cc->targetRadius) << std::endl;
	paramFile << "----------------------------------------------------------------" << std::endl;
	paramFile << "SLM SETTINGS:" << std::endl;
	paramFile << "Board Amount - " << std::to_string(this->sc->numBoards) << std::endl;
	paramFile << "Number of Boards being Optimized - " << std::to_string(this->popCount) << std::endl;
	for (int i = 0; i < int(this->sc->numBoards); i++) {
		paramFile << "Board #" << i << " LUT filePath - " << this->sc->boards[i]->LUTFileName << std::endl;
		paramFile << "Board #" << i << " WFC filePath - " << this->sc->boards[i]->PhaseCompensationFileName << std::endl;
	}
	paramFile.close();
}

//[CHECKS]
bool Optimization::stopConditionsReached(double curFitness, double curSecPassed, double curGenerations) {
	// If reached fitness to stop and minimum time and minimum generations to perform
	if ((curFitness > this->fitnessToStop && curSecPassed > this->minSecondsToStop && curGenerations > this->genEvalToStop) ) {
		return true;
	}
	// If the stop button was pressed
	if (dlg->stopFlag == true) {
		return true;
	}
	// If exceeded the maximum allowed time (negative or zero value indicated indefinite)
	if (this->maxSecondsToStop > 0 && curSecPassed >= this->maxSecondsToStop) {
		return true;
	}
	// If exceeded the maximum allowed time (negative or zero value indicated indefinite)
	if (this->maxGenenerations > 0 && curGenerations >= this->maxGenenerations) {
		return true;
	}
	return false;
}
