/*	$Id$
	Copyright Drs M.L. Hekkelman
	Created 28-09-07 11:18:30
*/

#include "MJapieG.h"

#include <iostream>
#include <cassert>

#include "MView.h"

using namespace std;

MView::MView(
	GtkWidget*		inWidget,
	bool			inCanActivate)
	: mFocusInEvent(this, &MView::OnFocusInEvent)
	, mFocusOutEvent(this, &MView::OnFocusOutEvent)
	, mButtonPressEvent(this, &MView::OnButtonPressEvent)
	, mMotionNotifyEvent(this, &MView::OnMotionNotifyEvent)
	, mButtonReleaseEvent(this, &MView::OnButtonReleaseEvent)
	, mKeyPressEvent(this, &MView::OnKeyPressEvent)
	, mConfigureEvent(this, &MView::OnConfigureEvent)
	, mScrollEvent(this, &MView::OnScrollEvent)
	, mRealize(this, &MView::OnRealize)
	, mGtkWidget(nil)
{
	SetWidget(inWidget, inCanActivate);
}

MView::MView()
	: mFocusInEvent(this, &MView::OnFocusInEvent)
	, mFocusOutEvent(this, &MView::OnFocusOutEvent)
	, mButtonPressEvent(this, &MView::OnButtonPressEvent)
	, mMotionNotifyEvent(this, &MView::OnMotionNotifyEvent)
	, mButtonReleaseEvent(this, &MView::OnButtonReleaseEvent)
	, mKeyPressEvent(this, &MView::OnKeyPressEvent)
	, mConfigureEvent(this, &MView::OnConfigureEvent)
	, mScrollEvent(this, &MView::OnScrollEvent)
	, mRealize(this, &MView::OnRealize)
	, mGtkWidget(nil)
{
}

void MView::SetWidget(
	GtkWidget*		inWidget,
	bool			inCanActivate)
{
	assert(mGtkWidget == nil);
	
	mGtkWidget = inWidget;
	
	if (inCanActivate)
	{
		GTK_WIDGET_SET_FLAGS(mGtkWidget, GTK_CAN_FOCUS);
		
		mFocusInEvent.Connect(mGtkWidget, "focus-in-event");
		mFocusOutEvent.Connect(mGtkWidget, "focus-out-event");
		mButtonPressEvent.Connect(mGtkWidget, "button-press-event");
		mMotionNotifyEvent.Connect(mGtkWidget, "motion-notify-event");
		mButtonReleaseEvent.Connect(mGtkWidget, "button-release-event");
		mKeyPressEvent.Connect(mGtkWidget, "key-press-event");
		mConfigureEvent.Connect(mGtkWidget, "configure-event");
		mScrollEvent.Connect(mGtkWidget, "scroll-event");
		mRealize.Connect(mGtkWidget, "realize");
	}
}

MView::~MView()
{
}

MWindow* MView::GetWindow() const
{
	return nil;
}

void MView::GetBounds(
	MRect&			outBounds) const
{
	GtkWidget* parent = gtk_widget_get_parent(mGtkWidget);
	if (GTK_IS_VIEWPORT(parent))
	{
		outBounds = parent->allocation;
		
		outBounds.x = outBounds.y = 0;
		
		gtk_widget_translate_coordinates(parent, mGtkWidget,
			outBounds.x, outBounds.y,
			&outBounds.x, &outBounds.y);
	}
	else
	{
		outBounds = mGtkWidget->allocation;
		outBounds.x = outBounds.y = 0;
	}
}

void MView::SetBounds(
	const MRect&	inBounds)
{
	GtkRequisition r = { inBounds.width, inBounds.height };
	gtk_widget_size_request(mGtkWidget, &r);
}

void MView::ResizeTo(
	uint32			inWidth,
	uint32			inHeight)
{
	gtk_widget_set_size_request(GetGtkWidget(), inWidth, inHeight);
}

void MView::ConvertToGlobal(
	int32&			ioX,
	int32&			ioY)
{
	GtkWidget* w = mGtkWidget; 
	GtkWidget* toplevel = gtk_widget_get_toplevel(mGtkWidget);
	if (GTK_WIDGET_TOPLEVEL(toplevel))
	{
		gtk_widget_translate_coordinates(mGtkWidget, toplevel, ioX, ioY, &ioX, &ioY);
		w = toplevel;
	}

	int32 ox, oy;
	
	gdk_window_get_position(w->window, &ox, &oy);
	ioX += ox;
	ioY += oy;
}

void MView::Invalidate()
{
	gtk_widget_queue_draw(mGtkWidget);
}

void MView::Invalidate(
	const MRect&	inRect)
{
	gtk_widget_queue_draw_area(mGtkWidget,
		inRect.x, inRect.y, inRect.width, inRect.height);
}

void MView::UpdateNow()
{
	GtkWidget* w = mGtkWidget; 
	GtkWidget* toplevel = gtk_widget_get_toplevel(mGtkWidget);

	if (GTK_WIDGET_TOPLEVEL(toplevel))
		w = toplevel;

	gdk_window_process_all_updates();
}

void MView::Add(
	MView*			inSubView)
{
	assert(GTK_IS_CONTAINER(GetGtkWidget()));
	gtk_container_add(GTK_CONTAINER(GetGtkWidget()), inSubView->GetGtkWidget());
	inSubView->Added();
}

void MView::Added()
{
}

bool MView::OnRealize()
{
	int m = gdk_window_get_events(GetGtkWidget()->window);

	m |= GDK_FOCUS_CHANGE_MASK | GDK_STRUCTURE_MASK |
		GDK_KEY_PRESS_MASK |
		GDK_BUTTON_PRESS_MASK | GDK_POINTER_MOTION_MASK | GDK_BUTTON_RELEASE_MASK;
	gdk_window_set_events(GetGtkWidget()->window, (GdkEventMask)m);
	
	return false;
}

bool MView::OnFocusInEvent(
	GdkEventFocus*	inEvent)
{
	return false;
}

bool MView::OnFocusOutEvent(
	GdkEventFocus*	inEvent)
{
	return false;
}

bool MView::IsActive() const
{
	return GTK_WIDGET_HAS_FOCUS(mGtkWidget);
}

bool MView::OnButtonPressEvent(
	GdkEventButton*	inEvent)
{
	return false;
}

bool MView::OnMotionNotifyEvent(
	GdkEventMotion*	inEvent)
{
	return false;
}

bool MView::OnKeyPressEvent(
	GdkEventKey*	inEvent)
{
	return false;
}

bool MView::OnButtonReleaseEvent(
	GdkEventButton*	inEvent)
{
	return false;
}

bool MView::OnConfigureEvent(
	GdkEventConfigure*	inEvent)
{
	return false;
}

bool MView::OnScrollEvent(
	GdkEventScroll*	inEvent)
{
	return false;
}
