#include "DialogDimensions.h"
#include "Sizes.h"
#include "Colors.h"

#include "wx/wx.h"

DialogDimensions::DialogDimensions(wxWindow* parent) : wxDialog(parent, wxID_ANY, "Change Grid Dimensions")
{
	SetBackgroundColour(wxColor(Colors::COLOR_MAIN_R, Colors::COLOR_MAIN_G, Colors::COLOR_MAIN_B));

	Centre();

	BuildInterface();
}

DialogDimensions::~DialogDimensions()
{
}

int DialogDimensions::GetRows()
{
	return m_SpinRows->GetValue();
}

int DialogDimensions::GetCols()
{
	return m_SpinCols->GetValue();
}

void DialogDimensions::BuildInterface()
{
	wxBoxSizer* sizerRows = new wxBoxSizer(wxHORIZONTAL);

	wxStaticText* textRows = new wxStaticText(this, wxID_ANY, wxString::Format("Rows (%i-%i):", 1, Sizes::MAX_ROWS));
	m_SpinRows = new wxSpinCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS | wxSP_WRAP, 1, Sizes::MAX_ROWS, Sizes::N_ROWS);
	sizerRows->Add(textRows, 0, wxALIGN_CENTER_VERTICAL);
	sizerRows->Add(m_SpinRows, 1, wxLEFT | wxALIGN_CENTER_VERTICAL, 4);

	wxBoxSizer* sizerCols = new wxBoxSizer(wxHORIZONTAL);
	wxStaticText* textCols = new wxStaticText(this, wxID_ANY, wxString::Format("Columns (%i-%i):", 1, Sizes::MAX_ROWS));
	m_SpinCols = new wxSpinCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS | wxSP_WRAP, 1, Sizes::MAX_COLS, Sizes::N_COLS);
	sizerCols->Add(textCols, 0, wxALIGN_CENTER_VERTICAL);
	sizerCols->Add(m_SpinCols, 1, wxLEFT | wxALIGN_CENTER_VERTICAL, 4);

	wxSizer* buttons = CreateButtonSizer(wxOK | wxCANCEL);

	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	sizer->Add(sizerRows, 1, wxEXPAND | wxALL, 8);
	sizer->Add(sizerCols, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);
	sizer->Add(buttons, 0, wxLEFT | wxRIGHT | wxBOTTOM, 8);

	m_SpinRows->SetFocus();

	SetSizerAndFit(sizer);
}
