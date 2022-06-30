#include "Interpreter.h"
#include "Sizes.h"

#include <sstream>
#include "wx/log.h"

Interpreter::Interpreter()
{
}

Interpreter::~Interpreter()
{
}

vector<pair<int, string>> Interpreter::Process(string& rules)
{
	m_Transitions.clear();
	unordered_set<string> duplicates;

	// keep count of duplicates/invalid rules
	vector<pair<int, string>> invalid;

	// allow illegal characters into comments (when detecting '!'), disallow after new lines (when detecting '\n')
	bool comment = false;
	int comments = 0;

	// add spaces around specific symbols and convert characters to uppercase
	for (int i = 0; i < rules.size(); i++)
	{
		char c = rules[i];

		if (!comment && BOTH_SPACED.find(c) != BOTH_SPACED.npos)
		{
			// add space at the both sides of the symbol
			rules.insert(i + 1, " ");
			rules.insert(i, " ");

			if (c == '!')
			{
				if (!comment) comments++;
				comment = true;
			}

			i += 2;
			continue;
		}
		if (!comment && LEFT_SPACED.find(c) != LEFT_SPACED.npos)
		{
			// add space before the symbol
			rules.insert(i, " ");

			i++;
			continue;
		}
		if (!comment && RIGHT_SPACED.find(c) != RIGHT_SPACED.npos)
		{
			// add space after the symbol
			rules.insert(i + 1, " ");

			i += 2;
			continue;
		}

		if (!comment && NON_SPACED.find(c) != NON_SPACED.npos) continue;

		if (!comment && isalnum(c))
		{
			// convert to uppercase
			rules[i] = toupper(rules[i]);

			continue;
		}

		if (iswspace(c))
		{
			if (c == '\n') comment = false;

			continue;
		}

		// character is illegal
		if (!comment) invalid.push_back({ i + 1 - comments*2,"<ILLEGAL CHARACTER>" });
	}

	stringstream ss(rules);
	int cursor = 0;
	int size = 0;

	spaces = 0;
	cursorBeforeComment = 0;
	isComment = false;

	while (true)
	{
		int chars = 0;
		Transition transition = {};

		string state1;
		string symbol;
		string state2;
		bool valid = true;

		// read transitory state
		ss >> state1;
		
		// check if end of file
		if (!FindWord(cursor, rules, state1) || !SkipIfComment(cursor, rules, ss, state1)) break;

		if (!UpdateChars(chars, state1)) MarkInvalid(valid, invalid, state1, "<SIZE OF RULE SURPASSES MAXIMUM LIMIT>", cursor);

		// check if state is invalid
		if (valid && !CheckState(state1)) MarkInvalid(valid, invalid, state1,  "<INVALID FIRST STATE>", cursor);
		// check if rule is within the size limits

		// read transition symbol "/"
		if (valid)
		{
			ss >> symbol; FindWord(cursor, rules, symbol); SkipIfComment(cursor, rules, ss, symbol);

			// check if rule is within the size limits
			if (!UpdateChars(chars, symbol)) MarkInvalid(valid, invalid, symbol,  "<SIZE OF RULE SURPASSES MAXIMUM LIMIT>", cursor);
			
			// check if it's the right symbol
			if (valid && symbol != "/") MarkInvalid(valid, invalid, symbol,  "<INVALID TRANSITION SYMBOL, EXPECTED '/'>", cursor);
		}

		// read transition state
		if (valid)
		{
			ss >> state2; FindWord(cursor, rules, state2); SkipIfComment(cursor, rules, ss, state2);

			// check if rule is within the size limits
			if (!UpdateChars(chars, state2)) MarkInvalid(valid, invalid, state2,  "<SIZE OF RULE SURPASSES MAXIMUM LIMIT>", cursor);
			
			// check if state is invalid
			if (valid && !CheckState(state2)) MarkInvalid(valid, invalid, state2,  "<INVALID SECOND STATE>", cursor);

			if (valid)
			{
				// check if transition is a duplicate
				if (duplicates.find(state1 + "-" + state2) != duplicates.end()) MarkInvalid(valid, invalid, state2,  "<DUPLICATE RULE>", cursor);
				else if (state1 == state2) MarkInvalid(valid, invalid, state2,  "<ILLEGAL RULE>", cursor);

				if (valid) duplicates.insert({ state1 + "-" + state2 });

				// assign to transition
				if (valid) transition.state = state2;
			}
		}

		// read symbol ":" or ";"
		if (valid)
		{
			symbol.clear();
			ss >> symbol; FindWord(cursor, rules, symbol); SkipIfComment(cursor, rules, ss, symbol);

			// check if rule is within the size limits
			if (!UpdateChars(chars, symbol)) MarkInvalid(valid, invalid, symbol,  "<SIZE OF RULE SURPASSES MAXIMUM LIMIT>", cursor);

			if (valid)
			{
				// end of transition rules
				if (symbol == ";")
				{
					// add to transition table
					m_Transitions.push_back({ state1,transition });
				}
				// indicates that a list of rules will follow
				else if (symbol == ":")
				{
					transition.orRules.clear();
					transition.andRules.clear();
					transition.andRules.push_back({});
					
					// read every rule until ";" is detected
					while (valid)
					{
						symbol.clear();
						ss >> symbol; FindWord(cursor, rules, symbol); SkipIfComment(cursor, rules, ss, symbol);
						transition.condition += symbol;

						// check if rule is marked accordingly with a "("
						if (symbol != "(") MarkInvalid(valid, invalid, symbol,  "<INVALID RULE SYMBOL, EXPECTED '('>", cursor);

						// check if rule is within the size limits
						if (valid && !UpdateChars(chars, symbol)) MarkInvalid(valid, invalid, symbol,  "<SIZE OF RULE SURPASSES MAXIMUM LIMIT>", cursor);

						// read neighborhood and conditions
						if (valid)
						{
							symbol.clear();
							ss >> symbol; FindWord(cursor, rules, symbol); SkipIfComment(cursor, rules, ss, symbol);
							transition.condition += symbol;

							// check if symbol is "@"
							if (symbol != "@") MarkInvalid(valid, invalid, symbol,  "<INVALID NEIGHBORHOOD SYMBOL, EXPECTED '@'", cursor);

							// check if rule is within the size limits
							if (valid && !UpdateChars(chars, symbol)) MarkInvalid(valid, invalid, symbol,  "<SIZE OF RULE SURPASSES MAXIMUM LIMIT>", cursor);

							if (valid)
							{
								// could indicate either a group of directions or a specific one
								string neighborhood;
								ss >> neighborhood; FindWord(cursor, rules, neighborhood); SkipIfComment(cursor, rules, ss, neighborhood);
								transition.condition += neighborhood;

								if (!UpdateChars(chars, neighborhood)) MarkInvalid(valid, invalid, neighborhood,  "<SIZE OF RULE SURPASSES MAXIMUM LIMIT>", cursor);

								// a group of directions, eg. "[n,s,w,e]"
								if (valid && neighborhood == "[")
								{
									NEIGHBORS neighbors;

									// read every direction until "]" is detected
									while (valid)
									{
										string direction;
										ss >> direction; FindWord(cursor, rules, direction); SkipIfComment(cursor, rules, ss, direction);
										transition.condition += direction;

										// check if rule is within the size limits
										if (valid && !UpdateChars(chars, direction)) MarkInvalid(valid, invalid, direction,  "<SIZE OF RULE SURPASSES MAXIMUM LIMIT>", cursor);

										// check if direction is valid
										if (valid && !CheckDirection(direction)) MarkInvalid(valid, invalid, direction,  "<INVALID DIRECTION>", cursor);

										if (valid)
										{
											if (find(neighbors.begin(), neighbors.end(), direction) != neighbors.end()) 
												MarkInvalid(valid, invalid, direction, "<DUPLICATE NEIGHBOR>", cursor);

											if (valid)
											{
												neighbors.push_back(direction);

												if (transition.directions.find(direction) == transition.directions.end())
												{
													transition.directions.insert(direction);
												}
											}
										}

										if (!valid) break;

										// expect to read either "," or "]"
										symbol.clear();
										ss >> symbol; FindWord(cursor, rules, symbol); SkipIfComment(cursor, rules, ss, symbol);
										transition.condition += symbol;

										// more directions to follow
										if (symbol == ",")
										{
											if (!UpdateChars(chars, symbol)) MarkInvalid(valid, invalid, symbol,  "<SIZE OF RULE SURPASSES MAXIMUM LIMIT>", cursor);
											continue;
										}
										// neighborhood completed
										else if (symbol == "]")
										{
											if (!UpdateChars(chars, symbol)) MarkInvalid(valid, invalid, symbol,  "<SIZE OF RULE SURPASSES MAXIMUM LIMIT>", cursor);

											// assign to transition
											if (valid) transition.andRules.back().first = neighbors;

											break;
										}
										// invalid symbol
										else
										{
											MarkInvalid(valid, invalid, symbol,  "<INVALID SYMBOL, EXPECTED ',' OR ']'>", cursor);
											break;
										}
									}
								}
								// a specific direction or "ALL" meaning every possible valid direction
								else if (valid && (neighborhood == "ALL" || CheckDirection(neighborhood)))
								{
									// assign to transition
									transition.andRules.back().first = { neighborhood };

									if (neighborhood != "ALL" && transition.directions.find(neighborhood) == transition.directions.end())
									{
										transition.directions.insert(neighborhood);
									}
								}
								// invalid token
								else MarkInvalid(valid, invalid, neighborhood,  "<INVALID NEIGHBORHOOD>", cursor);

								if (!valid) break;

								// read assignment symbol "="
								symbol.clear();
								ss >> symbol; FindWord(cursor, rules, symbol); SkipIfComment(cursor, rules, ss, symbol);
								transition.condition += symbol;

								if (symbol != "=") MarkInvalid(valid, invalid, symbol,  "<INVALID ASSIGNMENT SYMBOL>", cursor);
								
								if (valid && !UpdateChars(chars, symbol)) MarkInvalid(valid, invalid, symbol,  "<SIZE OF RULE SURPASSES MAXIMUM LIMIT>", cursor);
								
								if (!valid) break;

								transition.orConditions.clear();
								transition.andConditions.clear();
								transition.andConditions.push_back({});

								int count = 0;

								// read conditions until ")" is detected
								while (valid)
								{
									// condition could be of different forms
									// eg. "<number>#<state>" / "<sign><number>#<state>"
									// or  "#<state>" / "<sign>#<state>"
									string condition;
									ss >> condition; FindWord(cursor, rules, condition); SkipIfComment(cursor, rules, ss, condition);
									transition.condition += condition;

									if (!UpdateChars(chars, condition)) MarkInvalid(valid, invalid, condition,  "<SIZE OF RULE SURPASSES MAXIMUM LIMIT>", cursor);

									if (!valid) break;

									// split by "#"
									stringstream tokenizer(condition);
									vector<string> tokens;
									string token;

									while (getline(tokenizer, token, '#')) tokens.push_back(token);

									if (tokens.size() == 2)
									{
										string sign = "";
										string number = tokens.front();
										string state = tokens.back();

										// "#<state>"
										if (number.empty()) number = "1";
										// <sign> is present
										if (number.size() && (number[0] == '+' || number[0] == '-'))
										{
											sign.push_back(number[0]);
											number.erase(number.begin());

											// no number specified
											if (number.empty()) number = "1";
										}

										// check if number is valid
										int n = CheckNumber(number, transition, count);
										if (n == -1) MarkInvalid(valid, invalid, number,  "<SIZE OF RULE SURPASSES MAXIMUM LIMIT>", cursor);

										// check if state is valid
										if (valid && !CheckState(state)) MarkInvalid(valid, invalid, state,  "<INVALID CONDITIONAL STATE>", cursor);

										// assign to transition
										if (valid)
										{
											count += n;

											int comparisonType = TYPE_EQUAL;
											if (sign == "+") comparisonType = TYPE_MORE;
											else if (sign == "-")
											{
												comparisonType = TYPE_LESS;
												count -= n;
											}

											// optimization for whether rules need to include all cells of this type or not
											if(comparisonType == TYPE_LESS || (comparisonType == TYPE_EQUAL && n == 0)) transition.all = true;

											// number of cells
											transition.andConditions.back().first.first = n;
											// comparison type
											transition.andConditions.back().first.second = comparisonType;
											// state
											transition.andConditions.back().second = state;

											if (transition.states.find(state) == transition.states.end())
												transition.states.insert(state);
										}
									}
									// invalid tokens
									else MarkInvalid(valid, invalid, condition,  "<INVALID CONDITION>", cursor);

									if (!valid) break;

									// expect to read either an AND/OR symbol or ")"
									symbol.clear();
									ss >> symbol; FindWord(cursor, rules, symbol); SkipIfComment(cursor, rules, ss, symbol);
									if (symbol == "AND") transition.condition += " AND ";
									else if (symbol == "OR") transition.condition += " OR ";
									else transition.condition += symbol;
									
									if (!UpdateChars(chars, symbol)) MarkInvalid(valid, invalid, symbol,  "<SIZE OF RULE SURPASSES MAXIMUM LIMIT>", cursor);

									if (find(AND.begin(), AND.end(), symbol) != AND.end())
									{
										transition.andConditions.push_back({});
									}
									else if (find(OR.begin(), OR.end(), symbol) != OR.end())
									{
										transition.orConditions.push_back(transition.andConditions);
										transition.andConditions.clear();
										transition.andConditions.push_back({});
										count = 0;
									}
									else if (symbol == ")")
									{
										transition.orConditions.push_back(transition.andConditions);
										transition.andRules.back().second = transition.orConditions;
										break;
									}
									else valid = false;
								}
							}
						}
					
						if (!valid) break;

						// expect to read either an AND/OR symbol or ";"
						symbol.clear();
						ss >> symbol; FindWord(cursor, rules, symbol); SkipIfComment(cursor, rules, ss, symbol);
						if (symbol != ";") transition.condition += symbol;

						if (!UpdateChars(chars, symbol)) MarkInvalid(valid, invalid, symbol,  "<SIZE OF RULE SURPASSES MAXIMUM LIMIT>", cursor);

						if (!valid) break;

						if (find(AND.begin(), AND.end(), symbol) != AND.end())
						{
							transition.andRules.push_back({});
						}
						else if (find(OR.begin(), OR.end(), symbol) != OR.end())
						{
							transition.orRules.push_back(transition.andRules);
							transition.andRules.clear();
							transition.andRules.push_back({});
						}
						else if (symbol == ";")
						{
							transition.orRules.push_back(transition.andRules);
							break;
						}
						else MarkInvalid(valid, invalid, symbol, "<INVALID CHAINING SYMBOL, EXPECTED EITHER '&', '|' OR ';'>", cursor);
					}
					
					// add to transition table
					if (valid) m_Transitions.push_back({ state1,transition });
				}
				else MarkInvalid(valid, invalid, symbol, "<INVALID RULE MARKING SYMBOL, EXPECTED EITHER ':' OR ';'>", cursor);
			}
		}
		
		// go to the next transition if there's any left
		if (!valid)
		{
			if (!NextTransition(cursor, rules, ss)) break;
		}
	}

	if (m_Transitions.size() > Sizes::RULES_MAX) invalid = { {-1,"<THE NUMBER OF RULES SURPASSES THE MAXIMUM LIMIT>"} };

	return invalid;
}

vector<pair<string, Transition>>& Interpreter::GetTransitions()
{
	return m_Transitions;
}

bool Interpreter::FindWord(int& cursor, string &rules, string& s, bool comment)
{
	int pos = 0;
	bool ret = true;

	// reached end of file	
	if (s.empty())
	{
		pos = rules.size();
		ret = false;
	}
	else
	{
		pos = rules.find(s, cursor);

		// reached end of file
		if (pos == rules.npos)
		{
			pos = rules.size();
			ret = false;
		}
		// not a whole word
		else if (pos > 0 && !iswspace(rules[pos - 1]))
		{
			pos = pos + 1;
			ret = false;
		}
		// not a whole word
		else if (pos + s.size() < rules.size() && !iswspace(rules[pos + s.size()]))
		{
			pos = pos + 1;
			ret = false;
		}
		// whole word -> place cursor at the start of word
		else
		{
			pos = pos + 1;
		}
	}

	// count extra white spaces along the way; ignore if it's a comment
	if (!comment)
	{
		for (int i = cursor; i < pos; i++)
		{
			char c = rules[i];

			if (BOTH_SPACED.find(c) != BOTH_SPACED.npos) spaces += 2;
		}
	}

	cursor = pos;

	return ret;
}

bool Interpreter::NextTransition(int& cursor, string& rules, stringstream& ss)
{
	string semicolon = ";";

	// end of file
	if (!FindWord(cursor, rules, semicolon)) return false;

	// otherwise, update stream position
	ss.seekg(cursor);

	return true;
}

bool Interpreter::CheckState(string& state)
{
	return state.size() >= Sizes::CHARS_STATE_MIN && state.size() <= Sizes::CHARS_STATE_MAX;
}

bool Interpreter::UpdateChars(int& chars, string& s)
{
	chars += s.size();

	return chars <= Sizes::CHARS_RULE_MAX;
}

bool Interpreter::UpdateSize(int& size)
{
	size++;

	return size <= Sizes::RULES_MAX;
}

void Interpreter::MarkInvalid(bool& valid, vector<pair<int, string>>& invalid, string& s, string reason, int& cursor)
{
	int spaced = 0;
	
	if (s.size() && BOTH_SPACED.find(s) != BOTH_SPACED.npos) spaced = 1;

	int pos = (!isComment) ? cursor : cursorBeforeComment;

	invalid.push_back({ pos - spaces + spaced, reason });

	valid = false;
}

bool Interpreter::CheckDirection(string& direction)
{
	unordered_set<string> directions(
		{"NW", "N", "NE",
		 "W", "C", "E",
		"SW", "S", "SE"
		}
	);

	return directions.find(direction) != directions.end();
}

int Interpreter::CheckNumber(string& number, Transition& transition, int& count)
{
	int n = -1;

	// convert to number if valid
	if (number.size() && number.find_first_not_of("0123456789") == string::npos) n = stoi(number);

	return n;
}

bool Interpreter::SkipIfComment(int& cursor, string& rules, stringstream& ss, string& s)
{
	bool ret = true;

	isComment = false;

	// save last position before the beggining of a comment
	if (s == "!")
	{
		isComment = true;
		cursorBeforeComment = cursor + 1;
	}

	// if this string represents a comment, skip to non-comment next line
	while (s == "!")
	{
		string endline = "\n";
		FindWord(cursor, rules, endline, true);

		ss.seekg(cursor);

		s = "";
		ss >> s; ret = FindWord(cursor, rules, s);
	}

	return ret;
}
