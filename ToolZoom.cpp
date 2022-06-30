#include "ToolZoom.h"

ToolZoom::ToolZoom(wxWindow* parent) : wxPanel(parent)
{
    //SetBackgroundColour(wxColour(242, 204, 143));

	BuildInterface();
}

ToolZoom::~ToolZoom()
{
}

int ToolZoom::GetSize()
{
	return m_Size;
}

void ToolZoom::ZoomIn(bool center)
{
	if (m_Size == m_MaximumSize) return;

	m_Size *= 2;
	m_Grid->SetSize(m_Size, center);

	UpdateTextScale();
}

void ToolZoom::ZoomOut(bool center)
{
	if (m_Size == m_MinimumSize) return;

	m_Size /= 2;
	m_Grid->SetSize(m_Size, center);

	UpdateTextScale();
}

void ToolZoom::SetGrid(Grid* grid)
{
	m_Grid = grid;
}

void ToolZoom::BuildInterface()
{
	wxBitmapButton* zoomIn = new wxBitmapButton(this, Ids::ID_ZOOM_IN, wxBitmap("BTN_ZOOM-IN", wxBITMAP_TYPE_PNG_RESOURCE), wxDefaultPosition, wxSize(32, 32));
	wxBitmapButton* zoomOut = new wxBitmapButton(this, Ids::ID_ZOOM_OUT, wxBitmap("BTN_ZOOM-OUT", wxBITMAP_TYPE_PNG_RESOURCE), wxDefaultPosition, wxSize(32, 32));

	zoomIn->SetToolTip("Zoom In\t(Ctrl-+)");
	zoomOut->SetToolTip("Zoom out\t(Ctrl+-)");
	
	zoomIn->Bind(wxEVT_BUTTON, &ToolZoom::OnZoomIn, this);
	zoomOut->Bind(wxEVT_BUTTON, &ToolZoom::OnZoomOut, this);

	std::string label = "Scale=1:" + std::to_string(m_Size);
	m_TextScale = new wxStaticText(this, wxID_ANY, label);

	wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);
	sizer->Add(zoomOut, 0, wxALIGN_CENTER_VERTICAL);
	sizer->Add(zoomIn, 0, wxALIGN_CENTER_VERTICAL);
	sizer->Add(m_TextScale, 0, wxALIGN_CENTER_VERTICAL);
	sizer->AddSpacer(16);

	SetSizer(sizer);
}

void ToolZoom::UpdateTextScale()
{
	wxString label = wxString::Format("Scale=1:%i", m_Size);
	m_TextScale->SetLabel(label);
}

void ToolZoom::OnZoomIn(wxCommandEvent& evt)
{
	ZoomIn(true);
	m_Grid->SetFocus();
}

void ToolZoom::OnZoomOut(wxCommandEvent& evt)
{
	ZoomOut(true);
	m_Grid->SetFocus();
}
