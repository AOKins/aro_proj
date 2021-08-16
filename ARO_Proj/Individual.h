////////////////////
//  Inividual.h - 
//  Author(s): Patrick Price, Andrew O'Kins
//  Company: WSU ISP ASL
////////////////////

#ifndef INDIVIDUAL_H_
#define INDIVIDUAL_H_

#include <vector>

template <class T>
class Individual {
private:
	// The genome associated with the individual.
	std::vector<T>* genome_;

	// The fitness of the individual, this must be assigned with set_fitness
	// default value is -1 (an impossible real fitness value, so used to identify non-evualeted)
	double fitness_;

public:
	// Constructor, genome is initially null and fitness -1
	Individual() {
		this->genome_ = NULL;
		this->fitness_ = -1;
	}

	// Destructor
	~Individual() {
		if (this->genome_ != NULL) {
			delete this->genome_;
		}
	}

	// Returns the array(genome) associated with the individual.
	std::vector<T>* genome() {
		return this->genome_;
	}

	// Returns the fitness associated with the individual.
	double fitness() {
		return this->fitness_;
	}

	// Sets the genome to be associated with the individual.
	// Input: new_genome - genome to be set to this individual
	// Output: new_genome is assigned, old genome is deleted
	void set_genome(std::vector<T> * new_genome) {
		if (this->genome_ != NULL) {
			delete this->genome_;
		}
		this->genome_ = new_genome;
	}

	// Sets fitness to be associated with the individual.
	// Input: fitness - the fitness to be associated.
	// Output: this->fitness is assigned input value
	void set_fitness(double fitness){
		this->fitness_ = fitness;
	}

	// Overloaded = operator to perform deep copy of genome data
	template <typename T>
	Individual<T>& operator=(const Individual<T> &from) {
		// If they are the same
		if (this == &from){
			return *this;
		}
		// Copy fitness
		this->set_fitness(from.fitness());
		// Deep Copy genome
			// Create temp to hold copy of equal size
		std::vector<T> * temp_genome = new std::vector<T>(from.genome_()->size(), 0);
			// Iterate through to copy each value
		for (int i = 0; i < from.genome_()->size(); i++) {
			(*temp_genome)[i] = (*from.genome())[i];
		}
			// Set resulting deep copy
		this->set_genome(temp_genome);

		return *this;
	}
}; // ... class Individual

// Comparative operator
template <typename T>
bool operator<(Individual<T> &a, Individual<T> &b) {
	return a.fitness() < b.fitness();
}

#endif
