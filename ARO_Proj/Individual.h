///////////////////////////////////////////////////
//  DTRA2 DVI OPT
//  Author: Patrick Price
//  Company: WSU ISP ASL
//	
//

#ifndef INDIVIDUAL_H_
#define INDIVIDUAL_H_
#include "BetterRandom.h"
#include <vector>
using std::vector;

template <class T, class img>
class Individual {
private:

	// The genome associated with the individual.
	vector<T>* genome_;

	// The fitness of the individual, this must be assigned with set_fitness
	double fitness_;

	int imgWidth_, imgHeight_; // The dimensions of the resulting image
	img* resultImage_; // Pointer to content of the image
public:

	// Constructor
	Individual(){
		this->genome_ = NULL;
		this->resultImage_ = NULL;
	}

	// Destructor
	~Individual(){
		delete this->genome_;
		delete this->resultImage_;
	}

	// Returns the array(genome) associated with the individual.
	vector<T>* genome() {
		return this->genome_;
	}

	// Returns the fitness associated with the individual.
	double fitness(){
		return this->fitness_;
	}

	// Sets the genome to be associated with the individual.
	// @param new_genome -> genome to be associated
	void set_genome(vector<T> * new_genome) {
		if (this->genome_ != NULL) {
			delete this->genome_;
		}
		this->genome_ = new_genome;
	}

	// Set the width and height of the image for this individual
	void set_dimensions(int imgWidth, imgHeight) {
		this->imgWidth_ = imgWidth;
		this->imgHeight_ = imgHeight;
	}

	// Set the pointer to resulting image data
	void set_image(img * image) {
		if (this->resultImage_ _ != NULL) {
			delete this->resultImage_;
		}
		this->resultImage_ = image;
	}

	img* image() {
		return this->image;
	}

	int img_width() {
		return this->imgWidth_;
	}

	int img_height() {
		return this->imgHeight_;
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
