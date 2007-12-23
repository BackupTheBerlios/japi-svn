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

/*	$Id$
	Copyright Drs M.L. Hekkelman
	Created 28-09-07 11:18:30
*/

#include "MJapieG.h"

#include <iostream>
#include <cassert>

#include "MView.h"
#include "MUtils.h"

using namespace std;

namespace
{

GdkCursor* gGdkCursors[eCursorCount];

}

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
	, mExposeEvent(this, &MView::OnExposeEvent)
	, mDragDataReceived(this, &MView::OnDragDataReceived)
	, mDragMotion(this, &MView::OnDragMotion)
	, mDragLeave(this, &MView::OnDragLeave)
	, mDragDataDelete(this, &MView::OnDragDataDelete)
	, mDragDataGet(this, &MView::OnDragDataGet)
	, mGtkWidget(nil)
	, mPangoContext(nil)
{
	SetWidget(inWidget, inCanActivate, false);
}

MView::MView(
	int32		inWidth,
	int32		inHeight)
	: mFocusInEvent(this, &MView::OnFocusInEvent)
	, mFocusOutEvent(this, &MView::OnFocusOutEvent)
	, mButtonPressEvent(this, &MView::OnButtonPressEvent)
	, mMotionNotifyEvent(this, &MView::OnMotionNotifyEvent)
	, mButtonReleaseEvent(this, &MView::OnButtonReleaseEvent)
	, mKeyPressEvent(this, &MView::OnKeyPressEvent)
	, mConfigureEvent(this, &MView::OnConfigureEvent)
	, mScrollEvent(this, &MView::OnScrollEvent)
	, mRealize(this, &MView::OnRealize)
	, mExposeEvent(this, &MView::OnExposeEvent)
	, mDragDataReceived(this, &MView::OnDragDataReceived)
	, mDragMotion(this, &MView::OnDragMotion)
	, mDragLeave(this, &MView::OnDragLeave)
	, mDragDataDelete(this, &MView::OnDragDataDelete)
	, mDragDataGet(this, &MView::OnDragDataGet)
	, mGtkWidget(nil)
	, mPangoContext(nil)
{
	SetWidget(gtk_drawing_area_new(), true, true);
	
	if (inWidth > 0 or inHeight > 0)
		gtk_widget_set_size_request(mGtkWidget, inWidth, inHeight);
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
	, mExposeEvent(this, &MView::OnExposeEvent)
	, mDragDataReceived(this, &MView::OnDragDataReceived)
	, mDragMotion(this, &MView::OnDragMotion)
	, mDragLeave(this, &MView::OnDragLeave)
	, mDragDataDelete(this, &MView::OnDragDataDelete)
	, mDragDataGet(this, &MView::OnDragDataGet)
	, mGtkWidget(nil)
	, mPangoContext(nil)
{
}

void MView::SetWidget(
	GtkWidget*		inWidget,
	bool			inCanActivate,
	bool			inCanDraw)
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
	
	if (inCanDraw)
		mExposeEvent.Connect(mGtkWidget, "expose-event");
}

MView::~MView()
{
	if (mPangoContext != nil)
		g_object_unref(mPangoContext);
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
	int32			inWidth,
	int32			inHeight)
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

void MView::Scroll(
	int32			inX,
	int32			inY)
{
	if (GDK_IS_WINDOW(mGtkWidget->window))
	{
//		UpdateNow();
	
		gdk_window_scroll(mGtkWidget->window, inX, inY);
	}
}

void MView::Scroll(
	const MRect&	inRect,
	int32			inX,
	int32			inY)
{
	if (GDK_IS_WINDOW(mGtkWidget->window))
	{
//		UpdateNow();
		
		MRect b;
		GetBounds(b);

		GdkRectangle gr = { inRect.x, inRect.y, inRect.width, inRect.height };

		GdkRegion* rgn = gdk_region_rectangle(&gr);
		gdk_window_move_region(mGtkWidget->window, rgn, inX, inY);
		gdk_region_destroy(rgn);
	}
}

void MView::UpdateNow()
{
	GtkWidget* w = mGtkWidget; 
	GtkWidget* toplevel = gtk_widget_get_toplevel(mGtkWidget);

	if (GTK_WIDGET_TOPLEVEL(toplevel))
		w = toplevel;

	gdk_window_process_all_updates();
}

void MView::SetCursor(
	MCursor			inCursor)
{
	assert(inCursor < eCursorCount);

	if (gGdkCursors[inCursor] == nil)
	{
		switch (inCursor)
		{
			case eNormalCursor:
				gGdkCursors[inCursor] = gdk_cursor_new(GDK_LEFT_PTR);
				break;
			
			case eIBeamCursor:	
				gGdkCursors[inCursor] = gdk_cursor_new(GDK_XTERM);
				break;
			
			case eRightCursor:
				gGdkCursors[inCursor] = gdk_cursor_new(GDK_RIGHT_PTR);
				break;
			
			case eBlankCursor:
			{
				GdkPixmap* pixmap = gdk_pixmap_new(nil, 1, 1, 1);
				GdkColor c = {};
				
				gGdkCursors[inCursor] = gdk_cursor_new_from_pixmap(
					pixmap, pixmap, &c, &c, 0, 0);
				gdk_pixmap_unref(pixmap);
				break;
			}
			
			default:
				assert(false);
		}
	}
	
	if (GDK_IS_WINDOW(mGtkWidget->window) and gGdkCursors[inCursor] != nil)
		gdk_window_set_cursor(mGtkWidget->window, gGdkCursors[inCursor]);
}

void MView::ObscureCursor()
{
	SetCursor(eBlankCursor);
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

bool MView::OnExposeEvent(
	GdkEventExpose*	inEvent)
{
	return false;
}

// Drag and Drop support

void MView::SetupDragAndDrop(
	const GtkTargetEntry	inTargets[],
	uint32					inTargetCount)
{
	gtk_drag_dest_set(mGtkWidget, GTK_DEST_DEFAULT_ALL,
		inTargets, inTargetCount,
		GdkDragAction(GDK_ACTION_COPY|GDK_ACTION_MOVE));
	
	mDragDataReceived.Connect(mGtkWidget, "drag-data-received");
	mDragMotion.Connect(mGtkWidget, "drag-motion");
	mDragLeave.Connect(mGtkWidget, "drag-leave");
	
	mDragDataGet.Connect(mGtkWidget, "drag-data-get");
	mDragDataDelete.Connect(mGtkWidget, "drag-data-delete");
	
	mDragWithin = false;
}

void MView::OnDragDataReceived(
	GdkDragContext*		inDragContext,
	gint				inX,
	gint				inY,
	GtkSelectionData*	inData,
	guint				inInfo,
	guint				inTime)
{
	bool ok = false;
	bool del = false;
	bool move = inDragContext->action == GDK_ACTION_MOVE;
	
	if (inData->length >= 0)
	{
		ok = DragAccept(
			move,
			inX, inY,
			reinterpret_cast<const char*>(inData->data), inData->length,
			inInfo);
		del = ok and move and gtk_drag_get_source_widget(inDragContext) != mGtkWidget;
	}

	gtk_drag_finish(inDragContext, ok, del, inTime);
}

bool MView::OnDragMotion(
	GdkDragContext*	inDragContext,
	gint			inX,
	gint			inY,
	guint			inTime)
{
	if (not mDragWithin)
	{
		DragEnter();
		mDragWithin = true;
	}
	
	bool copy =
		IsModifierDown(GDK_SHIFT_MASK) or
		mGtkWidget != gtk_drag_get_source_widget(inDragContext);
	
	DragWithin(inX, inY);
	
	gdk_drag_status(inDragContext, copy ? GDK_ACTION_COPY : GDK_ACTION_MOVE, inTime);

	return false;
}

void MView::OnDragLeave(
	GdkDragContext*	inDragContext,
	guint			inTime)
{
	mDragWithin = false;
	DragLeave();
}

void MView::OnDragDataDelete(
	GdkDragContext*	inDragContext)
{
	DragDeleteData();
}

void MView::OnDragDataGet(
	GdkDragContext*		inDragContext,
	GtkSelectionData*	inData,
	guint				inInfo,
	guint				inTime)
{
	string data;
	
	DragSendData(data);
	
	gtk_selection_data_set_text(inData, data.c_str(), data.length());
}

void MView::DragEnter()
{
}
	
void MView::DragWithin(
	int32			inX,
	int32			inY)
{
}

void MView::DragLeave()
{
}

bool MView::DragAccept(
	bool			inMove,
	int32			inX,
	int32			inY,
	const char*		inData,
	uint32			inLength,
	uint32			inType)
{
	return false;
}

void MView::DragBegin(
	const GtkTargetEntry	inTargets[],
	uint32					inTargetCount,
	GdkEventMotion*			inEvent)
{
	int button = 1;
	
	GtkTargetList* lst = gtk_target_list_new(inTargets, inTargetCount);
	
//	GdkDragAction action = GDK_ACTION_MOVE;
//	if (inEvent->state & GDK_SHIFT_MASK)
//		action = GDK_ACTION_COPY;
//	
	GdkDragContext* context = gtk_drag_begin(
		mGtkWidget, lst, GdkDragAction(GDK_ACTION_MOVE|GDK_ACTION_COPY),
		button, (GdkEvent*)inEvent);

	gtk_drag_set_icon_default(context);

	gtk_target_list_unref(lst);
}

void MView::DragSendData(
	string&		outData)
{
}

void MView::DragDeleteData()
{
}

void MView::GetMouse(
	int32&			outX,
	int32&			outY) const
{
	gtk_widget_get_pointer(mGtkWidget, &outX, &outY);
}

uint32 MView::GetModifiers() const
{
	GdkModifierType modifiers;
	gdk_window_get_pointer(mGtkWidget->window, nil, nil, &modifiers);
	return modifiers & gtk_accelerator_get_default_mod_mask();
}
