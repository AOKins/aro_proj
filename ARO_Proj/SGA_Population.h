////////////////////
// SGA_Population.h - handler for simple genetic algorithm that inherits from base Population
// Last edited: 08/24/2021 by Andrew O'Kins
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
		Individual<T> * temp = new Individual<T>[this->pop_size_];
		BetterRandom parent_selector(RAND_MAX);
		const double divisor = RAND_MAX / fitness_sum;

		// Lambda function to be used for generating new individual
		// Input: i - index for new individual
		// Captures:
		//		temp			- pointer to array of individuals to store current new individual at temp[i]
		//		parent_selector - RNG machine captured by reference
		//		divisor			- used in proportionate selection
		//		individuals_	- access population genomes
		// Output: temp[i] is set a new genome using crossover algorithm and mutation enabled
		auto genInd = [temp, &parent_selector, &divisor, this](int i) {
			this->same_check[i] = true;
			// select first parent with fitness proportionate selection and store associated genome into temp_image1
			double selected = parent_selector() / divisor;
			double temp_sum = this->individuals_[0].fitness(); // Recall 0 index has worst fitness
			int j = 0;
			while (temp_sum < selected) {
				j++;
				temp_sum += this->individuals_[j].fitness();
			}
			const std::vector<T> * parent1 = this->individuals_[j].genome();

			// Select second parent with fitness proportionate selection and store associated genome into temp_image2
			selected = parent_selector() / divisor;
			temp_sum = this->individuals_[0].fitness();
			j = 0;
			while (temp_sum < selected) {
				j++;
				temp_sum += this->individuals_[j].fitness();
			}
			const std::vector<T> * parent2 = this->individuals_[j].genome();

			// perform crossover with mutation
			temp[i].set_genome(Crossover(parent1, parent2, same_check[i], true));
		}; // ... genInd(i)


		// Use genInd and deep copy, but have fewer threads that perform a group of individual tasks to reduce resources required
		auto genSubGroup = [temp, &parent_selector, &divisor, &genInd, this](const int threadID, const int numThreads) {
			int groupSize = this->pop_size_ / numThreads;
			int remainder = this->pop_size_ - groupSize*numThreads;
			int start_index = threadID*groupSize;

			if (remainder != 0) {
				if (threadID < remainder) {
					groupSize++;
					start_index += threadID;
				}
				else {
					start_index += remainder;
				}
			}
			for (int id = start_index; id < start_index + groupSize; id++) {
				if (id < (this->pop_size_ - this->elite_size_)) {
					// Produce New Individuals
					genInd(id);
				}
				else { // Carry Elites
					this->DeepCopyIndividual(temp[id], this->individuals_[id]);
				}
			}
		}; // .. genSubGroup

		if (this->multiThread_) { // Parallel
			const int num_threads = std::thread::hardware_concurrency();

			for (int i = 0; i < num_threads; i++) {
				this->ind_threads.push_back(std::thread(genSubGroup, i, num_threads));
			}

			Utility::rejoinClear(this->ind_threads);		// Rejoin
		}
		else { // Serial
			// Produce New Individuals
			for (int i = 0; i < (this->pop_size_ - this->elite_size_); i++) {
				genInd(i);
			}
			// Carry Elites
			for (int i = (this->pop_size_ - this->elite_size_); i < this->pop_size_; i++) {
				this->DeepCopyIndividual(temp[i], this->individuals_[i]);
			}
		}

		// Collect the resulting same_check values,
		// if at least one is false (not similar) then the result is set to false
		bool same_check_result = true;
		for (int i = 0; i < (this->pop_size_ - this->elite_size_) && same_check_result; i++) {
			if (this->same_check[i] == false) {
				same_check_result = false;
			}
		}

		// if all of our individuals are labeled similar, replace half of them with new images
		if (same_check_result) {
			auto randInd = [temp, this](int id) {temp[id].set_genome(this->GenerateRandomImage()); };
			// Calling generate random image for half of pop individuals
			for (int i = 0; i < this->pop_size_ / 2; i++) {
				if (this->multiThread_) {
					this->ind_threads.push_back(std::thread(randInd, i));
				}
				else {
					randInd(i);
				}
			}
		}

		// Assign new population to individuals_
		delete[] this->individuals_;
		this->individuals_ = temp;
		return true; // No issues!
	}	// ... Function nextGeneration

}; // ... class SGAPopulation

#endif
