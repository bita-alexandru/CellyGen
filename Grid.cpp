#include "Grid.h"

#include "wx/richmsgdlg.h"

#include <stack>
#include <thread>
#include <chrono>
#include <random>
#include <algorithm>

wxBEGIN_EVENT_TABLE(Grid, wxHVScrolledWindow)
EVT_PAINT(Grid::OnPaint)
EVT_MOUSE_EVENTS(Grid::OnMouse)
EVT_TIMER(Ids::ID_TIMER_SELECTION, Grid::OnTimerSelection)
EVT_KEY_DOWN(Grid::OnKeyDown)
EVT_KEY_UP(Grid::OnKeyUp)
EVT_ERASE_BACKGROUND(Grid::OnEraseBackground)
EVT_SCROLLWIN(Grid::OnScroll)
EVT_SIZE(Grid::OnSize)
wxEND_EVENT_TABLE()


Grid::Grid(wxWindow* parent) : wxHVScrolledWindow(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxWANTS_CHARS | wxBORDER_SIMPLE)
{
	BuildInterface();

	InitializeTimers();
}

Grid::~Grid()
{
	wxDELETE(m_TimerSelection);
}

int Grid::GetSize()
{
	return m_Size;
}

void Grid::SetSize(int size, bool center)
{
	// resize the grid cells and center view on the middle cell (if center=true)

	m_Size = size;

	// still drawing
	if (m_IsDrawing || m_IsErasing)
	{
		m_IsDrawing = false;
		m_IsErasing = false;



		if (m_Cells != m_PrevCells)
		{
			m_ToolUndo->PushBack(m_Cells, m_StatePositions, m_PrevCells, m_PrevStatePositions);
			m_PrevCells = m_Cells;
			m_PrevStatePositions = m_StatePositions;
		}
	}

	m_JustResized = true;

	if (!center)
	{
		wxVarHScrollHelper::RefreshAll();
		wxVarVScrollHelper::RefreshAll();
		return;
	}

	// get (x,y) located at the center of the screen
	wxPosition visibleBegin = GetVisibleBegin();
	wxPosition visibleEnd = GetVisibleEnd();

	int x = visibleEnd.GetCol() - (visibleEnd.GetCol() - visibleBegin.GetCol()) / 2;
	int y = visibleEnd.GetRow() - (visibleEnd.GetRow() - visibleBegin.GetRow()) / 2;

	wxVarHScrollHelper::RefreshAll();
	wxVarVScrollHelper::RefreshAll();

	ScrollToCenter(x, y);
}

void Grid::ScrollToCenter(int x, int y)
{
	// make sure (x,y) will be located at the center of the screen after zooming
	wxPosition visibleBegin = GetVisibleBegin();
	wxPosition visibleEnd = GetVisibleEnd();

	int row = y;
	int column = x;

	// calculate the middle cell's position
	row = row - (visibleEnd.GetRow() - visibleBegin.GetRow()) / 2;
	column = column - (visibleEnd.GetCol() - visibleBegin.GetCol()) / 2;

	// make sure it doesn't go past the borders
	row = std::max(0, row); row = std::min(Sizes::N_ROWS - 1, row);
	column = std::max(0, column); column = std::min(Sizes::N_COLS - 1, column);

	// even length, current positions differ by one unit -> don't reposition (avoid flickering)
	if ((visibleEnd.GetCol() - visibleBegin.GetCol()) % 2 == 0)
	{
		if (std::abs(column - visibleBegin.GetCol()) == 1) column = visibleBegin.GetCol();
		if (std::abs(row - visibleBegin.GetRow()) == 1) row = visibleBegin.GetRow();
	}

	ScrollToRowColumn(row, column);

	m_RedrawAll = true;

	Refresh(false);
	Update();
}

void Grid::SetDimensions(int rows, int cols)
{
	if (rows == Sizes::N_ROWS && cols == Sizes::N_COLS) return;

	if (m_Generating || !m_Paused)
	{
		wxMessageBox("Can't change grid dimensions while the simulation is playing. Try pausing it first.", "Error", wxICON_WARNING);
		return;
	}

	ResetUniverse();

	Sizes::N_ROWS = rows;
	Sizes::N_COLS = cols;

	m_OffsetX = Sizes::N_COLS / 2;
	m_OffsetY = Sizes::N_ROWS / 2;

	SetRowColumnCount(Sizes::N_ROWS, Sizes::N_COLS);

	Reset();
	ScrollToCenter();
}

void Grid::SetCells(
	std::unordered_map<std::pair<int, int>, std::pair<std::string, wxColour>, Hashes::PairInt> cells,
	std::unordered_map<std::string, std::unordered_set<std::pair<int, int>, Hashes::PairInt>> statePositions
)
{
	// which cells should be erased?
	for (auto& it : m_PrevCells)
	{
		m_RedrawAll = false;

		if (InVisibleBounds(it.first.first, it.first.second))
		{
			m_RedrawXYs.push_back({ it.first.first,it.first.second });
			m_RedrawColors.push_back(wxColour("white"));
		}
	}

	std::unordered_map<std::string, wxColor> colors = GetColors();

	// update color of cells (if modified before an undo/redo operation)
	m_Cells = std::unordered_map<std::pair<int, int>, std::pair<std::string, wxColour>, Hashes::PairInt>();
	for (auto& cell : cells) m_Cells.insert({ cell.first, {cell.second.first, colors[cell.second.first]} });

	m_StatePositions = statePositions;
	m_PrevCells = m_Cells;
	m_PrevStatePositions = m_StatePositions;

	// which cells should be redrawn?
	for (auto& it : m_Cells)
	{
		m_RedrawAll = false;
		if (InVisibleBounds(it.first.first, it.first.second))
		{
			m_RedrawXYs.push_back({ it.first.first,it.first.second });
			m_RedrawColors.push_back(it.second.second);
		}
	}

	m_StatusCells->SetCountPopulation(m_Cells.size());

	Refresh(false);
	Update();
}

void Grid::SetInputRules(InputRules* inputRules)
{
	m_InputRules = inputRules;
}

void Grid::SetToolZoom(ToolZoom* toolZoom)
{
	m_ToolZoom = toolZoom;
}

void Grid::SetToolUndo(ToolUndo* toolUndo)
{
	m_ToolUndo = toolUndo;
}

void Grid::SetToolModes(ToolModes* toolModes)
{
	m_ToolModes = toolModes;
}

void Grid::SetToolStates(ToolStates* toolStates)
{
	m_ToolStates = toolStates;
}

void Grid::SetToolCoords(ToolCoords* toolCoords)
{
	m_ToolCoords = toolCoords;
}

void Grid::SetStatusControls(StatusControls* statusControls)
{
	m_StatusControls = statusControls;
}

void Grid::SetStatusCells(StatusCells* statusCells)
{
	m_StatusCells = statusCells;
}

void Grid::SetStatusDelay(StatusDelay* statusDelay)
{
	m_StatusDelay = statusDelay;
}

bool Grid::InsertCell(int x, int y, std::string state, wxColour color, bool multiple)
{
	if (GetState(x, y) != "FREE")
	{
		// current cell is of state "FREE" but there's already a cell of another state
		// on this exact position -> remove it
		if (color == wxColour("white")) return RemoveCell(x, y, state, color, multiple);
		// position is occupied
		else
		{
			// same state -> don't do anything
			if (m_Cells[{x, y}].first == state) return false;

			EraseCell(x, y);

			m_Cells[{x, y}] = { state, color };
			m_StatePositions[state].insert({ x,y });

			if (multiple)
			{
				// draw the cells at the same time when finished

				m_RedrawAll = false;

				if (InVisibleBounds(x, y))
				{
					m_RedrawXYs.push_back({ x,y });
					m_RedrawColors.push_back(color);
				}
			}
			else
			{
				m_RedrawAll = false;
				m_RedrawXY = { x,y };
				m_RedrawColor = color;

				Refresh(false);
				Update();
			}

			return true;
		}
	}
	// position is available
	else if (color != wxColour("white"))
	{
		m_Cells[{x, y}] = { state, color };
		m_StatePositions[state].insert({ x,y });

		if (multiple)
		{
			// draw the cells at the same time when finished

			m_RedrawAll = false;

			if (InVisibleBounds(x, y))
			{
				m_RedrawXYs.push_back({ x,y });
				m_RedrawColors.push_back(color);
			}
		}
		else
		{
			m_StatusCells->UpdateCountPopulation(1);

			m_RedrawAll = false;
			m_RedrawXY = { x,y };
			m_RedrawColor = color;

			Refresh(false);
			Update();
		}

		return true;
	}

	return false;
}

bool Grid::RemoveCell(int x, int y, std::string state, wxColour color, bool multiple)
{
	if (GetState(x, y) != "FREE")
	{
		EraseCell(x, y, multiple);

		// do we want to update the grid now or update it manually after
		// all operations of this type are completed?
		if (!multiple)
		{
			m_StatusCells->UpdateCountPopulation(-1);

			Refresh(false);
			Update();
		}

		return true;
	}

	return false;
}

void Grid::RemoveState(std::string state, bool update)
{
	// cells of this state have been placed on the grid
	if (m_StatePositions.find(state) != m_StatePositions.end())
	{
		// remove every cell of this state from our map
		for (auto& it : m_StatePositions[state])
		{
			// remove from m_Cells, one by one
			m_Cells.erase(it);

			m_RedrawAll = false;
			if (InVisibleBounds(it.first, it.second))
			{
				m_RedrawXYs.push_back({ it.first,it.second });
				m_RedrawColors.push_back(wxColour("white"));
			}
		}

		// do we want to update the grid now or update it manually after
		// all operations of this type are completed?
		if (update)
		{
			Refresh(false);
			Update();

			m_StatusCells->SetCountPopulation(m_Cells.size());
		}

		// remove the corresponding map
		m_StatePositions[state].clear();
		m_StatePositions.erase(state);

		m_ToolUndo->Reset();

		m_PrevCells = m_Cells;
		m_PrevStatePositions = m_StatePositions;
	}
}

void Grid::UpdateState(std::string oldState, wxColour oldColor, std::string newState, wxColour newColor)
{
	// cells of this state need their names & colors updated
	if (m_StatePositions.find(oldState) != m_StatePositions.end())
	{
		// same state but a new color -> update it
		if (oldState == newState)
		{
			for (auto& it : m_StatePositions[oldState])
			{
				m_Cells[it].second = newColor;

				// do they need to be redrawn now?
				m_RedrawAll = false;
				if (InVisibleBounds(it.first, it.second))
				{
					m_RedrawXYs.push_back({ it.first,it.second });
					m_RedrawColors.push_back(newColor);
				}
			}

			Refresh(false);
			Update();
		}
		// same color but a new state -> update it
		else if (oldColor == newColor)
		{
			for (auto& it : m_StatePositions[oldState])
			{
				m_Cells[it].first = newState;
				m_StatePositions[newState].insert(it);
			}

			m_StatePositions.erase(oldState);

			m_ToolUndo->Reset();

			m_PrevCells = m_Cells;
			m_PrevStatePositions = m_StatePositions;
		}
	}
}

bool Grid::EraseCell(int x, int y, bool multiple)
{
	std::string state = m_Cells[{x, y}].first;

	m_Cells.erase({ x,y });
	m_StatePositions[state].erase({ x,y });

	// there are no more cells of this state anymore -> remove it from our map
	if (m_StatePositions[state].size() == 0)
	{
		m_StatePositions.erase(state);
	}

	m_RedrawAll = false;

	if (multiple)
	{
		// erase these cells at the same time when finished

		if (InVisibleBounds(x, y))
		{
			m_RedrawXYs.push_back({ x,y });
			m_RedrawColors.push_back(wxColour("white"));
		}
	}
	else
	{
		m_RedrawXY = { x,y };
		m_RedrawColor = wxColour("white");
	}

	return true;
}

std::string Grid::GetState(int x, int y)
{
	if (m_Cells.find({ x,y }) == m_Cells.end()) return "FREE";

	return m_Cells[{x, y}].first;
}

void Grid::Reset(bool refresh)
{
	if (m_Generating || !m_Paused)
	{
		wxMessageBox("Can't reset grid while the simulation is playing. Try pausing it first.", "Error", wxICON_WARNING);
		return;
	}

	m_Cells = std::unordered_map<std::pair<int, int>, std::pair<std::string, wxColour>, Hashes::PairInt>();
	m_StatePositions = std::unordered_map<std::string, std::unordered_set<std::pair<int, int>, Hashes::PairInt>>();
	m_PrevCells = m_Cells;
	m_PrevStatePositions = m_StatePositions;

	m_RedrawAll = true;
	m_JustResized = false;
	m_JustScrolled = { 0,0 };
	m_LastDrawn = { -1,-1 };
	m_LastHovered = { -1,-1 };

	m_IsDrawing = false;
	m_IsErasing = false;
	m_IsMoving = false;

	m_ToolUndo->Reset();
	m_StatusCells->SetCountPopulation(0);

	ResetUniverse();

	// update the grid display now?
	if (refresh)
	{
		Refresh(false);
		Update();
	}
}

void Grid::PlayUniverse()
{
	if (m_Finished) ResetUniverse();

	m_Paused = false;
	m_Finished = false;

	while (!m_Finished && !m_Paused && !m_ForceClose)
	{
		m_Generating = true;
		NextGeneration();
	}
}

void Grid::PauseUniverse()
{
	m_Generating = false;
	m_Paused = true;
}

void Grid::NextGeneration()
{
	if (m_ForceClose) return;

	if (m_Finished)
	{
		m_Generating = false;
		return;
	}

	std::pair<std::vector<std::pair<std::string, std::pair<int, int>>>, std::string> result = ParseAllRules();

	// error
	if (result.second.size())
	{
		m_StatusControls->SetPlayButton(true);

		std::string message = result.second;

		wxRichMessageDialog dialog(
			this, "Some of the rules appear to be invalid.", "Error",
			wxOK | wxICON_ERROR
		);
		dialog.ShowDetailedText(message);

		dialog.ShowModal();

		m_Finished = false;
		m_Paused = true;
		m_Generating = false;

		return;
	}

	if (m_ForceClose) return;

	UpdateGeneration(result.first);
	UpdateCoordsHovered();

	if (m_Cells != m_PrevCells)
	{
		m_ToolUndo->PushBack(m_Cells, m_StatePositions, m_PrevCells, m_PrevStatePositions);
		m_PrevCells = m_Cells;
		m_PrevStatePositions = m_StatePositions;
	}

	// universe has come to an end
	if (result.first.empty())
	{
		m_Finished = true;
		m_Paused = true;

		m_StatusControls->SetPlayButton(1);
		m_StatusCells->SetGenerationMessage(" [OVER]");
	}
	// continue with the next generation
	else
	{
		m_StatusCells->UpdateCountGeneration(+1);
		m_StatusCells->SetCountPopulation(m_Cells.size());

		std::this_thread::sleep_for(std::chrono::milliseconds(m_StatusDelay->GetDelay()));
	}

	m_Generating = false;
}

void Grid::OnNextGeneration()
{
	if (!m_Generating)
	{
		if (m_Finished) ResetUniverse();

		m_Generating = true;
		m_Finished = false;

		std::thread t(&Grid::NextGeneration, this);
		t.detach();
	}
}

void Grid::OnPlayUniverse()
{
	std::thread t(&Grid::PlayUniverse, this);
	t.detach();
}

void Grid::OnPopulate(double probability)
{
	// randomly populate the grid

	if (!m_Generating)
	{
		Reset();

		vector<pair<string, wxColour>> states;
		for (auto& it : m_ToolStates->GetColors())
		{
			states.push_back(it);
		}

		const int statesSize = states.size();
		if (statesSize > 1)
		{
			vector<pair<pair<int, int>, pair<string, wxColour>>> population;

			const int n = Sizes::N_ROWS;
			const int m = Sizes::N_COLS;

			srand(time(NULL));

			// iterate through all of the cells of the grid
			for (int i = 0; i < n; i++)
			{
				for (int j = 0; j < m; j++)
				{
					double p = (rand() % 10) * 0.1;

					if (p <= probability)
					{
						// assign a random state at this position

						int k = rand() % statesSize;

						population.push_back({ {i,j}, states[k] });
					}
				}
			}

			if (population.size())
			{
				// update the grid

				m_Cells.clear();
				m_StatePositions.clear();

				for (auto& cell : population)
				{
					int y = cell.first.first;
					int x = cell.first.second;

					string state = cell.second.first;
					wxColour color = cell.second.second;

					InsertCell(x, y, state, color, true);
				}

				if (m_Cells != m_PrevCells)
				{
					m_ToolUndo->PushBack(m_Cells, m_StatePositions, m_PrevCells, m_PrevStatePositions);
					m_PrevCells = m_Cells;
					m_PrevStatePositions = m_StatePositions;
				}
			}
		}

		RefreshUpdate();
	}
}

int Grid::GetPaused()
{
	return m_Paused;
}

int Grid::GetFinished()
{
	return m_Finished;
}

int Grid::GetGenerating()
{
	return m_Generating;
}

void Grid::ResetUniverse()
{
	if (m_StatusCells->GetCountGeneration() == 0) return;

	m_StatusCells->SetCountGeneration(0);

	m_Paused = true;
	m_Finished = false;
	m_Generating = false;

	m_StatusControls->SetPlayButton(1);
}

void Grid::WaitForPause()
{
	m_ForceClose = true;

	PauseUniverse();
}

void Grid::DecrementGenerationCount()
{
	if (m_StatusCells->GetCountGeneration()) m_StatusCells->UpdateCountGeneration(-1);
}

void Grid::ResetGenerationCount()
{
	if (m_StatusCells->GetCountGeneration()) m_StatusCells->SetCountGeneration(0);
}

void Grid::RefreshUpdate()
{
	Refresh(false);
	Update();

	m_StatusCells->SetCountPopulation(m_Cells.size());
}

void Grid::UpdatePrev()
{
	m_PrevCells = m_Cells;
	m_PrevStatePositions = m_StatePositions;

	m_StatusCells->SetCountPopulation(m_Cells.size());
}

std::unordered_map<std::pair<int, int>, std::pair<std::string, wxColour>, Hashes::PairInt> Grid::GetCells()
{
	return m_Cells;
}

std::unordered_map<std::string, std::unordered_set<std::pair<int, int>, Hashes::PairInt>> Grid::GetStatePositions()
{
	return m_StatePositions;
}

std::unordered_map<std::pair<int, int>, std::pair<std::string, wxColour>, Hashes::PairInt> Grid::GetPrevCells()
{
	return m_PrevCells;
}

std::unordered_map<std::string, std::unordered_set<std::pair<int, int>, Hashes::PairInt>> Grid::GetPrevStatePositions()
{
	return m_PrevStatePositions;
}

std::unordered_map<std::string, wxColour>& Grid::GetColors()
{
	return m_ToolStates->GetColors();
}

void Grid::UpdateCoordsHovered()
{
	if (m_ForceClose) return;

	// is the cursor inside the grid?
	if (!wxWindow::GetScreenRect().Contains(wxGetMousePosition())) return;

	// update cursor's position relative to the grid
	wxPoint xy = ScreenToClient(wxGetMouseState().GetPosition()) / m_Size;

	// out of grid bounds
	if (xy.x >= Sizes::N_COLS || xy.y >= Sizes::N_ROWS) return;

	// map to visible cell coordinates
	xy.x = GetVisibleColumnsBegin() + xy.x;
	xy.y = GetVisibleRowsBegin() + xy.y;

	// same cell as the previously hover cell
	if (xy.x != m_LastHovered.first || xy.y != m_LastHovered.second) return;

	ControlUpdateCoords(xy.x, xy.y);
}

wxCoord Grid::OnGetRowHeight(size_t row) const
{
	return wxCoord(m_Size);
}

wxCoord Grid::OnGetColumnWidth(size_t row) const
{
	return wxCoord(m_Size);
}

void Grid::OnPaint(wxPaintEvent& evt)
{
	wxAutoBufferedPaintDC dc(this);

	PrepareDC(dc);
	OnDraw(dc);
}

void Grid::BuildInterface()
{
	SetRowColumnCount(Sizes::N_ROWS, Sizes::N_COLS);

	SetBackgroundStyle(wxBG_STYLE_PAINT);
}

void Grid::InitializeTimers()
{
	// timer for updating the selected state when using Shift+Click
	m_TimerSelection = new wxTimer(this, Ids::ID_TIMER_SELECTION);
}

std::pair<int, int> Grid::GetHoveredCell(int X, int Y)
{
	// get hovered cell information 
	wxPosition visible = GetVisibleBegin();

	int x = X / m_Size + visible.GetCol();
	int y = Y / m_Size + visible.GetRow();

	m_LastHovered = { x,y };

	return m_LastHovered;
}

bool Grid::ControlSelectState()
{
	wxMouseState mouse = wxGetMouseState();

	// shortcut for alternating between states: Shift + L/R Click
	if ((mouse.LeftIsDown() || mouse.RightIsDown()) && wxGetKeyState(WXK_SHIFT))
	{
		if (!m_TimerSelection->IsRunning()) m_TimerSelection->Start(80);

		return true;
	}

	return false;
}

bool Grid::ControlZoom(int x, int y, int rotation)
{
	// shortcut for zooming in/out with focus on hovered cell
	if (rotation && wxGetKeyState(WXK_CONTROL))
	{
		if (rotation > 0 && m_ToolZoom->GetSize() < Sizes::CELL_SIZE_MAX)
		{
			m_ToolZoom->ZoomIn(false);
			ScrollToCenter(x, y);
		}
		if (rotation < 0 && m_ToolZoom->GetSize() > Sizes::CELL_SIZE_MIN)
		{
			m_ToolZoom->ZoomOut(false);
			ScrollToCenter(x, y);
		}

		wxPoint XY = ScreenToClient(wxGetMouseState().GetPosition());
		ControlUpdateCoords(XY.x, XY.y);

		return true;
	}

	// scroll horizontally

	if (rotation && wxGetKeyState(WXK_ALT))
	{
		int sign = (rotation > 0) ? 1 : -1;
		m_JustScrolled = { sign, 0 };
		ScrollColumns(sign);

		return true;
	}

	if (rotation)
	{
		int sign = (rotation > 0) ? 1 : -1;
		m_JustScrolled = { 0, -sign };
		ScrollRows(-sign);

		return true;
	}

	return false;
}

std::string Grid::ControlUpdateCoords(int x, int y)
{
	// update coordinates displayed on screen

	std::string state = GetState(x, y);

	if (m_ToolCoords->GetState() == state) return state;

	// map to grid coordinates
	m_ToolCoords->SetCoords(x - m_OffsetX, y - m_OffsetY, state);

	return state;
}

bool Grid::ModeDraw(int x, int y, char mode)
{
	// "draw" mode
	if (mode == 'D')
	{
		wxMouseState mouse = wxGetMouseState();

		// left click -> place a cell
		if (mouse.LeftIsDown())
		{
			// don't interfere with the grid when the universe is playing
			if (m_Generating || !m_Paused)
			{
				wxMessageBox("Can't draw cells while the simulation is playing. Try pausing it first.", "Error", wxICON_WARNING);
				return false;
			}

			SetFocus();
			std::pair<std::string, wxColour> state = m_ToolStates->GetState();

			bool structure = false;

			// fill the whole area with cells of this state
			if (wxGetKeyState(WXK_ALT))
			{
				DrawStructure(x, y, state.first, state.second);

				structure = true;
			}

			// just clicked
			if (!m_IsDrawing)
			{
				m_IsDrawing = true;
				m_IsErasing = false;
				m_LastDrawn = { x,y };

				if (!structure)
				{
					if (InsertCell(x, y, state.first, state.second));// ResetUniverse();
				}
			}
			// holding click
			else
			{
				// mouse was dragged so fast that it didn't register
				// some of the cells meant to be drawn -> apply fix
				if (!structure) DrawLine(x, y, state.first, state.second);
			}
		}
		// right click -> remove a cell
		else if (mouse.RightIsDown())
		{
			// don't interfere with the grid when the universe is playing
			if (m_Generating || !m_Paused)
			{
				wxMessageBox("Can't erase cells while the simulation is playing. Try pausing it first.", "Error", wxICON_WARNING);
				return false;
			}

			SetFocus();
			if (GetState(x, y) == "FREE")
			{
				m_LastDrawn = { x,y };
				return true;
			}

			std::pair<std::string, wxColour> state = m_Cells[{x, y}];

			bool structure = false;

			// delete the whole area
			if (wxGetKeyState(WXK_CONTROL))
			{
				// delete the cells with the same state as the selected cell
				if (wxGetKeyState(WXK_ALT)) DeleteStructure(x, y, "");
				// delete cells of any state
				else DeleteStructure(x, y, state.first);

				structure = true;
			}

			// just clicked
			if (!m_IsErasing)
			{
				m_IsErasing = true;
				m_IsDrawing = false;
				m_LastDrawn = { x,y };

				if (!structure)
				{
					if (RemoveCell(x, y, state.first, state.second));
				}
			}
			// holding click
			else
			{
				// mouse was dragged so fast that it didn't register
				// some of the cells meant to be drawn -> apply fix
				if (!structure) DrawLine(x, y, "FREE", wxColour("white"), true);
			}
		}
		// not doing any drawing operation anymore
		else if (m_IsDrawing || m_IsErasing)
		{
			if (m_Cells != m_PrevCells)
			{
				m_ToolUndo->PushBack(m_Cells, m_StatePositions, m_PrevCells, m_PrevStatePositions);
				m_PrevCells = m_Cells;
				m_PrevStatePositions = m_StatePositions;

				ResetGenerationCount();
			}

			m_IsDrawing = false;
			m_IsErasing = false;
		}

		return true;
	}

	return false;
}

bool Grid::ModePick(int x, int y, char mode, std::string state)
{
	// "pick" mode
	if (mode == 'P')
	{
		// still drawing -> save progress and disable drawing
		if (m_IsDrawing || m_IsErasing)
		{
			m_IsDrawing = false;
			m_IsErasing = false;

			if (m_Cells != m_PrevCells)
			{
				m_ToolUndo->PushBack(m_Cells, m_StatePositions, m_PrevCells, m_PrevStatePositions);
				m_PrevCells = m_Cells;
				m_PrevStatePositions = m_StatePositions;
			}
		}

		// still moving -> disable moving
		if (m_IsMoving) m_IsMoving = false;

		wxMouseState mouse = wxGetMouseState();

		// select state and switch to drawing
		if (mouse.LeftIsDown())
		{
			m_ToolStates->SetState(state);
			m_ToolModes->SetMode('D');
		}
		// cancel selection and switch to drawing
		else if (mouse.RightIsDown())
		{
			m_ToolModes->SetMode('D');
		}

		return true;
	}

	return false;
}

bool Grid::ModeMove(int x, int y, char mode)
{
	wxMouseState mouse = wxGetMouseState();

	if ((mode == 'M' && mouse.LeftIsDown()) || mouse.MiddleIsDown())
	{
		// still drawing -> save progress and disable it
		if (m_IsDrawing || m_IsErasing)
		{
			m_IsDrawing = false;
			m_IsErasing = false;

			if (m_Cells != m_PrevCells)
			{
				m_ToolUndo->PushBack(m_Cells, m_StatePositions, m_PrevCells, m_PrevStatePositions);
				m_PrevCells = m_Cells;
				m_PrevStatePositions = m_StatePositions;
			}
		}

		if (!m_IsMoving)
		{
			m_IsMoving = true;

			SetFocus();
			m_PrevCell = { x,y };

			return true;
		}
		else
		{
			// pan around the previous cell reference
			if (m_PrevCell.first != x || m_PrevCell.second != y)
			{
				int deltaX = m_PrevCell.first - x;
				int deltaY = m_PrevCell.second - y;

				if (deltaX)
				{
					m_PrevCell.first = x;

					if (m_PrevScrolledCol)
					{
						m_PrevScrolledCol = false;
						deltaX = 0;
					}
					else
					{
						m_PrevScrolledCol = true;

						// don't scroll beyond the minimum limit
						if (deltaX < 0) deltaX = std::max(-GetScrollPos(wxHORIZONTAL), deltaX);
						// don't scroll beyond the maximum limit
						if (deltaX > 0) deltaX = std::min(Sizes::N_COLS - GetScrollPos(wxHORIZONTAL) - GetScrollThumb(wxHORIZONTAL), deltaX);
					}
				}
				if (deltaY)
				{
					m_PrevCell.second = y;

					if (m_PrevScrolledRow)
					{
						m_PrevScrolledRow = false;
						deltaY = 0;
					}
					else
					{
						m_PrevScrolledRow = true;

						// don't scroll beyond the minimum limit
						if (deltaY < 0) deltaY = std::max(-GetScrollPos(wxVERTICAL), deltaY);
						// don't scroll beyond the maximum limit
						if (deltaY > 0) deltaY = std::min(Sizes::N_ROWS - GetScrollPos(wxVERTICAL) - GetScrollThumb(wxVERTICAL), deltaY);
					}
				}

				if (deltaX || deltaY)
				{
					m_JustScrolled.first += deltaX;
					m_JustScrolled.second += deltaY;

					ScrollColumns(deltaX);
					ScrollRows(deltaY);
				}
			}

			return true;
		}
	}
	// cancel panning around and switch to drawing
	else if (mode == 'M' && mouse.RightIsDown())
	{
		m_IsMoving = false;

		m_ToolModes->SetMode('D');
		return true;
	}
	// no more panning around detected
	else m_IsMoving = false;

	return false;
}

void Grid::OnDraw(wxDC& dc)
{
	wxBrush brush = dc.GetBrush();
	wxPen pen = dc.GetPen();

	pen.SetStyle(wxPENSTYLE_SOLID);
	pen.SetColour(wxColour(200, 200, 200));

	// draw borders around the cell if the size is big enough
	dc.SetPen(pen);

	// don't draw borders if the size is too small
	if (m_Size <= Sizes::CELL_SIZE_BORDER) dc.SetPen(*wxTRANSPARENT_PEN);

	wxPosition visibleBegin = GetVisibleBegin();
	wxPosition visibleEnd = GetVisibleEnd();

	// changed the size of the cells -> redraw completely
	if (m_JustResized)
	{
		m_JustResized = false;
		m_RedrawAll = true;
	}
	// scrolled a bit -> redraw only affected cells
	else if (m_JustScrolled != std::make_pair(0, 0))
	{
		std::unordered_set<std::pair<int, int>, Hashes::PairInt> alreadyDrawn;
		std::vector<std::pair<int, int>> beforeScrolling;

		// iterate through the cells in order of their states
		m_MutexCells.lock();
		for (auto& sp : m_StatePositions)
		{
			brush.SetColour(m_Cells[{sp.second.begin()->first, sp.second.begin()->second}].second);
			dc.SetBrush(brush);

			// redraw only the cells affected by the scroll
			for (auto& it : sp.second)
			{
				// the cell state before scrolling
				int x = it.first;
				int y = it.second;

				// visible
				if (InVisibleBounds(x, y))
				{
					alreadyDrawn.insert({ x,y });

					dc.DrawRectangle(x * m_Size, y * m_Size, m_Size, m_Size);
				}

				// the cell after scrolling
				x += m_JustScrolled.first;
				y += m_JustScrolled.second;

				// visible
				if (InVisibleBounds(x, y))
				{
					beforeScrolling.push_back({ x,y });
				}
			}
		}
		m_MutexCells.unlock();

		// should only redraw updated cells
		if (!m_RedrawAll)
		{
			if (m_RedrawXYs.size())
			{
				for (int i = 0; i < m_RedrawXYs.size(); i++)
				{
					int x = m_RedrawXYs[i].first;
					int y = m_RedrawXYs[i].second;

					if (alreadyDrawn.find({ x,y }) == alreadyDrawn.end())
					{
						alreadyDrawn.insert({ x,y });

						brush.SetColour(m_RedrawColors[i]);
						dc.SetBrush(brush);
						dc.DrawRectangle(x * m_Size, y * m_Size, m_Size, m_Size);
					}

					x += m_JustScrolled.first;
					y += m_JustScrolled.second;

					// visible
					if (InVisibleBounds(x, y))
					{
						beforeScrolling.push_back({ x,y });
					}
				}

				m_RedrawXYs.clear();
				m_RedrawColors.clear();
			}
			else
			{
				int x = m_RedrawXY.first;
				int y = m_RedrawXY.second;

				if (InVisibleBounds(x, y) && alreadyDrawn.find({ x,y }) == alreadyDrawn.end())
				{
					alreadyDrawn.insert({ x,y });

					brush.SetColour(m_RedrawColor);
					dc.SetBrush(brush);
					dc.DrawRectangle(x * m_Size, y * m_Size, m_Size, m_Size);
				}

				x += m_JustScrolled.first;
				y += m_JustScrolled.second;

				// visible
				if (InVisibleBounds(x, y))
				{
					beforeScrolling.push_back({ x,y });
				}
			}
		}

		brush.SetColour(wxColour("white"));
		dc.SetBrush(brush);
		for (auto& it : beforeScrolling)
		{
			int x = it.first;
			int y = it.second;

			if (alreadyDrawn.find({ x,y }) == alreadyDrawn.end())
			{
				dc.DrawRectangle(x * m_Size, y * m_Size, m_Size, m_Size);
			}
		}

		// redraw borders

		// scrolled right -> redraw right border
		if (m_JustScrolled.first > 0)
		{
			for (int i = visibleEnd.GetCol() - 1; i > visibleEnd.GetCol() - 1 - m_JustScrolled.first; i--)
			{
				for (int j = visibleBegin.GetRow(); j < visibleEnd.GetRow(); j++)
				{
					if (alreadyDrawn.find({ i,j }) == alreadyDrawn.end())
					{
						dc.DrawRectangle(i * m_Size, j * m_Size, m_Size, m_Size);
					}
				}
			}
		}
		// scrolled left -> redraw left border
		if (m_JustScrolled.first < 0)
		{
			for (int i = visibleBegin.GetCol(); i <= visibleBegin.GetCol() - m_JustScrolled.first; i++)
			{
				for (int j = visibleBegin.GetRow(); j < visibleEnd.GetRow(); j++)
				{
					if (alreadyDrawn.find({ i,j }) == alreadyDrawn.end())
					{
						dc.DrawRectangle(i * m_Size, j * m_Size, m_Size, m_Size);
					}
				}
			}
		}
		// scrolled down -> redraw bottom border
		if (m_JustScrolled.second > 0)
		{
			for (int i = visibleEnd.GetRow() - 1; i > visibleEnd.GetRow() - 1 - m_JustScrolled.second; i--)
			{
				for (int j = visibleBegin.GetCol(); j < visibleEnd.GetCol(); j++)
				{
					if (alreadyDrawn.find({ j,i }) == alreadyDrawn.end())
					{
						dc.DrawRectangle(j * m_Size, i * m_Size, m_Size, m_Size);
					}
				}
			}
		}
		// scrolled up -> redraw upper border
		if (m_JustScrolled.second < 0)
		{
			for (int i = visibleBegin.GetRow(); i <= visibleBegin.GetRow() - m_JustScrolled.second; i++)
			{
				for (int j = visibleBegin.GetCol(); j < visibleEnd.GetCol(); j++)
				{
					if (alreadyDrawn.find({ j,i }) == alreadyDrawn.end())
					{
						dc.DrawRectangle(j * m_Size, i * m_Size, m_Size, m_Size);
					}
				}
			}
		}

		// redraw the background out of the borders
		dc.SetPen(*wxTRANSPARENT_PEN);
		brush.SetColour(GetBackgroundColour());
		dc.SetBrush(brush);

		if (m_JustScrolled.first > 0 && GetVisibleColumnsEnd() == Sizes::N_COLS)
		{
			int i = GetVisibleColumnsEnd();
			for (int j = visibleBegin.GetRow(); j < visibleEnd.GetRow(); j++)
				dc.DrawRectangle(i * m_Size, j * m_Size, m_Size, m_Size);
		}
		if (m_JustScrolled.second > 0 && GetVisibleRowsEnd() == Sizes::N_ROWS)
		{
			int i = GetVisibleRowsEnd();
			for (int j = visibleBegin.GetCol(); j < visibleEnd.GetCol(); j++)
				dc.DrawRectangle(j * m_Size, i * m_Size, m_Size, m_Size);
		}

		m_JustScrolled = { 0,0 };
		m_RedrawAll = true;

		return;
	}

	// should redraw only updated cells
	if (!m_RedrawAll)
	{
		m_RedrawAll = true;

		// more than 1 cell to be redrawn
		if (m_RedrawXYs.size())
		{
			for (int i = 0; i < m_RedrawXYs.size(); i++)
			{
				int x = m_RedrawXYs[i].first;
				int y = m_RedrawXYs[i].second;

				brush.SetColour(m_RedrawColors[i]);
				dc.SetBrush(brush);

				dc.DrawRectangle(x * m_Size, y * m_Size, m_Size, m_Size);
			}

			m_RedrawXYs.clear();
			m_RedrawColors.clear();

			return;
		}

		int x = m_RedrawXY.first;
		int y = m_RedrawXY.second;

		// is in visible bounds?
		if (!InVisibleBounds(x, y)) return;

		brush.SetColour(m_RedrawColor);
		dc.SetBrush(brush);

		dc.DrawRectangle(x * m_Size, y * m_Size, m_Size, m_Size);
		return;
	}

	// redraw everything

	dc.Clear();

	brush.SetColour(wxColour("white"));
	dc.SetBrush(brush);

	m_MutexCells.lock();
	for (int y = visibleBegin.GetRow(); y < visibleEnd.GetRow(); y++)
	{
		for (int x = visibleBegin.GetCol(); x < visibleEnd.GetCol(); x++)
		{
			if (GetState(x, y) != "FREE")
			{
				brush.SetColour(m_Cells[{x, y}].second);
				dc.SetBrush(brush);

				dc.DrawRectangle(x * m_Size, y * m_Size, m_Size, m_Size);

				brush.SetColour(wxColour("white"));
				dc.SetBrush(brush);
				continue;
			}

			dc.DrawRectangle(x * m_Size, y * m_Size, m_Size, m_Size);
		}
	}
	m_MutexCells.unlock();

	if (!m_Centered)
	{
		m_Centered = true;
		ScrollToCenter();
	}
}

void Grid::OnMouse(wxMouseEvent& evt)
{
	// handle all mouse events

	// mouse leaving grid area
	if (evt.Leaving())
	{
		m_ToolCoords->Reset();

		// still drawing -> save progress and disable drawing
		if (m_IsDrawing || m_IsErasing)
		{
			m_IsDrawing = false;
			m_IsErasing = false;

			if (m_Cells != m_PrevCells)
			{
				m_ToolUndo->PushBack(m_Cells, m_StatePositions, m_PrevCells, m_PrevStatePositions);
				m_PrevCells = m_Cells;
				m_PrevStatePositions = m_StatePositions;
			}
		}

		return;
	}

	// selecting states with Shift+Click ?
	if (ControlSelectState()) return;

	std::pair<int, int> hoveredCell = GetHoveredCell(evt.GetX(), evt.GetY());
	int x = hoveredCell.first;
	int y = hoveredCell.second;

	if (evt.Entering()) m_LastDrawn = { x,y };

	// zooming
	if (ControlZoom(x, y, evt.GetWheelRotation())) return;

	// out of grid borders?
	if (!InBounds(x, y))
	{
		m_ToolCoords->Reset();
		return;
	}

	std::string state = ControlUpdateCoords(x, y);

	char mode = m_ToolModes->GetMode();

	if (ModeMove(x, y, mode)) return;
	if (ModeDraw(x, y, mode)) return;
	if (ModePick(x, y, mode, state)) return;
}

void Grid::OnTimerSelection(wxTimerEvent& evt)
{
	// update state selection while the shortcut is being actioned

	wxMouseState mouseState = wxGetMouseState();
	if (!wxGetKeyState(WXK_SHIFT) || (!mouseState.LeftIsDown() && !mouseState.RightIsDown()))
	{
		m_TimerSelection->Stop();
		return;
	}

	if (mouseState.LeftIsDown()) m_ToolStates->SelectPrevState();
	if (mouseState.RightIsDown()) m_ToolStates->SelectNextState();
}

void Grid::OnKeyDown(wxKeyEvent& evt)
{
	// register the keys used for moving on the grid

	int key = evt.GetKeyCode();

	switch (key)
	{
	case WXK_UP:
	case 'W':
	case WXK_DOWN:
	case 'S':
	case WXK_LEFT:
	case 'A':
	case WXK_RIGHT:
	case 'D':
		m_Keys.insert(key);
		ProcessKeys();
	default:
		break;
	}
}

void Grid::OnKeyUp(wxKeyEvent& evt)
{
	// unregister the keys used for moving on the map

	int key = evt.GetKeyCode();

	switch (key)
	{
	case WXK_UP:
	case 'W':
	case WXK_DOWN:
	case 'S':
	case WXK_LEFT:
	case 'A':
	case WXK_RIGHT:
	case 'D':
		if (m_Keys.find(key) != m_Keys.end()) m_Keys.erase(key);
	default:
		return;
	}
}

void Grid::ProcessKeys()
{
	// move on the grid based on which keys are being actioned

	int deltaX = 0;
	int deltaY = 0;

	for (auto key : m_Keys)
	{
		switch (key)
		{
		case WXK_UP:
		case 'W':
			deltaY -= 1;
			continue;
		case WXK_DOWN:
		case 'S':
			deltaY += 1;
			continue;
		case WXK_LEFT:
		case 'A':
			deltaX -= 1;
			continue;
		case WXK_RIGHT:
		case 'D':
			deltaX += 1;
			continue;
		}
	}

	// don't scroll beyond the minimum limit
	if (deltaY < 0) deltaY = std::max(-GetScrollPos(wxVERTICAL), deltaY);
	if (deltaX < 0) deltaX = std::max(-GetScrollPos(wxHORIZONTAL), deltaX);

	// don't scroll beyond the maximum limit
	if (deltaX > 0) deltaX = std::min(Sizes::N_COLS - GetScrollPos(wxHORIZONTAL) - GetScrollThumb(wxHORIZONTAL), deltaX);
	if (deltaY > 0) deltaY = std::min(Sizes::N_ROWS - GetScrollPos(wxVERTICAL) - GetScrollThumb(wxVERTICAL), deltaY);

	if (deltaX || deltaY)
	{
		m_JustScrolled.first += deltaX;
		m_JustScrolled.second += deltaY;

		ScrollColumns(deltaX);
		ScrollRows(deltaY);
	}

	// handle mouse events while moving with the keyboard

	wxMouseState mouseState = wxGetMouseState();
	wxPoint XY = mouseState.GetPosition();

	int X = ScreenToClient(XY).x;
	int Y = ScreenToClient(XY).y;

	std::pair<int, int> hoveredCell = GetHoveredCell(X, Y);
	int x = hoveredCell.first;
	int y = hoveredCell.second;

	ControlUpdateCoords(x, y);

	ModeDraw(x, y, m_ToolModes->GetMode());
}

void Grid::OnEraseBackground(wxEraseEvent& evt)
{
	// this needs to be an empty function (old wxwidgets issue)
}

void Grid::DeleteStructure(int X, int Y, std::string state)
{
	// erase all the cells from the selected area

	int dx[8] = { 0, 1, 1, 1, 0, -1, -1 ,-1 };
	int dy[8] = { -1, -1, 0, 1, 1, 1, 0, -1 };

	std::unordered_set<std::pair<int, int>, Hashes::PairInt> visited;
	std::stack<std::pair<int, int>> neighbors;
	neighbors.push({ X,Y });

	int changes = 0;

	while (neighbors.size())
	{
		std::pair<int, int> neighbor = neighbors.top();
		neighbors.pop();

		visited.insert(neighbor);

		std::string neighborState = GetState(neighbor.first, neighbor.second);

		// doesn't matter if the structure is composed of cells of the same stats
		if (state == "") changes += EraseCell(neighbor.first, neighbor.second, true);
		// otherwise erase only if they share the same state
		else if (state == neighborState) changes += EraseCell(neighbor.first, neighbor.second, true);

		for (int d = 0; d < 8; d++)
		{
			int x = neighbor.first + dx[d];
			int y = neighbor.second + dy[d];

			// valid neighbor -> push onto stack
			if (state == "" && GetState(x, y) != "FREE" && visited.find({ x, y }) == visited.end()) neighbors.push({ x,y });
			else if (state == neighborState && GetState(x, y) == state && visited.find({ x, y }) == visited.end()) neighbors.push({ x,y });
		}
	}

	if (changes)
	{
		m_StatusCells->SetCountPopulation(m_Cells.size());

		Refresh(false);
		Update();
	}
}

void Grid::DrawStructure(int X, int Y, std::string state, wxColour color)
{
	// fill the selected area with cells of the currently selected state

	int dx[4] = { 0, 1, 0, -1 };
	int dy[4] = { -1, 0, 1, 0 };

	std::unordered_set<std::pair<int, int>, Hashes::PairInt> visited;
	std::stack<std::pair<int, int>> neighbors;
	neighbors.push({ X,Y });

	std::string replace = GetState(X, Y);

	wxPosition visibleBegin = GetVisibleBegin();
	wxPosition visibleEnd = GetVisibleEnd();

	int changes = 0;

	while (neighbors.size())
	{
		std::pair<int, int> neighbor = neighbors.top();
		neighbors.pop();

		visited.insert(neighbor);

		std::string neighborState = GetState(neighbor.first, neighbor.second);

		// only fill the different cells
		if (neighborState == state) continue;
		if (neighborState != replace) continue;

		changes += InsertCell(neighbor.first, neighbor.second, state, color, true);

		for (int d = 0; d < 4; d++)
		{
			int x = neighbor.first + dx[d];
			int y = neighbor.second + dy[d];

			// not in visible bounds
			if (!(x >= visibleBegin.GetCol() && x < visibleEnd.GetCol() && y >= visibleBegin.GetRow() && y < visibleEnd.GetRow())) continue;
			// valid neighbor -> push onto stack
			if (GetState(x, y) != state && visited.find({ x, y }) == visited.end())
			{
				neighbors.push({ x,y });
			}
		}
	}

	if (changes)
	{
		m_StatusCells->SetCountPopulation(m_Cells.size());

		Refresh(false);
		Update();
	}
}

void Grid::DrawLine(int x, int y, std::string state, wxColour color, bool remove)
{
	// algorithm found on old wxwidgets version repository

	if (m_LastDrawn != std::make_pair(x, y))
	{
		// apply the following algorithm to draw a line
		// starting from the last drawn {x,y} to the current {x,y}
		int changes = 0;
		int d, ii, jj, di, ai, si, dj, aj, sj;
		di = x - m_LastDrawn.first;
		ai = abs(di) << 1;
		si = (di < 0) ? -1 : 1;
		dj = y - m_LastDrawn.second;
		aj = abs(dj) << 1;
		sj = (dj < 0) ? -1 : 1;

		ii = m_LastDrawn.first;
		jj = m_LastDrawn.second;

		if (ai > aj)
		{
			d = aj - (ai >> 1);
			while (ii != x)
			{
				std::string currState = GetState(ii, jj);
				if (currState != state)
				{
					if (!remove && state != "FREE")
					{
						changes += InsertCell(ii, jj, state, color, true);
						m_StatusCells->UpdateCountPopulation(1);
					}
					else
					{
						changes += EraseCell(ii, jj, true);
						m_StatusCells->UpdateCountPopulation(-1);
					}
				}
				if (d >= 0)
				{
					jj += sj;
					d -= ai;
				}
				ii += si;
				d += aj;
			}
		}
		else
		{
			d = ai - (aj >> 1);
			while (jj != y)
			{
				std::string currState = GetState(ii, jj);
				if (currState != state)
				{
					if (!remove && state != "FREE")
					{
						changes += InsertCell(ii, jj, state, color, true);
						m_StatusCells->UpdateCountPopulation(1);
					}
					else
					{
						changes += EraseCell(ii, jj, true);
						m_StatusCells->UpdateCountPopulation(-1);
					}
				}
				if (d >= 0)
				{
					ii += si;
					d -= aj;
				}
				jj += sj;
				d += ai;
			}
		}

		m_LastDrawn = { x,y };

		std::string currState = GetState(x, y);
		if (currState != state)
		{
			if (!remove && state != "FREE")
			{
				changes += InsertCell(ii, jj, state, color, true);
				m_StatusCells->UpdateCountPopulation(1);
			}
			else
			{
				changes += EraseCell(ii, jj, true);
				m_StatusCells->UpdateCountPopulation(-1);
			}
		}

		if (changes > 0)
		{
			Refresh(false);
			Update();
		}
	}
}

bool Grid::InBounds(int x, int y)
{
	return (x >= 0 && x < Sizes::N_COLS&& y >= 0 && y < Sizes::N_ROWS);
}

bool Grid::InVisibleBounds(int x, int y)
{
	return (
		x >= GetVisibleColumnsBegin() && x < GetVisibleColumnsEnd()
		&& y >= GetVisibleRowsBegin() && y < GetVisibleRowsEnd()
		);
}

std::pair<std::vector<std::pair<int, int>>, std::string> Grid::ParseRule(
	std::pair<std::string, Transition>& rule,
	std::unordered_set<int>& visited
)
{
	if (m_ForceClose)
	{
		return { {}, "" };
	}

	std::unordered_map<std::string, std::string> states = m_InputRules->GetInputStates()->GetStates();
	std::unordered_set<std::string> neighbors = m_InputRules->GetInputNeighbors()->GetNeighbors();
	std::vector<std::pair<int, int>> applied;

	// check if rule might contain invalid states
	if (states.find(rule.first) == states.end()) return { {}, "<INVALID FIRST STATE>" };
	if (states.find(rule.second.state) == states.end()) return { {}, "<INVALID SECOND STATE>" };
	for (auto& state : rule.second.states)
	{
		if (states.find(state) == states.end()) return { {}, "<INVALID CONDITION STATE>" };
	}
	// check for neighborhood as well
	for (auto& direction : rule.second.directions)
	{
		if (neighbors.find(direction) == neighbors.end()) return { {}, "<INVALID NEIGHBORHOOD>" };
	}

	// if state is "FREE", apply rule to all "FREE" cells
	if (rule.first == "FREE")
	{
		// iterate through all cells
		if (rule.second.all || rule.second.condition.empty())
		{
			const int N = Sizes::N_ROWS * Sizes::N_COLS;

			for (int i = 0; i < N; i++)
			{
				int x = i % Sizes::N_COLS;
				int y = i / Sizes::N_COLS;
				int k = y * Sizes::N_COLS + x;

				if (GetState(x, y) == "FREE" && visited.find(k) == visited.end() && ApplyOnCell(x, y, rule.second, neighbors))
				{
					applied.push_back({ x,y });
					visited.insert(k);
				}
			}
		}
		// decide if it's faster to iterate through all cells
		// or through the condition states' neighbors
		else
		{
			int n1 = Sizes::N_ROWS * Sizes::N_COLS - m_Cells.size();
			int n2 = 0;
			for (auto& state : rule.second.states)
			{
				if (m_StatePositions.find(state) == m_StatePositions.end()) continue;
				n2 += m_StatePositions[state].size();
			}

			// faster to iterate through all cells
			if (n1 <= n2 || rule.second.condition.empty())
			{
				const int N = Sizes::N_COLS * Sizes::N_ROWS;

				for (int i = 0; i < N; i++)
				{
					int x = i % Sizes::N_COLS;
					int y = i / Sizes::N_COLS;
					int k = y * Sizes::N_COLS + x;

					if (GetState(x, y) == "FREE" && visited.find(k) == visited.end() && ApplyOnCell(x, y, rule.second, neighbors))
					{
						applied.push_back({ x,y });
						visited.insert(k);
					}
				}
			}
			// faster to iterate through the condition states' neighbors
			else
			{
				for (auto& state : rule.second.states)
				{
					if (state == "FREE")
					{
						const int N = Sizes::N_ROWS * Sizes::N_COLS;

						for (int i = 0; i < N; i++)
						{
							int x = i % Sizes::N_COLS;
							int y = i / Sizes::N_COLS;
							int k = y * Sizes::N_COLS + x;

							if (GetState(x, y) == "FREE" && visited.find(k) == visited.end() && ApplyOnCell(x, y, rule.second, neighbors))
							{
								applied.push_back({ x,y });
								visited.insert(k);
							}
						}
					}
					// cells of this type are placed on grid
					else if (m_StatePositions.find(state) != m_StatePositions.end())
					{
						int dx[8] = { 0,1,1,1,0,-1,-1,-1 };
						int dy[8] = { -1,-1,0,1,1,1,0,-1 };

						for (auto i = m_StatePositions.at(state).begin(); i != m_StatePositions.at(state).end(); i++)
						{
							int x = i->first;
							int y = i->second;

							for (int d = 0; d < 8; d++)
							{
								int nx = x + dx[d];
								int ny = y + dy[d];
								int k = ny * Sizes::N_COLS + nx;

								if (InBounds(nx, ny) && GetState(nx, ny) == rule.first && visited.find(k) == visited.end() && ApplyOnCell(nx, ny, rule.second, neighbors))
								{
									applied.push_back({ nx,ny });
									visited.insert(k);
								}
							}
						}
					}
				}
			}
		}
	}
	// else, get all cells of that type
	else
	{
		if (m_StatePositions.find(rule.first) == m_StatePositions.end()) return { {},"" };

		// iterate through all cells
		if (rule.second.all || rule.second.condition.empty())
		{
			string state = rule.first;
			for (auto i = m_StatePositions.at(state).begin(); i != m_StatePositions.at(state).end(); i++)
			{
				int x = i->first;
				int y = i->second;
				int k = y * Sizes::N_COLS + x;

				if (visited.find(k) == visited.end() && ApplyOnCell(x, y, rule.second, neighbors))
				{
					visited.insert(k);
					applied.push_back({ x,y });
				}
			}
		}
		// decide if it's faster to iterate through all cells
		// or through the condition states' neighbors
		else
		{
			int n1 = m_StatePositions[rule.first].size();
			int n2 = 0;
			for (auto& state : rule.second.states)
			{
				if (m_StatePositions.find(state) == m_StatePositions.end()) continue;
				n2 += m_StatePositions[state].size();
			}

			// faster to iterate through all cells
			if (n1 <= n2 || rule.second.condition.empty())
			{
				string state = rule.first;
				for (auto i = m_StatePositions.at(state).begin(); i != m_StatePositions.at(state).end(); i++)
				{
					int x = i->first;
					int y = i->second;
					int k = y * Sizes::N_COLS + x;

					if (visited.find(k) == visited.end() && ApplyOnCell(x, y, rule.second, neighbors))
					{
						visited.insert(k);
						applied.push_back({ x,y });
					}
				}
			}
			// faster to iterate through the condition states' neighbors
			else
			{
				for (auto& state : rule.second.states)
				{
					if (state == "FREE")
					{
						string state = rule.first;
						for (auto i = m_StatePositions.at(state).begin(); i != m_StatePositions.at(state).end(); i++)
						{
							int x = i->first;
							int y = i->second;
							int k = y * Sizes::N_COLS + x;

							if (visited.find(k) == visited.end() && ApplyOnCell(x, y, rule.second, neighbors))
							{
								applied.push_back({ x,y });
								visited.insert(k);
							}
						}
					}
					// cells of this type are placed on grid
					else if (m_StatePositions.find(state) != m_StatePositions.end())
					{
						int dx[8] = { 0,1,1,1,0,-1,-1,-1 };
						int dy[8] = { -1,-1,0,1,1,1,0,-1 };

						for (auto i = m_StatePositions.at(state).begin(); i != m_StatePositions.at(state).end(); i++)
						{
							int x = i->first;
							int y = i->second;

							for (int d = 0; d < 8; d++)
							{
								int nx = x + dx[d];
								int ny = y + dy[d];
								int k = ny * Sizes::N_COLS + nx;

								if (visited.find(k) == visited.end() && InBounds(nx, ny) && GetState(nx, ny) == rule.first && ApplyOnCell(nx, ny, rule.second, neighbors))
								{
									applied.push_back({ nx,ny });
									visited.insert(k);
								}
							}
						}
					}
				}
			}
		}
	}

	return { applied, "" };
}

std::pair<std::vector<std::pair<std::string, std::pair<int, int>>>, std::string> Grid::ParseAllRules()
{
	std::vector<std::pair<std::string, Transition>> rules = m_InputRules->GetRules();
	std::vector<std::pair<std::string, std::pair<int, int>>> changes;
	std::unordered_set<int> visited;

	for (auto& rule : rules)
	{
		std::pair<std::vector<std::pair<int, int>>, std::string> result = ParseRule(rule, visited);

		// error
		if (result.second.size())
		{
			int index = -1;

			// mark the problematic rule index
			for (int i = 0; i < m_InputRules->GetList()->GetItemCount(); i++)
			{
				if (m_InputRules->GetList()->GetState1(i) == rule.first
					&& m_InputRules->GetList()->GetState2(i) == rule.second.state
					&& m_InputRules->GetList()->GetCond(i) == rule.second.condition)
				{
					index = i;
					break;
				}
			}

			if (index != -1) result.second += " at rule number " + std::to_string(index + 1);

			return { {}, result.second };
		}
		// concatenate changes
		else
		{
			std::string newstate = rule.first + "*" + rule.second.state + "*";

			for (auto& change : result.first)
			{
				changes.push_back({ newstate , change });
			}
		}
	}

	return { changes,"" };
}

bool Grid::ApplyOnCell(int x, int y, Transition& rule, std::unordered_set<std::string>& neighbors)
{
	std::unordered_map<std::string, std::string> neighborhood = GetNeighborhood({ x,y }, neighbors);

	bool ruleValid = true;
	// iterate through the chain of "OR" rules
	for (auto& rulesOr : rule.orRules)
	{
		ruleValid = true;

		// iterate through the chain of "AND" rules
		for (auto& rulesAnd : rulesOr)
		{
			std::vector<std::string> ruleNeighborhood = rulesAnd.first;

			bool conditionValid = true;
			// iterate through the chain of "OR" conditions
			for (auto& conditionsOr : rulesAnd.second)
			{
				conditionValid = true;

				// iterate through the chain of "AND" conditions
				for (auto& conditionsAnd : conditionsOr)
				{
					std::string conditionState = conditionsAnd.second;

					int occurences = 0;
					if (ruleNeighborhood[0] == "ALL")
					{
						for (auto& neighbor : neighborhood)
							if (neighbor.second == conditionState) occurences++;
					}
					else for (auto& neighbor : ruleNeighborhood)
					{
						if (neighbors.find(neighbor) != neighbors.end())
						{
							if (neighborhood[neighbor] == conditionState) occurences++;
						}
					}

					int conditionNumber = conditionsAnd.first.first;
					int conditionType = conditionsAnd.first.second;

					switch (conditionType)
					{
					case TYPE_EQUAL:
						if (occurences != conditionNumber) conditionValid = false;
						break;
					case TYPE_LESS:
						if (occurences >= conditionNumber) conditionValid = false;
						break;
					case TYPE_MORE:
						if (occurences <= conditionNumber) conditionValid = false;
						break;
					default:
						break;
					}
				}

				if (conditionValid) break;
			}

			if (!conditionValid)
			{
				ruleValid = false;
				break;
			}
		}

		if (ruleValid) break;
	}

	return ruleValid;
}

std::unordered_map<std::string, std::string> Grid::GetNeighborhood(std::pair<int, int> xy, std::unordered_set<std::string>& neighbors)
{
	std::unordered_map<std::string, std::string> neighborhood;
	std::unordered_map<std::string, std::pair<int, int>> dxy(
		{
			{ "NW",{-1,-1} }, { "N",{0,-1} }, { "NE",{1,-1} },
			{ "W",{-1,0} }, { "C",{0,0} }, { "E",{1,0} },
			{ "SW",{-1,1} }, { "S",{0,1} }, { "SE",{1,1} },
		}
	);

	// iterate through neighbors and mark the state they have
	for (auto& neighbor : neighbors)
	{
		int x = xy.first;
		int y = xy.second;

		std::pair<int, int> d = dxy[neighbor];

		int nx = x + d.first;
		int ny = y + d.second;

		if (InBounds(nx, ny)) neighborhood.insert({ neighbor,GetState(nx,ny) });
	}

	return neighborhood;
}

void Grid::UpdateGeneration(std::vector<std::pair<std::string, std::pair<int, int>>> changes)
{
	std::unordered_map<std::string, wxColour> colors = GetColors();

	m_MutexCells.lock();
	for (auto& change : changes)
	{
		std::string state = change.first;

		state.pop_back();

		std::string prevState = "";
		std::string currState = "";
		bool separator = false;

		// regain information of previous and current state
		for (int i = 0; i < state.size(); i++)
		{
			if (state[i] == '*')
			{
				separator = true;
				continue;
			}

			if (!separator) prevState.push_back(state[i]);
			else currState.push_back(state[i]);
		}

		// insert positions into the current state map and remove them from the previous one
		auto position = change.second;

		InsertCell(position.first, position.second, currState, colors[currState], true);
	}
	m_MutexCells.unlock();

	Refresh(false);
	Update();
}

void Grid::OnScroll(wxScrollWinEvent& evt)
{
	int newPosition = 0;
	int deltaX = 0;
	int deltaY = 0;

	if (evt.GetOrientation() == wxHORIZONTAL)
	{
		newPosition = wxVarHScrollHelper::GetNewScrollPosition(evt);
		deltaX = newPosition - GetScrollPos(wxHORIZONTAL);
	}
	else if (evt.GetOrientation() == wxVERTICAL)
	{
		newPosition = wxVarVScrollHelper::GetNewScrollPosition(evt);
		deltaY = newPosition - GetScrollPos(wxVERTICAL);
	}

	if (deltaX || deltaY)
	{
		m_JustScrolled.first += deltaX;
		m_JustScrolled.second += deltaY;

		ScrollColumns(deltaX);
		ScrollRows(deltaY);
	}
}

void Grid::OnSize(wxSizeEvent& evt)
{
	m_JustResized = true;
	Refresh(false);
	evt.Skip();
}
