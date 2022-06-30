#pragma once
#include "wx/wx.h"
#include "wx/srchctrl.h"
#include "wx/colordlg.h"

#include "Ids.h"
#include "ListStates.h"
#include "ToolStates.h"
#include "Grid.h"
#include "EditorStates.h"
#include "InputRules.h"

#include <deque>

class EditorStates;
class ToolStates;
class InputRules;

class InputStates : public wxPanel
{
public:
	InputStates(wxWindow* parent);
	~InputStates();

	ListStates* GetList();
	std::unordered_map<std::string, std::string>& GetStates();

	void SetStates(std::vector<std::string> states);
	void SetToolStates(ToolStates* toolModes);
	void SetGrid(Grid* grid);
	void SetEditorStates(EditorStates* editorStates);
	void SetInputRules(InputRules* inputRules);

	Grid* GetGrid();
private:
	ListStates* m_List = nullptr;
	ToolStates* m_ToolStates = nullptr;
	Grid* m_Grid = nullptr;
	EditorStates* m_EditorStates = nullptr;
	InputRules* m_InputRules = nullptr;

	wxMenu* m_Menu = nullptr;
	wxSearchCtrl* m_Search = nullptr;

	std::deque<std::string> m_Colors;
	std::unordered_map<std::string, std::string> m_States;

	void BuildInterface();
	void InitializeColors();
	void MakeColorAvailable(std::string color);
	void MakeColorUnavailable(std::string color);

	void Search(wxCommandEvent& evt);
	void SearchEnter(wxCommandEvent& evt);

	void BuildMenu();
	void OnItemActivated(wxListEvent& evt);
	void OnItemRightClick(wxContextMenuEvent& evt);
	void OnMenuSelected(wxCommandEvent& evt);

	void StateSelect();
	void StateGoTo();
	void StateChangeColor();
	void StateErase();
	void StateDelete();

	void OnEdit(wxCommandEvent& evt);
	void FocusSearch(wxCommandEvent& evt);
};

