#include "MJapi.h"
#include "CTestWindow.h"
#include "MListView.h"

CTestWindow::CTestWindow()
	: MWindow("Test", MRect(0, 0, 400, 400), kMPostionDefault, "")
{
	AddChild(new MListView("lijst", MRect(10, 10, 380, 380)));
	
}
