////////////////////
// Additional source file to export some of the contents of MainDialog.cpp for easier navigation
// SettingsOutput.cpp - behavior of the Save Settings and Load Settings buttons within MainDialog
// Last edited: 07/08/2021 by Andrew O'Kins
////////////////////
#include "stdafx.h"				// Required in source
#include "resource.h"
#include "MainDialog.h"			// Header file for dialog functions
#include "SLMController.h"		// References to SLM controller for setting LUT and WFC files
#include <fstream>				// for file i/o

#include "Utility.h"			// Collection of static helper functions

#define MAX_CFileDialog_FILE_COUNT 99
#define FILE_LIST_BUFFER_SIZE ((MAX_CFileDialog_FILE_COUNT * (MAX_PATH + 1)) + 1)

// Handle process of loading settings, requesting file to select and attempt load the contents
void MainDialog::OnBnClickedLoadSettings() {
	bool tryAgain; // Boolean that if true means to try another attempt to load a file
	CString fileName;
	LPWSTR p = fileName.GetBuffer(FILE_LIST_BUFFER_SIZE);
	std::string filePath;
	do {
		tryAgain = false; // Initially assuming that we won't be needing to try again

		// Default to .cfg file extension (this program uses that extension in generation of saving settings)
		static TCHAR BASED_CODE filterFiles[] = _T("Config Files (*.cfg)|*.cfg|ALL Files (*.*)|*.*||");

		CFileDialog dlgFile(TRUE, NULL, L"./", OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, filterFiles);

		OPENFILENAME& ofn = dlgFile.GetOFN();
		ofn.lpstrFile = p;
		ofn.nMaxFile = FILE_LIST_BUFFER_SIZE;

		if (dlgFile.DoModal() == IDOK) { // Given a path to file to use
			fileName = dlgFile.GetPathName();
			fileName.ReleaseBuffer();

			filePath = CT2A(fileName);

			if (!setUIfromFile(filePath)) {
				int err_response = MessageBox(
					(LPCWSTR)L"An error occurred while trying load the file\nTry Again?",
					(LPCWSTR)L"ERROR in file load!",
					MB_ICONERROR | MB_RETRYCANCEL);
				// Respond to decision
				switch (err_response) {
				case IDRETRY:
					tryAgain = true;
					break;
				default: // Cancel or other unknown response will not have try again to make sure not stuck in undesired loop
					tryAgain = false;
				}
			}
		}
		else {
			tryAgain = false;
		}

	} while (tryAgain);
}

// Set the UI according to given file path, called from Load Settings
bool MainDialog::setUIfromFile(std::string filePath) {
	// Open file
	std::ifstream inputFile(filePath);
	std::string lineBuffer;
	try {
		// Read each line
		while (std::getline(inputFile, lineBuffer)) {
			// If not empty and not a commented out line
			if (lineBuffer != "" && (lineBuffer.find("#") != 0)) {
				// Locate the "=" and use as pivot where prior is the variable name and after is variable value
				int equals_pivot = int(lineBuffer.find("="));
				// Assumption made that there are no spaces from variable name to variable value, rather only occurring after a variable is assigned a value and afterwards may be an in-line comment
				int end_point = int(lineBuffer.find_first_of(" "));

				if (equals_pivot != end_point) {
					// With the two positions acquired, capture the varable's name and the value it is being assigned
					std::string variableName = lineBuffer.substr(0, equals_pivot);
					std::string variableValue = lineBuffer.substr(equals_pivot + 1, end_point - equals_pivot - 1);

					if (!setValueByName(variableName, variableValue)) {
						Utility::printLine("WARNING: Failure to interpret variable '" + variableName + "' with value '" + variableValue + "'! Continuing on to next variable");
					}
				}
			}
		}
	}
	catch (std::exception &e) {
		Utility::printLine("ERROR: " + std::string(e.what()));
		return false;
	}
	return true;
}

// Set a value in the UI according to a given name and value as string (used by setUIfromFile)
// Input: varName - string that should be identified to be a for a UI setting
//		  varValue - string containing the value that the associated variable should be assigned (exact type depends on variable identified)
// Output: variable associated with name is assigned varValue, returns false if error or inputs are empty
bool MainDialog::setValueByName(std::string name, std::string value) {
	CString valueStr(value.c_str());
	// Do a pre-emptive check if either name or value are empty
	if (name == "" || value == "") {
		return false;
	}
	// Camera Dialog
	else if (name == "framesPerSecond")
		this->m_cameraControlDlg.m_FramesPerSecond.SetWindowTextW(valueStr);
	else if (name == "initialExposureTime")
		this->m_cameraControlDlg.m_initialExposureTimeInput.SetWindowTextW(valueStr);
	else if (name == "gamma")
		this->m_cameraControlDlg.m_gammaValue.SetWindowTextW(valueStr);
	// AOI Dialog
	else if (name == "leftAOI")
		this->m_aoiControlDlg.m_leftInput.SetWindowTextW(valueStr);
	else if (name == "rightAOI")
		this->m_aoiControlDlg.m_rightInput.SetWindowTextW(valueStr);
	else if (name == "widthAOI")
		this->m_aoiControlDlg.m_widthInput.SetWindowTextW(valueStr);
	else if (name == "heightAOI")
		this->m_aoiControlDlg.m_heightInput.SetWindowTextW(valueStr);
	// Optimization Dialog
	else if (name == "binSize")
		this->m_optimizationControlDlg.m_binSize.SetWindowTextW(valueStr);
	else if (name == "binNumber")
		this->m_optimizationControlDlg.m_numberBins.SetWindowTextW(valueStr);
	else if (name == "targetRadius")
		this->m_optimizationControlDlg.m_targetRadius.SetWindowTextW(valueStr);
	else if (name == "minFitness")
		this->m_optimizationControlDlg.m_minFitness.SetWindowTextW(valueStr);
	else if (name == "minSeconds")
		this->m_optimizationControlDlg.m_minSeconds.SetWindowTextW(valueStr);
	else if (name == "maxSeconds")
		this->m_optimizationControlDlg.m_maxSeconds.SetWindowTextW(valueStr);
	else if (name == "minGenerations")
		this->m_optimizationControlDlg.m_minGenerations.SetWindowTextW(valueStr);
	else if (name == "maxGenerations")
		this->m_optimizationControlDlg.m_maxGenerations.SetWindowTextW(valueStr);
	// SLM Dialog
	else if (name == "slmConfigMode") {
		if (valueStr == "multi") {
			this->m_slmControlDlg.multiEnable.SetCheck(BST_CHECKED);
			this->m_slmControlDlg.dualEnable.SetCheck(BST_UNCHECKED);
		}
		else if (valueStr == "dual") {
			this->m_slmControlDlg.multiEnable.SetCheck(BST_UNCHECKED);
			this->m_slmControlDlg.dualEnable.SetCheck(BST_CHECKED);
		}
		else {
			this->m_slmControlDlg.multiEnable.SetCheck(BST_UNCHECKED);
			this->m_slmControlDlg.dualEnable.SetCheck(BST_UNCHECKED);
		}
	}
	else if (name.substr(0, 14) == "slmLutFilePath") {
		// We are dealing with LUT file setting
		int boardID = std::stoi(name.substr(14, name.length()-14)) - 1;
		Utility::printLine("DEBUG: LUT boardID->" + std::to_string(boardID));
		if (this->slmCtrl != NULL) {
			if (boardID < this->slmCtrl->boards.size() && boardID >= 0) {
				// Resource for conversion https://stackoverflow.com/questions/258050/how-do-you-convert-cstring-and-stdstring-stdwstring-to-each-other
				CT2CA converString(valueStr);
				this->m_slmControlDlg.attemptLUTload(boardID, std::string(converString));
			}
			else {
				Utility::printLine("WARNING: Load setting attempted to assign LUT file out of bounds!");
				return false;
			}
		}
		else {
			return false;
		}
	}
	else if (name.substr(0, 14) == "slmWfcFilePath") {
		// We are dealing with WFC file setting
		int boardID = std::stoi(name.substr(14, name.length() - 14)) - 1;
		Utility::printLine("DEBUG: WFC boardID->" + std::to_string(boardID));
		if (this->slmCtrl != NULL) {
			if (boardID < this->slmCtrl->boards.size() && boardID >= 0) {
				// Resource for conversion https://stackoverflow.com/questions/258050/how-do-you-convert-cstring-and-stdstring-stdwstring-to-each-other
				CT2CA converString(valueStr);
				this->m_slmControlDlg.attemptWFCload(boardID, std::string(converString));
			}
			else {
				Utility::printLine("WARNING: Load setting attempted to assign WFC file out of bounds!");
			}
		}
	}

	else if (name == "phaseCompensation") {
		if (value == "true") {
			this->m_slmControlDlg.m_CompensatePhaseCheckbox.SetCheck(BST_CHECKED);
			this->m_slmControlDlg.m_CompensatePhase = true;
		}
		else {
			this->m_slmControlDlg.m_CompensatePhaseCheckbox.SetCheck(BST_UNCHECKED);
			this->m_slmControlDlg.m_CompensatePhase = false;
		}
	}

	// Output Controls Dialog variables
	else if (name == "saveEliteImage") {
		if (value == "true") {
			this->m_outputControlDlg.m_SaveEliteImagesCheck.SetCheck(BST_CHECKED);
		}
		else {
			this->m_outputControlDlg.m_SaveEliteImagesCheck.SetCheck(BST_UNCHECKED);
		}
	}
	else if (name == "displayCamera") {
		if (value == "true") {
			this->m_outputControlDlg.m_displayCameraCheck.SetCheck(BST_CHECKED);
		}
		else {
			this->m_outputControlDlg.m_displayCameraCheck.SetCheck(BST_UNCHECKED);
		}
	}
	else if (name == "displaySLM") {
		if (value == "true") {
			this->m_outputControlDlg.m_displaySLM.SetCheck(BST_CHECKED);
		}
		else {
			this->m_outputControlDlg.m_displaySLM.SetCheck(BST_UNCHECKED);
		}
	}
	else if (name == "logFilesEnable") {
		if (value == "true") {
			this->m_outputControlDlg.m_logAllFilesCheck.SetCheck(BST_CHECKED);
		}
		else {
			this->m_outputControlDlg.m_logAllFilesCheck.SetCheck(BST_UNCHECKED);
		}
	}
	else if (name == "multiThreading") {
		if (value == "true") {
			this->m_MultiThreadEnable.SetCheck(BST_CHECKED);
		}
		else {
			this->m_MultiThreadEnable.SetCheck(BST_UNCHECKED);
		}
	}
	else if (name == "outputFolder") {
		this->m_outputControlDlg.m_OutputLocationField.SetWindowTextW(valueStr);
	}
	else {	// Unidentfied variable name, return false
		return false;
	}
	return true;
}

// When the Save Settings button is pressed, prompt the user to give where to save the file to for storing all parameters & preferences
void MainDialog::OnBnClickedSaveSettings() {
	bool tryAgain;
	CString fileName;
	LPWSTR p = fileName.GetBuffer(FILE_LIST_BUFFER_SIZE);
	std::string filePath;
	do {
		tryAgain = false;

		// Default Save As dialog box with extension as .cfg
		CFileDialog dlgFile(FALSE, L"cfg");

		OPENFILENAME& ofn = dlgFile.GetOFN();
		ofn.lpstrFile = p;
		ofn.nMaxFile = FILE_LIST_BUFFER_SIZE;

		// If action is OK
		if (dlgFile.DoModal() == IDOK) {
			fileName = dlgFile.GetPathName();
			fileName.ReleaseBuffer();

			filePath = CT2A(fileName);

			if (!saveUItoFile(filePath)) {
				int err_response = MessageBox(
					(LPCWSTR)L"An error occurred while trying save settings\nTry Again?",
					(LPCWSTR)L"ERROR in saving!",
					MB_ICONERROR | MB_RETRYCANCEL);
				// Respond to decision
				switch (err_response) {
				case IDRETRY:
					tryAgain = true;
					break;
				default: // Cancel or other unknown response will not have try again to make sure not stuck in undesired loop
					tryAgain = false;
				}
			}
			else {// Give success message when no issues
				MessageBox((LPCWSTR)L"Successfully saved settings.",
					(LPCWSTR)L"Success!",
					MB_ICONINFORMATION | MB_OK);
				tryAgain = false;
			}
		}
		// If action is cancel
		else {
			tryAgain = false;
		}

	} while (tryAgain);
}

// Write current UI values with given file location
bool MainDialog::saveUItoFile(std::string filePath) {
	std::ofstream outFile; // output stream
	CString tempBuff; // Temporary hold of what's in dialog window of given variable
	outFile.open(filePath);

	outFile << "# ARO PROJECT CONFIGURATION FILE" << std::endl;
	// Main Dialog
	outFile << "# Multithreading" << std::endl;
	outFile << "multiThreading=";
	if (this->m_MultiThreadEnable.GetCheck() == BST_CHECKED) { outFile << "true" << std::endl; }
	else { outFile << "false" << std::endl; }
	// Camera Dialog
	outFile << "# Camera Settings" << std::endl;
	this->m_cameraControlDlg.m_FramesPerSecond.GetWindowTextW(tempBuff);
	outFile << "framesPerSecond=" << _tstof(tempBuff) << std::endl;
	this->m_cameraControlDlg.m_initialExposureTimeInput.GetWindowTextW(tempBuff);
	outFile << "initialExposureTime=" << _tstof(tempBuff) << std::endl;
	this->m_cameraControlDlg.m_gammaValue.GetWindowTextW(tempBuff);
	outFile << "gamma=" << _tstof(tempBuff) << std::endl;
	// AOI Dialog
	outFile << "# AOI Settings" << std::endl;
	this->m_aoiControlDlg.m_leftInput.GetWindowTextW(tempBuff);
	outFile << "leftAOI=" << _tstof(tempBuff) << std::endl;
	this->m_aoiControlDlg.m_rightInput.GetWindowTextW(tempBuff);
	outFile << "rightAOI=" << _tstof(tempBuff) << std::endl;
	this->m_aoiControlDlg.m_widthInput.GetWindowTextW(tempBuff);
	outFile << "widthAOI=" << _tstof(tempBuff) << std::endl;
	this->m_aoiControlDlg.m_heightInput.GetWindowTextW(tempBuff);
	outFile << "heightAOI=" << _tstof(tempBuff) << std::endl;
	// Optimization Dialog
	outFile << "# Optimization Settings" << std::endl;
	this->m_optimizationControlDlg.m_binSize.GetWindowTextW(tempBuff);
	outFile << "binSize=" << _tstof(tempBuff) << std::endl;
	this->m_optimizationControlDlg.m_numberBins.GetWindowTextW(tempBuff);
	outFile << "binNumber=" << _tstof(tempBuff) << std::endl;
	this->m_optimizationControlDlg.m_targetRadius.GetWindowTextW(tempBuff);
	outFile << "targetRadius=" << _tstof(tempBuff) << std::endl;
	this->m_optimizationControlDlg.m_minFitness.GetWindowTextW(tempBuff);
	outFile << "minFitness=" << _tstof(tempBuff) << std::endl;
	this->m_optimizationControlDlg.m_minSeconds.GetWindowTextW(tempBuff);
	outFile << "minSeconds=" << _tstof(tempBuff) << std::endl;
	this->m_optimizationControlDlg.m_maxSeconds.GetWindowTextW(tempBuff);
	outFile << "maxSeconds=" << _tstof(tempBuff) << std::endl;
	this->m_optimizationControlDlg.m_minGenerations.GetWindowTextW(tempBuff);
	outFile << "minGenerations=" << _tstof(tempBuff) << std::endl;
	this->m_optimizationControlDlg.m_maxGenerations.GetWindowTextW(tempBuff);
	outFile << "maxGenerations=" << _tstof(tempBuff) << std::endl;
	// SLM Dialog
	outFile << "# SLM Configuratoin Settings" << std::endl;
	outFile << "slmConfigMode=";
	if (this->m_slmControlDlg.dualEnable.GetCheck() == BST_CHECKED) { outFile << "dual\n"; }
	else if (this->m_slmControlDlg.multiEnable.GetCheck() == BST_CHECKED) { outFile << "multi\n"; }
	else { outFile << "single\n"; }
	// Quick Check for SLMs
	if (this->slmCtrl != NULL) {
		// Output the LUT file paths being used for every board and PhaseCompensationFile
		for (int i = 0; i < this->slmCtrl->boards.size(); i++) {
			outFile << "slmLutFilePath" << std::to_string(i + 1) << "=" << this->slmCtrl->boards[i]->LUTFileName << "\n";
			outFile << "slmWfcFilePath" << std::to_string(i + 1) << "=" << this->slmCtrl->boards[i]->PhaseCompensationFileName << "\n";
		}
	}
	outFile << "phaseCompensation=";
	if (this->m_slmControlDlg.m_CompensatePhase) { outFile << "true\n"; }
	else { outFile << "false\n"; }

	// Output Dialog
	outFile << "saveEliteImage=";
	if (this->m_outputControlDlg.m_SaveEliteImagesCheck.GetCheck() == BST_CHECKED) {	outFile << "true\n"; }
	else { outFile << "false\n"; }
	
	outFile << "displayCamera=";
	if (this->m_outputControlDlg.m_displayCameraCheck.GetCheck() == BST_CHECKED) { outFile << "true\n"; }
	else { outFile << "false\n"; }
	
	outFile << "displaySLM=";
	if (this->m_outputControlDlg.m_displaySLM.GetCheck() == BST_CHECKED) { outFile << "true\n"; }
	else { outFile << "false\n"; }
	
	outFile << "logAllFilesEnable=";
	if (this->m_outputControlDlg.m_logAllFilesCheck.GetCheck() == BST_CHECKED) { outFile << "true\n"; }
	else { outFile << "false\n"; }
	this->m_outputControlDlg.m_OutputLocationField.GetWindowTextW(tempBuff);
	outFile << "outputFolder=" << tempBuff << std::endl;

	return true;
}
