// This class contains a list of the time stamp object members

#ifndef TIME_STAMP
#define TIME_STAMP

#include <iomanip>
#include <sstream>
#include <string>

class TimeStamp {
private:
	std::string label;
	double duration;

public:
	//Contructor
	TimeStamp(double duration, std::string label);
	
	//Get Time Taken
	double GetDurationSec();
	double GetDurationMSec();
	std::string GetLabel();
};

#endif
