////////////////////
// Population handler for standard genetic algorithm that inherits from base Population
// Last edited: 06/02/2021 by Andrew O'Kins
////////////////////
#ifndef UGAPOPULATION_H_
#define UGAPOPULATION_H_
#include "Individual.h"
#include "Population.h"
#include "BetterRandom.h"
#include <vector>
////
// TODOs:	Implement and debug uGA's StartNextGeneration() method

template <class T>
class uGAPopulation : public Population {
public:
	// Constructor that inherits from Population class
	// Input:
	//	genome_length:	the image size (genome) for an individual
	//	population_size: the number of individuals for the population
	//	elite_size:		 the number of individuals for the population that are kept as elite
	//	accepted_similarity: precentage of similarity to be counted as same between individuals (default 90%)
	uGAPopulation(int genome_length, int population_size, int elite_size, double accepted_similarity = .9) : Population(genome_length, population_size, elite_size, accepted_similarity);

	// Get number of individuals in population
	int getSize() : getSize();

	// Getter for image of individual at inputted index
	// Input: i - individual at given index (population not guranteed sorted)
	// Output: the image for the individual
	T* getImage(int i) : getImage(i);

	// Setter for the fitness of the individual at given index
	// Input:
	//	i - individual at given index (population not guranteed sorted)
	//	fitness - the fitness value to set to individual
	// Output: individual at index i has its set_fitness() called with fitness as input
	void setFitness(int i, double fitness) : setFitness(i, fitness);

	// Starts next generation using fitness of individuals.
	bool StartNextGeneration() {
	}	// ... Function nextGeneration
}; // ... class uGAPopulation

#endif
