#include "EditorStates.h"

#include <algorithm>
#include <sstream>
#include <unordered_set>
#include <fstream>

#include "wx/richmsgdlg.h"

wxBEGIN_EVENT_TABLE(EditorStates, wxFrame)
	EVT_CLOSE(EditorStates::OnCloseEvent)
	EVT_SET_FOCUS(EditorStates::OnFocusEvent)
wxEND_EVENT_TABLE()

EditorStates::EditorStates(wxFrame* parent) : wxFrame(parent, wxID_ANY, "CellyGen::States", wxDefaultPosition, wxSize(Sizes::EDITOR_WIDTH, Sizes::EDITOR_HEIGHT))
{
	SetIcon(wxICON(aaaIcon));

	Center();

	BuildMenuBar();

	BuildInterface();
}

EditorStates::~EditorStates()
{
	wxDELETE(m_FindData);
	wxDELETE(m_FindDialog);
}

void EditorStates::SetInputStates(InputStates* inputStates)
{
	m_InputStates = inputStates;
}

void EditorStates::SetHelpWindow(HelpWindow* helpWindow)
{
	m_HelpWindow = helpWindow;
}

std::pair<std::vector<std::string>, std::vector<std::pair<int, std::string>>> EditorStates::Process(wxString text)
{
	// parse input text and return the processed states

	text.MakeUpper();

	// count lines (states) and mark duplicates/invalid states
	std::unordered_set<std::string> setStates({ "FREE" });
	std::vector<std::pair<int, std::string>> indexInvalid;

	std::stringstream ssText(text.ToStdString());
	std::string line;
	std::vector<std::string> states({ "FREE" });

	int cntLine = -1;
	while (std::getline(ssText, line, ';'))
	{
		cntLine++;

		if (line.empty()) continue;

		// inline comment, ignore it but continue with the state before it
		if (line.find("!") != line.npos)
		{
			int curr = (ssText.tellg() == -1) ? text.size() : (int)ssText.tellg();
			int lf = text.find('\n', curr - line.size());

			if (lf != text.npos)
			{
				ssText.clear();
				ssText.seekg(lf + 1);
			}

			line = line.substr(0, line.find("!"));
		}

		if (line.empty()) continue;

		std::stringstream ss(line);
		std::string state = "";
		std::string s = "";

		ss >> state;
		ss >> s;

		if (state.empty()) continue;

		if (ssText.tellg() == -1)
		{
			indexInvalid.push_back({ text.size(), "<EXPECTED ';' AFTER STATE DECLARATION>" });
			continue;
		}

		int pos = (int)ssText.tellg() - state.size();

		if (!s.empty())
		{
			indexInvalid.push_back({ pos, "<ILLEGAL SPACE CHARACTER, USE '_' INSTEAD>" });
			continue;
		}

		// state's name does not respect the character limits
		if (state.size() < Sizes::CHARS_STATE_MIN || state.size() > Sizes::CHARS_STATE_MAX)
		{
			indexInvalid.push_back({ pos, "<INVALID STATE SIZE>" });
			continue;
		}

		// state's name contains illegal characters
		if (std::find_if(state.begin(), state.end(), [](char c) { return !(isalnum(c) || (c == '_')); }) != state.end())
		{
			indexInvalid.push_back({ pos, "<ILLEGAL CHARACTERS>" });
			continue;
		}

		// duplicate found
		if (setStates.find(state) != setStates.end())
		{
			indexInvalid.push_back({ pos,"<DUPLICATE STATE>" });
			continue;
		}
		else
		{
			setStates.insert(state);
		}

		states.push_back(state);
	}

	return { states,indexInvalid };
}

std::vector<std::string> EditorStates::GetData()
{
	std::pair<std::vector<std::string>, std::vector<std::pair<int, std::string>>> data = Process(m_TextCtrl->GetText());
	std::vector<std::string> states = data.first;
	std::vector<std::pair<int, std::string>> indexInvalid = data.second;

	if (states.size() > Sizes::STATES_MAX)
	{
		wxMessageDialog dialog(
			this, "The maximum allowed number of states has been surpassed.\nMake sure you're within the given limit before saving.", "Error",
			wxOK | wxICON_ERROR
		);
		dialog.SetExtendedMessage(
			"Limit: " + std::to_string(Sizes::STATES_MAX - 1) + "\nCurrent number: " + std::to_string(states.size() - 1)
		);

		int answer = dialog.ShowModal();

		m_InvalidInput = true;
		return {};
	}
	else if (indexInvalid.size())
	{
		 wxRichMessageDialog dialog(
			this, "Some of the states appear to be invalid.", "Warning",
			wxYES_NO | wxCANCEL | wxICON_EXCLAMATION
		);
		dialog.SetYesNoLabels("Mark && Resolve", "Ignore");

		// write the errors log
		std::string extendedMessage = "";
		for (auto& it : indexInvalid)
		{
			int ncol = it.first;
			int nline = 0;
			int cnt = 0;

			for (int i = 0; i < m_TextCtrl->GetLineCount(); i++)
			{
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

			for (int i = 0; i < indexInvalid.size(); i++)
			{
				m_TextCtrl->MarkerAdd(indexInvalid[i].first, wxSTC_MARK_CIRCLE);
			}
			m_MenuBar->Enable(Ids::ID_MARK_NEXT_STATES, true);
			m_MenuBar->Enable(Ids::ID_MARK_PREV_STATES, true);

			m_InvalidInput = true;
			return {};
		}
		if (answer == wxID_NO)
		{
			m_InvalidInput = false;
			return states;
		}
		if (answer == wxID_CANCEL)
		{
			m_InvalidInput = true;
			return {};
		}
	}

	m_InvalidInput = false;
	return states;
}

void EditorStates::GoTo(std::string state)
{
	// attempt to higlight the queried state

	std::pair<int, int> position = FindState(state);

	// not found
	if (position.first == -1)
	{
		wxMessageDialog dialog(
			this, "No occurence found.", "Go To",
			wxOK | wxICON_INFORMATION
		);

		int answer = dialog.ShowModal();

		return;
	}
	else
	{
		m_TextCtrl->ShowPosition(position.first);

		m_TextCtrl->SetSelection(position.first, position.second);

		Show();
		SetFocus();
	}
}

void EditorStates::DeleteState(std::string state)
{
	std::pair<int, int> position = FindState(state);

	if (position.first != -1)
	{
		m_TextCtrl->PositionFromLine(position.first);
		if (position.first > 0)position.first--;
		m_TextCtrl->Remove(position.first, position.second);

		m_PrevText = m_TextCtrl->GetText();
	}
}

void EditorStates::ForceClose()
{
	m_ForceClose = true;

	Close();
}

void EditorStates::SetText(std::string text)
{
	m_TextCtrl->SetText(text);

	m_PrevText = text;
	m_InvalidInput = false;
}

wxString EditorStates::GetText()
{
	return m_TextCtrl->GetText();
}

void EditorStates::BuildMenuBar()
{
	wxMenu* menuFile = new wxMenu();
	wxMenu* menuEdit = new wxMenu();
	wxMenu* menuHelp = new wxMenu();

	menuFile->Append(Ids::ID_IMPORT_STATES, "&Import\tCtrl+O");
	menuFile->Append(Ids::ID_EXPORT_STATES, "Ex&port\tCtrl+Shift+S");
	menuFile->AppendSeparator();
	menuFile->Append(Ids::ID_SAVE_STATES, "&Save\tCtrl+S");
	menuFile->Append(Ids::ID_SAVE_CLOSE_STATES, "Sa&ve && Close\tAlt+S");
	menuFile->AppendSeparator();
	menuFile->Append(Ids::ID_CLOSE_STATES, "&Close\tEsc");

	menuEdit->Append(Ids::ID_FIND_STATES, "&Find\tCtrl+F");
	menuEdit->Append(Ids::ID_REPLACE_STATES, "&Replace\tCtrl+H");
	menuEdit->AppendSeparator();
	menuEdit->Append(Ids::ID_MARK_NEXT_STATES, "&Next Mark\tCtrl+E");
	menuEdit->Append(Ids::ID_MARK_PREV_STATES, "&Previous Mark\tCtrl+Q");
	menuEdit->AppendSeparator();
	menuEdit->Append(Ids::ID_FORMAT_STATES, "Forma&t\tCtrl+T");

	menuHelp->Append(Ids::ID_HELP_STATES, "&Defining States\tF1");

	m_MenuBar = new wxMenuBar();
	m_MenuBar->Append(menuFile, "&File");
	m_MenuBar->Append(menuEdit, "&Edit");
	m_MenuBar->Append(menuHelp, "&Help");

	m_MenuBar->Enable(Ids::ID_MARK_NEXT_STATES, false);
	m_MenuBar->Enable(Ids::ID_MARK_PREV_STATES, false);

	SetMenuBar(m_MenuBar);

	Bind(wxEVT_COMMAND_MENU_SELECTED, &EditorStates::OnImport, this, Ids::ID_IMPORT_STATES);
	Bind(wxEVT_COMMAND_MENU_SELECTED, &EditorStates::OnExport, this, Ids::ID_EXPORT_STATES);
	Bind(wxEVT_COMMAND_MENU_SELECTED, &EditorStates::OnClose, this, Ids::ID_CLOSE_STATES);
	Bind(wxEVT_COMMAND_MENU_SELECTED, &EditorStates::OnSave, this, Ids::ID_SAVE_STATES);
	Bind(wxEVT_COMMAND_MENU_SELECTED, &EditorStates::OnSaveClose, this, Ids::ID_SAVE_CLOSE_STATES);
	Bind(wxEVT_COMMAND_MENU_SELECTED, &EditorStates::OnMenuFind, this, Ids::ID_FIND_STATES);
	Bind(wxEVT_COMMAND_MENU_SELECTED, &EditorStates::OnMenuReplace, this, Ids::ID_REPLACE_STATES);
	Bind(wxEVT_COMMAND_MENU_SELECTED, &EditorStates::OnPrevMark, this, Ids::ID_MARK_PREV_STATES);
	Bind(wxEVT_COMMAND_MENU_SELECTED, &EditorStates::OnNextMark, this, Ids::ID_MARK_NEXT_STATES);
	Bind(wxEVT_COMMAND_MENU_SELECTED, &EditorStates::OnFormat, this, Ids::ID_FORMAT_STATES);
	Bind(wxEVT_COMMAND_MENU_SELECTED, &EditorStates::OnHelp, this, Ids::ID_HELP_STATES);
}

void EditorStates::BuildInterface()
{
	m_TextCtrl = new wxStyledTextCtrl(this);
	m_TextCtrl->SetMarginWidth(wxSTC_MARGIN_NUMBER, 80);
	m_TextCtrl->SetMarginType(wxSTC_MARGINOPTION_SUBLINESELECT, wxSTC_MARGIN_NUMBER);
	m_TextCtrl->MarkerSetBackground(wxSTC_MARK_CIRCLE, wxColour("red"));

	m_TextCtrl->Bind(wxEVT_KEY_UP, &EditorStates::UpdateLineColKey, this);
	m_TextCtrl->Bind(wxEVT_LEFT_UP, &EditorStates::UpdateLineColMouse, this);

	wxFont font = wxFont(16, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false);
	m_TextCtrl->StyleSetFont(wxSTC_STYLE_DEFAULT, font);

	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	sizer->Add(m_TextCtrl, 1, wxEXPAND);

	CreateStatusBar(); GetStatusBar()->SetStatusText("Line=1\tColumn=1");

	this->SetSizer(sizer);
}

void EditorStates::BuildDialogFind(std::string title, long style)
{
	wxDELETE(m_FindDialog);
	wxDELETE(m_FindData);

	m_FindData = new wxFindReplaceData(wxFR_DOWN);
	m_FindDialog = new wxFindReplaceDialog(this, m_FindData, title, style);

	m_FindDialog->Bind(wxEVT_FIND, &EditorStates::OnFind, this);
	m_FindDialog->Bind(wxEVT_FIND_NEXT, &EditorStates::OnFindNext, this);
	m_FindDialog->Bind(wxEVT_FIND_REPLACE, &EditorStates::OnReplace, this);
	m_FindDialog->Bind(wxEVT_FIND_REPLACE_ALL, &EditorStates::OnReplaceAll, this);
}

void EditorStates::OnCloseEvent(wxCloseEvent& evt)
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

void EditorStates::OnFocusEvent(wxFocusEvent& evt)
{
	m_TextCtrl->SetFocus();
}

void EditorStates::OnClose(wxCommandEvent& evt)
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

void EditorStates::OnSave(wxCommandEvent& evt)
{
	std::vector<std::string> data = GetData();
	if (m_InvalidInput) return;

	if (m_InputStates->GetGrid()->GetGenerating() || !m_InputStates->GetGrid()->GetPaused())
	{
		wxMessageBox("Can't change states while the simulation is playing. Try pausing it first.", "Error", wxICON_WARNING);
		return;
	}

	m_TextCtrl->MarkerDeleteAll(wxSTC_MARK_CIRCLE);
	m_TextCtrl->Refresh(false);
	m_MenuBar->Enable(Ids::ID_MARK_NEXT_STATES, false);
	m_MenuBar->Enable(Ids::ID_MARK_PREV_STATES, false);

	m_PrevText = m_TextCtrl->GetText();
	m_InputStates->SetStates(data);
}

void EditorStates::OnSaveClose(wxCommandEvent& evt)
{
	CloseEditor(true);
}

void EditorStates::OnMenuFind(wxCommandEvent& evt)
{
	BuildDialogFind("Find", 0);

	m_FindDialog->Show();
}

void EditorStates::OnMenuReplace(wxCommandEvent& evt)
{
	BuildDialogFind("Find & Replace", wxFR_REPLACEDIALOG);

	m_FindDialog->Show();
}

void EditorStates::OnFind(wxFindDialogEvent& evt)
{
	// attempt to highlight queried input

	wxString find = m_FindData->GetFindString();

	int flags = m_FindData->GetFlags();
	int result;

	if (flags & wxFR_DOWN) result = m_TextCtrl->FindText(0, m_TextCtrl->GetLastPosition(), find, flags);
	else result = m_TextCtrl->FindText(m_TextCtrl->GetLastPosition(), 0, find, flags);

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

void EditorStates::OnFindNext(wxFindDialogEvent& evt)
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

void EditorStates::OnReplace(wxFindDialogEvent& evt)
{
	// attempt to replace queried input with another input

	wxString replace = m_FindData->GetReplaceString();
	wxString find = m_FindData->GetFindString();

	if (m_TextCtrl->GetSelectedText() != find) return;

	m_TextCtrl->ReplaceSelection(replace);
}

void EditorStates::OnReplaceAll(wxFindDialogEvent& evt)
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

void EditorStates::OnFormat(wxCommandEvent& evt)
{
	std::string text = (std::string)m_TextCtrl->GetText().Upper();

	// remove empty lines, white spaces and carriage symbols
	text.erase(remove(text.begin(), text.end(), ' '), text.end());
	text.erase(remove(text.begin(), text.end(), '\r'), text.end());
	text.erase(std::unique(text.begin(), text.end(), [](char a, char b) {return a == '\n' && b == '\n'; }), text.end());

	m_TextCtrl->SetText(text);
}

void EditorStates::OnPrevMark(wxCommandEvent& evt)
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

void EditorStates::OnNextMark(wxCommandEvent& evt)
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

void EditorStates::OnImport(wxCommandEvent& evt)
{
	if (m_InputStates->GetGrid()->GetGenerating() || !m_InputStates->GetGrid()->GetPaused())
	{
		wxMessageBox("Can't import while the simulation is playing. Try pausing it first.", "Error", wxICON_WARNING);
		return;
	}

	wxFileDialog dialogFile(this, "Import States", "", "", "TXT files (*.txt)|*.txt", wxFD_OPEN | wxFD_FILE_MUST_EXIST);

	if (dialogFile.ShowModal() == wxID_CANCEL) return;

	std::ifstream in(dialogFile.GetPath().ToStdString());
	std::stringstream ss; ss << in.rdbuf();
	
	// read everything and ignore until "[STATES]"
	std::string text;
	bool valid = false;
	while (true)
	{
		std::string s;

		if (!std::getline(ss, s, '\n')) break;

		wxString symbol = wxString(s).Upper();
		if (symbol == "[STATES]")
		{
			std::unordered_set<wxString> marks = { "[STATES]","[RULES]","[NEIGHBORS]","[SIZE]","[CELLS]" };
			// read everything and store into "states" until another mark is detected
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
		wxMessageBox("Invalid import file. Line marker \"[STATES]\" not found.", "Error", wxICON_ERROR| wxOK);
		return;
	}

	if (text.size())
	{
		std::pair<std::vector<std::string>, std::vector<std::pair<int, std::string>>> data = Process(text);
		std::vector<std::string> states = data.first;
		std::vector<std::pair<int, std::string>> errors = data.second;


		if (states.size())
		{
			SetText(text);
			m_InputStates->SetStates(states);
		}

		if (errors.size())
		{
			wxMessageBox("Some of the imported states appear to be invalid, as a result they have been ignored.", "Warning", wxICON_WARNING | wxOK);
		}
	}
}

void EditorStates::OnExport(wxCommandEvent& evt)
{
	wxFileDialog dialogFile(this, "Export States", "", "", "TXT files (*.txt)|*.txt", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

	if (dialogFile.ShowModal() == wxID_CANCEL) return;

	std::ofstream out(dialogFile.GetPath().ToStdString());
	out << "[STATES]\n";
	out << m_TextCtrl->GetText().ToStdString();
}

void EditorStates::OnHelp(wxCommandEvent& evt)
{
	m_HelpWindow->SetPage("defining-states.html");
	m_HelpWindow->Show();
	m_HelpWindow->SetFocus();
}

void EditorStates::UpdateLineColMouse(wxMouseEvent& evt)
{
	long line = 0;
	long col = 0;
	long pos = m_TextCtrl->GetInsertionPoint();

	if (!m_TextCtrl->PositionToXY(pos, &col, &line))
	{
		line = m_TextCtrl->GetLineCount() - 1;
		col = m_TextCtrl->GetLine(line).size() - m_TextCtrl->GetWhitespaceSize() + 1;
	}

	std::string message = "Line=" + std::to_string(line + 1) + "\tColumn=" + std::to_string(col + 1);

	GetStatusBar()->SetStatusText(message);

	evt.Skip();
}

void EditorStates::UpdateLineColKey(wxKeyEvent& evt)
{
	long line = 0;
	long col = 0;
	int pos = m_TextCtrl->GetInsertionPoint();

	if (!m_TextCtrl->PositionToXY(pos, &col, &line))
	{
		line = m_TextCtrl->GetLineCount() - 1;
		col = m_TextCtrl->GetLine(line).size() - m_TextCtrl->GetWhitespaceSize() + 1;
	}

	std::string message = "Line=" + std::to_string(line + 1) + "\tColumn=" + std::to_string(col + 1);

	GetStatusBar()->SetStatusText(message);

	evt.Skip();
}

std::pair<int, int> EditorStates::FindState(std::string state)
{
	int pos = 0;

	std::string s = "";
	bool firstchar = true;
	int posBegin = 0;
	for (int i = 0; i < m_TextCtrl->GetLineCount(); i++)
	{
		wxString line = m_TextCtrl->GetLine(i);
		wxString states = line;

		firstchar = true;

		// inline comment, ignore it but continue with the state before it
		if (line.find("!") != line.npos) states = line.substr(0, line.find("!"));

		if (states.empty())
		{
			pos += line.size();
			continue;
		}
		states.MakeUpper();

		// compose full state and check if it matches the query
		for (int j = 0; j < states.size(); j++)
		{
			char c = states[j];

			if (iswspace(c)) continue;

			if (c == ';')
			{
				if (s == state)
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

void EditorStates::CloseEditor(bool save)
{
	wxDELETE(m_FindData); wxDELETE(m_FindDialog);

	if (m_ForceClose)
	{
		Close();
		return;
	}

	if (save)
	{
		if (m_InputStates->GetGrid()->GetGenerating() || !m_InputStates->GetGrid()->GetPaused())
		{
			wxMessageBox("Can't change states while the simulation is playing. Try pausing it first.", "Error", wxICON_WARNING);
			return;
		}

		std::vector<std::string> data = GetData();
		if (m_InvalidInput) return;

		m_TextCtrl->MarkerDeleteAll(wxSTC_MARK_CIRCLE);
		m_TextCtrl->Refresh(false);
		m_MenuBar->Enable(Ids::ID_MARK_NEXT_STATES, false);
		m_MenuBar->Enable(Ids::ID_MARK_PREV_STATES, false);

		m_PrevText = m_TextCtrl->GetText();
		m_InputStates->SetStates(data);
	}
	else m_TextCtrl->SetText(m_PrevText);

	Hide();
}
