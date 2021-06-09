////////////////////
// Population handler for micro genetic algorithm that inherits from base Population
// Last edited: 06/09/2021 by Andrew O'Kins
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
		bool * same_check = new bool[(this->pop_size_ - this->elite_size_)];

		// temp for storing new population
		Individual<T>* temp = new Individual<T>[this->pop_size_];

		// Lambda function to do assignment in parallel
		// Input: indID - index of location to store individual in temp array
		//		parent1 - index of a parent in sorted_temp
		//		parent2 - index of the other parent in sorted_temp
		// Captures:
		//		temp - pointer array to store new individuals
		//		sorted_temp - pointer to array of sorted individuals to draw parents from
		//		same_check - unused bool passed in for Crossover()
		auto genInd = [temp, sorted_temp, &same_check](int indID, int parent1, int parent2) {
			temp[indID].set_genome(Crossover(sorted_temp[parent1], sorted_temp[parent2], same_check[indID], false));
		};

		// Crossover generation for new population
			// Assumes population is 5
			// Lower index is higher fitness, so index 4 is most fit individual
		// Setting threads to perform crossover in parallel
		this->ind_threads.push_back(thread(genInd, 0, 4, 3));
		this->ind_threads.push_back(thread(genInd, 1, 4, 2));
		this->ind_threads.push_back(thread(genInd, 2, 3, 2));
		this->ind_threads.push_back(thread(genInd, 3, 3, 2));
			// Keeping current best onto next generation
		temp[4].set_genome(sorted_temp[4].genome()));		// Doing here since it's just a simple assignment while other threads perform genInd()
		
		rejoinClear();	// rejoin

		// Collect the resulting same_check values,
		// if at least one is false (not similar) then the result is set to false
		bool same_check_result = true;
		for (int i = 0; i < (this->pop_size_ - this->elite_size_); i++) {
			if (same_check[i] == false) {
				same_check_result = false;
				break;
			}
		}

		// if all of our individuals are labeled similar, replace half of them with new images
		if (same_check_result) {
			// Lambda function to ensure that generating random image is done in parallel
			// Input: id - index for individual to be set
			// Captures: temp - pointer to array of individuals to store new random genomes in
			auto randInd = [temp](int id) {
				temp[id].set_genome(GenerateRandomImage());
			}
			// Calling generate random image for bottom 4 individuals (keeping best)
			for (int i = 0; i < 4; i++) {
				this->ind_threads.push_back(std::thread(randInd, i));
			}
			rejoinClear();			// Rejoin
		}

		delete[] same_check;
		// Assign new population to individuals_
		delete[] individuals_;
		individuals_ = temp;
		return true; // No issues!
	}	// ... Function nextGeneration
}; // ... class uGAPopulation

#endif
