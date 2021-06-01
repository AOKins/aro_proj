#ifndef SGAPOPULATION_H_
#define SGAPOPULATION_H_
#include "Individual.h"
#include "Population.h"
#include "BetterRandom.h"
#include <vector>
////
// TODOs:	Verify update of inherit from Population works (would assume so as the methods are largely unchanged)

template <class T>
class SGAPopulation : public Population {
public:
	// Constructor that inherits from Population class
	// Input:
	//	genome_length:	the image size (genome) for an individual
	//	population_size: the number of individuals for the population
	//	elite_size:		 the number of individuals for the population that are kept as elite
	//	accepted_similarity: precentage of similarity to be counted as same between individuals (default 90%)
	SGAPopulation(int genome_length, int population_size, int elite_size, double accepted_similarity = .9) : Population(genome_length, population_size, elite_size, accepted_similarity);

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
		Individual<T>* sorted_temp = SortIndividuals(individuals_, this->pop_size_);
		delete[] individuals_;
		individuals_ = sorted_temp;

		// calculate total fitness of all individuals (necessary for fitness proportionate selection)
		double fitness_sum = 0;
		for (int i = 0; i < this->pop_size_; i++) {
			fitness_sum += individuals_[i].fitness();
		}

		// Breeding
		bool same_check = true;
		Individual<T>* temp = new Individual<T>[this->pop_size_];
		BetterRandom parent_selector(RAND_MAX);
		double divisor = RAND_MAX / fitness_sum;

		// for each new individual
		for (int i = 0; i < this->pop_size_ - this->elite_size_; i++) {
			// select first parent with fitness proportionate selection
			double selected = parent_selector() / divisor;
			double temp_sum = individuals_[0].fitness();
			int j = 0;
			while (temp_sum < selected) {
				j++;
				temp_sum += individuals_[j].fitness();
			}
			std::shared_ptr<std::vector<T>> temp_image1 = individuals_[j].genome();

			// Select second parent with fitness proportionate selection
			selected = parent_selector() / divisor;
			temp_sum = individuals_[0].fitness();
			j = 0;
			while (temp_sum < selected) {
				j++;
				temp_sum += individuals_[j].fitness();
			}
			std::shared_ptr<std::vector<T>> temp_image2 = individuals_[j].genome();

			// perform crossover
			temp[i].set_genome(Crossover(temp_image1, temp_image2, same_check));
		}

		// for the elites, copy directly into new generation
		for (int i = this->pop_size_ - this->elite_size_; i < this->pop_size_; i++) {
			DeepCopyIndividual(temp[i], individuals_[i]);
		}

		// if all of our individuals are labeled similar, replace half of them with new images
		if (same_check) {
			for (int i = 0; i < this->pop_size_ / 2; i++) {
				temp[i].set_genome(GenerateRandomImage());
			}
		}
		delete[] individuals_;
		individuals_ = temp;
		counter_ = 0;
		return true;
	}	// ... Function nextGeneration
}; // ... class SGAPopulation

#endif
