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

using std::thread;
////
// TODOs:	Verify/Debug uGA implementation
//			Consider possible race condition issues

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
		bool same_check; // Not really used, but made for use with base crossover method

		// temp for storing new population
		Individual<T>* temp = new Individual<T>[this->pop_size_];

		/* Serial (non-multithreaded) implementation to be kept as reference
		temp[0].set_genome(Crossover(sorted_temp[4].genome(), sorted_temp[3].genome(), same_check));
		temp[1].set_genome(Crossover(sorted_temp[4].genome(), sorted_temp[2].genome(), same_check));
		temp[2].set_genome(Crossover(sorted_temp[3].genome(), sorted_temp[2].genome(), same_check));
		temp[3].set_genome(Crossover(sorted_temp[3].genome(), sorted_temp[2].genome(), same_check));
			// Keeping current best onto next generation
		temp[4].set_genome(sorted_temp[4].genome()));
		*/

		// Lambda function to do assignment in parallel
		// Input: indID - index of location to store individual in temp array
		//		parent1 - index of a parent in sorted_temp
		//		parent2 - index of the other parent in sorted_temp
		// Captures
		//		temp - pointer array to store new individuals
		//		sorted_temp - pointer to array of sorted individuals to draw parents from
		//		same_check - unused bool passed in for Crossover()
		auto genInd = [temp, sorted_temp, &same_check](int indID, int parent1, int parent2) {
			temp[indID].set_genome(Crossover(sorted_temp[parent1], sorted_temp[parent2], same_check));
		};

		// Crossover generation for new population
			// Assumes population is 5
			// Lower index is higher fitness, so index 4 is most fit individual
		// Setting threads to perform crossover in parallel
		this->ind_threads.push_back(thread(genInd, 0, 4, 3));
		this->ind_threads.push_back(thread(genInd, 1, 4, 2));
		this->ind_threads.push_back(thread(genInd, 2, 3, 2));
		this->ind_threads.push_back(thread(genInd, 3, 3, 2));
		temp[4].set_genome(sorted_temp[4].genome()));		// Doing here since it's just a simple assignment while other threads perform genInd()
		
		rejoinClear();	// rejoin

		// Assign new population to individuals_
		delete[] individuals_;
		individuals_ = temp;
		return true;		// No issues!
	}	// ... Function nextGeneration
}; // ... class uGAPopulation

#endif
