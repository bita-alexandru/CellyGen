#include "InputStates.h"

#include <algorithm>
#include <random>
#include <chrono>
#include <unordered_set>

InputStates::InputStates(wxWindow* parent) : wxPanel(parent)
{
    BuildInterface();

    BuildMenu();

    InitializeColors();
}

InputStates::~InputStates()
{
    wxDELETE(m_Menu);
}

ListStates* InputStates::GetList()
{
	return m_List;
}

std::unordered_map<std::string, std::string>& InputStates::GetStates()
{
	return m_States;
}

void InputStates::SetStates(std::vector<std::string> states)
{
    // states appear in our map but not in the given list -> they got recently deleted
    for (auto it = m_States.begin(); it != m_States.end();)
    {
        if (std::find(states.begin(), states.end(), it->first) == states.end())
        {
            // update color on rules list
            m_InputRules->UpdateColor(it->first, wxColor("white"));

            MakeColorAvailable(it->second);
            it = m_States.erase(it);

        }
        else it++;
    }

    // states appear in the given list but not in our map -> they got recently introduced
    for (int i = 0; i < states.size(); i++)
    {
        if (m_States.find(states[i]) == m_States.end())
        {
            m_States.insert({ states[i], m_Colors[0]});
            MakeColorUnavailable(m_Colors[0]);
        }
    }

    // update list display
    int nOfItems = m_List->GetItemCount();
    int i = 0;
    std::unordered_set<std::string> alreadyUpdated;
    for (i = 0; i < states.size(); i++)
    {
        int id = i;
        std::string state = states[i];

        if (alreadyUpdated.find(state) != alreadyUpdated.end()) continue;

        wxColour color = wxColour(m_States[states[i]]);
        // if background is black -> font should be white and viceversa - inequation found on stackoverflow
        wxColour blackwhite = (color.Red() * 0.299 + color.Green() * 0.587 + color.Blue() * 0.114) > 186.0 ? wxColour("black") : wxColour("white");

        // new items -> push them to the list
        if (i > nOfItems - 1)
        {
            m_List->PushBack({ id, state }, { color, blackwhite });
            m_List->ChangeItemState(i, state);

            m_InputRules->UpdateColor(state, color);

            continue;
        }

        // updated items -> detect changes and update the list as well

        int itmId = m_List->Get(i).first;
        std::string itmState = m_List->Get(itmId).second;
        wxColour itmColor = m_List->GetItemColor(itmId);

        if (itmId != id) m_List->ChangeItemId(i, id);
        if (itmState != state)
        {
            m_List->ChangeItemState(i, state);

            // old state deleted -> update on grid (if necessary)
            if (m_States.find(itmState) == m_States.end()) m_Grid->RemoveState(itmState);
            else m_Grid->UpdateState(itmState, itmColor, state, color);
        }
        if (itmColor != color)
        {
            m_List->ChangeItemColor(i, color, blackwhite);

            m_Grid->UpdateState(itmState, itmColor, state, color);
            m_InputRules->UpdateColor(state, color);
        }

        alreadyUpdated.insert(state);
    }

    // deleted items -> delete them from the list as well
    bool deletion = false;
    while (i < nOfItems--)
    {
        deletion = true;

        std::string state = m_List->Get(i).second;

        m_List->Erase(i);

        if (alreadyUpdated.find(state) != alreadyUpdated.end()) continue;

        // update on grid
        m_Grid->RemoveState(state, false);
    }
    if (deletion) m_Grid->RefreshUpdate();

    m_List->RefreshAfterUpdate();

    // update ToolModes (state, color) list
    std::vector<std::pair<std::string, wxColour>> statesColors;
    for (auto& it : states) statesColors.push_back({ it, wxColour(m_States[it]) });

    m_ToolStates->SetStates(statesColors);
}

void InputStates::SetToolStates(ToolStates* toolStates)
{
    m_ToolStates = toolStates;
}

void InputStates::SetGrid(Grid* grid)
{
    m_Grid = grid;
}

void InputStates::SetEditorStates(EditorStates* editorStates)
{
    m_EditorStates = editorStates;
}

void InputStates::SetInputRules(InputRules* inputRules)
{
    m_InputRules = inputRules;
}

Grid* InputStates::GetGrid()
{
    return m_Grid;
}

void InputStates::BuildInterface()
{
    wxButton* edit = new wxButton(this, Ids::ID_EDIT_STATES, wxString("Edit States"));
    edit->SetToolTip("Launch the States Editor\t(Ctrl+1)");
    edit->Bind(wxEVT_BUTTON, &InputStates::OnEdit, this);
    
    m_Search = new wxSearchCtrl(this, wxID_ANY);
    m_Search->SetToolTip("Search for a state...\t(Ctrl+Shift+1)");
    m_Search->Bind(wxEVT_TEXT, &InputStates::Search, this);
    m_Search->Bind(wxEVT_SEARCHCTRL_SEARCH_BTN, &InputStates::SearchEnter, this);

    wxButton* focusSearch = new wxButton(this, Ids::ID_SEARCH_STATES);
    focusSearch->Hide();
    focusSearch->Bind(wxEVT_BUTTON, &InputStates::FocusSearch, this);

    m_List = new ListStates(this);
    m_List->PushBack({ 0, "FREE" }, { wxColour("white"), wxColour("black") });
    m_List->RefreshAfterUpdate();

    m_List->Bind(wxEVT_CONTEXT_MENU, &InputStates::OnItemRightClick, this);
    m_List->Bind(wxEVT_LIST_ITEM_ACTIVATED, &InputStates::OnItemActivated, this);

    wxStaticBoxSizer* sizer = new wxStaticBoxSizer(wxVERTICAL, this, "States");
    sizer->Add(edit, 0, wxEXPAND);
    sizer->Add(m_Search, 0, wxEXPAND);
    sizer->Add(m_List, 1, wxEXPAND);

    SetSizer(sizer);
}

void InputStates::InitializeColors()
{
    // 256 colors; should be as distinct as virtually possible - found on stackoverflow
    m_Colors = std::deque<std::string>({
        "#000000", "#FFFF00", "#1CE6FF", "#FF34FF", "#FF4A46", "#008941", "#006FA6", "#A30059",
        "#FFDBE5", "#7A4900", "#0000A6", "#63FFAC", "#B79762", "#004D43", "#8FB0FF", "#997D87",
        "#5A0007", "#809693", "#FEFFE6", "#1B4400", "#4FC601", "#3B5DFF", "#4A3B53", "#FF2F80",
        "#61615A", "#BA0900", "#6B7900", "#00C2A0", "#FFAA92", "#FF90C9", "#B903AA", "#D16100",
        "#DDEFFF", "#000035", "#7B4F4B", "#A1C299", "#300018", "#0AA6D8", "#013349", "#00846F",
        "#372101", "#FFB500", "#C2FFED", "#A079BF", "#CC0744", "#C0B9B2", "#C2FF99", "#001E09",
        "#00489C", "#6F0062", "#0CBD66", "#EEC3FF", "#456D75", "#B77B68", "#7A87A1", "#788D66",
        "#885578", "#FAD09F", "#FF8A9A", "#D157A0", "#BEC459", "#456648", "#0086ED", "#886F4C",
        "#34362D", "#B4A8BD", "#00A6AA", "#452C2C", "#636375", "#A3C8C9", "#FF913F", "#938A81",
        "#575329", "#00FECF", "#B05B6F", "#8CD0FF", "#3B9700", "#04F757", "#C8A1A1", "#1E6E00",
        "#7900D7", "#A77500", "#6367A9", "#A05837", "#6B002C", "#772600", "#D790FF", "#9B9700",
        "#549E79", "#FFF69F", "#201625", "#72418F", "#BC23FF", "#99ADC0", "#3A2465", "#922329",
        "#5B4534", "#FDE8DC", "#404E55", "#0089A3", "#CB7E98", "#A4E804", "#324E72", "#6A3A4C",
        "#83AB58", "#001C1E", "#D1F7CE", "#004B28", "#C8D0F6", "#A3A489", "#806C66", "#222800",
        "#BF5650", "#E83000", "#66796D", "#DA007C", "#FF1A59", "#8ADBB4", "#1E0200", "#5B4E51",
        "#C895C5", "#320033", "#FF6832", "#66E1D3", "#CFCDAC", "#D0AC94", "#7ED379", "#012C58",
        "#7A7BFF", "#D68E01", "#353339", "#78AFA1", "#FEB2C6", "#75797C", "#837393", "#943A4D",
        "#B5F4FF", "#D2DCD5", "#9556BD", "#6A714A", "#001325", "#02525F", "#0AA3F7", "#E98176",
        "#DBD5DD", "#5EBCD1", "#3D4F44", "#7E6405", "#02684E", "#962B75", "#8D8546", "#9695C5",
        "#E773CE", "#D86A78", "#3E89BE", "#CA834E", "#518A87", "#5B113C", "#55813B", "#E704C4",
        "#00005F", "#A97399", "#4B8160", "#59738A", "#FF5DA7", "#F7C9BF", "#643127", "#513A01",
        "#6B94AA", "#51A058", "#A45B02", "#1D1702", "#E20027", "#E7AB63", "#4C6001", "#9C6966",
        "#64547B", "#97979E", "#006A66", "#391406", "#F4D749", "#0045D2", "#006C31", "#DDB6D0",
        "#7C6571", "#9FB2A4", "#00D891", "#15A08A", "#BC65E9", "#000000", "#C6DC99", "#203B3C",
        "#671190", "#6B3A64", "#F5E1FF", "#FFA0F2", "#CCAA35", "#374527", "#8BB400", "#797868",
        "#C6005A", "#3B000A", "#C86240", "#29607C", "#402334", "#7D5A44", "#CCB87C", "#B88183",
        "#AA5199", "#B5D6C3", "#A38469", "#9F94F0", "#A74571", "#B894A6", "#71BB8C", "#00B433",
        "#789EC9", "#6D80BA", "#953F00", "#5EFF03", "#E4FFFC", "#1BE177", "#BCB1E5", "#76912F",
        "#003109", "#0060CD", "#D20096", "#895563", "#29201D", "#5B3213", "#A76F42", "#89412E",
        "#1A3A2A", "#494B5A", "#A88C85", "#F4ABAA", "#A3F3AB", "#00C6C8", "#EA8B66", "#958A9F",
        "#BDC9D2", "#9FA064", "#BE4700", "#658188", "#83A485", "#453C23", "#47675D", "#3A3F00",
        "#DFFB71", "#868E7E", "#98D058", "#6C8F7D", "#D7BFC2", "#3C3E6E", "#D83D66"
        });

    // shuffle them for color variation inbetween uses
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    shuffle(m_Colors.begin(), m_Colors.end(), std::default_random_engine(seed));
    m_Colors.push_back("#FFFFFF");

    m_States.insert({ "FREE", "#FFFFFF" });
}

void InputStates::MakeColorAvailable(std::string color)
{
    for (auto it = m_Colors.rbegin(); it != m_Colors.rend(); it++)
    {
        if (*it == color)
        {
            m_Colors.erase((++it).base());
            m_Colors.push_front(color);
            return;
        }
    }
}

void InputStates::MakeColorUnavailable(std::string color)
{
    m_Colors.pop_front();
    m_Colors.push_back(color);
}

void InputStates::Search(wxCommandEvent& evt)
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
        std::string state = m_List->Get(i).second;

        if (state.find(query) != state.npos)
        {
            m_List->Select(i);
            m_List->EnsureVisible(i);

            return;
        }
    }
}

void InputStates::SearchEnter(wxCommandEvent& evt)
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
            std::string state = m_List->Get(i).second;

            if (state.find(query) != state.npos)
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
        std::string state = m_List->Get(i).second;

        if (state.find(query) != state.npos)
        {
            m_List->Select(selection, false);
            m_List->Select(i);
            m_List->EnsureVisible(i);

            return;
        }
    }
}

void InputStates::BuildMenu()
{
    m_Menu = new wxMenu();

    m_Menu->Append(Ids::ID_SELECT_STATE, "Select");
    m_Menu->AppendSeparator();
    m_Menu->Append(Ids::ID_GOTO_STATE, "Go To");
    m_Menu->Append(Ids::ID_COLOR_STATE, "Change Color");
    m_Menu->AppendSeparator();
    m_Menu->Append(Ids::ID_ERASE_STATE, "Erase");
    m_Menu->Append(Ids::ID_DELETE_STATE, "Delete");

    m_Menu->Bind(wxEVT_COMMAND_MENU_SELECTED, &InputStates::OnMenuSelected, this);
}

void InputStates::OnItemActivated(wxListEvent& evt)
{
    if (m_List->GetSelectedItemCount() != 1) return;

    StateSelect();
}

void InputStates::OnItemRightClick(wxContextMenuEvent& evt)
{
    int itemCount = m_List->GetSelectedItemCount();
    if (itemCount != 1)
    {
        m_Menu->Enable(Ids::ID_SELECT_STATE, false);
        m_Menu->Enable(Ids::ID_GOTO_STATE, false);
        m_Menu->Enable(Ids::ID_COLOR_STATE, false);
        
        if (itemCount) m_Menu->Enable(Ids::ID_DELETE_STATE, true);
        else m_Menu->Enable(Ids::ID_DELETE_STATE, false);
    }
    else
    {
        m_Menu->Enable(Ids::ID_SELECT_STATE, true);
        m_Menu->Enable(Ids::ID_GOTO_STATE, true);
        m_Menu->Enable(Ids::ID_COLOR_STATE, true);
        m_Menu->Enable(Ids::ID_DELETE_STATE, true);

        // state "FREE" -> invalidate any action
        if (m_List->GetFirstSelected() == 0)
        {
            m_Menu->Enable(Ids::ID_GOTO_STATE, false);
            m_Menu->Enable(Ids::ID_COLOR_STATE, false);
            m_Menu->Enable(Ids::ID_DELETE_STATE, false);
        }
    }

    PopupMenu(m_Menu);
}

void InputStates::OnMenuSelected(wxCommandEvent& evt)
{
    switch (evt.GetId())
    {
    case Ids::ID_SELECT_STATE:
        StateSelect();
        break;
    case Ids::ID_GOTO_STATE:
        StateGoTo();
        break;
    case Ids::ID_COLOR_STATE:
        StateChangeColor();
        break;
    case Ids::ID_ERASE_STATE:
        StateErase();
        break;
    case Ids::ID_DELETE_STATE:
        StateDelete();
        break;
    default:
        break;
    }
}

void InputStates::StateSelect()
{
    m_ToolStates->SetIndex(m_List->GetFirstSelected());
}

void InputStates::StateGoTo()
{
    int selection = m_List->GetFirstSelected();
    std::string state = m_List->Get(selection).second;

    m_EditorStates->GoTo(state);
}

void InputStates::StateChangeColor()
{
    int selection = m_List->GetFirstSelected();

    std::string state = m_List->Get(selection).second;
    std::string color = m_States[state];

    int availableColors = m_Colors.size() - m_States.size();

    // if no color available then do nothing
    if (!availableColors)
    {
        m_List->Select(selection, false);
        return;
    }
    
    // get random available color
    std::srand(std::time(NULL));
    int randInt = std::rand() % availableColors;
    std::string newColor = m_Colors[randInt];

    // assign new color to selected state
    m_States[state] = newColor;
    MakeColorUnavailable(newColor);
    MakeColorAvailable(color);

    // update color on states list
    m_List->SetItemColor(selection, wxColour(newColor));
    m_List->RefreshAfterUpdate();

    // update color on rules list
    m_InputRules->UpdateColor(state, wxColor(newColor));

    // update color on grid
    m_Grid->UpdateState(state, wxColour(color), state, wxColour(newColor));

    // update color on the states tool
    m_ToolStates->SetStateColor(selection, wxColour(newColor));

    m_List->Select(selection, false);
}

void InputStates::StateErase()
{
    int selection = m_List->GetFirstSelected();

    if (selection == -1) return;

    if (m_Grid->GetGenerating() || !m_Grid->GetPaused())
    {
        wxMessageBox("Can't erase states while the simulation is playing. Try pausing it first.", "Error", wxICON_WARNING);
        return;
    }

    // erase cells of the selected states from the grid
    while (selection != -1)
    {
        std::string state = m_List->Get(selection).second;
        selection = m_List->GetNextSelected(selection);

        if (state == "FREE") continue;

        m_Grid->RemoveState(state, false);
    }

    m_Grid->RefreshUpdate();
}

void InputStates::StateDelete()
{
    int selection = m_List->GetFirstSelected();

    if (selection == -1) return;

    if (m_Grid->GetGenerating() || !m_Grid->GetPaused())
    {
        wxMessageBox("Can't delete states while the simulation is playing. Try pausing it first.", "Error", wxICON_WARNING);
        return;
    }

    std::unordered_set<std::string> toBeDeleted;
    // delete selected states
    while (selection != -1)
    {
        std::string state = m_List->Get(selection).second;
        selection = m_List->GetNextSelected(selection);

        if (state == "FREE") continue;

        toBeDeleted.insert(state);

        m_EditorStates->DeleteState(state);
    }

    // update list
    std::vector<std::string> states;
    for (int i = 0; i < m_List->GetItemCount(); i++)
    {
        std::string state = m_List->Get(i).second;

        if (toBeDeleted.find(state) == toBeDeleted.end()) states.push_back(state);
    }

    SetStates(states);
}

void InputStates::OnEdit(wxCommandEvent& evt)
{
    m_List->SetFocus();

    m_EditorStates->Show();
    m_EditorStates->SetFocus();
}

void InputStates::FocusSearch(wxCommandEvent& evt)
{
    m_Search->SetFocus();
}
