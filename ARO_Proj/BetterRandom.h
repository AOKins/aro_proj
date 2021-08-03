////////////////////
// BetterRandom.h - project's randomizer
////////////////////

#ifndef BETTER_RANDOM_H_
#define BETTER_RANDOM_H_
#include <random>

// This class acts as a simpler interface to the much more random & capable random number generator -> random_device.
struct BetterRandom {
	std::random_device rd;
	std::mt19937 *mt;
	std::uniform_int_distribution<int> *dist;

	BetterRandom(int cap){
		mt = new std::mt19937(rd());
		dist = new std::uniform_int_distribution<int>(0, cap - 1);
	}

	// () operator, use this to get a random number
	int operator()() {
		return (*dist)(*mt);
	}
};

#endif
