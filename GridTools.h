#pragma once
#include "wx/wx.h"
#include "wx/statline.h"

#include "ToolZoom.h"
#include "ToolUndo.h"
#include "ToolModes.h"
#include "ToolStates.h"
#include "ToolCoords.h"

class GridTools: public wxPanel
{
public:
	GridTools(wxWindow* parent);
	~GridTools();
	
	ToolZoom* GetToolZoom();
	ToolUndo* GetToolUndo();
	ToolModes* GetToolModes();
	ToolStates* GetToolStates();
	ToolCoords* GetToolCoords();
private:
	ToolZoom* m_ToolZoom = nullptr;
	ToolUndo* m_ToolUndo = nullptr;
	ToolModes* m_ToolModes = nullptr;
	ToolStates* m_ToolStates = nullptr;
	ToolCoords* m_ToolCoords = nullptr;

	void BuildInterface();
};

