#include "App.h"

wxIMPLEMENT_APP(App);

App::App()
{

}

App::~App()
{

}

bool App::OnInit()
{
	// add required support for handling png files
	wxImage::AddHandler(new wxPNGHandler);

	m_Main = new Main();
	m_Main->Show();

	return true;
}