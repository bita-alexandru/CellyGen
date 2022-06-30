#include "AlgorithmParameters.h"

AlgorithmParameters::AlgorithmParameters(wxWindow* parent) : wxPanel(parent)
{
	BuildInterface();
}

AlgorithmParameters::~AlgorithmParameters()
{
}

int AlgorithmParameters::GetPopulationSize()
{
	return m_PopulationSize->GetValue();
}

double AlgorithmParameters::GetProbabilityMutation()
{
	return m_ProbabilityMutation->GetValue();
}

double AlgorithmParameters::GetProbabilityCrossover()
{
	return m_ProbabilityCrossover->GetValue();
}

int AlgorithmParameters::GetGenerationTarget()
{
	return m_GenerationTarget->GetValue();
}

int AlgorithmParameters::GetPopulationTarget()
{
	return m_PopulationTarget->GetValue();
}

int AlgorithmParameters::GetEpochsTarget()
{
	return m_EpochsTarget->GetValue();
}

double AlgorithmParameters::GetGenerationMultiplier()
{
	return m_GenerationMultiplier->GetValue();
}

double AlgorithmParameters::GetPopulationMultiplier()
{
	return m_PopulationMultiplier->GetValue();
}

double AlgorithmParameters::GetInitialSizeMultiplier()
{
	return m_InitialSizeMultiplier->GetValue();
}

wxString AlgorithmParameters::GetSelectionMethod()
{
	return m_SelectionMethod->GetValue();
}

void AlgorithmParameters::BuildInterface()
{
	// POP SIZE, PM, PC
	wxStaticText* textPopulationSize = new wxStaticText(this, wxID_ANY, "Population Size");
	textPopulationSize->SetToolTip("3 - 1000");
	
	m_PopulationSize = new wxSpinCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_WRAP | wxSP_ARROW_KEYS);
	m_PopulationSize->SetRange(3, 1000);
	m_PopulationSize->SetValue(30);

	wxStaticText* textProbabilityMutation = new wxStaticText(this, wxID_ANY, "Mutation Probability");
	textProbabilityMutation->SetToolTip("0.000 - 1.000");

	m_ProbabilityMutation = new wxSpinCtrlDouble(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_WRAP | wxSP_ARROW_KEYS);
	m_ProbabilityMutation->SetDigits(3);
	m_ProbabilityMutation->SetRange(0.0, 1.0);
	m_ProbabilityMutation->SetIncrement(0.005);
	m_ProbabilityMutation->SetValue(0.01);

	wxStaticText* textProbabilityCrossover = new wxStaticText(this, wxID_ANY, "Crossover Probability");
	textProbabilityCrossover->SetToolTip("0.000 - 1.000");
	
	m_ProbabilityCrossover = new wxSpinCtrlDouble(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_WRAP | wxSP_ARROW_KEYS);
	m_ProbabilityCrossover->SetDigits(3);
	m_ProbabilityCrossover->SetRange(0.0, 1.0);
	m_ProbabilityCrossover->SetIncrement(0.005);
	m_ProbabilityCrossover->SetValue(0.25);

	wxGridSizer* sizerP = new wxGridSizer(4, 3, 0, 6);
	sizerP->Add(textPopulationSize, 0);
	sizerP->Add(textProbabilityMutation, 0);
	sizerP->Add(textProbabilityCrossover, 0);
	sizerP->Add(m_PopulationSize, 0, wxEXPAND);
	sizerP->Add(m_ProbabilityMutation, 0, wxEXPAND);
	sizerP->Add(m_ProbabilityCrossover, 0, wxEXPAND);

	// FITNESS MULTIPLIERS
	wxStaticText* textGenerationMultiplier = new wxStaticText(this, wxID_ANY, "Generation Fitness Multiplier");
	textGenerationMultiplier->SetToolTip("0.000 - 1.000");

	m_GenerationMultiplier = new wxSpinCtrlDouble(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_WRAP | wxSP_ARROW_KEYS);
	m_GenerationMultiplier->SetDigits(3);
	m_GenerationMultiplier->SetRange(0.0, 1.0);
	m_GenerationMultiplier->SetIncrement(0.005);
	m_GenerationMultiplier->SetValue(1.0);

	wxStaticText* textPopulationMultiplier = new wxStaticText(this, wxID_ANY, "Population Fitness Multiplier");
	textPopulationMultiplier->SetToolTip("0.000 - 1.000");
	
	m_PopulationMultiplier = new wxSpinCtrlDouble(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_WRAP | wxSP_ARROW_KEYS);
	m_PopulationMultiplier->SetDigits(3);
	m_PopulationMultiplier->SetRange(0.0, 1.0);
	m_PopulationMultiplier->SetIncrement(0.005);
	m_PopulationMultiplier->SetValue(0.0);

	wxStaticText* textInitialSizeMultiplier = new wxStaticText(this, wxID_ANY, "Initial Size Fitness Multiplier");
	textInitialSizeMultiplier->SetToolTip("0.000 - 1.000");
	
	m_InitialSizeMultiplier = new wxSpinCtrlDouble(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_WRAP | wxSP_ARROW_KEYS);
	m_InitialSizeMultiplier->SetDigits(3);
	m_InitialSizeMultiplier->SetRange(0, 1.0);
	m_InitialSizeMultiplier->SetIncrement(0.005);
	m_InitialSizeMultiplier->SetValue(0.0);

	sizerP->Add(textGenerationMultiplier, 0);
	sizerP->Add(textPopulationMultiplier, 0);
	sizerP->Add(textInitialSizeMultiplier, 0);
	sizerP->Add(m_GenerationMultiplier, 0, wxEXPAND);
	sizerP->Add(m_PopulationMultiplier, 0, wxEXPAND);
	sizerP->Add(m_InitialSizeMultiplier, 0, wxEXPAND);

	// PATTERN TARGET
	wxStaticText* textPopulationTarget = new wxStaticText(this, wxID_ANY, "Population Target");
	textPopulationTarget->SetToolTip("0 - 10,000");
	m_PopulationTarget = new wxSpinCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_WRAP | wxSP_ARROW_KEYS);
	m_PopulationTarget->SetRange(0, 10000);
	m_PopulationTarget->SetValue(0);

	wxStaticText* textGenerationTarget = new wxStaticText(this, wxID_ANY, "Generation Target");
	textGenerationTarget->SetToolTip("0 - 10,000");
	m_GenerationTarget = new wxSpinCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_WRAP | wxSP_ARROW_KEYS);
	m_GenerationTarget->SetRange(0, 10000);
	m_GenerationTarget->SetValue(100);

	wxStaticText* textEpochsTarget = new wxStaticText(this, wxID_ANY, "Epochs Target");
	textEpochsTarget->SetToolTip("0 - 10,000");
	m_EpochsTarget = new wxSpinCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_WRAP | wxSP_ARROW_KEYS);
	m_EpochsTarget->SetRange(0, 10000);
	m_EpochsTarget->SetValue(10);

	wxGridSizer* sizerTargets = new wxGridSizer(2, 3, 0, 6);
	sizerTargets->Add(textPopulationTarget, 0);
	sizerTargets->Add(textGenerationTarget, 0);
	sizerTargets->Add(textEpochsTarget, 0);
	sizerTargets->Add(m_PopulationTarget, 0, wxEXPAND);
	sizerTargets->Add(m_GenerationTarget, 0, wxEXPAND);
	sizerTargets->Add(m_EpochsTarget, 0, wxEXPAND);

	// SELECTION
	wxStaticText* textSelection = new wxStaticText(this, wxID_ANY, "Selection Method");
	m_SelectionMethod = new wxComboBox(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, {}, wxCB_READONLY | wxCB_DROPDOWN);
	m_SelectionMethod->Set({ "Roulette Wheel", "Rank", "Steady State", "Tournament", "Elitism", "Random" });
	m_SelectionMethod->SetValue("Roulette Wheel");

	wxBoxSizer* sizerSelection = new wxBoxSizer(wxHORIZONTAL);
	sizerSelection->Add(textSelection, 0, wxALIGN_CENTER_VERTICAL);
	sizerSelection->Add(m_SelectionMethod, 0, wxEXPAND | wxLEFT, 8);

	//wxStaticBoxSizer* sizer = new wxStaticBoxSizer(wxVERTICAL, this, "Algorithm Parameters");
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	sizer->Add(sizerP, 0, wxLEFT, 8);
	sizer->AddSpacer(16);
	sizer->Add(sizerTargets, 0, wxLEFT, 8);
	sizer->AddSpacer(16);
	sizer->Add(sizerSelection, 0, wxLEFT, 8);

	SetSizer(sizer);
}
