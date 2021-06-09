////////////////////
// Population handler for standard genetic algorithm that inherits from base Population
// Last edited: 06/09/2021 by Andrew O'Kins
////////////////////
#ifndef SGAPOPULATION_H_
#define SGAPOPULATION_H_
#include "Individual.h"
#include "Population.h"
#include "BetterRandom.h"
#include <vector>

////
// TODOs:	Debug mulithreading in StartNextGeneration consider/add mutexes to address possible critical sections

template <class T>
class SGAPopulation : public Population<T> {
public:
	// Constructor that inherits from Population class
	// Input:
	//	genome_length:	the image size (genome) for an individual
	//	population_size: the number of individuals for the population
	//	elite_size:		 the number of individuals for the population that are kept as elite
	//	accepted_similarity: precentage of similarity to be counted as same between individuals (default 90%)
	SGAPopulation(int genome_length, int population_size, int elite_size, double accepted_similarity = .9) : Population<T>(genome_length, population_size, elite_size, accepted_similarity) {};

	// Starts next generation using fitness of individuals.
	bool nextGeneration() {
		Individual<T>* sorted_temp = SortIndividuals(individuals_, this->pop_size_);
		delete[] individuals_;
		individuals_ = sorted_temp;

		// calculate total fitness of all individuals (necessary for fitness proportionate selection)
		double fitness_sum = 0;
		for (int i = 0; i < this->pop_size_; i++) {
			fitness_sum += individuals_[i].fitness();
		}

		// Breeding
		bool * same_check = new bool[(this->pop_size_ - this->elite_size_)];
		Individual<T> * temp = new Individual<T>[this->pop_size_];
		BetterRandom parent_selector(RAND_MAX);
		double divisor = RAND_MAX / fitness_sum;

		// Lambda function to be used for generating new individual
		// Input: i - index for new individual
		// Captures:
		//		temp - pointer to array of individuals to store current new individual at temp[i]
		//		parent_selector - RNG machine
		//		divisor - used in proportionate selection
		// Output: temp[i] is set a new genome using crossover algorithm
		auto genInd = [temp, &parent_selector, divisor, same_check](int i) {
			same_check[i] = true;
			// select first parent with fitness proportionate selection and store associated genome into temp_image1
			double selected = parent_selector() / divisor;
			double temp_sum = this->individuals_[0].fitness(); // Recall 0 index has worst fitness
			int j = 0;
			while (temp_sum < selected) {
				j++;
				temp_sum += this->individuals_[j].fitness();
			}
			std::shared_ptr<std::vector<T>> temp_image1 = this->individuals_[j].genome();

			// Select second parent with fitness proportionate selection and store associated genome into temp_image2
			selected = parent_selector() / divisor;
			temp_sum = this->individuals_[0].fitness();
			j = 0;
			while (temp_sum < selected) {
				j++;
				temp_sum += this->individuals_[j].fitness();
			}
			std::shared_ptr<std::vector<T>> temp_image2 = this->individuals_[j].genome();

			// perform crossover with temp_image1 & temp_image2 into temp[i]
			temp[i].set_genome(Crossover(temp_image1, temp_image2, same_check[i], true));
		}; // ... genInd(i)

		// for each new individual a thread with call to genInd()
		for (int i = 0; i < (this->pop_size_ - this->elite_size_); i++) {
			this->ind_threads.push_back(std::thread(genInd(), i));
		}
		rejoinClear();		// Rejoin

		// for the elites, copy directly into new generation
		// Performing deep copy for individuals in parallel
		for (int i = (this->pop_size_ - this->elite_size_); i < this->pop_size_; i++) {
			this->ind_threads.push_back(std::thread(DeepCopyIndividual(), temp[i], individuals_[i]));
		}
		rejoinClear();		// Rejoin

		// Collect the resulting same_check values,
			// if at least one is false (not similar) then the result is set to false
		bool same_check_result = true;
		for (int i = 0; i < (this->pop_size_ - this->elite_size_) && same_check_result; i++) {
			if (same_check[i] == false) {
				same_check_result = false;
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
			// Calling generate random image for half of pop individuals
			for (int i = 0; i < this->pop_size_ / 2; i++) {
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
}; // ... class SGAPopulation

#endif
