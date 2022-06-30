#pragma once

#include "wx/wx.h"
#include "wx/wxhtml.h"

#include <string>
#include <stack>

class HelpWindow : public wxFrame
{
public:
	HelpWindow(wxWindow* parent);
	~HelpWindow();

	void SetPage(wxString page);
private:
	wxHtmlWindow* m_Html = nullptr;
	wxButton* m_Prev = nullptr;
	wxButton* m_Next = nullptr;

	std::stack<wxString> m_Undo;
	std::stack<wxString> m_Redo;

	void BuildInterface();
	void SetShortcuts();

	wxDECLARE_EVENT_TABLE();

	void OnPrev(wxCommandEvent& evt);
	void OnNext(wxCommandEvent& evt);
	void OnMainMenu(wxCommandEvent& evt);
	void OnClose(wxCommandEvent& evt);
	void OnCloseEvent(wxCloseEvent& evt);

	void OnLinkClick(wxHtmlLinkEvent& evt);
};

