////////////////////
// Population handler for standard genetic algorithm that inherits from base Population
// Last edited: 06/07/2021 by Andrew O'Kins
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
class uGAPopulation : public Population<T> {
public:
	// Constructor that inherits from Population class
	// Input:
	//	genome_length:	the image size (genome) for an individual
	//	population_size: the number of individuals for the population
	//	elite_size:		 the number of individuals for the population that are kept as elite
	//	accepted_similarity: precentage of similarity to be counted as same between individuals (default 90%)
	uGAPopulation(int genome_length, int population_size, int elite_size, double accepted_similarity = .9) : Population<T>(genome_length, population_size, elite_size, accepted_similarity) {};

	// Starts next generation using fitness of individuals.
	bool nextGeneration() {
		// temp for storing sorted current population
		Individual<T>* sorted_temp = SortIndividuals(individuals_, this->pop_size_);
		bool same_check; // Not really used, but made for use with crossover

		// temp for storing new population
		Individual<T>* temp = new Individual<T>[this->pop_size_];

		// Assumes population is 5
			// Crossover generation for new population
			// Lower index is higher fitness, so index 4 is most fit individual
		temp[0].set_genome(Crossover(sorted_temp[4].genome(), sorted_temp[3].genome(), same_check));
		temp[1].set_genome(Crossover(sorted_temp[4].genome(), sorted_temp[2].genome(), same_check));
		temp[2].set_genome(Crossover(sorted_temp[3].genome(), sorted_temp[2].genome(), same_check));
		temp[3].set_genome(Crossover(sorted_temp[3].genome(), sorted_temp[2].genome(), same_check));
			// Keeping current best onto next generation
		temp[4].set_genome(sorted_temp[4].genome()));

		// Assign new population to individuals_
		delete[] individuals_;
		individuals_ = temp;
		// No issues!
		return true;
	}	// ... Function nextGeneration
}; // ... class uGAPopulation

#endif
