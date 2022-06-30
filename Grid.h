#pragma once
#include "wx/wx.h"
#include "wx/vscroll.h"
#include "wx/dcclient.h"
#include "wx/dcmemory.h"
#include "wx/dcbuffer.h"

#include <unordered_set>
#include <mutex>

#include "Ids.h"
#include "Sizes.h"
#include "Hashes.h"
#include "ToolZoom.h"
#include "ToolUndo.h"
#include "ToolModes.h"
#include "ToolStates.h"
#include "ToolCoords.h"
#include "StatusCells.h"
#include "StatusControls.h"
#include "StatusDelay.h"
#include "InputRules.h"
#include "Transition.h"

class ToolZoom;
class ToolUndo;
class ToolStates;
class ToolModes;
class InputRules;
class StatusControls;
class StatusDelay;

class Grid : public wxHVScrolledWindow
{
public:
	Grid(wxWindow* parent);
	~Grid();

	int GetSize();
	void SetSize(int size, bool center = true);
	void ScrollToCenter(int x = Sizes::N_COLS / 2, int y = Sizes::N_ROWS / 2);
	void SetDimensions(int rows, int cols);

	void SetCells(
		std::unordered_map<std::pair<int, int>, std::pair<std::string, wxColour>, Hashes::PairInt> cells,
		std::unordered_map<std::string, std::unordered_set<std::pair<int, int>, Hashes::PairInt>> statePositions
	);

	void SetInputRules(InputRules* inputRules);
	void SetToolZoom(ToolZoom* toolZoom);
	void SetToolUndo(ToolUndo* toolUndo);
	void SetToolModes(ToolModes* toolModes);
	void SetToolStates(ToolStates* toolStates);
	void SetToolCoords(ToolCoords* toolCoords);
	void SetStatusControls(StatusControls* statusControls);
	void SetStatusCells(StatusCells* statusCells);
	void SetStatusDelay(StatusDelay* statusDelay);

	bool InsertCell(int x, int y, std::string state, wxColour color, bool multiple = false);
	bool RemoveCell(int x, int y, std::string state, wxColour color, bool multiple = false);
	void RemoveState(std::string state, bool update = true);
	void UpdateState(std::string oldState, wxColour oldColor, std::string newState, wxColour newColor);
	bool EraseCell(int x, int y, bool multiple = false);
	std::string GetState(int x, int y);

	void RefreshUpdate();
	void UpdatePrev();
	std::unordered_map<std::pair<int, int>, std::pair<std::string, wxColour>, Hashes::PairInt> GetCells();
	std::unordered_map<std::string, std::unordered_set<std::pair<int, int>, Hashes::PairInt>> GetStatePositions();
	std::unordered_map<std::pair<int, int>, std::pair<std::string, wxColour>, Hashes::PairInt> GetPrevCells();
	std::unordered_map<std::string, std::unordered_set<std::pair<int, int>, Hashes::PairInt>> GetPrevStatePositions();
	std::unordered_map<std::string, wxColour>& GetColors();

	void Reset(bool refresh = true);
	void PlayUniverse();
	void PauseUniverse();
	void NextGeneration();
	void OnNextGeneration();
	void OnPlayUniverse();
	void OnPopulate(double probability);

	int GetPaused();
	int GetFinished();
	int GetGenerating();

	void ResetUniverse();
	void WaitForPause();

	void DecrementGenerationCount();
	void ResetGenerationCount();
private:
	InputRules* m_InputRules = nullptr;
	ToolZoom* m_ToolZoom = nullptr;
	ToolUndo* m_ToolUndo = nullptr;
	ToolModes* m_ToolModes = nullptr;
	ToolStates* m_ToolStates = nullptr;
	ToolCoords* m_ToolCoords = nullptr;
	StatusControls* m_StatusControls = nullptr;
	StatusCells* m_StatusCells = nullptr;
	StatusDelay* m_StatusDelay = nullptr;

	int m_Size = Sizes::CELL_SIZE_DEFAULT;
	int m_OffsetX = Sizes::N_COLS / 2;
	int m_OffsetY = Sizes::N_ROWS / 2;
	bool m_Centered = false;

	bool m_Paused = true;
	bool m_Finished = false;
	bool m_Generating = false;
	bool m_ForceClose = false;

	std::unordered_map<std::pair<int, int>, std::pair<std::string, wxColour>, Hashes::PairInt> m_Cells;
	std::unordered_map<std::string, std::unordered_set<std::pair<int, int>, Hashes::PairInt>> m_StatePositions;
	std::unordered_map<std::pair<int, int>, std::pair<std::string, wxColour>, Hashes::PairInt> m_PrevCells;
	std::unordered_map<std::string, std::unordered_set<std::pair<int, int>, Hashes::PairInt>> m_PrevStatePositions;

	wxTimer* m_TimerSelection = nullptr;

	bool m_PrevScrolledCol = false;
	bool m_PrevScrolledRow = false;
	bool m_PrevUpdated = false;
	bool m_AlreadyDrawn = false;
	bool m_IsDrawing = false;
	bool m_IsErasing = false;
	bool m_IsMoving = false;

	std::pair<int, int> m_PrevCell;
	std::pair<int, int> m_LastDrawn;
	std::pair<int, int> m_JustScrolled = { 0,0 };
	std::pair<int, int> m_LastHovered = { -1,-1 };

	std::unordered_set<int> m_Keys;

	bool m_RedrawAll = true;
	bool m_JustResized = false;

	std::pair<int, int> m_RedrawXY;
	std::vector<std::pair<int, int>> m_RedrawXYs;
	std::vector<wxColour> m_RedrawColors;
	wxColour m_RedrawColor;

	std::mutex m_MutexCells;
	std::mutex m_MutexStatus;

	virtual wxCoord OnGetRowHeight(size_t row) const;
	virtual wxCoord OnGetColumnWidth(size_t row) const;
	void OnPaint(wxPaintEvent& evt);

	void BuildInterface();
	void InitializeTimers();

	std::pair<int, int> GetHoveredCell(int X, int Y);
	bool ControlSelectState();
	bool ControlZoom(int x, int y, int rotation);
	std::string ControlUpdateCoords(int x, int y);
	bool ModeDraw(int x, int y, char mode);
	bool ModePick(int x, int y, char mode, std::string state);
	bool ModeMove(int x, int y, char mode);
	void ProcessKeys();

	wxDECLARE_EVENT_TABLE();
	void OnDraw(wxDC& dc);
	void OnMouse(wxMouseEvent& evt);
	void OnTimerSelection(wxTimerEvent& evt);
	void OnKeyDown(wxKeyEvent& evt);
	void OnKeyUp(wxKeyEvent& evt);
	void OnEraseBackground(wxEraseEvent& evt);
	void OnScroll(wxScrollWinEvent& evt);
	void OnSize(wxSizeEvent& evt);

	void DeleteStructure(int X, int Y, std::string state = "");
	void DrawStructure(int X, int Y, std::string state, wxColour color);
	void DrawLine(int x, int y, std::string state, wxColour color, bool remove = false);
	bool InBounds(int x, int y);
	bool InVisibleBounds(int x, int y);

	std::pair<std::vector<std::pair<int, int>>, std::string> ParseRule(std::pair<std::string, Transition>& rule, std::unordered_set<int>& visited);
	std::pair<std::vector<std::pair<std::string, std::pair<int, int>>>, std::string> ParseAllRules();
	bool ApplyOnCell(int x, int y, Transition& rule, std::unordered_set<std::string>& neighbors);
	std::unordered_map<std::string, std::string> GetNeighborhood(std::pair<int, int> xy, std::unordered_set<std::string>& neighbors);
	void UpdateGeneration(std::vector<std::pair<std::string, std::pair<int, int>>> changes);

	void UpdateCoordsHovered();
};