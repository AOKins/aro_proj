#ifndef TIMING_H_
#define TIMING_H_
#include <Windows.h>


//Keeps track of time intervals relative to the timer's start time. Pausing
//will stop the program until the current time interval is finished. This will
//work well if you must stay in sync with something whose timing is persistent
//and you don't want variable time function calls throwing you out of sync.
class IntervalTimer{
private:
	__int64 frequency_;
	__int64 start_time_;
	__int64 ticks_per_interval_;
	__int64 ticks_per_shove_;

public:
	//constructor
	//@param pause_interval -> the size of each interval in ms
	IntervalTimer(double pause_interval, double shove_interval){
		QueryPerformanceFrequency((LARGE_INTEGER *)&frequency_);
		ticks_per_interval_ = frequency_ * (pause_interval / 1000);
		ticks_per_shove_ = frequency_ * (shove_interval / 1000);
	}

	//constructor
	IntervalTimer(int ticks_per_interval, double shove_interval){
		QueryPerformanceFrequency((LARGE_INTEGER *)&frequency_);
		ticks_per_interval_ = ticks_per_interval;
		ticks_per_shove_ = frequency_ * (shove_interval / 1000);
	}

	//constructor
	IntervalTimer(double pause_interval, int ticks_per_shove){
		QueryPerformanceFrequency((LARGE_INTEGER *)&frequency_);
		ticks_per_interval_ = frequency_ * (pause_interval / 1000);
		ticks_per_shove_ = ticks_per_shove;
	}

	//constructor
	IntervalTimer(int ticks_per_interval, int ticks_per_shove){
		QueryPerformanceFrequency((LARGE_INTEGER *)&frequency_);
		ticks_per_interval_ = ticks_per_interval;
		ticks_per_shove_ = ticks_per_shove;
	}

	//set the start of the timer
	void StartTimer() {
		QueryPerformanceCounter((LARGE_INTEGER *)&start_time_);
	}

	//pause until a new interval has begun
	void PauseToNextInterval() {
		__int64 stopTime = 0;
		QueryPerformanceCounter((LARGE_INTEGER *)&stopTime);
		__int64 intervalsPassed = (stopTime - start_time_) / ticks_per_interval_;
		__int64 nextIntervalPoint = start_time_ + ((intervalsPassed + 1) * ticks_per_interval_);
		while (stopTime < nextIntervalPoint){
			QueryPerformanceCounter((LARGE_INTEGER *)&stopTime);
		}
	}

	//moves intervals forward
	void ShoveForward() {
		start_time_ += ticks_per_shove_;
	}

	//moves intervals backward
	void ShoveBackward() {
		start_time_ -= ticks_per_shove_;
	}
};

//This timer acts like a stopwatch, as it can be started and stopped again.
//It will take the interval length and, when you pause, make sure you halt
//until that much time has passed since you started, but will not do anything
//after that.
class StopwatchTimer {
private:
	__int64 frequency_;
	__int64 start_time_;
	__int64 ticks_per_interval_;

public:
	//constructor
	//@param pause_interval -> the size of each interval in ms
	StopwatchTimer(double pause_interval){
		QueryPerformanceFrequency((LARGE_INTEGER *)&frequency_);
		ticks_per_interval_ = frequency_ * (pause_interval / 1000);
	}

	//constructor
	StopwatchTimer(int ticks_per_interval) {
		QueryPerformanceFrequency((LARGE_INTEGER *)&frequency_);
		ticks_per_interval_ = ticks_per_interval;
	}

	void StartTimer() {
		QueryPerformanceCounter((LARGE_INTEGER *)&start_time_);
	}

	//pause until a new interval has begun
	void PauseToIntervalCompletion() {
		__int64 stopTime = 0;
		__int64 intervalEndPoint = start_time_ + ticks_per_interval_;
		QueryPerformanceCounter((LARGE_INTEGER *)&stopTime);
		while (stopTime < intervalEndPoint) {
			QueryPerformanceCounter((LARGE_INTEGER *)&stopTime);
		}
	}
};

class TimeStampGenerator {
private:
	__int64 frequency_;
	__int64 start_time_;
public:
	//constructor
	TimeStampGenerator() {
		QueryPerformanceCounter((LARGE_INTEGER *)&start_time_);
		QueryPerformanceFrequency((LARGE_INTEGER *)&frequency_);
	}

	double S_SinceStart() {
		__int64 currentTime;
		QueryPerformanceCounter((LARGE_INTEGER *)&currentTime);
		double seconds = ((currentTime - start_time_) / frequency_);
		return seconds;
	}

	double MS_SinceStart() {
		__int64 currentTime;
		QueryPerformanceCounter((LARGE_INTEGER *)&currentTime);
		double milliseconds = ((currentTime - start_time_) / (frequency_ / 1000));
		return milliseconds;
	}
};
#endif