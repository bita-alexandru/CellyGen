#pragma once
#include "wx/wx.h"
#include "wx/srchctrl.h"

#include "Ids.h"
#include "ListRules.h"
#include "EditorRules.h"
#include "InputStates.h"
#include "InputNeighbors.h"
#include "Transition.h"

class EditorRules;
class InputStates;
class InputNeighbors;

class InputRules : public wxPanel
{
public:
	InputRules(wxWindow* parent);
	~InputRules();

	ListRules* GetList();
	std::vector<std::pair<std::string, Transition>>& GetRules();

	InputStates* GetInputStates();
	InputNeighbors* GetInputNeighbors();
	
	void SetEditorRules(EditorRules* editorRules);
	void SetInputStates(InputStates* inputStates);
	void SetInputNeighbors(InputNeighbors* inputNeighbors);

	void SetRules(std::vector<std::pair<std::string, Transition>> rules);
	void UpdateColor(std::string state, wxColour color);
private:
	ListRules* m_List = nullptr;
	EditorRules* m_EditorRules = nullptr;
	InputStates* m_InputStates = nullptr;
	InputNeighbors* m_InputNeighbors = nullptr;

	wxMenu* m_Menu = nullptr;
	wxSearchCtrl* m_Search = nullptr;

	std::vector<std::pair<std::string, Transition>> m_Rules;
	std::unordered_map<std::string, std::string> m_States;

	void BuildInterface();

	void Search(wxCommandEvent& evt);
	void SearchEnter(wxCommandEvent& evt);

	void BuildMenu();
	void OnItemRightClick(wxContextMenuEvent& evt);
	void OnMenuSelected(wxCommandEvent& evt);

	void RuleGoTo();
	void RuleDelete();

	void OnEdit(wxCommandEvent& evt);
	void FocusSearch(wxCommandEvent& evt);
};
