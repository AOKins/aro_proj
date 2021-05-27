#ifndef POPULATION_H_
#define POPULATION_H_
#include "Individual.h"
#include "BetterRandom.h"
#include <vector>
template <class T>
class SGAPopulation{

private:

	// The array of individuals in the population.
	Individual<T>* individuals_;

	// Counter to remember what individual is being tested.
	int counter_;

	// Number of individuals in the population.
	int population_size_;

	// Percentage of genome that must be shared for images to be counted as similar
	double accepted_similarity_;

	// genome length
	int genome_length_;
	int genome_length;

public:

	// Constructor
	// @param genome_length -> the length of an individual's genome
	// @param accepted_similarity -> the percentage of genome that must be shared by two images to be counted as similar.
	// @param population_size -> the number of individuals in the population.
	SGAPopulation(int genome_length_, double accepted_similarity = .9, int population_size = 30)
	{
		population_size_ = population_size;
		this->genome_length_ = genome_length_;
		this->accepted_similarity_ = accepted_similarity;
		counter_ = 0;
		individuals_ = new Individual<T>[population_size_];

		// Initialize images.
		for (int i = 0; i < population_size_; i++) 
		{
			individuals_[i].set_genome(GenerateRandomImage());
		}//	... for each individual

	}

	// Destructor
	~SGAPopulation()
	{
		delete[] individuals_;
	}

	// Accessor for population_size
	int population_size()
	{
		return population_size_;
	}

	// Generates a random char image
	std::shared_ptr<std::vector<T>> GenerateRandomImage()
	{
		static BetterRandom ran(256);
		std::shared_ptr<std::vector<T>> image = std::make_shared<std::vector<T>>(genome_length_, 0);
		for (int j = 0; j < (genome_length_); j++) 
		{
			(*image)[j] = (T)ran();
		} // ... for each pixel in image
		return image;
	}

	// Gets the next image to be evaluated.
	std::shared_ptr<std::vector<T>> GetNextImage()
	{
		return individuals_[counter_].genome();
	}

	// Gets the image at a specific index in the population.
	// @param index -> the index of the population to get the genome of
	std::shared_ptr<std::vector<T>> GetImageAt(int index)
	{
		return individuals_[index].genome();
	}

	// Sets the fitness of the last individual called by GetNextImage, moves counter to next individual.
	// @param fitness -> assigned fitness
	void SetLastImageFitness(double fitness)
	{
		individuals_[counter_].set_fitness(fitness);
		counter_ = (counter_ + 1) % population_size_;
	}

	// Starts next generation using fitness of individuals.
	bool StartNextGeneration()
	{
		Individual<T>* sorted_temp = SortIndividuals(individuals_, population_size_);
		delete[] individuals_;
		individuals_ = sorted_temp;

		// calculate total fitness of all individuals (necessary for fitness proportionate selection)
		double fitness_sum = 0;
		for (int i = 0; i < population_size_; i++) 
		{
			fitness_sum += individuals_[i].fitness();
		}

		// Breeding
		bool same_check = true;
		Individual<T>* temp = new Individual<T>[population_size_];
		BetterRandom parent_selector(RAND_MAX);
		double divisor = RAND_MAX / fitness_sum;

		// for each new individual
		for (int i = 0; i < 25; i++) // todo: magic number here, make that pop-size - num-elites
		{
			// select first parent with fitness proportionate selection
			double selected = parent_selector() / divisor;
			double temp_sum = individuals_[0].fitness();
			int j = 0;
			while (temp_sum < selected)
			{
				j++;
				temp_sum += individuals_[j].fitness();
			}
			std::shared_ptr<std::vector<T>> temp_image1 = individuals_[j].genome();

			// Select second parent with fitness proportionate selection
			selected = parent_selector() / divisor;
			temp_sum = individuals_[0].fitness();
			j = 0;
			while (temp_sum < selected)
			{
				j++;
				temp_sum += individuals_[j].fitness();
			}
			std::shared_ptr<std::vector<T>> temp_image2 = individuals_[j].genome();

			// perform crossover
			temp[i].set_genome(Crossover(temp_image1, temp_image2, same_check));
		}

		// for the elites, copy directly into new generation
		for (int i = 25; i < population_size_; i++) 
		{
			DeepCopyIndividual(temp[i], individuals_[i]);
		}

		// if all of our individuals are labeled similar, replace half of them with new images
		if (same_check) 
		{
			for (int i = 0; i < population_size_ / 2; i++) 
			{
				temp[i].set_genome(GenerateRandomImage());
			}
		}
		delete[] individuals_;
		individuals_ = temp;
		counter_ = 0;
		return true;

	}	// ... Function nextGeneration

	// Crosses over information between individuals.
	// @param a -> First individual to be crossed over.
	// @param b -> Second individual to be crossed over.
	// @param same_check -> Boolean will be turned to false if the arrays are different.
	std::shared_ptr<std::vector<T>> Crossover(std::shared_ptr<std::vector<T>> a, std::shared_ptr<std::vector<T>> b, bool& same_check)
	{
		std::shared_ptr<std::vector<T>> temp = std::make_shared<std::vector<T>>(genome_length_, 0);
		double same_counter = 0; // counter keeping track of how many indices in the genomes are the same
		static BetterRandom ran(100);
		static BetterRandom mut(200);
		static BetterRandom mutatedValue(256);
		for (int i = 0; i < genome_length_; i++)
		{
			// for each index in the genome, 50% chance of coming from either parent
			if (ran() < 50) 
			{
				(*temp)[i] = (*a)[i];
			}
			else 
			{
				(*temp)[i] = (*b)[i];
			}

			// if the values at an index are the same, increment the same counter
			if ((*a)[i] == (*b)[i])
			{
				same_counter += 1;
			}

			// mutation
			if (mut() == 0) 
			{
				(*temp)[i] = (T)mutatedValue();
			}
		}
		// ... End image creation

		// if the percentage of indices that are the same is less than the accepted similarity, label the two genomes as not the same (same_check = false)
		same_counter /= genome_length_;
		if (same_counter < accepted_similarity_) 
		{
			same_check = false;
		}
		return temp;
	}

	// Sorts an array of individuals
	// @param to_sort -> the individuals to be sorted
	// @param size -> the size of the array to_sort
	Individual<T> *SortIndividuals(Individual<T> *to_sort, int size) 
	{
		// todo: use a sort function provided by c++ instead of this sad little insertion sort
		bool found = false;
		Individual<T> *temp = new Individual<T>[size];
		DeepCopyIndividual(temp[0], to_sort[0]);
		for (int i = 1; i < size; i++) 
		{
			for (int j = 0; j < i; j++) 
			{
				if (to_sort[i].fitness() < temp[j].fitness())
				{
					found = true;
					// insert individual
					Individual<T> holder1;
					DeepCopyIndividual(holder1, to_sort[i]);
					Individual<T> holder2;
					for (int k = j; k < i; k++)
					{
						DeepCopyIndividual(holder2, temp[k]);
						DeepCopyIndividual(temp[k], holder1);
						DeepCopyIndividual(holder1, holder2);
					}
					DeepCopyIndividual(temp[i], holder1);
					break;
				}
			}
			if (!found) 
			{
				DeepCopyIndividual(temp[i], to_sort[i]);
			}
			else 
			{
				found = false;
			}
		}
		return temp;
	}

	// Deep copies the image from one individual to another
	// @param to -> the individual to be copied to
	// @param from -> the individual copied
	void DeepCopyIndividual(Individual<T> &to, Individual<T> &from) 
	{
		to.set_fitness(from.fitness());
		std::shared_ptr<std::vector<T>> temp_image1 = std::make_shared<std::vector<T>>(genome_length_, 0);
		std::shared_ptr<std::vector<T>> temp_image2 = from.genome();
		for (int i = 0; i < genome_length_; i++)
		{
			(*temp_image1)[i] = (*temp_image2)[i];
		}
		to.set_genome(temp_image1);
	}

}; // ... class Population

#endif