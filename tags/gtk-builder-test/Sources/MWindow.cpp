/*
	Copyright (c) 2007, Maarten L. Hekkelman
	All rights reserved.

	Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions
	are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in
      the documentation and/or other materials provided with the
      distribution.
    * Neither the name of the Maarten L. Hekkelman nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
	"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
	LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
	FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
	COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
	INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
	(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
	SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
	HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
	STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
	ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
	OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "MJapieG.h"

#include <iostream>
#include <boost/thread.hpp>
#include <boost/bind.hpp>

#include "MCommands.h"
#include "MWindow.h"
#include "MResources.h"
#include "MError.h"

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
	, mGladeXML(nil)
	, mChildFocus(this, &MWindow::ChildFocus)
{
	mOnDestroy.Connect(GetGtkWidget(), "destroy");
	mOnDelete.Connect(GetGtkWidget(), "delete_event");

	mNext = sFirst;
	sFirst = this;
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
	
	if (not LoadResource(inWindowResourceName, xml, size))
		THROW(("Could not load dialog resource %s", inWindowResourceName));
	
	mGladeXML = glade_xml_new_from_buffer(xml, size, nil, "japi");
	if (mGladeXML == nil)
		THROW(("Failed to create glade from resource"));
	
	GtkWidget* w = glade_xml_get_widget(mGladeXML, inRootWidgetName);
	if (w == nil)
		THROW(("Failed to extract root widget from glade (%s)", inRootWidgetName));
	
	SetWidget(w, false, false);

	gtk_container_foreach(GTK_CONTAINER(w), &MWindow::DoForEachCallBack, this);
	
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
	gtk_widget_show(GetGtkWidget());
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
