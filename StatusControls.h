#pragma once
#include "wx/wx.h"
#include "wx/spinctrl.h"

#include "Ids.h"
#include "Grid.h"

class Grid;

class StatusControls : public wxPanel
{
public:
	StatusControls(wxWindow* parent);
	~StatusControls();

	void SetGrid(Grid* grid);
	void SetPlayButton(bool play);
private:
	Grid* m_Grid = nullptr;

	wxBitmapButton* m_PlayButton = nullptr;
	wxSpinCtrlDouble* m_SpinPopulate = nullptr;

	void BuildInterface();

	void GoToCenter(wxCommandEvent& evt);
	void Reset(wxCommandEvent& evt);
	void Play(wxCommandEvent& evt);
	void NextGeneration(wxCommandEvent& evt);
	void Populate(wxCommandEvent& evt);
};

