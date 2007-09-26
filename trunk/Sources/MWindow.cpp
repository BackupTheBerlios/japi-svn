#include "MJapieG.h"

#include <iostream>

#include "MCommands.h"
#include "MWindow.h"

using namespace std;

MWindow::MWindow()
	: MHandler(gApp)
	, mOnDestroy(this, &MWindow::OnDestroy)
	, mOnDelete(this, &MWindow::OnDelete)
{
	mWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	
	mOnDestroy.Connect(mWindow, "destroy");
	mOnDelete.Connect(mWindow, "delete_event");
}

MWindow::~MWindow()
{
	cout << "deleting MWindow" << endl;
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

void MWindow::Close()
{
	gtk_widget_destroy(GetGtkWidget());
}

void MWindow::SetTitle(
	const string&	inTitle)
{
	gtk_window_set_title(GTK_WINDOW(GetGtkWidget()), inTitle.c_str());
}

string MWindow::GetTitle() const
{
	string result;
	return result;
}

void MWindow::OnDestroy()
{
	cout << "destroy" << endl;
	gApp->RecycleWindow(this);
}

bool MWindow::OnDelete(
	GdkEvent*		inEvent)
{
	Close();
	return false;
}

bool MWindow::UpdateCommandStatus(
	uint32			inCommand,
	bool&			outEnabled,
	bool&			outChecked)
{
	return MHandler::UpdateCommandStatus(inCommand, outEnabled, outChecked);
}

bool MWindow::ProcessCommand(
	uint32			inCommand)
{
	bool result = true;	
	
	switch (inCommand)
	{
		case cmd_Close:
			Close();
			break;
		
		default:
			result = MHandler::ProcessCommand(inCommand);
			break;
	}
	
	return result;
}
	
