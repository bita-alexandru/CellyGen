#pragma once
#include "wx/wx.h"
#include "wx/spinctrl.h"

class AlgorithmParameters : public wxPanel
{
public:
	AlgorithmParameters(wxWindow* parent);
	~AlgorithmParameters();

	int GetPopulationSize();
	double GetProbabilityMutation();
	double GetProbabilityCrossover();

	int GetGenerationTarget();
	int GetPopulationTarget();
	int GetEpochsTarget();

	double GetGenerationMultiplier();
	double GetPopulationMultiplier();
	double GetInitialSizeMultiplier();

	wxString GetSelectionMethod();
private:
	wxSpinCtrl* m_PopulationSize = nullptr;
	wxSpinCtrlDouble* m_ProbabilityMutation = nullptr;
	wxSpinCtrlDouble* m_ProbabilityCrossover = nullptr;

	wxSpinCtrlDouble* m_GenerationMultiplier = nullptr;
	wxSpinCtrlDouble* m_PopulationMultiplier = nullptr;
	wxSpinCtrlDouble* m_InitialSizeMultiplier = nullptr;

	wxSpinCtrl* m_GenerationTarget = nullptr;
	wxSpinCtrl* m_PopulationTarget = nullptr;
	wxSpinCtrl* m_EpochsTarget = nullptr;
	wxComboBox* m_SelectionMethod = nullptr;

	void BuildInterface();
};

