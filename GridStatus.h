#pragma once
#include "wx/wx.h"

#include "StatusDelay.h"
#include "StatusControls.h"
#include "StatusCells.h"

class GridStatus: public wxPanel
{
public:
	GridStatus(wxWindow* parent);
	~GridStatus();

	StatusDelay* GetStatusDelay();
	StatusControls* GetStatusControls();
	StatusCells* GetStatusCells();
private:
	StatusDelay* m_StatusDelay = nullptr;
	StatusControls* m_StatusControls = nullptr;
	StatusCells* m_StatusCells = nullptr;

	void BuildInterface();
};

