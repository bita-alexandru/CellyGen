#pragma once
#include "wx/wx.h"

#include "Ids.h"
#include "Grid.h"

class Grid;

class StatusDelay : public wxPanel
{
public:
	StatusDelay(wxWindow* parent);
	~StatusDelay();

	int GetDelay();
	
	void SetGrid(Grid* grid);
private:
	Grid* m_Grid = nullptr;

	int m_Delay = 0;
	int m_Delays[5] = { 100, 250, 500, 1000, 2000 };

	wxStaticText* m_TextDelay = nullptr;

	void BuildInterface();
	void UpdateTextDelay();

	void IncreaseDelay(wxCommandEvent& evt);
	void DecreaseDelay(wxCommandEvent& evt);
};

