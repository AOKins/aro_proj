#ifndef POPULATION_H_
#define POPULATION_H_
#include "Individual.h"
#include "RandomInt.h"
#include <vector>
#include <fstream>

template <class T>
class Population{
private:
	//the array of individuals in the population
	Individual<T>* individuals_;
	//counter to track what individual is being tested
	int counter_;
	//number of individuals in the population
	int size_;

	double accepted_similarity_;
	int image_height_;
	int image_width_;
	int image_density_;
	T* saved_peak_image_;
	double saved_peak_value_;
	int warning_;
	int trust_counter_;
	int gen_;

	std::ofstream tfile;

public:
	//constructor
	//@param size -> the number of individuals in the population
	Population(int image_height, int image_width, int image_density, double accepted_similarity = .9, int size = 5){
		size_ = size;
		image_density_ = image_density;
		image_height_ = image_height;
		image_width_ = image_width;
		accepted_similarity_ = accepted_similarity;
		trust_counter_ = 0;
		warning_ = 0;
		counter_ = 0;
		saved_peak_image_ = new T[image_density*image_height*image_width];
		for (int i = 0; i < image_density*image_height*image_width; i++){
			saved_peak_image_[i] = 1;
		}
		individuals_ = new Individual<T>[size_];

		//initialize images
		for (int i = 0; i < size_; i++){
			individuals_[i].SetImage(RandomImage());
		}//for each individual

		tfile.open("test.txt", std::ios::app);
	}

	//destructor
	~Population(){
		tfile.close();
		remove("test.txt");
		delete[] individuals_;
	}

	int GetSize(){
		return size_;
	}

	//char random image
	T* RandomImage(){
		static RandomInt ran(256);
		T* image = new T[image_height_*image_width_*image_density_];
		for (int j = 0; j < (image_density_*image_height_*image_width_); j++){
			image[j] = (T)ran();
		}//for each pixel in image
		return image;
	}

	//gets the next image to be evaluated
	T* GetImage(){
		return individuals_[counter_].GetImage();
	}

	//sets the fitness of the last individual called by GetImage
	void SetFitness(double fitness){
		individuals_[counter_].SetFitness(fitness);
		tfile << counter_ << " " << fitness << std::endl;
		counter_ = (counter_ + 1) % size_;
	}

	//starts next generation using fitness of individuals
	bool NextGeneration(int gennum){
		gen_ = gennum;
		int first = 0;
		int second = 1;
		int third = 2;

		//tournament
		for (int i = 1; i < size_; i++){
			if (individuals_[i].GetFitness() > individuals_[first].GetFitness() && i > 1){
				third = second;
				second = first;
				first = i;
			}
			else if (individuals_[i].GetFitness() > individuals_[first].GetFitness() && i == 1){
				int temp = first;
				first = second;
				second = temp;
			}
			else if (individuals_[i].GetFitness() > individuals_[second].GetFitness() && i != first){
				third = second;
				second = i;
			}
			else if (individuals_[i].GetFitness() > individuals_[third].GetFitness() && i != first && i != second){
				third = i;
			}
		}

		if (individuals_[0].GetFitness() < (saved_peak_value_ * .75)){
			warning_++;
		}
		else {
			warning_ = 0;
		}

		if (first == 0 && individuals_[0].GetFitness() >= saved_peak_value_){
			trust_counter_++;
		}
		else {
			trust_counter_ = 0;
		}

		if (trust_counter_ == 2){
			T* tmpptr = individuals_[0].GetImage();
			for (int i = 0; i < image_density_*image_height_*image_width_; i++){
				saved_peak_image_[i] = tmpptr[i];
			}
			saved_peak_value_ = individuals_[0].GetFitness();
		}

		///////////////////test
		ofstream efile("elites.txt", std::ios::app);
		efile << "Gen: " << gennum << " Elite: " << first << std::endl;
		efile.close();
		///////////////////

		//crossover
		Individual<T>* temp = new Individual<T>[size_];
		bool sameCheck = true;
		temp[0].SetImage(Crossover(individuals_[first].GetImage(), individuals_[first].GetImage(), sameCheck));
		temp[1].SetImage(Crossover(individuals_[second].GetImage(), individuals_[first].GetImage(), sameCheck));
		temp[2].SetImage(Crossover(individuals_[third].GetImage(), individuals_[first].GetImage(), sameCheck));
		temp[3].SetImage(Crossover(individuals_[third].GetImage(), individuals_[second].GetImage(), sameCheck));
		temp[4].SetImage(Crossover(individuals_[second].GetImage(), individuals_[third].GetImage(), sameCheck));

		if (sameCheck){
			for (int i = 1; i < size_; i++){
				temp[i].SetImage(RandomImage());
			}
		}

		if (warning_ > 1){
			///////////////////////test
			ofstream tfile("reload.txt", std::ios::app);
			tfile << "Elite reloaded in gen: " << gen_ << " Peak: " << saved_peak_value_ << std::endl;
			tfile.close();
			///////////////////////
			T* tmpptr = new T[image_density_*image_height_*image_width_];
			for (int i = 0; i < image_density_*image_height_*image_width_; i++){
				tmpptr[i] = saved_peak_image_[i];
			}
			temp[1].SetImage(tmpptr);
		}
		delete[] individuals_;
		individuals_ = temp;
		counter_ = 0;
		return true;
	}// ... function NextGeneration

	//Crosses over information between individuals
	//@param a -> first individual to be crossed over
	//@param b -> second individual to be crossed over
	//@param sameCheck -> boolian will be turned to false if the arrays are different
	T* Crossover(T* a, T* b, bool &same_check){
		T* temp = new T[image_density_*image_height_*image_width_];
		double sameCounter = 0;
		RandomInt ran(100);
		for (int i = 0; i < image_density_*image_height_*image_width_; i++){
			if (ran() < 50){
				temp[i] = a[i];
			}
			else{
				temp[i] = b[i];
			}
			if (a[i] == b[i]) sameCounter += 1;
		}
		// ... end image creation
		
		sameCounter /= image_density_*image_height_*image_width_;

		if (sameCounter < accepted_similarity_){
			same_check = false;
		}
		return temp;
	}
}; // ... class Population

#endif