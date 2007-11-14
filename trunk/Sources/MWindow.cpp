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
	, mModified(false)
{
	mOnDestroy.Connect(GetGtkWidget(), "destroy");
	mOnDelete.Connect(GetGtkWidget(), "delete_event");
}

MWindow::MWindow(
	GtkWidget*		inWindow)
	: MView(inWindow, false)
	, MHandler(gApp)
	, mOnDestroy(this, &MWindow::OnDestroy)
	, mOnDelete(this, &MWindow::OnDelete)
	, mModified(false)
{
	mOnDestroy.Connect(GetGtkWidget(), "destroy");
	mOnDelete.Connect(GetGtkWidget(), "delete_event");
}

MWindow::~MWindow()
{
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
	mTitle = inTitle;
	
	if (mModified)
		gtk_window_set_title(GTK_WINDOW(GetGtkWidget()), (mTitle + " *").c_str());
	else
		gtk_window_set_title(GTK_WINDOW(GetGtkWidget()), mTitle.c_str());
}

string MWindow::GetTitle() const
{
	return mTitle;
}

void MWindow::SetModifiedMarkInTitle(
	bool		inModified)
{
	if (mModified != inModified)
	{
		mModified = inModified;
		SetTitle(mTitle);
	}
}

bool MWindow::OnDestroy()
{
	eWindowClosed(this);	
	
	gApp->RecycleWindow(this);
	return true;
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
	uint32			inCommand,
	const MMenu*	inMenu,
	uint32			inItemIndex)
{
	bool result = true;	
	
	switch (inCommand)
	{
		case cmd_Close:
			Close();
			break;
		
		default:
			result = MHandler::ProcessCommand(inCommand, inMenu, inItemIndex);
			break;
	}
	
	return result;
}

