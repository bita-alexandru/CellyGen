#pragma once
#include "wx/dialog.h"
#include "wx/spinctrl.h"

class DialogDimensions : public wxDialog
{
public:
	DialogDimensions(wxWindow* parent);
	~DialogDimensions();

	int GetRows();
	int GetCols();
private:
	wxSpinCtrl* m_SpinRows = nullptr;
	wxSpinCtrl* m_SpinCols = nullptr;

	void BuildInterface();
};

