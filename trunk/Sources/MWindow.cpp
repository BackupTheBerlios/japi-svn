#include "MJapieG.h"

#include <iostream>
#include <boost/thread.hpp>
#include <boost/bind.hpp>

#include "MCommands.h"
#include "MWindow.h"

using namespace std;

MWindow* MWindow::sFirst = nil;

MWindow::MWindow()
	: MView(gtk_window_new(GTK_WINDOW_TOPLEVEL), false)
	, MHandler(gApp)
	, mOnDestroy(this, &MWindow::OnDestroy)
	, mOnDelete(this, &MWindow::OnDelete)
	, mModified(false)
	, mTransitionThread(nil)
{
	mOnDestroy.Connect(GetGtkWidget(), "destroy");
	mOnDelete.Connect(GetGtkWidget(), "delete_event");

	mNext = sFirst;
	sFirst = this;
}

MWindow::MWindow(
	GtkWidget*		inWindow)
	: MView(inWindow, false)
	, MHandler(gApp)
	, mOnDestroy(this, &MWindow::OnDestroy)
	, mOnDelete(this, &MWindow::OnDelete)
	, mModified(false)
	, mTransitionThread(nil)
{
	mOnDestroy.Connect(GetGtkWidget(), "destroy");
	mOnDelete.Connect(GetGtkWidget(), "delete_event");

	mNext = sFirst;
	sFirst = this;
}

MWindow::~MWindow()
{
#if DEBUG
	MWindow* w = sFirst;
	while (w != nil)
	{
		if (w == this)
		{
			if (GTK_IS_WINDOW(GetGtkWidget()))
				PRINT(("Window was not removed from list: %s", gtk_window_get_title(GTK_WINDOW(GetGtkWidget()))));
			else
				PRINT(("Window was not removed from list: [deleted]"));

			RemoveWindowFromList(this);

			break;
		}
		w = w->mNext;
	}
#endif
}

void MWindow::RemoveWindowFromList(
	MWindow*		inWindow)
{
	if (inWindow == sFirst)
		sFirst = inWindow->mNext;
	else if (sFirst != nil)
	{
		MWindow* w = sFirst;
		while (w != nil)
		{
			MWindow* next = w->mNext;
			if (next == inWindow)
			{
				w->mNext = inWindow->mNext;
				break;
			}
			w = next;
		}
	}
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
	RemoveWindowFromList(this);

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
	MMenu*			inMenu,
	uint32			inItemIndex,
	bool&			outEnabled,
	bool&			outChecked)
{
	return MHandler::UpdateCommandStatus(
		inCommand, inMenu, inItemIndex, outEnabled, outChecked);
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

void MWindow::GetWindowPosition(
	MRect&			outPosition)
{
	int x, y;
	gtk_window_get_position(GTK_WINDOW(GetGtkWidget()), &x, &y);
	
	int w, h;
	gtk_window_get_size(GTK_WINDOW(GetGtkWidget()), &w, &h);
	
	outPosition = MRect(x, y, w, h);
}

void MWindow::SetWindowPosition(
	const MRect&	inPosition,
	bool			inTransition)
{
	if (inTransition)
	{
		if (mTransitionThread != nil)
			THROW(("SetWindowPosition called to fast"));
		
		mTransitionThread =
			new boost::thread(boost::bind(&MWindow::TransitionTo, this, inPosition));
	}
	else
	{
		gtk_window_move(GTK_WINDOW(GetGtkWidget()),
			inPosition.x, inPosition.y);
	
		gtk_window_resize(GTK_WINDOW(GetGtkWidget()),
			inPosition.width, inPosition.height);
	}
}

// try to be nice to those with multiple monitors:

void MWindow::GetMaxPosition(
	MRect&			outRect) const
{
	GdkScreen* screen = gtk_widget_get_screen(GetGtkWidget());
	
	uint32 monitor = gdk_screen_get_monitor_at_window(screen, GetGtkWidget()->window);
	
	GdkRectangle r;
	gdk_screen_get_monitor_geometry(screen, monitor, &r);
	outRect = r;
}

void MWindow::TransitionTo(
	MRect			inPosition)
{
	MRect start;

	gdk_threads_enter();
	GetWindowPosition(start);
	gdk_threads_leave();
	
	uint32
		kSleep = 10000,
		kSteps = 10;
	
	for (uint32 step = 0; step < kSteps; ++step)
	{
		MRect r;
		
		r.x = ((kSteps - step) * start.x + step * inPosition.x) / kSteps;
		r.y = ((kSteps - step) * start.y + step * inPosition.y) / kSteps;
		r.width = ((kSteps - step) * start.width + step * inPosition.width) / kSteps;
		r.height = ((kSteps - step) * start.height + step * inPosition.height) / kSteps;

		gdk_threads_enter();
		SetWindowPosition(r, false);
		gdk_window_process_all_updates();
		gdk_threads_leave();
		
		usleep(kSleep);
	}

	gdk_threads_enter();
	SetWindowPosition(inPosition, false);
	gdk_threads_leave();
	
	mTransitionThread = nil;
}
