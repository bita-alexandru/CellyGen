#include "StatusDelay.h"

#include <string>

StatusDelay::StatusDelay(wxWindow* parent) : wxPanel(parent)
{
    BuildInterface();
}

StatusDelay::~StatusDelay()
{
}

int StatusDelay::GetDelay()
{
    return m_Delays[m_Delay];
}

void StatusDelay::SetGrid(Grid* grid)
{
    m_Grid = grid;
}

void StatusDelay::BuildInterface()
{
    wxBitmapButton* slower = new wxBitmapButton(this, Ids::ID_BUTTON_SLOWER, wxBitmap("BTN_SLOWER", wxBITMAP_TYPE_PNG_RESOURCE), wxDefaultPosition, wxSize(32, 32));
    wxBitmapButton* faster = new wxBitmapButton(this, Ids::ID_BUTTON_FASTER, wxBitmap("BTN_FASTER", wxBITMAP_TYPE_PNG_RESOURCE), wxDefaultPosition, wxSize(32, 32));
    slower->SetToolTip("Lower\t(Ctrl+,)");
    faster->SetToolTip("Higher\t(Ctrl+.)");

    slower->Bind(wxEVT_BUTTON, &StatusDelay::DecreaseDelay, this);
    faster->Bind(wxEVT_BUTTON, &StatusDelay::IncreaseDelay, this);

    std::string delay = "0.10";
    delay += "s";

    std::string label = "Delay=" + delay;
    m_TextDelay = new wxStaticText(this, wxID_ANY, label);

    wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);
    sizer->Add(slower, 0, wxALIGN_CENTER_VERTICAL);
    sizer->Add(faster, 0, wxALIGN_CENTER_VERTICAL);
    sizer->Add(m_TextDelay, 0, wxALIGN_CENTER_VERTICAL);
    sizer->AddSpacer(24);

    SetSizer(sizer);
}

void StatusDelay::UpdateTextDelay()
{
    std::string delay = std::to_string(GetDelay() * 0.001f);
    for (int i = 4; i < delay.size();) delay.pop_back();

    delay += "s";

    m_TextDelay->SetLabel("Delay=" + delay);
}

void StatusDelay::IncreaseDelay(wxCommandEvent& evt)
{
    m_Grid->SetFocus();

    if (m_Delay == 4) return;

    m_Delay++;

    UpdateTextDelay();
}

void StatusDelay::DecreaseDelay(wxCommandEvent& evt)
{
    m_Grid->SetFocus();

    if (m_Delay == 0) return;

    m_Delay--;

    UpdateTextDelay();
}