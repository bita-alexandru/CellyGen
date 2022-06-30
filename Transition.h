#pragma once
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <string>

#define TYPE_EQUAL 0
#define TYPE_LESS -1
#define TYPE_MORE 1

#define CONDITIONS_AND vector<pair<pair<int, int>, string>> // pair = ((<number-of-cells>, <comparison-type>), <state name>)
#define CONDITIONS_OR vector<CONDITIONS_AND>
#define NEIGHBORS vector<string>
#define DIRECTIONS unordered_set<string>
#define STATES unordered_set<string>
#define RULES_AND vector<pair<NEIGHBORS, CONDITIONS_OR>> // pair = (<neighborhood>, ...)
#define RULES_OR vector<RULES_AND>

using namespace std;

struct Transition
{
	string state;
	string condition;
	bool all = false;

	STATES states;
	DIRECTIONS directions;
	
	CONDITIONS_AND andConditions;
	CONDITIONS_OR orConditions;

	RULES_AND andRules;
	RULES_OR orRules;
};
