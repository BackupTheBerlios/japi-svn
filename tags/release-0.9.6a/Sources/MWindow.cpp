//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "MJapi.h"

#include <iostream>
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <glade/glade-xml.h>
#include <gdk/gdkx.h>

#include "MCommands.h"
#include "MWindow.h"
#include "MResources.h"
#include "MError.h"
#include "MJapiApp.h"

using namespace std;

MWindow* MWindow::sFirst = nil;

MWindow::MWindow()
	: MView(gtk_window_new(GTK_WINDOW_TOPLEVEL), false)
	, MHandler(gApp)
	, mOnDestroy(this, &MWindow::OnDestroy)
	, mOnDelete(this, &MWindow::OnDelete)
	, mModified(false)
	, mTransitionThread(nil)
	, mGladeXML(nil)
	, mChildFocus(this, &MWindow::ChildFocus)
{
	Init();
}

MWindow::MWindow(
	GtkWidget*		inWindow)
	: MView(inWindow, false)
	, MHandler(gApp)
	, mOnDestroy(this, &MWindow::OnDestroy)
	, mOnDelete(this, &MWindow::OnDelete)
	, mModified(false)
	, mTransitionThread(nil)
	, mGladeXML(nil)
	, mChildFocus(this, &MWindow::ChildFocus)
{
	Init();
}

MWindow::MWindow(
	const char*		inWindowResourceName,
	const char*		inRootWidgetName)
	: MHandler(gApp)
	, mOnDestroy(this, &MWindow::OnDestroy)
	, mOnDelete(this, &MWindow::OnDelete)
	, mModified(false)
	, mTransitionThread(nil)
	, mGladeXML(nil)
	, mChildFocus(this, &MWindow::ChildFocus)
{
	const char* xml;
	uint32 size;
	
	string rsrc;
	
	if (strcmp(inRootWidgetName, "dialog") == 0)
		rsrc = string("Dialogs/") + inWindowResourceName + ".glade";
	else
		rsrc = string("Windows/") + inWindowResourceName + ".glade";
	
	if (not LoadResource(rsrc, xml, size))
		THROW(("Could not load dialog resource %s", inWindowResourceName));
	
	mGladeXML = glade_xml_new_from_buffer(xml, size, nil, "japi");
	if (mGladeXML == nil)
		THROW(("Failed to create glade from resource"));
	
	GtkWidget* w = glade_xml_get_widget(mGladeXML, inRootWidgetName);
	if (w == nil)
		THROW(("Failed to extract root widget from glade (%s)", inRootWidgetName));
	
	SetWidget(w, false, false);

	gtk_container_foreach(GTK_CONTAINER(w), &MWindow::DoForEachCallBack, this);

	Init();
}

void MWindow::Init()
{
	mOnDestroy.Connect(GetGtkWidget(), "destroy");
	mOnDelete.Connect(GetGtkWidget(), "delete_event");

	mNext = sFirst;
	sFirst = this;
}

MWindow::~MWindow()
{
	if (mGladeXML != nil)
		g_object_unref(mGladeXML);

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

void MWindow::ConnectChildSignals()
{
	gtk_container_foreach(GTK_CONTAINER(GetGtkWidget()), &MWindow::DoForEachCallBack, this);
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
	gtk_widget_show(GetGtkWidget());
}

void MWindow::Hide()
{
	gtk_widget_hide(GetGtkWidget());
}

void MWindow::Select()
{
	gtk_window_present(GTK_WINDOW(GetGtkWidget()));

	TakeFocus();
	
	// trick learned from EEL
	gdk_error_trap_push();
	XSetInputFocus(GDK_DISPLAY(),
		GDK_WINDOW_XWINDOW(gtk_widget_get_window(GetGtkWidget())),
		RevertToParent, GDK_CURRENT_TIME);
	gdk_flush();
	gdk_error_trap_pop ();
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
	bool result = true;	
	
	switch (inCommand)
	{
		case cmd_Close:
			outEnabled = true;
			break;
		
		default:
			result = MHandler::UpdateCommandStatus(
				inCommand, inMenu, inItemIndex, outEnabled, outChecked);
	}
	
	return result;
}

bool MWindow::ProcessCommand(
	uint32			inCommand,
	const MMenu*	inMenu,
	uint32			inItemIndex,
	uint32			inModifiers)
{
	bool result = true;	
	
	switch (inCommand)
	{
		case cmd_Close:
			Close();
			break;
		
		default:
			result = MHandler::ProcessCommand(inCommand, inMenu, inItemIndex, inModifiers);
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
		kSteps = 6;
	
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

const char* MWindow::IDToName(
	uint32			inID,
	char			inName[5])
{
	inName[4] = 0;
	inName[3] = inID & 0x000000ff; inID >>= 8;
	inName[2] = inID & 0x000000ff; inID >>= 8;
	inName[1] = inID & 0x000000ff; inID >>= 8;
	inName[0] = inID & 0x000000ff;
	
	return inName;
}

GtkWidget* MWindow::GetWidget(
	uint32			inID) const
{
	char name[5];
	GtkWidget* wdgt = glade_xml_get_widget(GetGladeXML(), IDToName(inID, name));
	if (wdgt == nil)
		THROW(("Widget '%s' does not exist", name));
	return wdgt;
}

void MWindow::Beep()
{
//	gdk_window_beep(GetGtkWidget()->window);
	cout << "beep!" << endl;
	gdk_beep();
}

void MWindow::DoForEachCallBack(
	GtkWidget*			inWidget,
	gpointer			inUserData)
{
	MWindow* w = reinterpret_cast<MWindow*>(inUserData);
	w->DoForEach(inWidget);
}

void MWindow::DoForEach(
	GtkWidget*			inWidget)
{
	gboolean canFocus = false;

	g_object_get(G_OBJECT(inWidget), "can-focus", &canFocus, NULL);

	if (canFocus)
		mChildFocus.Connect(inWidget, "focus-in-event");
	
	if (GTK_IS_CONTAINER(inWidget))
		gtk_container_foreach(GTK_CONTAINER(inWidget), &MWindow::DoForEachCallBack, this);
}

bool MWindow::ChildFocus(
	GdkEventFocus*		inEvent)
{
	try
	{
		TakeFocus();
		FocusChanged(0);
	}
	catch (...) {}
	return false;
}

void MWindow::FocusChanged(
	uint32				inFocussedID)
{
}

void MWindow::PutOnDuty(
	MHandler*		inHandler)
{
	MWindow* w = sFirst;
	while (w != nil)
	{
		if (w == this)
		{
			RemoveWindowFromList(this);

			mNext = sFirst;
			sFirst = this;

			break;
		}
		w = w->mNext;
	}
}
