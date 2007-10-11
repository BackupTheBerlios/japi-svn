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
	: mGtkWidget(inWidget)
	, mFocusInEvent(this, &MView::OnFocusInEvent)
	, mFocusOutEvent(this, &MView::OnFocusOutEvent)
{
	if (inCanActivate)
	{
//		int m = gdk_window_get_events(GetGtkWidget()->window);
//		m |= GDK_FOCUS_CHANGE_MASK;
//		gdk_window_set_events(GetGtkWidget()->window, (GdkEventMask)m);

		GTK_WIDGET_SET_FLAGS(mGtkWidget, GTK_CAN_FOCUS);
		
		mFocusInEvent.Connect(mGtkWidget, "focus-in-event");
		mFocusOutEvent.Connect(mGtkWidget, "focus-out-event");
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
	outBounds = mGtkWidget->allocation;
}

void MView::SetBounds(
	const MRect&	inBounds)
{
}

void MView::ResizeTo(
	uint32			inWidth,
	uint32			inHeight)
{
	gtk_widget_set_size_request(GetGtkWidget(), inWidth, inHeight);
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

void MView::Add(
	MView*			inSubView)
{
	assert(GTK_IS_CONTAINER(GetGtkWidget()));
	gtk_container_add(GTK_CONTAINER(GetGtkWidget()), inSubView->GetGtkWidget());
}

bool MView::OnFocusInEvent(
	GdkEventFocus*	inEvent)
{
	return true;
}

bool MView::OnFocusOutEvent(
	GdkEventFocus*	inEvent)
{
	return true;
}

bool MView::IsActive() const
{
	return GTK_WIDGET_HAS_FOCUS(mGtkWidget);
}
