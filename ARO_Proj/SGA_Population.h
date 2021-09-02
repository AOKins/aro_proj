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
	// _threadCount:		 when multithread is enabled, defines how many threads this population will use
	//  myThreadPool:		 set the thread pool to be used when multithreading enabled
	SGAPopulation(int genome_length, int population_size, int elite_size, double accepted_similarity = .9, bool multiThread = true, int _threadCount = std::thread::hardware_concurrency(), threadPool * myThreadPool = NULL)
		: Population<T>(genome_length, population_size, elite_size, accepted_similarity, multiThread, _threadCount, myThreadPool) {};

	// Starts next generation using fitness of individuals.  Following the simple genetic algorithm approach.
	bool nextGeneration() {
		// Setting individuals to sorted (best is at end of the array)
		SortIndividuals(this->individuals_, this->pop_size_);

		// calculate total fitness of all individuals (necessary for fitness proportionate selection)
		double fitness_sum = 0;
		for (int i = 0; i < this->pop_size_; i++) {
			fitness_sum += this->individuals_[i].fitness();
		}

		// Breeding
		Individual<T> * temp = new Individual<T>[this->pop_size_];
		Individual<T> * pool = this->individuals_;
		const double divisor = RAND_MAX / fitness_sum;

		// Lambda function to be used for generating new individual
		// Input: i - index for new individual
		// Captures:
		//		temp			- pointer to array of individuals to store current new individual at temp[i]
		//		parent_selector - RNG machine captured by reference
		//		divisor			- used in proportionate selection
		//		individuals_	- access population genomes
		// Output: temp[i] is set a new genome using crossover algorithm and mutation enabled
		auto genInd = [temp, divisor, pool, this](int i) {
			BetterRandom parent_selector(RAND_MAX);

			this->same_check[i] = true;
			// select first parent with fitness proportionate selection and store associated genome into temp_image1
			double selected = parent_selector() / divisor;
			double temp_sum = pool[0].fitness(); // Recall 0 index has worst fitness
			int j = 0;
			while (temp_sum < selected) {
				j++;
				temp_sum += pool[j].fitness();
			}
			const T * parent1 = pool[j].genome();

			// Select second parent with fitness proportionate selection and store associated genome into temp_image2
			selected = parent_selector() / divisor;
			temp_sum = pool[0].fitness();
			j = 0;
			while (temp_sum < selected) {
				j++;
				temp_sum += pool[j].fitness();
			}
			const T * parent2 = pool[j].genome();

			// perform crossover with mutation
			temp[i].set_genome(this->Crossover(parent1, parent2, same_check[i], true));
		}; // ... genInd(i)

		// Lambda function for multithreading to perform generation of next pool with fewer given threads
		// Input: threadID - current thread index
		//		  numThreads - total number of threads being launched
		// Captures: temp - array of individuals to store results into
		//			parent_selector - RNG machine for selecting parents (used in genInd) and captured by reference
		//			divisor - value used in conjunction with parent_selector to select individual (used in genInd) and captured by reference
		//			this - pointer to current population instance
		auto genSubGroup = [temp, pool, divisor, genInd, this](const int threadID, const int numThreads, const int pop_size_, const int elite_size_) {
			int groupSize = pop_size_ / numThreads;
			int remainder = pop_size_ - groupSize*numThreads;
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
			for (int id = start_index; id < start_index + groupSize && id < pop_size_; id++) {
				if (id < (pop_size_ - elite_size_)) {
					// Produce New Individuals
					genInd(id);
				}
				else { // Carry Elites
					this->DeepCopyIndividual(temp[id], pool[id]);
				}
			}
		}; // .. genSubGroup

		if (this->multiThread_) { // Parallel
			for (int i = 0; i < this->threadCount_; i++) {
				this->myThreadPool_->pushJob(std::bind(genSubGroup, i, this->threadCount_, this->pop_size_, this->elite_size_));
			}
			this->myThreadPool_->wait();
		}
		else { // Serial
			// Produce New Individuals
			genSubGroup(0, 1, this->pop_size_, this->elite_size_);
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
			// Lambda function to capture thread behavior
			// Input: threadID - current thread
			//		  numThreads - total number of threads being launched
			//		  pop_size - population size to be randomizing
			//		  genome_length - size of the individual images to randomly generate with
			// Captures - temp - array of individuals to store new random genomes within
			auto randSubGroup = [temp](const int threadID, const int numThreads, int pop_size, int genome_length) {
				int groupSize = (pop_size) / (numThreads);
				int remainder = (pop_size)-groupSize*numThreads;
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
				for (int id = start_index; id < start_index + groupSize && id < pop_size; id++) {
					temp[id].set_genome(Utility::generateRandomImage<T>(genome_length));
				}
			}; // .. randSubGroup

			// Calling generate random image for half of pop individuals
			if (this->multiThread_ == true) {
				for (int i = 0; i < this->threadCount_; i++) {
					this->myThreadPool_->pushJob(std::bind(randSubGroup, i, this->threadCount_, this->pop_size_ / 2, this->genome_length_));
				}
				this->myThreadPool_->wait();
			}
			else {
				for (int i = 0; i < this->pop_size_ / 2; i++) {
					temp[i].set_genome(Utility::generateRandomImage<T>(this->genome_length_));
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
