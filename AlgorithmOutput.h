#pragma once
#include "wx/wx.h"

#include "Grid.h"
#include "InputStates.h"
#include "InputRules.h"
#include "InputNeighbors.h"
#include "AlgorithmParameters.h"
#include "Chromosome.h"

#include <random>

class AlgorithmOutput : public wxPanel
{
public:
	AlgorithmOutput(wxWindow* parent);
	~AlgorithmOutput();

	void SetGrid(Grid* grid);
	void SetInputStates(InputStates* inputStates);
	void SetInputRules(InputRules* inputRules);
	void SetInputNeighbors(InputNeighbors* inputNeighbors);
	void SetAlgorithmParameters(AlgorithmParameters* algorithmParameters);
private:
	Grid* m_Grid = nullptr;
	InputStates* m_InputStates = nullptr;
	InputRules* m_InputRules = nullptr;
	InputNeighbors* m_InputNeighbors = nullptr;
	AlgorithmParameters* m_AlgorithmParameters = nullptr;

	wxButton* m_Start = nullptr;
	wxButton* m_Stop = nullptr;
	wxButton* m_Save = nullptr;
	wxStaticText* m_TextEpoch = nullptr;
	wxStaticText* m_TextElapsed = nullptr;

	wxStaticText* m_TextLastAvgPopulation = nullptr;
	wxStaticText* m_TextBestAvgPopulation = nullptr;

	wxStaticText* m_TextLastNofGeneration = nullptr;
	wxStaticText* m_TextBestNofGeneration = nullptr;

	wxStaticText* m_TextLastInitialSize = nullptr;
	wxStaticText* m_TextBestInitialSize = nullptr;

	wxStaticText* m_TextLastFitness = nullptr;
	wxStaticText* m_TextBestFitness = nullptr;

	wxTimer* m_Timer = nullptr;

	vector<string> m_States;
	vector<string> m_Rules;
	vector<string> m_Neighbors;

	bool m_RenderOnScreen;
	bool m_Running;

	int m_Epoch;
	int m_TimeElapsed;

	double m_LastAvgPopulation;
	double m_LastNofGeneration;
	double m_LastInitialSize;
	double m_LastFitness;

	double m_BestAvgPopulation;
	double m_BestNofGeneration;
	double m_BestInitialSize;
	double m_BestFitness;

	const int NUMBER_OF_ELITES = 2;
	const int TOURNAMENT_SIZE = 2;
	const double FITNESS_CUTOFF = 0.1;

	int popSize;
	int rows;
	int cols;
	double pc;
	double pm;
	double generationMultiplier;
	double populationMultiplier;
	double initialSizeMultiplier;
	int epochsTarget;
	int generationTarget;
	int populationTarget;
	default_random_engine generator;
	wxString selectionMethod;
	Chromosome m_BestChromosome;

	unordered_set<int> eliteChromosomes;
	unordered_set<int> unfitChromosomes;

	void BuildInterface();
	void RunAlgorithm();
	void GetParameters();

	vector<Chromosome> InitializePopulation();
	void EvaluatePopulation(vector<Chromosome>& population, unordered_map<string, string>& states,
		vector<pair<string, Transition>>& rules, unordered_set<string>& neighbors);
	vector<Chromosome> SelectPopulation(vector<Chromosome>& population);
	vector<Chromosome> RouletteWheelSelection(vector<Chromosome>& population);
	vector<Chromosome> RankSelection(vector<Chromosome>& population);
	vector<Chromosome> SteadyStateSelection(vector<Chromosome>& population);
	vector<Chromosome> TournamentSelection(vector<Chromosome>& population);
	vector<Chromosome> ElitismSelection(vector<Chromosome>& population);
	vector<Chromosome> RandomSelection(vector<Chromosome>& population);
	vector<Chromosome> DoCrossover(vector<Chromosome>& population);
	void DoMutatiton(vector<Chromosome>& population);
	Chromosome GetBestChromosome(vector<Chromosome>& population, int epoch);
	void SetEliteChromosomes(vector<Chromosome> population);
	void SetUnfitChromosomes(vector<Chromosome> population);

	void OnStart(wxCommandEvent& evt);
	void OnStop(wxCommandEvent& evt);
	void OnSave(wxCommandEvent& evt);
	void OnRender(wxCommandEvent& evt);

	void Start();
	void Stop();
	void Save();

	void UpdateTextEpoch(int epoch);
	void UpdateTextElapsed(int elapsed);
	void UpdateTextLast(Chromosome& chromosome);
	void UpdateTextBest(Chromosome& chromosome);

	pair<vector<pair<string, pair<int, int>>>, string> ParseAllRules(
		unordered_map<int, string>& cells, unordered_map<string, unordered_set<int>>& statePositions,
		unordered_map<string, string>& states, vector<pair<string, Transition>>& rules, unordered_set<string>& neighbors);
	pair<vector<pair<int, int>>, string> ParseRule(pair<string, Transition>& rule,
		unordered_map<int, string>& cells, unordered_map<string, unordered_set<int>>& statePositions,
		unordered_map<string, string>& states, unordered_set<string>& neighbors,
		unordered_set<int>& visited);
	string CheckValidAutomaton(unordered_map<string,string>& states, vector<pair<string, Transition>>& rules, unordered_set<string>& neighbors);

	string GetState(int x, int y, unordered_map<int, string>& cells);
	bool InBounds(int x, int y);
	unordered_map<string, string> GetNeighborhood(int x, int y, unordered_map<int, string>& cells);
	bool ApplyOnCell(int x, int y, Transition& rule, unordered_map<int, string>& cells, unordered_set<string>& neighbors);
	void UpdateGeneration(vector<pair<string, pair<int, int>>>& changes, vector<int>& pattern,
		unordered_map<int, string>& cells, unordered_map<string, unordered_set<int>>& statePositions);

	void UpdateChromosomesMaps(vector<Chromosome>& population);
	void EndAlgorithm(bool save = true);

	wxDECLARE_EVENT_TABLE();
	void UpdateTimer(wxTimerEvent& evt);
};
