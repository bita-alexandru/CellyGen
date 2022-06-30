#include "HelpWindow.h"

#include "Ids.h"

wxBEGIN_EVENT_TABLE(HelpWindow, wxFrame)
	EVT_CLOSE(HelpWindow::OnCloseEvent)
wxEND_EVENT_TABLE()

HelpWindow::HelpWindow(wxWindow* parent) : wxFrame(parent, wxID_ANY, "CellyGen::Help", wxDefaultPosition, wxSize(1280, 720))
{
	SetIcon(wxICON(aaaIcon));

	Center();

	BuildInterface();

	SetShortcuts();
}

HelpWindow::~HelpWindow()
{
	wxDELETE(m_Html);
}

void HelpWindow::SetPage(wxString page)
{
	m_Html->LoadPage(wxString::Format("help/%s", page));

	m_Undo = std::stack<wxString>();
	m_Redo = std::stack<wxString>();

	m_Prev->Disable();
	m_Next->Disable();
}

void HelpWindow::BuildInterface()
{
	m_Html = new wxHtmlWindow(this);
	m_Html->LoadPage("help/index.html");
	m_Html->Bind(wxEVT_HTML_LINK_CLICKED, &HelpWindow::OnLinkClick, this);

	m_Prev = new wxButton(this, Ids::ID_PREV_HELP, "<");
	m_Next = new wxButton(this, Ids::ID_NEXT_HELP, ">");
	m_Prev->SetToolTip("Previous Page\tCtrl+Left");
	m_Next->SetToolTip("Next Page\tCtrl+Right");

	m_Prev->Bind(wxEVT_BUTTON, &HelpWindow::OnPrev, this);
	m_Next->Bind(wxEVT_BUTTON, &HelpWindow::OnNext, this);

	m_Prev->Disable();
	m_Next->Disable();

	wxButton* contents = new wxButton(this, Ids::ID_CONTENTS_HELP, "Main Contents");
	contents->Bind(wxEVT_BUTTON, &HelpWindow::OnMainMenu, this);

	wxBoxSizer* buttons = new wxBoxSizer(wxHORIZONTAL);
	buttons->Add(m_Prev, 1, wxEXPAND);
	buttons->Add(m_Next, 1, wxEXPAND);

	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	sizer->Add(m_Html, 1, wxEXPAND);
	sizer->Add(buttons, 0, wxEXPAND);
	sizer->Add(contents, 0, wxEXPAND);

	SetSizer(sizer);
}

void HelpWindow::SetShortcuts()
{
	wxAcceleratorEntry entries[3];
	entries[0].Set(wxACCEL_ALT, WXK_LEFT, Ids::ID_PREV_HELP);
	entries[1].Set(wxACCEL_ALT, WXK_RIGHT, Ids::ID_NEXT_HELP);
	entries[2].Set(wxACCEL_ALT, WXK_RETURN, Ids::ID_CONTENTS_HELP);

	wxAcceleratorTable accel(3, entries);
	SetAcceleratorTable(accel);
}

void HelpWindow::OnPrev(wxCommandEvent& evt)
{
	wxString prevLink = m_Undo.top();
	wxString currLink = m_Html->GetOpenedPage();

	m_Redo.push(currLink);
	m_Undo.pop();

	if (m_Undo.empty()) m_Prev->Disable();
	m_Next->Enable();

	m_Html->LoadPage(prevLink);
}

void HelpWindow::OnNext(wxCommandEvent& evt)
{
	wxString prevLink = m_Redo.top();
	wxString currLink = m_Html->GetOpenedPage();

	m_Undo.push(currLink);
	m_Redo.pop();

	if (m_Redo.empty()) m_Next->Disable();
	m_Prev->Enable();

	m_Html->LoadPage(prevLink);
}

void HelpWindow::OnMainMenu(wxCommandEvent& evt)
{
	if (m_Html->GetOpenedPage() != "help/index.html")
	{
		m_Redo = std::stack<wxString>();
		m_Next->Disable();

		m_Undo.push(m_Html->GetOpenedPage());
		m_Prev->Enable();
	}

	m_Html->LoadPage("help/index.html");
}

void HelpWindow::OnClose(wxCommandEvent& evt)
{
	Hide();
}

void HelpWindow::OnCloseEvent(wxCloseEvent& evt)
{
	Hide();
}

void HelpWindow::OnLinkClick(wxHtmlLinkEvent& evt)
{
	wxString link = evt.GetLinkInfo().GetHref();

	// currently on this link already -> do nothing
	if (m_Undo.size() && m_Html->GetOpenedPage() == link)
	{
		evt.Skip();
		return;
	}

	// push the current link to the stack so that we mantain a history of links
	m_Undo.push(m_Html->GetOpenedPage());
	m_Prev->Enable();

	m_Redo = std::stack<wxString>();
	m_Next->Disable();

	evt.Skip();
}
