#pragma once
#include "wx/wx.h"

#include "Ids.h"
#include "Sizes.h"
#include "Grid.h"

class Grid;

class ToolZoom : public wxPanel
{
public:
	ToolZoom(wxWindow* parent);
	~ToolZoom();

	int GetSize();
	void ZoomIn(bool center = true);
	void ZoomOut(bool center = true);

	void SetGrid(Grid* grid);
private:
	int m_Size = Sizes::CELL_SIZE_DEFAULT;
	int m_MaximumSize = Sizes::CELL_SIZE_MAX;
	int m_MinimumSize = Sizes::CELL_SIZE_MIN;

	wxStaticText* m_TextScale = nullptr;

	Grid* m_Grid = nullptr;

	void BuildInterface();
	void UpdateTextScale();

	void OnZoomIn(wxCommandEvent& evt);
	void OnZoomOut(wxCommandEvent& evt);
};

