///////////////////////////////////////////////////
//  DTRA2 DVI OPT
//  Author(s): Patrick Price, Andrew O'Kins
//  Company: WSU ISP ASL
//

#ifndef INDIVIDUAL_H_
#define INDIVIDUAL_H_

#include <vector>

template <class T>
class Individual {
private:
	// The genome associated with the individual.
	std::vector<T>* genome_;

	// The fitness of the individual, this must be assigned with set_fitness
	// default value is -1 (an impossible real fitness value)
	double fitness_;

public:

	// Constructor, genome is initially null and fitness -1
	Individual(){
		this->genome_ = NULL;
		this->fitness_ = -1;
	}

	// Destructor
	~Individual(){
		delete this->genome_;
	}

	// Returns the array(genome) associated with the individual.
	std::vector<T>* genome() {
		return this->genome_;
	}

	// Returns the fitness associated with the individual.
	double fitness(){
		return this->fitness_;
	}

	// Sets the genome to be associated with the individual.
	// @param new_genome -> genome to be associated
	void set_genome(std::vector<T> * new_genome) {
		if (this->genome_ != NULL) {
			delete this->genome_;
		}
		this->genome_ = new_genome;
	}


	// Sets fitness to be associated with the individual.
	// @param fitness -> the fitness to be associated.
	void set_fitness(double fitness){
		this->fitness_ = fitness;
	}

}; // ... class Individual

// Comparative operator
template <typename T>
bool operator<(Individual<T> &a, Individual<T> &b) {
	return a.fitness() < b.fitness();
}

#endif
