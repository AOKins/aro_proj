//This class contains a list of the time stamp object members

#ifndef TIME_STAMP
#define TIME_STAMP

#include <iomanip>
#include <sstream>
#include <string>

using namespace std;

class TimeStamp
{
private:
	string label;
	double duration;

public:
	//Contructor
	TimeStamp(double duration, string label);
	
	//Get Time Taken
	double GetDurationSec();
	double GetDurationMSec();
	string GetLabel();
};

#endif