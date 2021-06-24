// Header for TimeStamp class to generate label

#ifndef TIME_STAMP
#define TIME_STAMP

#include <iomanip>
#include <sstream>
#include <string>

class TimeStamp {
private:
	std::string label;	// A label for identifying what this time duration refers to
	double duration;	// Duration in milliseconds

public:
	//Contructor
	TimeStamp(double duration, std::string label);
	
	//Get Time Taken
	double GetDurationSec();
	double GetDurationMSec();
	std::string GetLabel();
};

#endif
