#pragma once
#include "wx/wx.h"

#include "Ids.h"
#include "Sizes.h"
#include "Hashes.h"
#include "Grid.h"

#include <deque>
#include <unordered_set>

class Grid;

class ToolUndo : public wxPanel
{
public:
	ToolUndo(wxWindow* parent);
	~ToolUndo();

	void SetGrid(Grid* grid);

	void PushBack(
		std::unordered_map<std::pair<int, int>, std::pair<std::string, wxColour>, Hashes::PairInt>& cells,
		std::unordered_map<std::string, std::unordered_set<std::pair<int, int>, Hashes::PairInt>>& statePositions,
		std::unordered_map<std::pair<int, int>, std::pair<std::string, wxColour>, Hashes::PairInt>& prevCells,
		std::unordered_map<std::string, std::unordered_set<std::pair<int, int>, Hashes::PairInt>>& prevStatePositions
	);

	void Reset();
private:
	Grid* m_Grid = nullptr;

	std::deque<std::vector<std::pair<std::pair<int, int>, std::pair<std::string, wxColour>>>> m_UndoCells;
	std::deque<std::vector<std::pair<std::pair<int, int>, std::pair<std::string, wxColour>>>> m_RedoCells;
	std::deque<std::vector<std::pair<std::string, std::pair<int, int>>>> m_UndoStatePositions;
	std::deque<std::vector<std::pair<std::string, std::pair<int, int>>>> m_RedoStatePositions;

	wxBitmapButton* m_Undo = nullptr;
	wxBitmapButton* m_Redo = nullptr;

	int m_StackSize = 50;

	void Undo(wxCommandEvent& evt);
	void Redo(wxCommandEvent& evt);

	void BuildInterface();
};
