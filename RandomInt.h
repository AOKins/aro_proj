#ifndef RANDOM_H_
#define RANDOM_H_
#include <random>

struct RandomInt {
	std::random_device rd;
	std::mt19937 *mt;
	std::uniform_int_distribution<int> *dist;
	RandomInt(int cap){
		mt = new std::mt19937(rd());
		dist = new std::uniform_int_distribution<int>(0, cap - 1);
	}
	int operator()() {
		return (*dist)(*mt);
	}
};

#endif