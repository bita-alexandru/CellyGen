#include "ToolUndo.h"

ToolUndo::ToolUndo(wxWindow* parent) : wxPanel(parent)
{
	BuildInterface();
}

ToolUndo::~ToolUndo()
{
}

void ToolUndo::SetGrid(Grid* grid)
{
	m_Grid = grid;
}

void ToolUndo::PushBack(
	std::unordered_map<std::pair<int, int>, std::pair<std::string, wxColour>, Hashes::PairInt>& cells,
	std::unordered_map<std::string, std::unordered_set<std::pair<int, int>, Hashes::PairInt>>& statePositions,
	std::unordered_map<std::pair<int, int>, std::pair<std::string, wxColour>, Hashes::PairInt>& prevCells,
	std::unordered_map<std::string, std::unordered_set<std::pair<int, int>, Hashes::PairInt>>& prevStatePositions
)
{
	// store the actual changes
	std::vector<std::pair<std::pair<int, int>, std::pair<std::string, wxColour>>> cellChanges;
	std::vector<std::pair<std::string, std::pair<int, int>>> statePositionsChanges;

	for (auto& i : cells)
	{
		// new cell
		if (prevCells.find(i.first) == prevCells.end())
		{
			cellChanges.push_back({ i.first,i.second });
		}
		// new state
		else if (prevCells[i.first].first != cells[i.first].first)
		{
			cellChanges.push_back({ i.first,i.second });
		}
	}
	for (auto& i : statePositions)
	{
		// new state
		if (prevStatePositions.find(i.first) == prevStatePositions.end())
			for (auto& j : statePositions[i.first]) statePositionsChanges.push_back({ i.first,j });
		// check the positions
		else
		{
			for (auto& j : statePositions[i.first])
				// new position
				if (prevStatePositions[i.first].find(j) == prevStatePositions[i.first].end())
					statePositionsChanges.push_back({ i.first,j });
		}
	}

	for (auto& i : prevCells)
	{
		// new cell
		if (cells.find(i.first) == cells.end())
		{
			cellChanges.push_back({ i.first,i.second });
		}
		// new state
		else if (cells[i.first].first != prevCells[i.first].first)
		{
			cellChanges.push_back({ i.first,i.second });
		}
	}
	for (auto& i : prevStatePositions)
	{
		// new state
		if (statePositions.find(i.first) == statePositions.end())
			for (auto& j : prevStatePositions[i.first]) statePositionsChanges.push_back({ i.first,j });
		// check the positions
		else
		{
			for (auto& j : prevStatePositions[i.first])
				// new position
				if (statePositions[i.first].find(j) == statePositions[i.first].end())
					statePositionsChanges.push_back({ i.first,j });
		}
	}

	m_UndoCells.push_back(cellChanges);
	m_UndoStatePositions.push_back(statePositionsChanges);

	if (m_UndoCells.size() > m_StackSize)
	{
		m_UndoCells.pop_front();
		m_UndoStatePositions.pop_front();
	}

	m_RedoCells = std::deque<std::vector<std::pair<std::pair<int, int>, std::pair<std::string, wxColour>>>>();
	m_RedoStatePositions = std::deque<std::vector<std::pair<std::string, std::pair<int, int>>>>();
	m_Redo->Disable();

	if (m_UndoCells.size()) m_Undo->Enable();
}

void ToolUndo::Reset()
{
	m_Undo->Disable();
	m_Redo->Disable();

	m_RedoCells = std::deque<std::vector<std::pair<std::pair<int, int>, std::pair<std::string, wxColour>>>>();
	m_RedoStatePositions = std::deque<std::vector<std::pair<std::string, std::pair<int, int>>>>();

	m_UndoCells = std::deque<std::vector<std::pair<std::pair<int, int>, std::pair<std::string, wxColour>>>>();
	m_UndoStatePositions = std::deque<std::vector<std::pair<std::string, std::pair<int, int>>>>();
}

void ToolUndo::Undo(wxCommandEvent& evt)
{
	if (m_UndoCells.empty()) return;
	if (m_Grid->GetGenerating() || !m_Grid->GetPaused()) return;

	std::unordered_map<std::pair<int, int>, std::pair<std::string, wxColour>, Hashes::PairInt> cells = m_Grid->GetCells();
	std::unordered_map<std::string, std::unordered_set<std::pair<int, int>, Hashes::PairInt>> statePositions = m_Grid->GetStatePositions();

	// iterate through the most recent changes
	for (auto& it : m_UndoCells.back())
	{
		// if i find a change in the current configuration -> delete it
		if (cells.find(it.first) != cells.end())
		{
			cells.erase(it.first);
		}
		// otherwise -> add it
		else
		{
			cells.insert(it);
		}
	}
	for (auto& it : m_UndoStatePositions.back())
	{
		// if i find a change in the current configuration -> delete/insert it
		if (statePositions.find(it.first) != statePositions.end())
		{
			// cell is unaffected -> delete
			if (statePositions[it.first].find(it.second) != statePositions[it.first].end()) statePositions[it.first].erase(it.second);
			// insert
			else statePositions[it.first].insert(it.second);
		}
		// otherwise -> add it
		else
		{
			statePositions.insert({ it.first, std::unordered_set<std::pair<int,int>,Hashes::PairInt>() });
			statePositions[it.first].insert(it.second);
		}
	}

	m_RedoCells.push_back(m_UndoCells.back());
	m_RedoStatePositions.push_back(m_UndoStatePositions.back());

	m_UndoCells.pop_back();
	m_UndoStatePositions.pop_back();

	m_Redo->Enable();

	if (m_UndoCells.empty()) m_Undo->Disable();

	m_Grid->SetCells(cells, statePositions);
	m_Grid->DecrementGenerationCount();
	m_Grid->SetFocus();
}

void ToolUndo::Redo(wxCommandEvent& evt)
{
	if (m_RedoCells.empty()) return;
	if (m_Grid->GetGenerating() || !m_Grid->GetPaused()) return;

	std::unordered_map<std::pair<int, int>, std::pair<std::string, wxColour>, Hashes::PairInt> cells = m_Grid->GetCells();
	std::unordered_map<std::string, std::unordered_set<std::pair<int, int>, Hashes::PairInt>> statePositions = m_Grid->GetStatePositions();

	// iterate through the most recent changes
	for (auto it = m_RedoCells.back().rbegin(); it != m_RedoCells.back().rend(); it++)
	{
		// if i find a change in the current configuration -> delete it
		if (cells.find(it->first) != cells.end() && cells[it->first] == it->second)
		{
			cells.erase(it->first);
		}
		// otherwise -> add it
		else
		{
			it->second.second = m_Grid->GetColors()[it->second.first];
			cells[it->first] = it->second;
		}
	}
	for (auto& it : m_RedoStatePositions.back())
	{
		// if i find a change in the current configuration -> update/insert it
		if (statePositions.find(it.first) != statePositions.end())
		{
			// cell is unaffected -> delete
			if (statePositions[it.first].find(it.second) != statePositions[it.first].end()) statePositions[it.first].erase(it.second);
			// insert
			else statePositions[it.first].insert(it.second);
		}
		// otherwise -> add it
		else
		{
			statePositions.insert({ it.first, std::unordered_set<std::pair<int,int>,Hashes::PairInt>() });
			statePositions[it.first].insert(it.second);
		}
	}


	m_UndoCells.push_back(m_RedoCells.back());
	m_UndoStatePositions.push_back(m_RedoStatePositions.back());

	m_RedoCells.pop_back();
	m_RedoStatePositions.pop_back();

	m_Undo->Enable();

	if (m_RedoCells.empty()) m_Redo->Disable();

	m_Grid->SetCells(cells, statePositions);
	m_Grid->SetFocus();
}

void ToolUndo::BuildInterface()
{
	m_Undo = new wxBitmapButton(this, Ids::ID_BUTTON_UNDO, wxBitmap("BTN_UNDO", wxBITMAP_TYPE_PNG_RESOURCE), wxDefaultPosition, wxSize(32, 32));
	m_Redo = new wxBitmapButton(this, Ids::ID_BUTTON_REDO, wxBitmap("BTN_REDO", wxBITMAP_TYPE_PNG_RESOURCE), wxDefaultPosition, wxSize(32, 32));

	m_Undo->SetToolTip("Undo\t(Ctrl+Z)");
	m_Undo->Disable();
	m_Undo->Bind(wxEVT_BUTTON, &ToolUndo::Undo, this);
	m_Redo->SetToolTip("Redo\t(Ctrl+Shift+Z)");
	m_Redo->Disable();
	m_Redo->Bind(wxEVT_BUTTON, &ToolUndo::Redo, this);

	wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);
	sizer->Add(m_Undo, 0, wxALIGN_CENTER_VERTICAL);
	sizer->Add(m_Redo, 0, wxALIGN_CENTER_VERTICAL);
	sizer->AddSpacer(16);

	SetSizer(sizer);
}