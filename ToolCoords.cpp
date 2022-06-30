#include "ToolCoords.h"

ToolCoords::ToolCoords(wxWindow* parent) : wxPanel(parent)
{
	BuildInterface();
}

ToolCoords::~ToolCoords()
{
}

void ToolCoords::SetCoords(int x, int y, std::string state)
{
	wxString coords = wxString::Format("(X,Y)=(%i,%i)", x, y);
	wxString name = wxString::Format("State=%s", state);

	m_Coords->SetLabel(coords);
	m_State->SetLabel(name);
}

wxString ToolCoords::GetState()
{
	return m_State->GetLabel();
}

void ToolCoords::Reset()
{
	m_Coords->SetLabel("(X,Y)=");
	m_State->SetLabel("State=");
}

void ToolCoords::BuildInterface()
{
	m_Coords = new wxStaticText(this, wxID_ANY, "(X,Y)=");
	m_State = new wxStaticText(this, wxID_ANY, "State=");

	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	sizer->Add(m_Coords, 0, wxRIGHT, 256);
	sizer->Add(m_State, 0, wxRIGHT, 256);

	SetSizer(sizer);
}
