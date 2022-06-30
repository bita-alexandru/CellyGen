#include "InputNeighbors.h"

InputNeighbors::InputNeighbors(wxWindow* parent) : wxPanel(parent)
{
	BuildInterface();

	SetNeighbors({ "N","NE","E","SE","S","SW","W","NW"});
}

InputNeighbors::~InputNeighbors()
{
}

void InputNeighbors::SetGrid(Grid* grid)
{
	m_Grid = grid;
}

std::unordered_set<std::string>& InputNeighbors::GetNeighbors()
{
	return m_Neighbors;
}

std::vector<std::string> InputNeighbors::GetNeighborsAsVector()
{
	std::vector<std::string> neighbors;

	for (auto it : { "N", "NE", "E", "SE", "S", "SW", "W", "NW" })
	{
		if (m_Neighbors.find(it) != m_Neighbors.end()) neighbors.push_back(it);
	}

	return neighbors;
}

void InputNeighbors::SetNeighbors(std::vector<std::string> neighbors)
{
	for (auto& it : m_Buttons)
	{
		// neighbour found -> disable it
		if (m_Neighbors.find(it.first) != m_Neighbors.end())
		{
			it.second->SetValue(0);
			m_Neighbors.erase(it.first);
		}
	}

	for (int i = 0; i < neighbors.size(); i++)
	{
		// new neighbour -> enable it
		if (m_Neighbors.find(neighbors[i]) == m_Neighbors.end())
		{
			m_Neighbors.insert(neighbors[i]);
			m_Buttons[neighbors[i]]->SetValue(1);
		}
	}
}

void InputNeighbors::BuildInterface()
{
	wxPanel* row1 = new wxPanel(this);
	wxBoxSizer* sizer1 = new wxBoxSizer(wxHORIZONTAL);
	
	wxBitmapToggleButton* m_NW = new wxBitmapToggleButton(row1, Ids::ID_NEIGHBOR_NW, wxBitmap("BTN_NW", wxBITMAP_TYPE_PNG_RESOURCE), wxDefaultPosition, wxSize(48, 48));
	wxBitmapToggleButton* m_N = new wxBitmapToggleButton(row1, Ids::ID_NEIGHBOR_N, wxBitmap("BTN_N", wxBITMAP_TYPE_PNG_RESOURCE), wxDefaultPosition, wxSize(48, 48));
	wxBitmapToggleButton* m_NE = new wxBitmapToggleButton(row1, Ids::ID_NEIGHBOR_NE, wxBitmap("BTN_NE", wxBITMAP_TYPE_PNG_RESOURCE), wxDefaultPosition, wxSize(48, 48));
	sizer1->Add(m_NW, 1, wxEXPAND);
	sizer1->Add(m_N, 1, wxEXPAND);
	sizer1->Add(m_NE, 1, wxEXPAND);
	row1->SetSizer(sizer1);

	wxPanel* row2 = new wxPanel(this);
	wxBoxSizer* sizer2 = new wxBoxSizer(wxHORIZONTAL);
	wxBitmapToggleButton* m_W = new wxBitmapToggleButton(row2, Ids::ID_NEIGHBOR_W, wxBitmap("BTN_W", wxBITMAP_TYPE_PNG_RESOURCE), wxDefaultPosition, wxSize(48, 48));
	wxBitmapToggleButton* m_C = new wxBitmapToggleButton(row2, Ids::ID_NEIGHBOR_C, wxBitmap("BTN_C", wxBITMAP_TYPE_PNG_RESOURCE), wxDefaultPosition, wxSize(48, 48));
	wxBitmapToggleButton* m_E = new wxBitmapToggleButton(row2, Ids::ID_NEIGHBOR_E, wxBitmap("BTN_E", wxBITMAP_TYPE_PNG_RESOURCE), wxDefaultPosition, wxSize(48, 48));
	sizer2->Add(m_W, 1, wxEXPAND);
	sizer2->Add(m_C, 1, wxEXPAND);
	sizer2->Add(m_E, 1, wxEXPAND);
	row2->SetSizer(sizer2);

	wxPanel* row3 = new wxPanel(this);
	wxBoxSizer* sizer3 = new wxBoxSizer(wxHORIZONTAL);
	wxBitmapToggleButton* m_SW = new wxBitmapToggleButton(row3, Ids::ID_NEIGHBOR_SW, wxBitmap("BTN_SW", wxBITMAP_TYPE_PNG_RESOURCE), wxDefaultPosition, wxSize(48, 48));
	wxBitmapToggleButton* m_S = new wxBitmapToggleButton(row3, Ids::ID_NEIGHBOR_S, wxBitmap("BTN_S", wxBITMAP_TYPE_PNG_RESOURCE), wxDefaultPosition, wxSize(48, 48));
	wxBitmapToggleButton* m_SE = new wxBitmapToggleButton(row3, Ids::ID_NEIGHBOR_SE, wxBitmap("BTN_SE", wxBITMAP_TYPE_PNG_RESOURCE), wxDefaultPosition, wxSize(48, 48));
	sizer3->Add(m_SW, 1, wxEXPAND);
	sizer3->Add(m_S, 1, wxEXPAND);
	sizer3->Add(m_SE, 1, wxEXPAND);
	row3->SetSizer(sizer3);

	m_NW->SetToolTip("North-West"); m_N->SetToolTip("North"); m_NE->SetToolTip("North-East");
	m_W->SetToolTip("West"); m_C->SetToolTip("Center (The Cell Itself)"); m_E->SetToolTip("East");
	m_SW->SetToolTip("South-West"); m_S->SetToolTip("South"); m_SE->SetToolTip("South-East");

	wxStaticBoxSizer* sizer = new wxStaticBoxSizer(wxVERTICAL, this, "Neighbours");
	sizer->Add(row1, 1, wxEXPAND);
	sizer->Add(row2, 1, wxEXPAND);
	sizer->Add(row3, 1, wxEXPAND);

	SetSizer(sizer);

	m_Buttons = {
		{"NW", m_NW}, {"N", m_N}, {"NE", m_NE},
		{"W", m_W}, {"C", m_C}, {"E", m_E},
		{"SW", m_SW}, {"S", m_S}, {"SE", m_SE}
	};

	m_Ids = {
		{Ids::ID_NEIGHBOR_NW, "NW"}, {Ids::ID_NEIGHBOR_N, "N"}, {Ids::ID_NEIGHBOR_NE, "NE"},
		{Ids::ID_NEIGHBOR_W, "W"}, {Ids::ID_NEIGHBOR_C, "C"}, {Ids::ID_NEIGHBOR_E, "E"},
		{Ids::ID_NEIGHBOR_SW, "SW"}, {Ids::ID_NEIGHBOR_S, "S"}, {Ids::ID_NEIGHBOR_SE, "SE"}
	};

	for (auto& it : m_Ids)
	{
		m_Buttons[it.second]->Bind(wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, &InputNeighbors::OnToggleButton, this);
	}
}

void InputNeighbors::OnToggleButton(wxCommandEvent& evt)
{
	std::string neighbor = m_Ids[evt.GetId()];

	m_Grid->SetFocus();

	if (m_Grid->GetGenerating() || !m_Grid->GetPaused())
	{
		m_Buttons[neighbor]->SetValue(!m_Buttons[neighbor]->GetValue());

		wxMessageBox("Can't change neighborhood while the simulation is playing. Try pausing it first.", "Error", wxICON_WARNING);
		return;
	}

	if (m_Neighbors.find(neighbor) != m_Neighbors.end())
	{
		// neighbor already in our list -> remove it
		m_Neighbors.erase(neighbor);
	}
	else
	{
		// neighbor not in list -> add it
		m_Neighbors.insert(neighbor);
	}
}
