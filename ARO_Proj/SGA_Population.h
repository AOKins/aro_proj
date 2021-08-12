////////////////////
// SGA_Population.h - handler for simple genetic algorithm that inherits from base Population
// Last edited: 06/21/2021 by Andrew O'Kins
////////////////////

#ifndef SGAPOPULATION_H_
#define SGAPOPULATION_H_

#include "Population.h"

template <class T>
class SGAPopulation : public Population<T> {
public:
	// Constructor
	// Input:
	//	genome_length:		 the image size (genome) for an individual
	//	population_size:	 the number of individuals for the population
	//	elite_size:			 the number of individuals for the population that are kept as elite
	//	accepted_similarity: precentage of similarity to be counted as same between individuals (default 90%)
	//  multiThread:		 enable usage of multithreading (default true)
	SGAPopulation(int genome_length, int population_size, int elite_size, double accepted_similarity = .9, bool multiThread = true) 
		: Population<T>(genome_length, population_size, elite_size, accepted_similarity, multiThread) {};

	// Starts next generation using fitness of individuals.  Following the simple genetic algorithm approach.
	bool nextGeneration();

}; // ... class SGAPopulation

#endif
