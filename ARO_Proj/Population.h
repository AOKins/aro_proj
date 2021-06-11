////////////////////
// Base class to encapsulate a population in a genetic algorithm and some of the micro-genetic behaviors
// Last edited: 06/09/2021 by Andrew O'Kins
////////////////////
#ifndef POPULATION_H_
#define POPULATION_H_
#include "Individual.h"
#include "BetterRandom.h"
#include "Utility.h" // For printLine

#include <thread> // For ind_threads and general multithreaded behavior
using std::thread;

////
// TODOs:	Consider update sortIndividuals to use more efficient algorithm
//					(currently using insertion sort but with small size may not be needed/gain much)
//			Consider how to handle possible case of elite_size exceeding pop_size (necessarry?) currently gives warning
////

template <class T>
class Population {
protected:
	// The array of individuals in the population.
	shared_ptr<Individual<T>> individuals_;
	// Number of individuals in the population.
	int pop_size_;
	// Number of individuals in the population that are to be kept as elite.
	int elite_size_;
	// Percentage of genome that must be shared for images to be counted as similar
	double accepted_similarity_;
	// genome length for individual images
	int genome_length_;
	// vector for managing the multithreads for individuals used in genetic algorithm
	vector<thread> ind_threads;
public:
	// Constructor
	// Input:
	//	genome_length:	the image size (genome) for an individual
	//	population_size: the number of individuals for the population
	//	elite_size:		 the number of individuals for the population that are kept as elite
	//	accepted_similarity: precentage of similarity to be counted as same between individuals (default 90%)
	Population(int genome_length, int population_size, int elite_size, double accepted_similarity = .9){
		this->genome_length_ = genome_length;
		this->accepted_similarity_ = accepted_similarity;
		this->pop_size_ = population_size;
		this->elite_size_ = elite_size;
		// Clearing just in case there's something there beforehand
		this->ind_threads.clear();

		// Check to see if elite size exceeds the population size, currently just gives warning
		if (this->elite_size_ > this->pop_size_) {
			Utility::printLine("WARNING: Elite size (" + std::to_string(this->elite_size_) + ") of population exceeding population size (" + std::to_string(this->pop_size_) + ")!");
		}

		this->individuals_ = new Individual<T>[pop_size_];

		//initialize images for each individual
		// Lambda function to ensure that generating random image is done in parallel
			// Input: id - index for individual to be set
			// Captures
			//		individuals_ - pointer to array of individuals to store new random genomes in
		auto randInd = [this](int id) {
			this->individuals_[id].set_genome(this->GenerateRandomImage());
		};
		// Using multithreads for initializing each individual
		for (int i = 0; i < this->pop_size_; i++){
			this->ind_threads.push_back(thread(randInd, i));
		}
		rejoinClear();
	}

	//Destructor - delete individuals and call rejoinClear()
	~Population() {
		delete[] this->individuals_;
		rejoinClear();
	}

	// Get number of individuals in population
	int getSize() {
		return this->pop_size_;
	}

	// Get number of elite individuals in population
	int getEliteSize() {
		return this->elite_size_;
	}

	// Generates a random image using BetterRandom
	// Output: a randomly generated image that has size of genome_length for Population
	shared_ptr< vector<T> > GenerateRandomImage() {
		static BetterRandom ran(256);
		shared_ptr< vector<T>> image = std::make_shared<vector<T>>(this->genome_length_,0);
		for (int j = 0; j < this->genome_length_; j++) {
			(*image)[j] = (T)ran();
		} // ... for each pixel in image
		return image;
	}

	// Getter for image of individual at inputted index
	// Input: i - individual at given index (population not guranteed sorted)
	// Output: the image for the individual
	shared_ptr< vector<T>> getImage(int i){
		return this->individuals_[i].genome();
	}

	// Setter for the fitness of the individual at given index
	// Input:
	//	i - individual at given index (population not guranteed sorted)
	//	fitness - the fitness value to set to individual
	// Output: individual at index i has its set_fitness() called with fitness as input
	void setFitness(int i, double fitness) {
		this->individuals_[i].set_fitness(fitness);
	}

	// Crosses over information between individuals.
	// Input:
	//	a - First individual to be crossed over.
	//	b - Second individual to be crossed over.
	//	same_check - boolean will be set to false if the arrays are different.
	//  useMutation - boolean set if to perform mutation or not, defaults to true (enable).
	// Output: returns new individual as result of crossover algorithm
	shared_ptr< vector<T>> Crossover(shared_ptr< vector<T>> a, shared_ptr< vector<T>> b, bool& same_check, bool useMutation = true) {
		shared_ptr< vector<T>> temp = std::make_shared<vector<T>>(this->genome_length_, 0);
		double same_counter = 0; // counter keeping track of how many indices in the genomes are the same
		static BetterRandom ran(100);
		static BetterRandom mut(200);
		static BetterRandom mutatedValue(256);

		// For each index in the genome
		for (int i = 0; i < this->genome_length_; i++) {
			// 50% chance of coming from either parent
			if (ran() < 50) {
				(*temp)[i] = (*a)[i];
			}
			else {
				(*temp)[i] = (*b)[i];
			}
			// if the values at an index are the same, increment the same counter
			if ((*a)[i] == (*b)[i])	{
				same_counter += 1;
			}
			// mutation occuring if useMutation and at 0.05% chance
			if (mut() == 0 && useMutation)	{
				(*temp)[i] = (T)mutatedValue();
			}
		} // ... End image creation

		// if the percentage of indices that are the same is less than the accepted similarity, label the two genomes as not the same (same_check = false)
		same_counter /= this->genome_length_;
		if (same_counter < this->accepted_similarity_) {
			same_check = false;
		}
		return temp;
	}

	// Sorts an array of individuals
	//		Currently (June 7 2021) this is using insertion sort and DeepCopyIndividual() method
	// Input:
	//	to_sort - the individuals to be sorted
	//	size - the size of the array to_sort
	// Output: returns sorted pointer array of individuals originating from to_sort
	Individual<T>* SortIndividuals(Individual<T> *to_sort, int size) {
		bool found = false;
		Individual<T> *temp = new Individual<T>[size];
		DeepCopyIndividual(temp[0], to_sort[0]);
		for (int i = 1; i < size; i++) {
			for (int j = 0; j < i; j++)	{
				if (to_sort[i].fitness() < temp[j].fitness()) {
					found = true;
					// insert individual
					Individual<T> holder1;
					DeepCopyIndividual(holder1, to_sort[i]);
					Individual<T> holder2;
					for (int k = j; k < i; k++)	{
						DeepCopyIndividual(holder2, temp[k]);
						DeepCopyIndividual(temp[k], holder1);
						DeepCopyIndividual(holder1, holder2);
					}
					DeepCopyIndividual(temp[i], holder1);
					break;
				}
			}
			if (!found) {
				DeepCopyIndividual(temp[i], to_sort[i]);
			}
			else {
				found = false;
			}
		}
		return temp;
	}

	// Deep copies the image from one individual to another
	// Input:
	//	to - the individual to be copied to
	//	from - the individual copied
	// Output: to is contains deep copy of from
	void DeepCopyIndividual(Individual<T> &to, Individual<T> &from) {
		to.set_fitness(from.fitness());
		shared_ptr< vector<T>> temp_image1 = std::make_shared<vector<T>>(this->genome_length_, 0);
		shared_ptr< vector<T>> temp_image2 = from.genome();
		for (int i = 0; i < this->genome_length_; i++) {
			(*temp_image1)[i] = (*temp_image2)[i];
		}
		to.set_genome(temp_image1);
	}

	// Rejoin all the threads and clear ind_threads vector for future use
		// Note: if a thread is stuck in an indefinite duration, this will lock out
	void rejoinClear() {
		for (int i = 0; i < this->ind_threads.size(); i++) {
			this->ind_threads[i].join();
		}
		this->ind_threads.clear();
	}

	// Perform the genetic algorithm to create new individuals for next gneeration
	// virtual method to have child classes define this behavior
	// Output: False if error occurs, otherwise True
	virtual bool nextGeneration() = 0;

}; // ... class Population

#endif
