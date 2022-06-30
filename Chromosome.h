#pragma once
#include <vector>

using namespace std;

class Chromosome
{
public:
	int id = -1;

	vector<int> pattern;
	vector<int> initialPattern;

	int avgPopulation = 0;
	int nOfGenerations = 0;
	int initialSize = 0;

	double fitness = 0.0;

	unordered_map<int, string> cells;
	unordered_map<string, unordered_set<int>> statePositions;

	inline bool operator>(Chromosome& c)
	{
		return this->fitness > c.fitness;
	}

	inline bool operator<(Chromosome& c)
	{
		return this->fitness < c.fitness;
	}

	inline bool operator==(Chromosome& c)
	{
		return this->fitness == c.fitness;
	}

	inline bool operator() (const Chromosome& c1, const Chromosome& c2)
	{
		return c1.fitness < c2.fitness;
	}
};

