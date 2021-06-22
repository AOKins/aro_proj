//Implementation file for TimeStamp.h

//References: To remove trailing zeros from double to string converion: https://thispointer.com/c-convert-double-to-string-and-manage-precision-scientific-notation/

#include "../headers/stdafx.h"
#include "../headers/TimeStamp.h"

//Constructor
TimeStamp::TimeStamp(double duration, string label) {
	this->duration = duration;
	this->label = label;
}

//Geting time taken
double TimeStamp::GetDurationSec() {
	return duration / 1000;
}
double TimeStamp::GetDurationMSec() {
	return duration;
}

// Get a string format label
string TimeStamp::GetLabel() {
	//Properly convert interval amount to string
	ostringstream ss;
	ss << fixed << setprecision(4) << duration;
	string curNum = ss.str();

	//Add white space to small intervals with no thousandth value (needs to be atleast 8 charecters in length)
	if (curNum.length() < 10) {
		while (curNum.length() < 10) {
			curNum = " " + curNum;
		}
	}
	return  curNum + " MS - " + label;
}
