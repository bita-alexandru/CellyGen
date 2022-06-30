#pragma once
#include "wx/wx.h"

#include "Grid.h"

class Grid;

class ToolModes : public wxPanel
{
public:
	ToolModes(wxWindow* parent);
	~ToolModes();

	char GetMode();

	void SetMode(char mode);
	void SetGrid(Grid* grid);
private:
	char m_Mode = 'D';
	Grid* m_Grid = nullptr;
	
	wxBitmapButton* m_Draw = nullptr;
	wxBitmapButton* m_Pick = nullptr;
	wxBitmapButton* m_Move = nullptr;
	
	wxStaticText* m_TextMode = nullptr;

	void BuildInterface();
	void UpdateTextMode();

	void OnMode(wxCommandEvent& evt);
	void OnNext(wxCommandEvent& evt);
};

