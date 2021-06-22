////////////////////
// Population handler for simple genetic algorithm that inherits from base Population
// Last edited: 06/21/2021 by Andrew O'Kins
////////////////////
#ifndef SGAPOPULATION_H_
#define SGAPOPULATION_H_
#include "Population.h"
#include "BetterRandom.h"

////
// TODOs:	Debug mulithreading in StartNextGeneration
//			General code polishin

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

	// Starts next generation using fitness of individuals.  Following the simple genetic algorithm approach.
	bool nextGeneration() {
		// Setting individuals to sorted (best is at end of the array)
		Individual<T>* sorted_temp = SortIndividuals(this->individuals_, this->pop_size_);
		delete[] this->individuals_;
		this->individuals_ = sorted_temp;

		// calculate total fitness of all individuals (necessary for fitness proportionate selection)
		double fitness_sum = 0;
		for (int i = 0; i < this->pop_size_; i++) {
			fitness_sum += this->individuals_[i].fitness();
		}

		// Breeding
			// Size of same_check is being how many new individuals are being made
		bool * same_check = new bool[(this->pop_size_ - this->elite_size_)];
		Individual<T> * temp = new Individual<T>[this->pop_size_];
		BetterRandom parent_selector(RAND_MAX);
		double divisor = RAND_MAX / fitness_sum;

		// Lambda function to be used for generating new individual
		// Input: i - index for new individual
		// Captures:
		//		temp - pointer to array of individuals to store current new individual at temp[i]
		//		parent_selector - RNG machine captured by reference
		//		divisor - used in proportionate selection
		//		individuals_ access population genomes
		// Output: temp[i] is set a new genome using crossover algorithm and mutation enabled
		auto genInd = [temp, &parent_selector, divisor, same_check, this](int i) {
			same_check[i] = true;
			// select first parent with fitness proportionate selection and store associated genome into temp_image1
			double selected = parent_selector() / divisor;
			double temp_sum = this->individuals_[0].fitness(); // Recall 0 index has worst fitness
			int j = 0;
			while (temp_sum < selected) {
				j++;
				temp_sum += this->individuals_[j].fitness();
			}
			vector<T> * parent1 = this->individuals_[j].genome();

			// Select second parent with fitness proportionate selection and store associated genome into temp_image2
			selected = parent_selector() / divisor;
			temp_sum = this->individuals_[0].fitness();
			j = 0;
			while (temp_sum < selected) {
				j++;
				temp_sum += this->individuals_[j].fitness();
			}
			vector<T> * parent2 = this->individuals_[j].genome();

			// perform crossover with mutation
			temp[i].set_genome(Crossover(parent1, parent2, same_check[i], true));
		}; // ... genInd(i)

		// for each new individual a thread with call to genInd()
		for (int i = 0; i < (this->pop_size_ - this->elite_size_); i++) {
			this->ind_threads.push_back(thread(genInd, i));
		}
		Utility::rejoinClear(this->ind_threads);		// Rejoin

		// for the elites, copy directly into new generation
		// Performing deep copy for individuals in parallel
		for (int id = (this->pop_size_ - this->elite_size_); id < this->pop_size_; id++) {
			// Lambda function for using DeepCopyIndividual in parallel
			// Input: id - index for individual to be copied from individuals_ and to temp
			// Captures: temp - pointer to array of individuals to store in
			//  		 this - current Population instance for using appropriate methods
			this->ind_threads.push_back(thread(
				[this, temp](int id){this->DeepCopyIndividual(temp[id], this->individuals_[id]);}, id));
		}
		Utility::rejoinClear(this->ind_threads);		// Rejoin
		
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
			// Calling generate random image for half of pop individuals
			for (int i = 0; i < this->pop_size_ / 2; i++) {
				// Lambda function to ensure that generating random image is done in parallel
				// Input: id - index for individual to be set
				// Captures: temp - pointer to array of individuals to store new random genomes in
				//  		 this - current Population instance for using appropriate methods
				this->ind_threads.push_back(thread(
					[temp, this](int id) {temp[id].set_genome(this->GenerateRandomImage());}, i)
				);
			}
			Utility::rejoinClear(this->ind_threads);			// Rejoin
		}

		delete[] same_check;
		// Assign new population to individuals_
		delete[] this->individuals_;
		this->individuals_ = temp;

		return true; // No issues!
	}	// ... Function nextGeneration
}; // ... class SGAPopulation

#endif
