#include "InputRules.h"
#include "Interpreter.h"

#include <unordered_set>

InputRules::InputRules(wxWindow* parent) : wxPanel(parent)
{
	BuildInterface();

    BuildMenu();
}

InputRules::~InputRules()
{
    wxDELETE(m_Menu);
}

ListRules* InputRules::GetList()
{
    return m_List;
}

std::vector<std::pair<std::string, Transition>>& InputRules::GetRules()
{
	return m_Rules;
}

InputStates* InputRules::GetInputStates()
{
    return m_InputStates;
}

InputNeighbors* InputRules::GetInputNeighbors()
{
    return m_InputNeighbors;
}

void InputRules::SetEditorRules(EditorRules* editorRules)
{
    m_EditorRules = editorRules;
}

void InputRules::SetInputStates(InputStates* inputStates)
{
    m_InputStates = inputStates;
}

void InputRules::SetInputNeighbors(InputNeighbors* inputNeighbors)
{
    m_InputNeighbors = inputNeighbors;
}

void InputRules::SetRules(std::vector<std::pair<std::string, Transition>> rules)
{
    m_Rules = rules;

    m_States = m_InputStates->GetStates();

    // update list display
    int nOfItems = m_List->GetItemCount();
    int i = 0;
    std::unordered_set<std::string> alreadyUpdated;
    for (i = 0; i < rules.size(); i++)
    {
        int id = i + 1;
        std::string state1 = rules[i].first;
        std::string state2 = rules[i].second.state;
        std::string condition = rules[i].second.condition;

        if (alreadyUpdated.find(state1 + "/" + state2 + ":" + condition) != alreadyUpdated.end()) continue;

        wxColour color1 = wxColour("white");
        if (m_States.find(state1) != m_States.end()) color1 = wxColour(m_States[state1]);

        wxColour color2 = wxColour("white");
        if (m_States.find(state2) != m_States.end()) color2 = wxColour(m_States[state2]);

        // new items -> push them to the list
        if (i > nOfItems - 1)
        {
            wxColour bgColorA("white"), bgColorB("white");
            wxColour txtColorA("black"), txtColorB("black");
            
            if (m_States.find(state1) != m_States.end() && m_States.find(state2) != m_States.end())
            {
                bgColorA = wxColour(m_States[state1]); bgColorB = wxColour(m_States[state2]);
                // if background is black -> font should be white and viceversa - inequation found on stackoverflow
                txtColorA = (bgColorA.Red() * 0.299 + bgColorA.Green() * 0.587 + bgColorA.Blue() * 0.114) > 186.0 ? wxColour("black") : wxColour("white");
                txtColorB = (bgColorB.Red() * 0.299 + bgColorB.Green() * 0.587 + bgColorB.Blue() * 0.114) > 186.0 ? wxColour("black") : wxColour("white");
            }

            m_List->PushBack({ id, state1, state2, condition }, bgColorA, bgColorB);

            continue;
        }

        // updated items -> detect changes and update the list as well

        wxString itmId = m_List->GetItemText(i, 0);
        wxString itmState1 = m_List->GetState1(i);
        wxString itmState2 = m_List->GetState2(i);
        wxString itmCondition = m_List->GetCond(i);
        wxColour itmColor1 = m_List->GetColor1(i);
        wxColour itmColor2 = m_List->GetColor2(i);

        if (itmId != id) m_List->ChangeItemId(i, id);
        if (itmState1 != state1) m_List->ChangeItemState1(i, state1);
        if (itmState2 != state2) m_List->ChangeItemState2(i, state2);
        if (itmCondition != condition) m_List->ChangeItemCond(i, condition);
        if (itmColor1 != color1) m_List->ChangeColor1(i, color1);
        if (itmColor2 != color2) m_List->ChangeColor2(i, color2);

        alreadyUpdated.insert(state1 + "/" + state2 + ":" + condition);
    }

    // deleted items -> delete them from the list as well
    while (i < nOfItems--)
    {
        std::string state1 = m_List->GetState1(i);
        std::string state2 = m_List->GetState2(i);
        std::string condition = m_List->GetCond(i);

        m_List->Erase(i);

        if (alreadyUpdated.find(state1 + "/" + state2 + ":" + condition) != alreadyUpdated.end()) continue;
    }

    m_List->RefreshAfterUpdate();
}

void InputRules::UpdateColor(std::string state, wxColour color)
{
    int changes = 0;
    for (int i = 0; i < m_List->GetItemCount(); i++)
    {
        wxString itmState1 = m_List->GetState1(i);
        wxString itmState2 = m_List->GetState2(i);
        wxColour itmColor1 = m_List->GetColor1(i);
        wxColour itmColor2 = m_List->GetColor2(i);

        if (itmState1 == state)
        {
            m_List->ChangeColor1(i, color);
            changes++;
            continue;
        }

        if (itmState2 == state)
        {
            m_List->ChangeColor2(i, color);
            changes++;
            continue;
        }
    }

    if (changes > 0) m_List->Refresh(false);
}

void InputRules::BuildInterface()
{
	wxButton* edit = new wxButton(this, Ids::ID_EDIT_RULES, wxString("Edit Rules"));
    edit->SetToolTip("Launch the Rules Editor\t(Ctrl+1)");
    edit->Bind(wxEVT_BUTTON, &InputRules::OnEdit, this);

    m_Search = new wxSearchCtrl(this, wxID_ANY);
    m_Search->SetToolTip("Search for a rule...\t(Ctrl+Shift+2)");
    m_Search->Bind(wxEVT_TEXT, &InputRules::Search, this);
    m_Search->Bind(wxEVT_SEARCHCTRL_SEARCH_BTN, &InputRules::SearchEnter, this);

	m_List = new ListRules(this);
    m_List->Bind(wxEVT_CONTEXT_MENU, &InputRules::OnItemRightClick, this);

    wxButton* focusSearch = new wxButton(this, Ids::ID_SEARCH_RULES);
    focusSearch->Hide();
    focusSearch->Bind(wxEVT_BUTTON, &InputRules::FocusSearch, this);

	wxStaticBoxSizer* sizer = new wxStaticBoxSizer(wxVERTICAL, this, "Rules");
	sizer->Add(edit, 0, wxEXPAND);
    sizer->Add(m_Search, 0, wxEXPAND);
	sizer->Add(m_List, 1, wxEXPAND);

	SetSizer(sizer);
}

void InputRules::Search(wxCommandEvent& evt)
{
    std::string query = std::string(evt.GetString().MakeUpper());

    // unselect previous found queries
    int selection = m_List->GetFirstSelected();
    while (selection != -1)
    {
        m_List->Select(selection, false);
        selection = m_List->GetNextSelected(selection);
    }

    if (query.size() < 1) return;

    // select found queries
    for (int i = 0; i < m_List->GetItemCount(); i++)
    {
        std::string state1 = m_List->GetState1(i);
        std::string state2 = m_List->GetState2(i);
        std::string cond = m_List->GetCond(i);

        std::string rule = state1 + "/" + state2;
        if (!cond.empty()) rule += ":" + cond;
        rule += ";";

        if (rule.find(query) != rule.npos)
        {
            m_List->Select(i);
            m_List->EnsureVisible(i);

            return;
        }
    }
}

void InputRules::SearchEnter(wxCommandEvent& evt)
{
    std::string query = std::string(evt.GetString().MakeUpper());

    int selection = m_List->GetFirstSelected();

    if (query.size() < 1) return;

    // continue searching
    if (wxGetKeyState(WXK_SHIFT))
    {
        // search upwards
        for (int i = selection - 1; i > -1; i--)
        {
            std::string state1 = m_List->GetState1(i);
            std::string state2 = m_List->GetState2(i);
            std::string cond = m_List->GetCond(i);

            std::string rule = state1 + "/" + state2;
            if (!cond.empty()) rule += ":" + cond;
            rule += ";";

            if (rule.find(query) != rule.npos)
            {
                m_List->Select(selection, false);
                m_List->Select(i);
                m_List->EnsureVisible(i);

                return;
            }
        }

        return;
    }

    // search downwards
    for (int i = selection + 1; i < m_List->GetItemCount(); i++)
    {
        std::string state1 = m_List->GetState1(i);
        std::string state2 = m_List->GetState2(i);
        std::string cond = m_List->GetCond(i);

        std::string rule = state1 + "/" + state2;
        if (!cond.empty()) rule += ":" + cond;
        rule += ";";

        if (rule.find(query) != rule.npos)
        {
            m_List->Select(selection, false);
            m_List->Select(i);
            m_List->EnsureVisible(i);

            return;
        }
    }
}

void InputRules::BuildMenu()
{
    m_Menu = new wxMenu();

    m_Menu->Append(Ids::ID_GOTO_RULE, "Go To");
    m_Menu->AppendSeparator();
    m_Menu->Append(Ids::ID_DELETE_RULE, "Delete");

    m_Menu->Bind(wxEVT_COMMAND_MENU_SELECTED, &InputRules::OnMenuSelected, this);
}

void InputRules::OnItemRightClick(wxContextMenuEvent& evt)
{
    int itemCount = m_List->GetSelectedItemCount();
    if (itemCount != 1)
    {
        m_Menu->Enable(Ids::ID_GOTO_RULE, false);

        if (itemCount) m_Menu->Enable(Ids::ID_DELETE_RULE, true);
        else m_Menu->Enable(Ids::ID_DELETE_RULE, false);
    }
    else
    {
        m_Menu->Enable(Ids::ID_GOTO_RULE, true);
        m_Menu->Enable(Ids::ID_DELETE_RULE, true);
    }

    PopupMenu(m_Menu);
}

void InputRules::OnMenuSelected(wxCommandEvent& evt)
{
    switch (evt.GetId())
    {
    case Ids::ID_GOTO_RULE:
        RuleGoTo();
        break;
    case Ids::ID_DELETE_RULE:
        RuleDelete();
        break;
    default:
        break;
    }
}

void InputRules::RuleGoTo()
{
    int selection = m_List->GetFirstSelected();
    std::string state1 = m_List->GetState1(selection);
    std::string state2 = m_List->GetState2(selection);
    std::string cond = m_List->GetCond(selection);

    std::string rule = state1 +"/" + state2;
    if (!cond.empty()) rule += ":" + cond;
    rule += ";";

    m_EditorRules->GoTo(rule);
}

void InputRules::RuleDelete()
{
    int selection = m_List->GetFirstSelected();

    if (selection == -1) return;

    if (m_InputStates->GetGrid()->GetGenerating() || !m_InputStates->GetGrid()->GetPaused())
    {
        wxMessageBox("Can't delete rules while the simulation is playing. Try pausing it first.", "Error", wxICON_WARNING);
        return;
    }

    std::unordered_set<std::string> toBeDeleted;
    // delete selected rules
    while (selection != -1)
    {
        std::string state1 = m_List->GetState1(selection);
        std::string state2 = m_List->GetState2(selection);
        std::string cond = m_List->GetCond(selection);

        std::string rule = state1 +"/" + state2;
        if (!cond.empty()) rule += ":" + cond;
        rule += ";";

        selection = m_List->GetNextSelected(selection);

        toBeDeleted.insert(rule);

        m_EditorRules->DeleteRule(rule);
    }

    // update list
    std::string rules = "";
    for (int i = 0; i < m_List->GetItemCount(); i++)
    {
        std::string state1 = m_List->GetState1(i);
        std::string state2 = m_List->GetState2(i);
        std::string cond = m_List->GetCond(i);

        std::string rule = state1 + "/" + state2;
        if (!cond.empty()) rule += ":" + cond;
        rule += ";";

        if (toBeDeleted.find(rule) == toBeDeleted.end()) rules += rule + "\n";
    }

    Interpreter interpreter;
    interpreter.Process(rules);
    
    SetRules(interpreter.GetTransitions());
}

void InputRules::OnEdit(wxCommandEvent& evt)
{
    m_List->SetFocus();

    m_EditorRules->Show();
    m_EditorRules->SetFocus();
}

void InputRules::FocusSearch(wxCommandEvent& evt)
{
    m_Search->SetFocus();
}
