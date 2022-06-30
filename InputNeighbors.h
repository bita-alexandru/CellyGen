#pragma once
#include "wx/wx.h"
#include "wx/tglbtn.h"

#include "Ids.h"
#include "Grid.h"

#include <unordered_set>

class InputNeighbors : public wxPanel
{
public:
	InputNeighbors(wxWindow* parent);
	~InputNeighbors();

	void SetGrid(Grid* grid);

	std::unordered_set<std::string>& GetNeighbors();
	std::vector<std::string> GetNeighborsAsVector();
	void SetNeighbors(std::vector<std::string> neighbors);
private:
	Grid* m_Grid = nullptr;

	std::unordered_set<std::string> m_Neighbors;

	std::unordered_map<std::string, wxBitmapToggleButton*> m_Buttons;
	std::unordered_map<int, std::string> m_Ids;

	void BuildInterface();

	void OnToggleButton(wxCommandEvent& evt);
};

