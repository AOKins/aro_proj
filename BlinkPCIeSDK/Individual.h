///////////////////////////////////////////////////
//  DTRA2 DVI OPT
//  Author: Patrick Price
//  Company: WSU ISP ASL
//	
//

#ifndef INDIVIDUAL_H_
#define INDIVIDUAL_H_
#include "BetterRandom.h"
#include <memory>

template <class T>
class Individual {
private:

	// The genome associated with the individual.
	std::shared_ptr<std::vector<T>> genome_;

	// The fitness of the individual, this must be assigned with set_fitness
	double fitness_;

public:

	// Constructor
	Individual(){
	}

	// Destructor
	~Individual(){
	}

	// Returns the array(genome) associated with the individual.
	std::shared_ptr<std::vector<T>> genome(){
		return genome_;
	}

	// Returns the fitness associated with the individual.
	double fitness(){
		return fitness_;
	}

	// Sets the genome to be associated with the individual.
	// @param new_genome -> genome to be associated
	void set_genome(std::shared_ptr<std::vector<T>> new_genome){
		genome_ = new_genome;
	}

	// Sets fitness to be associated with the individual.
	// @param fitness -> the fitness to be associated.
	void set_fitness(double fitness){
		this->fitness_ = fitness;
	}

}; // ... class Individual

template <typename T>
bool operator<(Individual<T> &a, Individual<T> &b) {
	return a.fitness() < b.fitness();
}

#endif
