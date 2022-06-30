#pragma once
#include "wx/wx.h"
#include "wx/listctrl.h"

#include "ListStates.h"
#include "Grid.h"

class Grid;

class ToolStates : public wxPanel
{
public:
	ToolStates(wxWindow* parent);
	~ToolStates();

	int GetIndex();
	std::pair<std::string, wxColour> GetState();
	std::unordered_map<std::string, wxColour>& GetColors();

	void SetIndex(int index);
	void SetStates(std::vector<std::pair<std::string, wxColour>> states);
	void SetStateColor(int index, wxColour color);
	void SetListStates(ListStates* list);
	void SetState(std::string state);
	void SetGrid(Grid* grid);

	void SelectPrevState();
	void SelectNextState();
private:
	int m_Index = 0;
	int m_MaximumIndex = 0;
	ListStates* m_ListStates = nullptr;
	Grid* m_Grid = nullptr;

	std::vector<std::pair<std::string, wxColour>> m_States = std::vector<std::pair<std::string, wxColor>>({ {"FREE", wxColour("white")} });
	std::unordered_map<std::string, wxColour> m_Colors = std::unordered_map<std::string, wxColour>({ {"FREE", wxColour("white")} });

	wxStaticText* m_TextIndex = nullptr;
	wxPanel* m_State = nullptr;

	void BuildInterface();
	void UpdateTextIndex();
	void UpdateState();

	void OnPrev(wxCommandEvent& evt);
	void OnNext(wxCommandEvent& evt);

	void OnPanelClick(wxMouseEvent& evt);
};

