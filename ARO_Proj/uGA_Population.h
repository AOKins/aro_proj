////////////////////
// uGA_Population.h - handler for micro genetic algorithm that inherits from base Population
// Last edited: 06/23/2021 by Andrew O'Kins
////////////////////

#ifndef UGAPOPULATION_H_
#define UGAPOPULATION_H_

#include "Population.h"

template <class T>
class uGAPopulation : public Population<T> {
public:
	// Constructor
	// Input:
	//	genome_length:		 the image size (genome) for an individual
	//	population_size:	 the number of individuals for the population
	//	elite_size:			 the number of individuals for the population that are kept as elite
	//	accepted_similarity: precentage of similarity to be counted as same between individuals (default 90%)
	//  multiThread:		 enable usage of multithreading (default true)
	uGAPopulation(int genome_length, int population_size, int elite_size, double accepted_similarity = .9, bool multiThread = true)
		: Population<T>(genome_length, population_size, elite_size, accepted_similarity, multiThread) {};

	// Starts next generation using fitness of individuals.
	bool nextGeneration();
}; // ... class uGAPopulation

#endif
