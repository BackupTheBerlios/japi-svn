#include "MJapieG.h"

#include <iostream>

#include "MCommands.h"
#include "MWindow.h"

using namespace std;

MWindow::MWindow()
	: MView(gtk_window_new(GTK_WINDOW_TOPLEVEL), false)
	, MHandler(gApp)
	, mOnDestroy(this, &MWindow::OnDestroy)
	, mOnDelete(this, &MWindow::OnDelete)
{
	mOnDestroy.Connect(GetGtkWidget(), "destroy");
	mOnDelete.Connect(GetGtkWidget(), "delete_event");
}

MWindow::~MWindow()
{
	cout << "deleting MWindow" << endl;
}
	
void MWindow::Show()
{
	gtk_widget_show_all(GetGtkWidget());
}

void MWindow::Hide()
{
	gtk_widget_hide_all(GetGtkWidget());
}

void MWindow::Select()
{
	gtk_window_present(GTK_WINDOW(GetGtkWidget()));
}

bool MWindow::DoClose()
{
	cout << "close" << endl;
	return true;
}

void MWindow::Close()
{
	if (DoClose())
		gtk_widget_destroy(GetGtkWidget());
}

void MWindow::SetTitle(
	const string&	inTitle)
{
	gtk_window_set_title(GTK_WINDOW(GetGtkWidget()), inTitle.c_str());
}

string MWindow::GetTitle() const
{
	const char* title = gtk_window_get_title(GTK_WINDOW(GetGtkWidget()));
	if (title == nil)
		title = "";
	return title;
}

bool MWindow::OnDestroy()
{
	cout << "destroy" << endl;
	gApp->RecycleWindow(this);
	return false;
}

bool MWindow::OnDelete(
	GdkEvent*		inEvent)
{
	bool result = true;

	if (DoClose())
		result = false;
	
	return result;
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
	
