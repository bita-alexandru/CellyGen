#pragma once
#include "Sizes.h"

#include <utility>

using namespace std;

class Hashes
{
private:
	Hashes() {};
	~Hashes() {};
public:
	struct PairInt {
		inline size_t operator() (const pair<int, int>& p) const {
			// return the 1d index of a 2d point
			return p.second * Sizes::N_COLS + p.first;
		}
	};
};

