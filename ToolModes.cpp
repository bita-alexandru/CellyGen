#include "ToolModes.h"
#include "Ids.h"


ToolModes::ToolModes(wxWindow* parent) : wxPanel(parent)
{
	BuildInterface();
}

ToolModes::~ToolModes()
{
}

char ToolModes::GetMode()
{
	return m_Mode;
}

void ToolModes::SetMode(char mode)
{
	// set grid manipulation mode (Draw, Pick, Move)

	switch (mode)
	{
	case 'D':
		m_Draw->Disable();
		m_Pick->Enable();
		m_Move->Enable();
		break;
	case 'P':
		m_Draw->Enable();
		m_Pick->Disable();
		m_Move->Enable();
		break;
	case 'M':
		m_Draw->Enable();
		m_Pick->Enable();
		m_Move->Disable();
		break;
	default:
		return;
	}

	m_Mode = mode;
	UpdateTextMode();
}

void ToolModes::SetGrid(Grid* grid)
{
	m_Grid = grid;
}

void ToolModes::BuildInterface()
{
	m_Draw = new wxBitmapButton(this, Ids::ID_MODE_DRAW, wxBitmap("BTN_DRAW", wxBITMAP_TYPE_PNG_RESOURCE), wxDefaultPosition, wxSize(32, 32));
	m_Pick = new wxBitmapButton(this, Ids::ID_MODE_PICK, wxBitmap("BTN_PICK", wxBITMAP_TYPE_PNG_RESOURCE), wxDefaultPosition, wxSize(32, 32));
	m_Move = new wxBitmapButton(this, Ids::ID_MODE_MOVE, wxBitmap("BTN_MOVE", wxBITMAP_TYPE_PNG_RESOURCE), wxDefaultPosition, wxSize(32, 32));
	m_Draw->Disable();
	m_Draw->SetToolTip("Draw\t(Ctrl+Q)");
	m_Pick->SetToolTip("Pick\t(Ctrl+W)");
	m_Move->SetToolTip("Move\t(Ctrl+E)");

	m_TextMode = new wxStaticText(this, wxID_ANY, "Mode=Draw");

	wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);
	sizer->Add(m_Draw, 0, wxALIGN_CENTER_VERTICAL);
	sizer->Add(m_Pick, 0, wxALIGN_CENTER_VERTICAL);
	sizer->Add(m_Move, 0, wxALIGN_CENTER_VERTICAL);
	sizer->Add(m_TextMode, 0, wxALIGN_CENTER_VERTICAL);
	sizer->AddSpacer(8);

	SetSizer(sizer);

	for (auto it : { m_Draw, m_Pick, m_Move })
	{
		it->Bind(wxEVT_BUTTON, &ToolModes::OnMode, this);
	}
}

void ToolModes::UpdateTextMode()
{
	std::string mode;
	switch (m_Mode)
	{
		case 'D':
		{
			mode = "Draw";
			break;
		}
		case 'P':
		{
			mode = "Pick";
			break;
		}
		case 'M':
		{
			mode = "Move";
			break;
		}
		default :
		{
			mode = "";
			break;
		}
	}

	wxString label = wxString::Format("Mode=%s", mode);
	m_TextMode->SetLabel(label);
}

void ToolModes::OnMode(wxCommandEvent& evt)
{
	// get the letter corresponding to the selected mode
	char mode = (evt.GetId() - Ids::ID_MODE_DRAW)["DPM"];

	SetMode(mode);
	m_Grid->SetFocus();
}
