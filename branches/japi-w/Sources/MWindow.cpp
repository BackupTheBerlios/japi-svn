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
MWindow* MWindow::sRecycle = nil;

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
	MResource rsrc;
	
	if (strcmp(inRootWidgetName, "dialog") == 0)
		rsrc = MResource::root().find(
			string("Dialogs/") + inWindowResourceName + ".glade");
	else
		rsrc = MResource::root().find(
			string("Windows/") + inWindowResourceName + ".glade");
	
	if (not rsrc)
		THROW(("Could not load dialog resource %s", inWindowResourceName));
	
	mGladeXML = glade_xml_new_from_buffer(rsrc.data(), rsrc.size(), nil, "japi");
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
	if (mGladeXML != nil)
	{
		glade_xml_signal_connect_data(GetGladeXML(), "on_changed", 
			G_CALLBACK(&MWindow::ChangedCallBack), this);
	}

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
	if (GDK_IS_WINDOW(gtk_widget_get_window(GetGtkWidget())) and
		gdk_window_is_visible(gtk_widget_get_window(GetGtkWidget())))
	{
		gtk_window_present(GTK_WINDOW(GetGtkWidget()));
	}
	else
		Show();

	TakeFocus();

//	// trick learned from EEL
//	gdk_error_trap_push();
//	XSetInputFocus(GDK_DISPLAY(),
//		GDK_WINDOW_XWINDOW(gtk_widget_get_window(GetGtkWidget())),
//		RevertToParent, GDK_CURRENT_TIME);
//	gdk_flush();
//	gdk_error_trap_pop ();
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
	
	// and put window in the queue to be recycled at next event
	mNext = sRecycle;
	sRecycle = this;
	
	return true;
}

void MWindow::RecycleWindows()
{
	MWindow* w = sRecycle;
	sRecycle = nil;

	while (w != nil)
	{
		MWindow* next = w->mNext;
		delete w;
		w = next;
	}
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

void MWindow::SetFocus(
	uint32				inID)
{
	gtk_widget_grab_focus(GetWidget(inID));
}

string MWindow::GetText(
	uint32				inID) const
{
	string result;
	
	GtkWidget* wdgt = GetWidget(inID);
	if (GTK_IS_COMBO_BOX(wdgt))
	{
		char* text = gtk_combo_box_get_active_text(GTK_COMBO_BOX(wdgt));
		if (text != nil)
		{
			result = text;
			g_free(text);
		}
	}
	else if (GTK_IS_FONT_BUTTON(wdgt))
		result = gtk_font_button_get_font_name(GTK_FONT_BUTTON(wdgt));
	else if (GTK_IS_ENTRY(wdgt))
		result = gtk_entry_get_text(GTK_ENTRY(wdgt));
	else if (GTK_IS_TEXT_VIEW(wdgt))
	{
		GtkTextBuffer* buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(wdgt));
		if (buffer == nil)
			THROW(("Invalid text buffer"));
		
		GtkTextIter start, end;
		gtk_text_buffer_get_bounds(buffer, &start, &end);
		gchar* text = gtk_text_buffer_get_text(buffer, &start, &end, false);
		
		if (text != nil)
		{
			result = text;
			g_free(text);
		}
	}
	else
		THROW(("item is not an entry"));
	
	return result;
}

void MWindow::SetText(
	uint32				inID,
	const std::string&	inText)
{
	GtkWidget* wdgt = GetWidget(inID);
	if (GTK_IS_COMBO_BOX(wdgt))
assert(false);//		gtk_combo_box_set_active_text(GTK_COMBO_BOX(wdgt), inText.c_str());
	else if (GTK_IS_FONT_BUTTON(wdgt))
		gtk_font_button_set_font_name(GTK_FONT_BUTTON(wdgt), inText.c_str());
	else if (GTK_IS_ENTRY(wdgt))
		gtk_entry_set_text(GTK_ENTRY(wdgt), inText.c_str());
	else if (GTK_IS_LABEL(wdgt))
		gtk_label_set_text(GTK_LABEL(wdgt), inText.c_str());
	else if (GTK_IS_BUTTON(wdgt))
		gtk_button_set_label(GTK_BUTTON(wdgt), inText.c_str());
	else if (GTK_IS_TEXT_VIEW(wdgt))
	{
		GtkTextBuffer* buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(wdgt));
		if (buffer == nil)
			THROW(("Invalid text buffer"));
		gtk_text_buffer_set_text(buffer, inText.c_str(), inText.length());
	}
	else if (GTK_IS_PROGRESS_BAR(wdgt))
	{
		gtk_progress_bar_set_text(GTK_PROGRESS_BAR(wdgt), inText.c_str());
		gtk_progress_bar_set_ellipsize(GTK_PROGRESS_BAR(wdgt),
			PANGO_ELLIPSIZE_MIDDLE);
	}
	else
		THROW(("item is not an entry"));
}

void MWindow::SetPasswordField(
	uint32				inID,
	bool				isVisible)
{
	GtkWidget* wdgt = GetWidget(inID);
	if (GTK_IS_ENTRY(wdgt))
		g_object_set(G_OBJECT(wdgt), "visibility", isVisible, nil);
	else
		THROW(("item is not an entry"));
}

int32 MWindow::GetValue(
	uint32				inID) const
{
	int32 result = 0;
	GtkWidget* wdgt = GetWidget(inID);
	
	if (GTK_IS_COMBO_BOX(wdgt))
		result = gtk_combo_box_get_active(GTK_COMBO_BOX(wdgt)) + 1;
	else
		THROW(("Cannot get value"));
		
	return result;
}

void MWindow::SetValue(
	uint32				inID,
	int32				inValue)
{
	GtkWidget* wdgt = GetWidget(inID);

	if (GTK_IS_COMBO_BOX(wdgt))
		gtk_combo_box_set_active(GTK_COMBO_BOX(wdgt), inValue - 1);
	else
		THROW(("Cannot get value"));
}

// for comboboxes
void MWindow::GetValues(
	uint32				inID,
	vector<string>& 	outValues) const
{
	assert(false);
}

void MWindow::SetValues(
	uint32				inID,
	const vector<string>&
						inValues)
{
	GtkWidget* wdgt = GetWidget(inID);

	char name[5];
	if (not GTK_IS_COMBO_BOX(wdgt))
		THROW(("Item %s is not a combo box", IDToName(inID, name)));

	GtkTreeModel* model = gtk_combo_box_get_model(GTK_COMBO_BOX(wdgt));
	int32 count = gtk_tree_model_iter_n_children(model, nil);

	while (count-- > 0)
		gtk_combo_box_remove_text(GTK_COMBO_BOX(wdgt), count);

	for (vector<string>::const_iterator s = inValues.begin(); s != inValues.end(); ++s)
		gtk_combo_box_append_text(GTK_COMBO_BOX(wdgt), s->c_str());

	gtk_combo_box_set_active(GTK_COMBO_BOX(wdgt), 0);
}

void MWindow::SetColor(
	uint32				inID,
	MColor				inColor)
{
	GtkWidget* wdgt = GetWidget(inID);
	if (not GTK_IS_COLOR_BUTTON(wdgt))
		THROW(("Widget '%d' is not of the correct type", inID));
	
	GdkColor c = inColor;
	gtk_color_button_set_color(GTK_COLOR_BUTTON(wdgt), &c);
}

MColor MWindow::GetColor(
	uint32				inID) const
{
	GtkWidget* wdgt = GetWidget(inID);
	if (not GTK_IS_COLOR_BUTTON(wdgt))
		THROW(("Widget '%d' is not of the correct type", inID));
		
	GdkColor c;
	gtk_color_button_get_color(GTK_COLOR_BUTTON(wdgt), &c);
	return MColor(c);
}

bool MWindow::IsChecked(
	uint32				inID) const
{
	GtkWidget* wdgt = GetWidget(inID);
	if (not GTK_IS_TOGGLE_BUTTON(wdgt))
		THROW(("Widget '%d' is not of the correct type", inID));
	return gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(wdgt));
}

void MWindow::SetChecked(
	uint32				inID,
	bool				inOn)
{
	GtkWidget* wdgt = GetWidget(inID);
	if (not GTK_IS_TOGGLE_BUTTON(wdgt))
		THROW(("Widget '%d' is not of the correct type", inID));
	return gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(wdgt), inOn);
}	

bool MWindow::IsVisible(
	uint32				inID) const
{
	GtkWidget* wdgt = GetWidget(inID);
	return GTK_WIDGET_VISIBLE(wdgt);
}

void MWindow::SetVisible(
	uint32				inID,
	bool				inVisible)
{
	GtkWidget* wdgt = GetWidget(inID);
	if (inVisible)
		gtk_widget_show(wdgt);
	else
		gtk_widget_hide(wdgt);
}

bool MWindow::IsEnabled(
	uint32				inID) const
{
	GtkWidget* wdgt = GetWidget(inID);
	return GTK_WIDGET_IS_SENSITIVE(wdgt);
}

void MWindow::SetEnabled(
	uint32				inID,
	bool				inEnabled)
{
	GtkWidget* wdgt = GetWidget(inID);
	gtk_widget_set_sensitive(wdgt, inEnabled);
}

bool MWindow::IsExpanded(
	uint32				inID) const
{
	GtkWidget* wdgt = GetWidget(inID);
	assert(GTK_IS_EXPANDER(wdgt));
	return gtk_expander_get_expanded(GTK_EXPANDER(wdgt));
}

void MWindow::SetExpanded(
	uint32				inID,
	bool				inExpanded)
{
	GtkWidget* wdgt = GetWidget(inID);
	assert(GTK_IS_EXPANDER(wdgt));
	gtk_expander_set_expanded(GTK_EXPANDER(wdgt), inExpanded);
}

void MWindow::SetProgressFraction(
	uint32				inID,
	float				inFraction)
{
	GtkWidget* wdgt = GetWidget(inID);
	assert(GTK_IS_PROGRESS_BAR(wdgt));
	gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(wdgt), inFraction);
}

void MWindow::ValueChanged(
	uint32				inID)
{
//	char name[5];
//	cout << "Value Changed for " << IDToName(inID, name) << endl;
}

void MWindow::ChangedCallBack(
	GtkWidget*			inWidget,
	gpointer			inUserData)
{
	MWindow* self = reinterpret_cast<MWindow*>(inUserData);
	
	const char* name = glade_get_widget_name(inWidget);
	if (name != nil)
	{
		uint32 id = 0;
		for (uint32 i = 0; i < 4 and name[i]; ++i)
			id = (id << 8) | name[i];
		
		try
		{
			self->ValueChanged(id);
		}
		catch (exception& e)
		{
			DisplayError(e);
		}
		catch (...) {}
	}
}

