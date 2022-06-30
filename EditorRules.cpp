#include "EditorRules.h"
#include "Interpreter.h"

#include <algorithm>
#include <sstream>
#include <unordered_set>
#include <fstream>

#include "wx/richmsgdlg.h"

wxBEGIN_EVENT_TABLE(EditorRules, wxFrame)
	EVT_CLOSE(EditorRules::OnCloseEvent)
	EVT_SET_FOCUS(EditorRules::OnFocusEvent)
wxEND_EVENT_TABLE()

EditorRules::EditorRules(wxFrame* parent) : wxFrame(parent, wxID_ANY, "CellyGen::Rules", wxDefaultPosition, wxSize(Sizes::EDITOR_WIDTH, Sizes::EDITOR_HEIGHT))
{
	SetIcon(wxICON(aaaIcon));

	Center();

	BuildMenuBar();

	BuildInterface();
}

EditorRules::~EditorRules()
{
	wxDELETE(m_FindData);
	wxDELETE(m_FindDialog);
}

void EditorRules::SetInputRules(InputRules* inputRules)
{
	m_InputRules = inputRules;
}

void EditorRules::SetHelpWindow(HelpWindow* helpWindow)
{
	m_HelpWindow = helpWindow;
}

std::pair<std::vector<std::pair<std::string, Transition>>, std::vector<std::pair<int, std::string>>> EditorRules::Process(wxString text)
{
	// parse input text and return the processed rules

	text.MakeUpper();
	std::string rules = text.ToStdString();

	Interpreter interpreter;

	std::vector<std::pair<int, std::string>> invalidPositions = interpreter.Process(rules);
	std::vector<std::pair<std::string, Transition>> transitions = interpreter.GetTransitions();

	return { transitions,invalidPositions };
}

std::vector<std::pair<std::string, Transition>> EditorRules::GetData()
{
	std::pair<std::vector<std::pair<std::string, Transition>>, std::vector<std::pair<int, std::string>>> data = Process(m_TextCtrl->GetText());
	std::vector<std::pair<std::string, Transition>> transitions = data.first;
	std::vector<std::pair<int, std::string>> invalidPositions = data.second;

	// no errors
	if (invalidPositions.empty())
	{
		m_InvalidInput = false;
		return transitions;
	}

	// too many rules
	if (invalidPositions[0].first == -1)
	{
		wxMessageDialog dialog(
			this, "The maximum allowed number of rules has been surpassed.\nMake sure you're within the given limit before saving.", "Error",
			wxOK | wxICON_ERROR
		);
		dialog.SetExtendedMessage(
			"Limit: " + std::to_string(Sizes::RULES_MAX) + "\nCurrent number: " + std::to_string(transitions.size())
		);
		
		int answer = dialog.ShowModal();

		m_InvalidInput = true;
		return {};
	}
	
	wxRichMessageDialog dialog(
		this, "Some of the rules appear to be invalid.", "Warning",
		wxYES_NO | wxCANCEL | wxICON_EXCLAMATION
	);
	dialog.SetYesNoLabels("Mark && Resolve", "Ignore");

	// write the errors log
	std::string extendedMessage = "";
	for (auto& it : invalidPositions)
	{
		int ncol = it.first;
		int nline = -1;
		int cnt = 0;

		// map to real position (line, col)
		for (int i = 0; i < m_TextCtrl->GetLineCount(); i++)
		{
			// count linefeed whitespace
			wxString line = m_TextCtrl->GetLine(i);

			cnt += line.size();

			if (cnt >= ncol)
			{
				nline = i;
				ncol = line.size() - (cnt - ncol);
				break;
			}
		}

		std::string line = std::to_string(nline + 1);
		std::string col = std::to_string(ncol);

		extendedMessage += it.second + " at line " + line + ", after column " + col + "\n";

		it.first = nline;
	}
	dialog.ShowDetailedText(extendedMessage);

	int answer = dialog.ShowModal();

	if (answer == wxID_YES)
	{
		m_TextCtrl->MarkerDeleteAll(wxSTC_MARK_CIRCLE);
		m_TextCtrl->Refresh(false);

		for (int i = 0; i < invalidPositions.size(); i++)
		{
			int line = invalidPositions[i].first;
			m_TextCtrl->MarkerAdd(line, wxSTC_MARK_CIRCLE);
		}
		m_MenuBar->Enable(Ids::ID_MARK_NEXT_RULES, true);
		m_MenuBar->Enable(Ids::ID_MARK_PREV_RULES, true);

		m_InvalidInput = true;
		return {};
	}
	if (answer == wxID_NO)
	{
		m_InvalidInput = false;
		return transitions;
	}
	if (answer == wxID_CANCEL)
	{
		m_InvalidInput = true;
		return {};
	}

	return {};
}

void EditorRules::GoTo(std::string rule)
{
	// attempt to higlight the queried rule

	std::pair<int, int> position = FindRule(rule);

	// not found
	if (position.first == -1)
	{
		if (m_DialogShown) return;
		wxMessageDialog dialog (
			this, "No occurence found.", "Go To",
			wxOK | wxICON_INFORMATION
		);

		m_DialogShown = true;
		int answer = dialog.ShowModal();
		m_DialogShown = false;

		return;
	}

	m_TextCtrl->ShowPosition(position.first);
	m_TextCtrl->SetSelection(position.first, position.second);

	Show();
	SetFocus();
}

void EditorRules::DeleteRule(std::string rule)
{	
	std::pair<int, int> position = FindRule(rule);

	if (position.first != -1)
	{
		m_TextCtrl->ShowPosition(position.first);
		if (position.first > 0)position.first--;
		m_TextCtrl->Remove(position.first, position.second);

		m_PrevText = m_TextCtrl->GetText();
	}
}

void EditorRules::ForceClose()
{
	m_ForceClose = true;
}

void EditorRules::SetText(std::string text)
{
	m_TextCtrl->SetText(text);

	m_PrevText = text;
	m_InvalidInput = false;
}

wxString EditorRules::GetText()
{
	return m_TextCtrl->GetText();
}

void EditorRules::BuildMenuBar()
{
	wxMenu* menuFile = new wxMenu();
	wxMenu* menuEdit = new wxMenu();
	wxMenu* menuHelp = new wxMenu();

	menuFile->Append(Ids::ID_IMPORT_RULES, "&Import\tCtrl+O");
	menuFile->Append(Ids::ID_EXPORT_RULES, "Ex&port\tCtrl+Shift+S");
	menuFile->AppendSeparator();
	menuFile->Append(Ids::ID_SAVE_RULES, "&Save\tCtrl+S");
	menuFile->Append(Ids::ID_SAVE_CLOSE_RULES, "Sa&ve && Close\tAlt+S");
	menuFile->AppendSeparator();
	menuFile->Append(Ids::ID_CLOSE_RULES, "&Close\tEsc");

	menuEdit->Append(Ids::ID_FIND_RULES, "&Find\tCtrl+F");
	menuEdit->Append(Ids::ID_REPLACE_RULES, "&Replace\tCtrl+H");
	menuEdit->AppendSeparator();
	menuEdit->Append(Ids::ID_MARK_NEXT_RULES, "&Next Mark\tCtrl+E");
	menuEdit->Append(Ids::ID_MARK_PREV_RULES, "&Previous Mark\tCtrl+Q");
	menuEdit->AppendSeparator();
	menuEdit->Append(Ids::ID_FORMAT_RULES, "Forma&t\tCtrl+T");

	menuHelp->Append(Ids::ID_HELP_RULES, "&Defining Rules\tF1");

	m_MenuBar = new wxMenuBar();
	m_MenuBar->Append(menuFile, "&File");
	m_MenuBar->Append(menuEdit, "&Edit");
	m_MenuBar->Append(menuHelp, "&Help");

	m_MenuBar->Enable(Ids::ID_MARK_NEXT_RULES, false);
	m_MenuBar->Enable(Ids::ID_MARK_PREV_RULES, false);

	SetMenuBar(m_MenuBar);

	Bind(wxEVT_COMMAND_MENU_SELECTED, &EditorRules::OnImport, this, Ids::ID_IMPORT_RULES);
	Bind(wxEVT_COMMAND_MENU_SELECTED, &EditorRules::OnExport, this, Ids::ID_EXPORT_RULES);
	Bind(wxEVT_COMMAND_MENU_SELECTED, &EditorRules::OnClose, this, Ids::ID_CLOSE_RULES);
	Bind(wxEVT_COMMAND_MENU_SELECTED, &EditorRules::OnSave, this, Ids::ID_SAVE_RULES);
	Bind(wxEVT_COMMAND_MENU_SELECTED, &EditorRules::OnSaveClose, this, Ids::ID_SAVE_CLOSE_RULES);
	Bind(wxEVT_COMMAND_MENU_SELECTED, &EditorRules::OnMenuFind, this, Ids::ID_FIND_RULES);
	Bind(wxEVT_COMMAND_MENU_SELECTED, &EditorRules::OnMenuReplace, this, Ids::ID_REPLACE_RULES);
	Bind(wxEVT_COMMAND_MENU_SELECTED, &EditorRules::OnPrevMark, this, Ids::ID_MARK_PREV_RULES);
	Bind(wxEVT_COMMAND_MENU_SELECTED, &EditorRules::OnNextMark, this, Ids::ID_MARK_NEXT_RULES);
	Bind(wxEVT_COMMAND_MENU_SELECTED, &EditorRules::OnFormat, this, Ids::ID_FORMAT_RULES);
	Bind(wxEVT_COMMAND_MENU_SELECTED, &EditorRules::OnHelp, this, Ids::ID_HELP_RULES);
}

void EditorRules::BuildInterface()
{
	m_TextCtrl = new wxStyledTextCtrl(this);
	m_TextCtrl->SetMarginWidth(wxSTC_MARGIN_NUMBER, 80);
	m_TextCtrl->SetMarginType(wxSTC_MARGINOPTION_SUBLINESELECT, wxSTC_MARGIN_NUMBER);
	m_TextCtrl->MarkerSetBackground(wxSTC_MARK_CIRCLE, wxColour("red"));

	m_TextCtrl->Bind(wxEVT_KEY_UP, &EditorRules::UpdateLineColKey, this);
	m_TextCtrl->Bind(wxEVT_LEFT_UP, &EditorRules::UpdateLineColMouse, this);

	wxFont font = wxFont(16, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false);
	m_TextCtrl->StyleSetFont(wxSTC_STYLE_DEFAULT, font);

	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	sizer->Add(m_TextCtrl, 1, wxEXPAND);

	CreateStatusBar(); GetStatusBar()->SetStatusText("Line=1\tColumn=1");

	SetSizer(sizer);
}

void EditorRules::BuildDialogFind(std::string title, long style)
{
	wxDELETE(m_FindDialog);
	wxDELETE(m_FindData);

	m_FindData = new wxFindReplaceData(wxFR_DOWN);
	m_FindDialog = new wxFindReplaceDialog(this, m_FindData, title, style);

	m_FindDialog->Bind(wxEVT_FIND, &EditorRules::OnFind, this);
	m_FindDialog->Bind(wxEVT_FIND_NEXT, &EditorRules::OnFindNext, this);
	m_FindDialog->Bind(wxEVT_FIND_REPLACE, &EditorRules::OnReplace, this);
	m_FindDialog->Bind(wxEVT_FIND_REPLACE_ALL, &EditorRules::OnReplaceAll, this);
}

void EditorRules::OnCloseEvent(wxCloseEvent& evt)
{
	if (m_ForceClose)
	{
		evt.Skip();
		return;
	}

	// changes unsaved -> show dialog
	if (m_PrevText != m_TextCtrl->GetText())
	{
		wxMessageDialog dialog(
			this, "Do you want to save the changes?", "Save",
			wxYES_NO | wxCANCEL | wxICON_INFORMATION
		);

		int answer = dialog.ShowModal();

		if (answer == wxID_YES)
		{
			CloseEditor(true);
			return;
		}
		if (answer == wxID_NO)
		{
			CloseEditor(false);
			return;
		}
		if (answer == wxID_CANCEL) return;
	}
	else CloseEditor(false);
}

void EditorRules::OnFocusEvent(wxFocusEvent& evt)
{
	m_TextCtrl->SetFocus();
}

void EditorRules::OnClose(wxCommandEvent& evt)
{
	// changes unsaved -> show dialog
	if (m_PrevText != m_TextCtrl->GetText())
	{
		wxMessageDialog dialog(
			this, "Do you want to save the changes?", "Save",
			wxYES_NO | wxCANCEL | wxICON_INFORMATION
		);

		int answer = dialog.ShowModal();

		if (answer == wxID_YES)
		{
			CloseEditor(true);
			return;
		}
		if (answer == wxID_NO)
		{
			CloseEditor(false);
			return;
		}
		if (answer == wxID_CANCEL) return;
	}
	else CloseEditor(false);
}

void EditorRules::OnSave(wxCommandEvent& evt)
{
	std::vector<std::pair<std::string, Transition>> data = GetData();
	if (m_InvalidInput) return;

	if (m_InputRules->GetInputStates()->GetGrid()->GetGenerating() && !m_InputRules->GetInputStates()->GetGrid()->GetPaused())
	{
		wxMessageBox("Can't change rules while the simulation is playing. Try pausing it first.", "Error", wxICON_WARNING);
		return;
	}

	m_TextCtrl->MarkerDeleteAll(wxSTC_MARK_CIRCLE);
	m_TextCtrl->Refresh(false);
	m_MenuBar->Enable(Ids::ID_MARK_NEXT_RULES, false);
	m_MenuBar->Enable(Ids::ID_MARK_PREV_RULES, false);

	m_PrevText = m_TextCtrl->GetText();
	m_InputRules->SetRules(data);
}

void EditorRules::OnSaveClose(wxCommandEvent& evt)
{
	CloseEditor(true);
}

void EditorRules::OnMenuFind(wxCommandEvent& evt)
{
	BuildDialogFind("Find", 0);

	m_FindDialog->Show();
}

void EditorRules::OnMenuReplace(wxCommandEvent& evt)
{
	BuildDialogFind("Find & Replace", wxFR_REPLACEDIALOG);

	m_FindDialog->Show();
}

void EditorRules::OnFind(wxFindDialogEvent& evt)
{
	// attempt to highlight queried input

	wxString find = m_FindData->GetFindString();

	int flags = m_FindData->GetFlags();
	int result = m_TextCtrl->FindText(0, m_TextCtrl->GetLastPosition(), find, flags);

	if (result == -1)
	{
		if (m_DialogShown) return;
		wxMessageDialog dialog(
			this, "No occurences found.", "Find",
			wxOK | wxICON_INFORMATION
		);

		m_DialogShown = true;
		int answer = dialog.ShowModal();
		m_DialogShown = false;

		return;
	}

	m_TextCtrl->ShowPosition(result);
	m_TextCtrl->SetSelection(result, result + find.size());
}

void EditorRules::OnFindNext(wxFindDialogEvent& evt)
{
	// attempt to highlight next occurence of queried input

	wxString find = m_FindData->GetFindString();

	int flags = m_FindData->GetFlags();
	int result;

	if (flags & wxFR_DOWN) result = m_TextCtrl->FindText(m_TextCtrl->GetAnchor() + 1, m_TextCtrl->GetLastPosition(), find, flags);
	else result = m_TextCtrl->FindText(m_TextCtrl->GetAnchor(), 0, find, flags);

	if (result == -1)
	{
		if (m_DialogShown) return;
		wxMessageDialog dialog(
			this, "No more occurences found.", "Find",
			wxOK | wxICON_INFORMATION
		);

		m_DialogShown = true;
		int answer = dialog.ShowModal();
		m_DialogShown = false;

		return;
	}

	m_TextCtrl->ShowPosition(result);
	m_TextCtrl->SetSelection(result, result + find.size());
}

void EditorRules::OnReplace(wxFindDialogEvent& evt)
{
	// attempt to replace queried input with another input

	wxString replace = m_FindData->GetReplaceString();
	wxString find = m_FindData->GetFindString();

	if (m_TextCtrl->GetSelectedText() != find) return;

	m_TextCtrl->ReplaceSelection(replace);
}

void EditorRules::OnReplaceAll(wxFindDialogEvent& evt)
{
	// attempt to replace next occurence of queried input with another input

	wxString find = m_FindData->GetFindString();
	wxString replace = m_FindData->GetReplaceString();

	int flags = m_FindData->GetFlags();

	int occurences = 0;
	int result;
	while ((result = m_TextCtrl->FindText(0, m_TextCtrl->GetLastPosition(), find, flags)) != -1)
	{
		occurences++;

		m_TextCtrl->SetSelection(result, result + find.size());
		m_TextCtrl->ReplaceSelection(replace);
	}

	std::string message = std::to_string(occurences);
	message += (occurences == 1) ? " occurence has been replaced." : " occurences have been replaced.";

	if (m_DialogShown) return;
	wxMessageDialog dialog(
		this, message, "Replace All",
		wxOK | wxICON_INFORMATION
	);

	m_DialogShown = true;
	int answer = dialog.ShowModal();
	m_DialogShown = false;
}

void EditorRules::OnFormat(wxCommandEvent& evt)
{
	std::string text = (std::string)m_TextCtrl->GetText().Upper();

	// remove empty lines, white spaces and carriage symbols
	text.erase(std::unique(text.begin(), text.end(), [](char a, char b) {return a == ' ' && b == ' '; }), text.end());
	text.erase(remove(text.begin(), text.end(), '\r'), text.end());
	text.erase(std::unique(text.begin(), text.end(), [](char a, char b) {return a == '\n' && b == '\n'; }), text.end());

	m_TextCtrl->SetText(text);
}

void EditorRules::OnPrevMark(wxCommandEvent& evt)
{
	// go to the previously marked line

	int line = m_TextCtrl->MarkerPrevious(--m_MarkLine, 1);

	if (line == -1) line = m_TextCtrl->MarkerPrevious(m_TextCtrl->GetLineCount(), 1);
	m_MarkLine = line;

	int position = m_TextCtrl->PositionFromLine(line);
	m_TextCtrl->ShowPosition(position);

	m_TextCtrl->SetSelectionMode(wxSTC_SEL_LINES);
	m_TextCtrl->SetSelection(position, position);
	m_TextCtrl->SetSelectionMode(wxSTC_SEL_STREAM);
}

void EditorRules::OnNextMark(wxCommandEvent& evt)
{
	// go to the next marked line

	int line = m_TextCtrl->MarkerNext(++m_MarkLine, 1);

	if (line == -1) line = m_TextCtrl->MarkerNext(0, 1);
	m_MarkLine = line;

	int position = m_TextCtrl->PositionFromLine(line);
	m_TextCtrl->ShowPosition(position);

	m_TextCtrl->SetSelectionMode(wxSTC_SEL_LINES);
	m_TextCtrl->SetSelection(position, position);
	m_TextCtrl->SetSelectionMode(wxSTC_SEL_STREAM);
}

void EditorRules::OnImport(wxCommandEvent& evt)
{
	if (m_InputRules->GetInputStates()->GetGrid()->GetGenerating() && !m_InputRules->GetInputStates()->GetGrid()->GetPaused())
	{
		wxMessageBox("Can't import while the simulation is playing. Try pausing it first.", "Error", wxICON_WARNING);
		return;
	}

	wxFileDialog dialogFile(this, "Import Rules", "", "", "TXT files (*.txt)|*.txt", wxFD_OPEN | wxFD_FILE_MUST_EXIST);

	if (dialogFile.ShowModal() == wxID_CANCEL) return;

	std::ifstream in(dialogFile.GetPath().ToStdString());
	std::stringstream ss; ss << in.rdbuf();

	// read everything and ignore until "[RULES]"
	std::string text;
	bool valid = false;
	while (true)
	{
		std::string s;

		if (!std::getline(ss, s, '\n')) break;

		wxString symbol = wxString(s).Upper();
		if (symbol == "[RULES]")
		{
			std::unordered_set<wxString> marks = { "[STATES]","[RULES]","[NEIGHBORS]","[SIZE]","[CELLS]" };
			// read everything and store into "rules" until another mark is detected
			while (true)
			{
				s = "";

				if (!std::getline(ss, s, '\n')) break;

				if (marks.find(s) != marks.end()) break;

				text += s + "\n";
			}

			valid = true;
			break;
		}
	}

	if (!valid)
	{
		wxMessageBox("Invalid import file. Line marker \"[RULES]\" not found.", "Error", wxICON_ERROR | wxOK);
		return;
	}

	if (text.size())
	{
		std::pair<std::vector<std::pair<std::string, Transition>>, std::vector<std::pair<int, std::string>>> data = Process(text);
		std::vector<std::pair<std::string, Transition>> rules = data.first;
		std::vector<std::pair<int, std::string>> errors = data.second;


		if (rules.size())
		{
			SetText(text);
			m_InputRules->SetRules(rules);
		}

		if (errors.size())
		{
			wxMessageBox("Some of the imported rules appear to be invalid, as a result they have been ignored.", "Warning", wxICON_WARNING | wxOK);
		}
	}
}

void EditorRules::OnExport(wxCommandEvent& evt)
{
	wxFileDialog dialogFile(this, "Export Rules", "", "", "TXT files (*.txt)|*.txt", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

	if (dialogFile.ShowModal() == wxID_CANCEL) return;

	std::ofstream out(dialogFile.GetPath().ToStdString());
	out << "[RULES]\n";
	out << m_TextCtrl->GetText().ToStdString();
}

void EditorRules::OnHelp(wxCommandEvent& evt)
{
	m_HelpWindow->SetPage("defining-rules.html");
	m_HelpWindow->Show();
	m_HelpWindow->SetFocus();
}

void EditorRules::CloseEditor(bool save)
{
	wxDELETE(m_FindData); wxDELETE(m_FindDialog);

	if (m_ForceClose)
	{
		Close();
		return;
	}
	
	if (save)
	{
		if (m_InputRules->GetInputStates()->GetGrid()->GetGenerating() && !m_InputRules->GetInputStates()->GetGrid()->GetPaused())
		{
			wxMessageBox("Can't change rules while the simulation is playing. Try pausing it first.", "Error", wxICON_WARNING);
			return;
		}

		std::vector<std::pair<std::string, Transition>> data = GetData();
		if (m_InvalidInput) return;

		m_TextCtrl->MarkerDeleteAll(wxSTC_MARK_CIRCLE);
		m_TextCtrl->Refresh(false);
		m_MenuBar->Enable(Ids::ID_MARK_NEXT_RULES, false);
		m_MenuBar->Enable(Ids::ID_MARK_PREV_RULES, false);

		m_PrevText = m_TextCtrl->GetText();
		m_InputRules->SetRules(data);
	}
	else m_TextCtrl->SetText(m_PrevText);

	Hide();
}

void EditorRules::UpdateLineColMouse(wxMouseEvent& evt)
{
	long line = 0;
	long col = 0;
	long pos = m_TextCtrl->GetInsertionPoint();

	if (!m_TextCtrl->PositionToXY(pos, &col, &line))
	{
		line = m_TextCtrl->GetLineCount() - 1;
		col = m_TextCtrl->GetLine(line).size() - m_TextCtrl->GetWhitespaceSize() + 1;
	}

	std::string message = "Line=" + std::to_string(line+1) + "\tColumn=" + std::to_string(col+1);

	GetStatusBar()->SetStatusText(message);

	evt.Skip();
}

void EditorRules::UpdateLineColKey(wxKeyEvent& evt)
{
	long line = 0;
	long col = 0;
	int pos = m_TextCtrl->GetInsertionPoint();

	if (!m_TextCtrl->PositionToXY(pos, &col, &line))
	{
		line = m_TextCtrl->GetLineCount() - 1;
		col = m_TextCtrl->GetLine(line).size() - m_TextCtrl->GetWhitespaceSize() + 1;
	}

	std::string message = "Line=" + std::to_string(line+1) + "\tColumn=" + std::to_string(col+1);

	GetStatusBar()->SetStatusText(message);

	evt.Skip();
}

std::pair<int, int> EditorRules::FindRule(std::string rule)
{
	int pos = 0;

	std::string s = "";
	bool firstchar = true;
	int posBegin = 0;
	for (int i = 0; i < m_TextCtrl->GetLineCount(); i++)
	{
		wxString line = m_TextCtrl->GetLine(i);
		wxString rules = line;

		// inline comment, ignore it but continue with the state before it
		if (line.find("!") != line.npos) rules = line.substr(0, line.find("!"));

		if (rules.empty())
		{
			pos += line.size();
			continue;
		}
		rules.MakeUpper();

		// compose full rule and check if it matches the query
		for (int j = 0; j < rules.size(); j++)
		{
			char c = rules[j];

			if (iswspace(c)) continue;

			if (c == ';')
			{
				s.push_back(';');

				if (s == rule)
				{
					int posEnd = pos + j + 1;
					return { posBegin, posEnd };
				}

				firstchar = true;
				s = "";
			}
			else
			{
				s.push_back(c);

				if (firstchar)
				{
					firstchar = false;
					posBegin = pos + j;
				}
			}
		}

		pos += line.size();
	}

	return { -1,-1 };
}
