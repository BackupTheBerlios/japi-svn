#include "MJapieG.h"

#include <iostream>

#include "MWindow.h"

using namespace std;

MWindow::MWindow()
	: mOnDestroy(this, &MWindow::OnDestroy)
	, mOnDelete(this, &MWindow::OnDelete)
{
	mWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	
	mOnDestroy.Connect(mWindow, "destroy");
	mOnDelete.Connect(mWindow, "delete_event");
}

MWindow::~MWindow()
{
}
	
void MWindow::Show()
{
	gtk_widget_show_all(mWindow);
}

void MWindow::Hide()
{
}

void MWindow::Select()
{
	gtk_window_present(GTK_WINDOW(mWindow));
}

void MWindow::OnDestroy()
{
	cout << "destroy" << endl;
	gApp->RecycleWindow(this);
}

bool MWindow::OnDelete(
	GdkEvent*		inEvent)
{
	cout << "delete_event" << endl;
	return false;
}
