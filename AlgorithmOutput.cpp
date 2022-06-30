#include "AlgorithmOutput.h"

#include "wx/richmsgdlg.h"

#include <thread>
#include <fstream>
#include <chrono>
#include <ctime>

using Clock = chrono::high_resolution_clock;

wxBEGIN_EVENT_TABLE(AlgorithmOutput, wxPanel)
EVT_TIMER(Ids::ID_TIMER_ELAPSED, AlgorithmOutput::UpdateTimer)
wxEND_EVENT_TABLE()

AlgorithmOutput::AlgorithmOutput(wxWindow* parent) : wxPanel(parent)
{
	BuildInterface();
}

AlgorithmOutput::~AlgorithmOutput()
{
	wxDELETE(m_Timer);
}

void AlgorithmOutput::SetGrid(Grid* grid)
{
	m_Grid = grid;
}

void AlgorithmOutput::SetInputStates(InputStates* inputStates)
{
	m_InputStates = inputStates;
}

void AlgorithmOutput::SetInputRules(InputRules* inputRules)
{
	m_InputRules = inputRules;
}

void AlgorithmOutput::SetInputNeighbors(InputNeighbors* inputNeighbors)
{
	m_InputNeighbors = inputNeighbors;
}

void AlgorithmOutput::SetAlgorithmParameters(AlgorithmParameters* algorithmParameters)
{
	m_AlgorithmParameters = algorithmParameters;
}

void AlgorithmOutput::BuildInterface()
{
	m_Timer = new wxTimer(this, Ids::ID_TIMER_ELAPSED);

	m_Start = new wxButton(this, wxID_ANY, "Start");
	m_Start->Bind(wxEVT_BUTTON, &AlgorithmOutput::OnStart, this);

	m_Stop = new wxButton(this, wxID_ANY, "Stop");
	m_Stop->Disable();
	m_Stop->Bind(wxEVT_BUTTON, &AlgorithmOutput::OnStop, this);

	m_Save = new wxButton(this, wxID_ANY, "Save");
	m_Save->Disable();
	m_Save->Bind(wxEVT_BUTTON, &AlgorithmOutput::OnSave, this);

	wxBoxSizer* sizerButtons = new wxBoxSizer(wxHORIZONTAL);
	sizerButtons->Add(m_Start, 0);
	sizerButtons->Add(m_Stop, 0, wxLEFT | wxRIGHT, 0);
	sizerButtons->Add(m_Save, 0);

	m_TextEpoch = new wxStaticText(this, wxID_ANY, "Epoch: 0");

	m_TextLastNofGeneration = new wxStaticText(this, wxID_ANY, "0");
	m_TextLastAvgPopulation = new wxStaticText(this, wxID_ANY, "0");
	m_TextLastInitialSize = new wxStaticText(this, wxID_ANY, "0");
	m_TextLastFitness = new wxStaticText(this, wxID_ANY, "0");

	m_TextBestNofGeneration = new wxStaticText(this, wxID_ANY, "0");
	m_TextBestAvgPopulation = new wxStaticText(this, wxID_ANY, "0");
	m_TextBestInitialSize = new wxStaticText(this, wxID_ANY, "0");
	m_TextBestFitness = new wxStaticText(this, wxID_ANY, "0");

	wxFlexGridSizer* sizerLast = new wxFlexGridSizer(2, 4, wxSize(24, 0));
	sizerLast->Add(new wxStaticText(this, wxID_ANY, "Last No. Generations"), 0, wxALIGN_RIGHT);
	sizerLast->Add(new wxStaticText(this, wxID_ANY, "Last Avg. Population"), 0, wxALIGN_RIGHT);
	sizerLast->Add(new wxStaticText(this, wxID_ANY, "Last Initial Size"), 0, wxALIGN_RIGHT);
	sizerLast->Add(new wxStaticText(this, wxID_ANY, "Last Fitness"), 0, wxALIGN_RIGHT);
	sizerLast->Add(m_TextLastNofGeneration, 0, wxALIGN_RIGHT);
	sizerLast->Add(m_TextLastAvgPopulation, 0, wxALIGN_RIGHT);
	sizerLast->Add(m_TextLastInitialSize, 0, wxALIGN_RIGHT);
	sizerLast->Add(m_TextLastFitness, 0, wxALIGN_RIGHT);

	wxFlexGridSizer* sizerBest = new wxFlexGridSizer(2, 4, wxSize(24, 0));
	sizerBest->Add(new wxStaticText(this, wxID_ANY, "Best No. Generations"), 0, wxALIGN_RIGHT);
	sizerBest->Add(new wxStaticText(this, wxID_ANY, "Best Avg. Population"), 0, wxALIGN_RIGHT);
	sizerBest->Add(new wxStaticText(this, wxID_ANY, "Best Initial Size"), 0, wxALIGN_RIGHT);
	sizerBest->Add(new wxStaticText(this, wxID_ANY, "Best Fitness"), 0, wxALIGN_RIGHT);
	sizerBest->Add(m_TextBestNofGeneration, 0, wxALIGN_RIGHT);
	sizerBest->Add(m_TextBestAvgPopulation, 0, wxALIGN_RIGHT);
	sizerBest->Add(m_TextBestInitialSize, 0, wxALIGN_RIGHT);
	sizerBest->Add(m_TextBestFitness, 0, wxALIGN_RIGHT);

	m_TextElapsed = new wxStaticText(this, wxID_ANY, "Time Elapsed: 00:00:00");

	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	sizer->Add(sizerButtons, 0);
	sizer->AddSpacer(8);
	sizer->Add(m_TextEpoch, 0);
	sizer->AddSpacer(8);
	sizer->Add(sizerLast, 0);
	sizer->AddSpacer(8);
	sizer->Add(sizerBest, 0);
	sizer->AddSpacer(8);
	sizer->Add(m_TextElapsed, 0);

	SetSizerAndFit(sizer);
}

void AlgorithmOutput::RunAlgorithm()
{
	// get CA configuration -> as simple vectors
	m_States = m_InputStates->GetList()->GetStates();
	m_Rules = m_InputRules->GetList()->GetRules();
	m_Neighbors = m_InputNeighbors->GetNeighborsAsVector();

	// get CA configuration -> as used in playing the simulations
	unordered_map<string, string> states = m_InputStates->GetStates();
	vector<pair<string, Transition>> rules = m_InputRules->GetRules();
	unordered_set<string> neighbors = m_InputNeighbors->GetNeighbors();

	if (states.size() <= 1 || rules.size() == 0)
	{
		wxRichMessageDialog dialog(
			this, "No cellular automaton detected.", "Error",
			wxOK | wxICON_INFORMATION
		);
		dialog.ShowModal();

		EndAlgorithm(false);
		return;
	}

	string errors = CheckValidAutomaton(states, rules, neighbors);
	if (errors.size())
	{
		wxRichMessageDialog dialog(
			this, "Some of the rules appear to be invalid.", "Error",
			wxOK | wxICON_ERROR
		);
		dialog.ShowDetailedText(errors);
		dialog.ShowModal();

		EndAlgorithm(false);
		return;
	}

	GetParameters();

	generator.seed(Clock::now().time_since_epoch().count());

	UpdateTextEpoch(0);

	m_BestChromosome = Chromosome();
	UpdateTextLast(m_BestChromosome);
	UpdateTextBest(m_BestChromosome);

	// I. create an initial population of chromosomes
	vector<Chromosome> population = InitializePopulation();
	EvaluatePopulation(population, states, rules, neighbors);

	Chromosome bestChromosome = GetBestChromosome(population, 0);
	m_BestChromosome = bestChromosome;

	UpdateTextLast(bestChromosome);
	UpdateTextBest(bestChromosome);

	// run the algorithm until the desired epoch target is hit
	// or until explicitely stopped
	int epochs = 0;
	while (++epochs && m_Running)
	{
		UpdateTextEpoch(epochs);

		// II. select which chromosomes will make up the next population
		population = SelectPopulation(population);

		// III. apply genetic operators on the new population
		// don't do crossover on SteadyState - it has its own version
		if (selectionMethod != "Steady State") population = DoCrossover(population);
		DoMutatiton(population);

		UpdateChromosomesMaps(population);

		// IV. evaluate and save the best chromosome of this generation
		EvaluatePopulation(population, states, rules, neighbors);

		bestChromosome = GetBestChromosome(population, epochs);
		UpdateTextLast(bestChromosome);

		// update best chromosome of all generations
		if (bestChromosome > m_BestChromosome)
		{
			m_BestChromosome = bestChromosome;
			UpdateTextBest(bestChromosome);
		}

		if (epochs == epochsTarget) break;
	}

	EndAlgorithm();
}

void AlgorithmOutput::GetParameters()
{
	rows = Sizes::N_ROWS;
	cols = Sizes::N_COLS;

	popSize = m_AlgorithmParameters->GetPopulationSize();
	pc = m_AlgorithmParameters->GetProbabilityCrossover();
	pm = m_AlgorithmParameters->GetProbabilityMutation();
	selectionMethod = m_AlgorithmParameters->GetSelectionMethod();

	generationMultiplier = m_AlgorithmParameters->GetGenerationMultiplier();
	populationMultiplier = m_AlgorithmParameters->GetPopulationMultiplier();
	initialSizeMultiplier = m_AlgorithmParameters->GetInitialSizeMultiplier();

	generationTarget = m_AlgorithmParameters->GetGenerationTarget();
	populationTarget = m_AlgorithmParameters->GetPopulationTarget();
	epochsTarget = m_AlgorithmParameters->GetEpochsTarget();
}

vector<Chromosome> AlgorithmOutput::InitializePopulation()
{
	// a chromosome is denoted by a vector of states (expressed as numbers)
	// return a list of randomly created chromosomes of the desired population size

	vector<Chromosome> population;

	uniform_real_distribution<double> r01(0, 1);
	uniform_int_distribution<int> i1n(1, m_States.size() - 1);

	for (int i = 0; i < popSize && m_Running; i++)
	{
		double cellProbability = r01(generator);

		vector<int> pattern(rows * cols);
		unordered_map<int, string> cells;
		unordered_map<string, unordered_set<int>> statePositions;
		int initialSize = 0;

		// create random genes for the current chromosome
		for (int j = 0; j < rows * cols && m_Running; j++)
		{
			double p = r01(generator);

			if (p <= cellProbability)
			{
				// assign a random state for this gene
				int cellType = i1n(generator);

				pattern[j] = cellType;
				initialSize++;

				cells[j] = m_States[cellType];
				statePositions[m_States[cellType]].insert(j);
			}
		}

		Chromosome chromosome;
		chromosome.id = i;
		chromosome.pattern = pattern;
		chromosome.initialPattern = pattern;
		chromosome.initialSize = initialSize;
		chromosome.cells = cells;
		chromosome.statePositions = statePositions;
		chromosome.nOfGenerations = 0;
		chromosome.avgPopulation = 0;
		chromosome.fitness = 1.0;

		population.push_back(chromosome);
	}

	return population;
}

void AlgorithmOutput::EvaluatePopulation(vector<Chromosome>& population, unordered_map<string, string>& states,
	vector<pair<string, Transition>>& rules, unordered_set<string>& neighbors)
{
	// calculate and store the fitness for every chromosome
	// to get the fitness, play out the simulation for each chromosome

	// iterate through the chromosomes
	for (int i = 0; i < popSize && m_Running; i++)
	{
		population[i].pattern = population[i].initialPattern;

		// run the simulation for the current chromosome and store the results
		int nOfGenerations = 0;
		double avgPopulation = 0;
		while (++nOfGenerations && m_Running)
		{
			pair<vector<pair<string, pair<int, int>>>, string> result =
				ParseAllRules(population[i].cells, population[i].statePositions, states, rules, neighbors);

			if (result.second.size())
			{
				break;
			}

			UpdateGeneration(result.first, population[i].pattern, population[i].cells, population[i].statePositions);

			if (result.first.empty())
			{
				break;
			}

			avgPopulation += population[i].cells.size();

			// any targets reached?
			if (generationTarget && nOfGenerations - 1 >= generationTarget) break;
			if (populationTarget && population[i].cells.size() >= populationTarget) break;
		}
		nOfGenerations--;

		if (nOfGenerations) avgPopulation /= nOfGenerations;
		population[i].nOfGenerations = nOfGenerations;
		population[i].avgPopulation = ceil(avgPopulation);

		// calculate fitness
		double fitness = (generationMultiplier * nOfGenerations + populationMultiplier * avgPopulation) * (1 - initialSizeMultiplier * population[i].initialSize / (rows * cols)) + 1.0;

		population[i].fitness = fitness;
	}
}

vector<Chromosome> AlgorithmOutput::SelectPopulation(vector<Chromosome>& population)
{
	if (selectionMethod == "Roulette Wheel") return RouletteWheelSelection(population);
	if (selectionMethod == "Rank") return RankSelection(population);
	if (selectionMethod == "Steady State") return SteadyStateSelection(population);
	if (selectionMethod == "Tournament") return TournamentSelection(population);
	if (selectionMethod == "Elitism") return ElitismSelection(population);
	if (selectionMethod == "Random") return RandomSelection(population);

	return population;
}

vector<Chromosome> AlgorithmOutput::RouletteWheelSelection(vector<Chromosome>& population)
{
	double totalFitness = 0.0;
	for (int i = 0; i < popSize; i++) totalFitness += population[i].fitness;

	// calculate selection probability and cumulative selection probability
	// the probability of selection is proportionate to the fitness
	vector<double> q(popSize + 1);
	for (int i = 0; i < popSize && m_Running; i++)
	{
		double p = population[i].fitness / totalFitness;
		q[i + 1] = q[i] + p;
	}

	uniform_real_distribution<double> r01(0, 1);

	vector<Chromosome> newPopulation;
	int j = 0;

	// "spin" the wheel until we have selected enough parents
	while (j != popSize && m_Running)
	{
		// iterate through each chromosome
		for (int i = 0; i < popSize && m_Running; i++)
		{
			double p = r01(generator);

			// select for the next generation
			if (q[j] < p && p <= q[j + 1])
			{
				j++;

				Chromosome chromosome = population[i];
				chromosome.id = j;

				newPopulation.push_back(chromosome);

				if (j == popSize) break;
			}
		}
	}
	
	return newPopulation;
}

vector<Chromosome> AlgorithmOutput::RankSelection(vector<Chromosome>& population)
{
	// sort by fitness, worst to best
	// now each chromosome is ranked accordingly, from 1 (the worst) to N (the best)
	sort(population.begin(), population.end());

	double totalFitness = (popSize + 1.0) * popSize / 2; // 1 + 2 + ... + N

	// calculate selection probability and cumulative selection probability
	// the probability of selection is proportionate to the rank
	vector<double> q(popSize + 1);
	for (int i = 0; i < popSize && m_Running; i++)
	{
		double p = 1.0 * (i + 1) / totalFitness;
		q[i + 1] = q[i] + p;
	}

	uniform_real_distribution<double> r01(0, 1);

	// apply this selection method until we have selected enough parents
	vector<Chromosome> newPopulation;
	int j = 0;
	while (j != popSize && m_Running)
	{
		for (int i = 0; i < popSize && m_Running; i++)
		{
			double p = r01(generator);

			// select for next generation
			if (q[j] < p && p <= q[j + 1])
			{
				j++;

				Chromosome chromosome = population[i];
				chromosome.id = j;

				newPopulation.push_back(chromosome);

				if (j == popSize) break;
			}
		}
	}

	return newPopulation;
}

vector<Chromosome> AlgorithmOutput::SteadyStateSelection(vector<Chromosome>& population)
{
	// similar to the regular crossover but
	// instead of replacing the whole population with offsprings
	// it only replaces the worst 20% chromosomes

	SetUnfitChromosomes(population);

	const int N = rows * cols;
	uniform_int_distribution<int> i0n(0, N - 1);
	uniform_real_distribution<double> r01(0, 1);

	vector<Chromosome> newPopulation;
	int j = 0;

	// couple the resulted parents (p1, p2), (p3, p4) etc.
	for (int i = 0; i < popSize - 1 && m_Running; i += 2)
	{
		Chromosome offspring1 = population[i];
		Chromosome offspring2 = population[i + 1];

		double p = r01(generator);

		// parents are identical or the couple are not going to produce new offsprings
		if (offspring1.id == offspring2.id || p > pc)
		{
			offspring1.id = j++;
			offspring2.id = j++;

			newPopulation.push_back(offspring1);
			newPopulation.push_back(offspring2);

			continue;
		}

		// apply crossover

		// generate 2 cut-points
		int xp1 = i0n(generator);
		int xp2 = i0n(generator);

		while (xp1 == xp2 && N > 1 && m_Running)
		{
			xp2 = i0n(generator);
		}

		if (xp1 > xp2) swap(xp1, xp2);

		// iterate through the genes located between the cut-points
		for (int k = xp1; k <= xp2 && m_Running; k++)
		{
			// exchange gene information
			swap(offspring1.initialPattern[k], offspring2.initialPattern[k]);
		}

		offspring1.id = j++;
		offspring2.id = j++;

		// revert changes if these parents don't make up the bottom 20%
		if (unfitChromosomes.find(population[i].id) == unfitChromosomes.end()) offspring1.initialPattern = population[i].initialPattern;
		if (unfitChromosomes.find(population[i+1].id) == unfitChromosomes.end()) offspring2.initialPattern = population[i + 1].initialPattern;

		newPopulation.push_back(offspring1);
		newPopulation.push_back(offspring2);

		if (j >= popSize) break;
	}
	// odd number of parents -> copy the last chromosome
	if (popSize % 2 == 1)
	{
		Chromosome chromosome = population.back();
		chromosome.id = j++;

		newPopulation.push_back(chromosome);
	}

	// sort by worst to best
	sort(newPopulation.begin(), newPopulation.end());

	return newPopulation;
}

vector<Chromosome> AlgorithmOutput::TournamentSelection(vector<Chromosome>& population)
{
	// randomly create a group of 2 chromosomes and select the fittest one for next generation

	uniform_int_distribution<int> i0popSize(0, popSize - 1);
	uniform_real_distribution<double> r01(0, 1);

	vector<Chromosome> newPopulation;

	vector<int> indexes(popSize);
	for (int i = 0; i < popSize && m_Running; i++) indexes[i] = i;

	// apply this method until we have selected enough parents
	int k = TOURNAMENT_SIZE;
	int j = 0;
	while (j != popSize && m_Running)
	{
		unordered_set<int> tournamentIndexes;
		
		// create a tournament with 2 distinct randomly chosen chromosomes
		while (tournamentIndexes.size() < TOURNAMENT_SIZE && m_Running)
		{
			int k = i0popSize(generator);

			// make sure not to include a chromosome more than once
			if (tournamentIndexes.find(k) == tournamentIndexes.end())
			{
				tournamentIndexes.insert(k);
			}
		}

		int bestIndex = *tournamentIndexes.begin();
		double bestFitness = population[bestIndex].fitness;

		int worstIndex = bestIndex;
		double worstFitness = bestFitness;

		for (auto i = tournamentIndexes.begin(); i != tournamentIndexes.end(); i++)
		{
			int index = *i;

			if (population[index].fitness <= worstFitness)
			{
				worstFitness = population[index].fitness;
				worstIndex = index;
			}
		}

		const double r = 0.75;
		double p = r01(generator);

		// select the best fit
		if (p <= r)
		{
			Chromosome chromosome = population[bestIndex];
			chromosome.id = j++;

			newPopulation.push_back(chromosome);
		}
		// select the worst fit
		else
		{
			Chromosome chromosome = population[worstIndex];
			chromosome.id = j++;

			newPopulation.push_back(chromosome);
		}

		if (j == popSize) break;
	}

	return newPopulation;
}

vector<Chromosome> AlgorithmOutput::ElitismSelection(vector<Chromosome>& population)
{
	// don't filter the population

	return population;
}

vector<Chromosome> AlgorithmOutput::RandomSelection(vector<Chromosome>& population)
{
	// iterate through the chromosomes and select parents at random

	uniform_real_distribution<double> r01(0, 1);

	vector<Chromosome> newPopulation;
	int j = 0;
	while (j != popSize && m_Running)
	{
		for (int i = 0; i < popSize && m_Running; i++)
		{
			double p = r01(generator);

			if (p > 0.5)
			{
				j++;

				Chromosome chromosome = population[i];
				chromosome.id = j;

				newPopulation.push_back(chromosome);

				if (j == popSize) break;
			}
		}
	}

	return newPopulation;
}

vector<Chromosome> AlgorithmOutput::DoCrossover(vector<Chromosome>& population)
{
	// select chromosomes for crossover
	// make pairs of chromosomes (called "parents")
	// and select their offsprings to make up the new generation
	// 
	// return the resulted population

	const int N = rows * cols;
	uniform_int_distribution<int> i0n(0, N - 1);
	uniform_real_distribution<double> r01(0, 1);

	vector<Chromosome> newPopulation;
	int j = 0;

	if (selectionMethod == "Elitism")
	{
		SetEliteChromosomes(population);

		// include elites first
		for (auto i = eliteChromosomes.begin(); i != eliteChromosomes.end(); i++)
		{
			Chromosome elite = population[*i];
			elite.id = j++;

			newPopulation.push_back(elite);
		}
	}

	// couple the resulted parents (p1, p2), (p3, p4) etc.
	for (int i = 0; i < popSize - 1 && m_Running; i += 2)
	{
		Chromosome offspring1 = population[i];
		Chromosome offspring2 = population[i + 1];

		double p = r01(generator);

		// not going to produce new offsprings
		if (p > pc)
		{
			offspring1.id = j++;
			offspring2.id = j++;

			newPopulation.push_back(offspring1);
			newPopulation.push_back(offspring2);

			continue;
		}

		// apply crossover

		// generate 2 cut-points
		int xp1 = i0n(generator);
		int xp2 = i0n(generator);

		while (xp1 == xp2 && N > 1 && m_Running)
		{
			xp2 = i0n(generator);
		}

		if (xp1 > xp2) swap(xp1, xp2);

		// iterate through the genes located between the cut-points
		for (int k = xp1; k <= xp2 && m_Running; k++)
		{
			// exchange gene information
			swap(offspring1.initialPattern[k], offspring2.initialPattern[k]);
		}

		offspring1.id = j++;
		offspring2.id = j++;

		newPopulation.push_back(offspring1);
		newPopulation.push_back(offspring2);

		if (j >= popSize) break;
	}
	// odd number of parents -> copy the last chromosome
	if (popSize % 2 == 1)
	{
		Chromosome chromosome = population.back();
		chromosome.id = j++;

		newPopulation.push_back(chromosome);
	}

	// too many chromosomes (might happen when the selection method is "Elitism")
	while (j-- > popSize) newPopulation.pop_back();

	return newPopulation;
}

void AlgorithmOutput::DoMutatiton(vector<Chromosome>& population)
{
	// select and alter genes to increase variety

	if (selectionMethod == "Elitism") SetEliteChromosomes(population);

	const int N = rows * cols;

	uniform_int_distribution<int> i0n(0, m_States.size() - 1);
	uniform_real_distribution<double> r01(0, 1);

	for (int i = 0; i < popSize && m_Running; i++)
	{
		// ignore chromosome if it's one of the elites
		if (selectionMethod == "Elitism" && eliteChromosomes.find(population[i].id) != eliteChromosomes.end()) continue;

		// iterate through the chromosome's genes
		for (int j = 0; j < N && m_Running; j++)
		{
			double p = r01(generator);

			if (p <= pm)
			{
				// modify this gene

				int cellType = i0n(generator);

				population[i].initialPattern[j] = cellType;
			}
		}
	}
}

Chromosome AlgorithmOutput::GetBestChromosome(vector<Chromosome>& population, int epoch)
{
	Chromosome chromosome = population[0];

	for (int i = 1; i < popSize && m_Running; i++)
	{
		if (population[i] > chromosome)
		{
			chromosome = population[i];
		}
	}

	return chromosome;
}

void AlgorithmOutput::SetEliteChromosomes(vector<Chromosome> population)
{
	// save the IDs of the 2 elites

	int k = NUMBER_OF_ELITES;
	eliteChromosomes.clear();

	sort(population.rbegin(), population.rend());

	for (int i = 0; i < k; i++) eliteChromosomes.insert(population[i].id);
}

void AlgorithmOutput::SetUnfitChromosomes(vector<Chromosome> population)
{
	// save the IDs of the worst 10% chromosomes

	int k = max(1, (int)ceil(FITNESS_CUTOFF * popSize));

	unfitChromosomes.clear();

	sort(population.begin(), population.end());

	for (int i = 0; i < k; i++) unfitChromosomes.insert(population[i].id);
}

void AlgorithmOutput::OnStart(wxCommandEvent& evt)
{
	Start();
}

void AlgorithmOutput::OnStop(wxCommandEvent& evt)
{
	Stop();
}

void AlgorithmOutput::OnSave(wxCommandEvent& evt)
{
	Save();
}

void AlgorithmOutput::OnRender(wxCommandEvent& evt)
{
	m_RenderOnScreen = !m_RenderOnScreen;
}

void AlgorithmOutput::Start()
{
	if (m_Running) return;

	m_Running = true;

	m_Start->Disable();
	m_Stop->Enable();
	m_Save->Disable();

	m_BestChromosome.id = -1;

	m_TimeElapsed = -1;
	m_Timer->Start(1000);

	thread t(&AlgorithmOutput::RunAlgorithm, this);
	t.detach();
}

void AlgorithmOutput::Stop()
{
	if (!m_Running) return;

	m_Running = false;

	m_Start->Enable();
	m_Stop->Disable();
}

void AlgorithmOutput::Save()
{
	unsigned int now = time(0);
	wxString fileName = wxString::Format("%u", now);

	wxFileDialog dialogFile(this, "Export Pattern", "", fileName, "TXT files (*.txt)|*.txt", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

	if (dialogFile.ShowModal() == wxID_CANCEL) return;

	ofstream out(dialogFile.GetPath().ToStdString());

	out << "[ALGORITHM SETTINGS]\n";
	out << wxString::Format(
		"%s\n%s\n\n%s\n\nSelection Method: %s\nPopulation Size: %i\nProbability of Mutation: %f\nProbability of Crossover: %f\n\n%s",
		m_TextElapsed->GetLabel(), m_TextEpoch->GetLabel(),
		wxString::Format(
			"Reached generation: %i\nReached avg. population: %i\nInitial size: %i\nFitness: %f",
			m_BestChromosome.nOfGenerations, m_BestChromosome.avgPopulation, m_BestChromosome.initialSize, m_BestChromosome.fitness
		),
		selectionMethod, popSize, pm, pc,
		wxString::Format(
			"Generation Multiplier: %f\nPopullation Multiplier: %f\nInitial Size Multiplier: %f\n\n%s",
			generationMultiplier, populationMultiplier, initialSizeMultiplier,
			wxString::Format(
				"Epochs Target: %i\nGeneration Target: %i\nPopulation Target: %i\n",
				epochsTarget, generationTarget, populationTarget
			)
		)
	) << "\n";

	out << "[STATES]\n";
	for (int i = 1; i < m_States.size(); i++) out << m_States[i] << ";\n";

	out << "[RULES]\n";
	for (auto& rule : m_Rules) out << rule << "\n";

	out << "[NEIGHBORS]\n";
	for (auto& neighbor : m_Neighbors) out << neighbor << ' ';
	out << '\n';

	out << "[SIZE]\n";
	out << rows << ' ' << cols << '\n';

	out << "[CELLS]\n";
	for (int i = 0; i < rows; i++)
	{
		for (int j = 0; j < cols; j++)
		{
			int k = i * cols + j;

			if (m_BestChromosome.initialPattern[k])
			{
				int x = j - cols / 2;
				int y = i - rows / 2;
				string state = m_States[m_BestChromosome.initialPattern[k]];

				out << x << ' ' << y << ' ' << state << ';' << '\n';
			}
		}
	}
}

void AlgorithmOutput::UpdateTextEpoch(int epoch)
{
	m_TextEpoch->SetLabel(wxString::Format("Epoch: %i", epoch));
}

void AlgorithmOutput::UpdateTextElapsed(int elapsed)
{
	int seconds = elapsed % 60;
	int minutes = elapsed / 60;
	int hours = elapsed / 3600;

	wxString textSeconds = (seconds < 10) ? wxString::Format("0%i", seconds) : wxString::Format("%i", seconds);
	wxString textMinutes = (minutes < 10) ? wxString::Format("0%i", minutes) : wxString::Format("%i", minutes);
	wxString textHours = (hours < 10) ? wxString::Format("0%i", hours) : wxString::Format("%i", hours);

	m_TextElapsed->SetLabel(wxString::Format("Time Elapsed: %s:%s:%s", textHours, textMinutes, textSeconds));
}

void AlgorithmOutput::UpdateTextLast(Chromosome& chromosome)
{
	m_TextLastFitness->SetLabel(to_string(chromosome.fitness));
	m_TextLastNofGeneration->SetLabel(to_string(chromosome.nOfGenerations));
	m_TextLastAvgPopulation->SetLabel(to_string(chromosome.avgPopulation));
	m_TextLastInitialSize->SetLabel(to_string(chromosome.initialSize));
}

void AlgorithmOutput::UpdateTextBest(Chromosome& chromosome)
{
	m_TextBestFitness->SetLabel(to_string(chromosome.fitness));
	m_TextBestNofGeneration->SetLabel(to_string(chromosome.nOfGenerations));
	m_TextBestAvgPopulation->SetLabel(to_string(chromosome.avgPopulation));
	m_TextBestInitialSize->SetLabel(to_string(chromosome.initialSize));
}

pair<vector<pair<string, pair<int, int>>>, string> AlgorithmOutput::ParseAllRules(
	unordered_map<int, string>& cells, unordered_map<string, unordered_set<int>>& statePositions,
	unordered_map<string, string>& states, vector<pair<string, Transition>>& rules, unordered_set<string>& neighbors
)
{
	vector<pair<string, pair<int, int>>> changes;
	unordered_set<int> visited;

	for (int i = 0; i < rules.size(); i++)
	{
		if (!m_Running) break;

		pair<vector<pair<int, int>>, string> result = ParseRule(rules[i], cells, statePositions, states, neighbors, visited);

		// error
		if (result.second.size())
		{
			// mark the problematic rule index
			result.second += " at rule number " + to_string(i);

			return { {}, result.second };
		}
		// concatenate changes
		else
		{
			string newstate = rules[i].first + "*" + rules[i].second.state + "*";

			for (auto& change : result.first)
			{
				if (!m_Running) break;

				changes.push_back({ newstate , change });
			}
		}
	}

	return { changes,"" };
}

pair<vector<pair<int, int>>, string> AlgorithmOutput::ParseRule(pair<string, Transition>& rule,
	unordered_map<int, string>& cells, unordered_map<string, unordered_set<int>>& statePositions,
	unordered_map<string, string>& states, unordered_set<string>& neighbors,
	unordered_set<int>& visited)
{
	vector<pair<int, int>> applied;

	// if state is "FREE", apply rule to all "FREE" cells
	if (rule.first == "FREE")
	{
		// iterate through all cells
		if (rule.second.all || rule.second.condition.empty())
		{
			const int N = rows * cols;

			for (int i = 0; i < N && m_Running; i++)
			{
				int x = i % rows;
				int y = i / cols;

				if (GetState(x, y, cells) == "FREE" && visited.find(i) == visited.end() && ApplyOnCell(x, y, rule.second, cells, neighbors))
				{
					applied.push_back({ x,y });
					visited.insert(i);
				}
			}
		}
		// decide if it's faster to iterate through all cells
		// or through the condition states' neighbors
		else
		{
			int n1 = rows * cols - cells.size();
			int n2 = 0;
			for (auto& state : rule.second.states)
			{
				if (!m_Running) break;

				if (statePositions.find(state) == statePositions.end()) continue;
				n2 += statePositions[state].size();
			}

			// faster to iterate through all cells
			if (n1 <= n2 || rule.second.condition.empty())
			{
				const int N = rows * cols;

				for (int i = 0; i < N && m_Running; i++)
				{
					int x = i % rows;
					int y = i / cols;

					if (GetState(x, y, cells) == "FREE" && visited.find(i) == visited.end() && ApplyOnCell(x, y, rule.second, cells, neighbors))
					{
						applied.push_back({ x,y });
						visited.insert(i);
					}
				}
			}
			// faster to iterate through the condition states' neighbors
			else
			{
				for (auto& state : rule.second.states)
				{
					if (!m_Running) break;

					if (state == "FREE")
					{
						const int N = rows * cols;

						for (int i = 0; i < N && m_Running; i++)
						{
							int x = i % rows;
							int y = i / cols;
							int k = i;

							if (visited.find(k) == visited.end() && GetState(x, y, cells) == "FREE" && ApplyOnCell(x, y, rule.second, cells, neighbors))
							{
								applied.push_back({ x,y });
								visited.insert(k);
							}
						}
					}
					// cells of this type are placed on grid
					else if (statePositions.find(state) != statePositions.end())
					{
						int dx[8] = { 0,1,1,1,0,-1,-1,-1 };
						int dy[8] = { -1,-1,0,1,1,1,0,-1 };

						for (auto i = statePositions.at(state).begin(); i != statePositions.at(state).end(); i++)
						{
							if (!m_Running)break;

							int k = *i;
							int x = k % cols;
							int y = k / cols;

							for (int d = 0; d < 8 && m_Running; d++)
							{
								int nx = x + dx[d];
								int ny = y + dy[d];
								int k = ny * cols + nx;

								if (visited.find(k) == visited.end() && InBounds(nx, ny) && GetState(nx, ny, cells) == rule.first && ApplyOnCell(nx, ny, rule.second, cells, neighbors))
								{
									applied.push_back({ nx,ny });
									visited.insert(k);
								}
							}
						}
					}
				}
			}
		}
	}
	// else, get all cells of that type
	else
	{
		if (statePositions.find(rule.first) == statePositions.end()) return { {},"" };

		// iterate through all cells
		if (rule.second.all || rule.second.condition.empty())
		{
			string state = rule.first;
			for (auto i = statePositions.at(state).begin(); i != statePositions.at(state).end(); i++)
			{
				if (!m_Running) break;

				int k = *i;
				int x = k % cols;
				int y = k / cols;

				if (visited.find(k) == visited.end() && ApplyOnCell(x, y, rule.second, cells, neighbors))
				{
					applied.push_back({ x,y });
					visited.insert(k);
				}
			}
		}
		// decide if it's faster to iterate through all cells
		// or through the condition states' neighbors
		else
		{
			int n1 = statePositions[rule.first].size();
			int n2 = 0;
			for (auto& state : rule.second.states)
			{
				if (!m_Running) break;

				if (statePositions.find(state) == statePositions.end()) continue;
				n2 += statePositions[state].size();
			}

			// faster to iterate through all cells
			if (n1 <= n2 || rule.second.condition.empty())
			{
				string state = rule.first;
				for (auto i = statePositions.at(state).begin(); i != statePositions.at(state).end(); i++)
				{
					if (!m_Running) break;

					int k = *i;
					int x = k % cols;
					int y = k / cols;

					if (visited.find(k) == visited.end() && ApplyOnCell(x, y, rule.second, cells, neighbors))
					{
						applied.push_back({ x,y });
						visited.insert(k);
					}
				}
			}
			// faster to iterate through the condition states' neighbors
			else
			{
				for (auto& state : rule.second.states)
				{
					if (!m_Running) break;

					if (state == "FREE")
					{
						string state = rule.first;
						for (auto i = statePositions.at(state).begin(); i != statePositions.at(state).end(); i++)
						{
							int k = *i;
							int x = k % cols;
							int y = k / cols;

							if (visited.find(k) == visited.end() && ApplyOnCell(x, y, rule.second, cells, neighbors))
							{
								applied.push_back({ x,y });
								visited.insert(k);
							}
						}
					}
					// cells of this type are placed on grid
					else if (statePositions.find(state) != statePositions.end())
					{
						int dx[8] = { 0,1,1,1,0,-1,-1,-1 };
						int dy[8] = { -1,-1,0,1,1,1,0,-1 };

						for (auto i = statePositions.at(state).begin(); i != statePositions.at(state).end(); i++)
						{
							if (!m_Running) break;

							int k = *i;
							int x = k % cols;
							int y = k / cols;

							for (int d = 0; d < 8 && m_Running; d++)
							{
								int nx = x + dx[d];
								int ny = y + dy[d];
								int k = ny * cols + nx;

								if (visited.find(k) == visited.end() && InBounds(nx, ny) && GetState(nx, ny, cells) == rule.first && ApplyOnCell(nx, ny, rule.second, cells, neighbors))
								{
									applied.push_back({ nx,ny });
									visited.insert(k);
								}
							}
						}
					}
				}
			}
		}
	}

	return { applied, "" };
}

string AlgorithmOutput::CheckValidAutomaton(unordered_map<string, string>& states, vector<pair<string, Transition>>& rules, unordered_set<string>& neighbors)
{
	string errors = "";

	for (int i = 0; i < rules.size(); i++)
	{
		// check if rule might contain invalid states
		if (states.find(rules[i].first) == states.end())
			errors += (wxString::Format("<INVALID FIRST STATE> at rule number %i\n", i).ToStdString());

		if (states.find(rules[i].second.state) == states.end())
			errors += (wxString::Format("<INVALID SECOND STATE at rule number %i\n", i).ToStdString());

		for (auto& state : rules[i].second.states)
		{
			if (states.find(state) == states.end())
				errors += (wxString::Format("<INVALID CONDITION STATE at rule number %i\n", i).ToStdString());
		}
		// check for neighborhood as well
		for (auto& direction : rules[i].second.directions)
		{
			if (neighbors.find(direction) == neighbors.end())
				errors += (wxString::Format("<INVALID NEIGHBORHOOD at rule number %i\n", i).ToStdString());
		}
	}

	return errors;
}

string AlgorithmOutput::GetState(int x, int y, unordered_map<int, string>& cells)
{
	int k = y * cols + x;

	if (cells.find(k) == cells.end()) return "FREE";

	return cells[k];
}

bool AlgorithmOutput::InBounds(int x, int y)
{
	return (x >= 0 && x < cols&& y >= 0 && y < rows);
}

unordered_map<string, string> AlgorithmOutput::GetNeighborhood(int x, int y, unordered_map<int, string>& cells)
{
	unordered_map<string, string> neighborhood;

	unordered_map<string, pair<int, int>> dxy(
		{
			{ "NW",{-1,-1} }, { "N",{0,-1} }, { "NE",{1,-1} },
			{ "W",{-1,0} }, { "C",{0,0} }, { "E",{1,0} },
			{ "SW",{-1,1} }, { "S",{0,1} }, { "SE",{1,1} },
		}
	);

	// iterate through neighbors and mark the state they have
	for (auto& neighbor : m_Neighbors)
	{
		pair<int, int> d = dxy[neighbor];

		int nx = x + d.first;
		int ny = y + d.second;

		if (InBounds(nx, ny)) neighborhood.insert({ neighbor,GetState(nx,ny, cells) });
	}

	return neighborhood;
}

bool AlgorithmOutput::ApplyOnCell(int x, int y, Transition& rule, unordered_map<int, string>& cells, unordered_set<string>& neighbors)
{
	unordered_map<string, string> neighborhood = GetNeighborhood(x, y, cells);

	bool ruleValid = true;
	// iterate through the chain of "OR" rules
	for (auto& rulesOr : rule.orRules)
	{
		if (!m_Running) break;

		ruleValid = true;

		// iterate through the chain of "AND" rules
		for (auto& rulesAnd : rulesOr)
		{
			if (!m_Running) break;

			vector<string> ruleNeighborhood = rulesAnd.first;

			bool conditionValid = true;
			// iterate through the chain of "OR" conditions
			for (auto& conditionsOr : rulesAnd.second)
			{
				if (!m_Running) break;

				conditionValid = true;

				// iterate through the chain of "AND" conditions
				for (auto& conditionsAnd : conditionsOr)
				{
					if (!m_Running) break;

					string conditionState = conditionsAnd.second;

					int occurences = 0;
					if (ruleNeighborhood[0] == "ALL")
					{
						for (auto& neighbor : neighborhood)
						{
							if (!m_Running) break;

							if (neighbor.second == conditionState) occurences++;
						}
					}
					else for (auto& neighbor : ruleNeighborhood)
					{
						if (!m_Running) break;

						if (neighbors.find(neighbor) != neighbors.end())
						{
							if (neighborhood[neighbor] == conditionState) occurences++;
						}
					}

					int conditionNumber = conditionsAnd.first.first;
					int conditionType = conditionsAnd.first.second;

					switch (conditionType)
					{
					case TYPE_EQUAL:
						if (occurences != conditionNumber) conditionValid = false;
						break;
					case TYPE_LESS:
						if (occurences >= conditionNumber) conditionValid = false;
						break;
					case TYPE_MORE:
						if (occurences <= conditionNumber) conditionValid = false;
						break;
					default:
						break;
					}
				}

				if (conditionValid) break;
			}

			if (!conditionValid)
			{
				ruleValid = false;
				break;
			}
		}

		if (ruleValid) break;
	}

	return ruleValid;
}

void AlgorithmOutput::UpdateGeneration(vector<pair<string, pair<int, int>>>& changes, vector<int>& pattern,
	unordered_map<int, string>& cells, unordered_map<string, unordered_set<int>>& statePositions)
{
	for (auto& change : changes)
	{
		if (!m_Running) break;

		string state = change.first;

		state.pop_back();

		string prevState = "";
		string currState = "";
		bool separator = false;
		// regain information of previous and current state
		for (int i = 0; i < state.size() && m_Running; i++)
		{
			if (state[i] == '*')
			{
				separator = true;
				continue;
			}

			if (!separator) prevState.push_back(state[i]);
			else currState.push_back(state[i]);
		}

		// insert positions into the current state map and remove them from the previous one
		auto position = change.second;
		int x = position.first;
		int y = position.second;
		int k = y * cols + x;

		if (prevState == "FREE")
		{
			if (currState != "FREE")
			{
				cells[k] = currState;
				statePositions[currState].insert(k);
				pattern[k] = find(m_States.begin(), m_States.end(), currState) - m_States.begin();
			}
		}
		else
		{
			if (currState == "FREE")
			{
				cells.erase(k);
				statePositions[prevState].erase(k);
				pattern[k] = 0;

				// there are no more cells of this state anymore -> remove it from our map
				if (statePositions[prevState].size() == 0)
				{
					statePositions.erase(prevState);
				}
			}
			else if (cells[k] != currState)
			{
				cells.erase(k);
				statePositions[prevState].erase(k);

				// there are no more cells of this state anymore -> remove it from our map
				if (statePositions[prevState].size() == 0)
				{
					statePositions.erase(prevState);
				}

				cells[k] = currState;
				statePositions[currState].insert(k);
				pattern[k] = find(m_States.begin(), m_States.end(), currState) - m_States.begin();
			}
		}
	}
}

void AlgorithmOutput::UpdateChromosomesMaps(vector<Chromosome>& population)
{
	for (int i = 0; i < popSize && m_Running; i++)
	{
		int initialSize = 0;
		unordered_map<int, string> cells;
		unordered_map<string, unordered_set<int>> statePositions;

		for (int j = 0; j < rows * cols && m_Running; j++)
		{
			int cellType = population[i].initialPattern[j];

			if (cellType)
			{
				initialSize++;
				cells[j] = m_States[cellType];
				statePositions[m_States[cellType]].insert(j);
			}
		}

		population[i].initialSize = initialSize;
		population[i].cells = cells;
		population[i].statePositions = statePositions;
	}
}

void AlgorithmOutput::EndAlgorithm(bool save)
{
	m_Timer->Stop();

	m_Start->Enable();
	m_Stop->Disable();

	if (save && m_BestChromosome.id != -1) m_Save->Enable();

	m_Running = false;
}

void AlgorithmOutput::UpdateTimer(wxTimerEvent& evt)
{
	if (!m_Running) m_Timer->Stop();

	m_TimeElapsed++;

	UpdateTextElapsed(m_TimeElapsed);
}
