////////////////////
// BetterRandom.h - project's randomizer
////////////////////

#ifndef BETTER_RANDOM_H_
#define BETTER_RANDOM_H_

#include <random>
#include <mutex>

// This class acts as a simpler interface to the much more random & capable random number generator -> random_device.
struct BetterRandom {
	std::random_device rd;
	std::mt19937 *mt;
	std::uniform_int_distribution<int> *dist;

	std::mutex randMutex;

	// Default use max
	BetterRandom() : BetterRandom(RAND_MAX) {}

	BetterRandom(int cap) {
		mt = new std::mt19937(rd());
		dist = new std::uniform_int_distribution<int>(0, cap - 1);
	}

	~BetterRandom() {
		delete mt;
		delete dist;
	}

	// () operator, use this to get a random number
	const int operator()() {
		//std::unique_lock<std::mutex> myLock(this->randMutex, std::defer_lock);
		//myLock.lock();
		int result = (*dist)(*mt);
		//myLock.unlock();
		return result;
	}
};

#endif
