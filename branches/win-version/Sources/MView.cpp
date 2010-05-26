//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

/*	$Id$
	Copyright Drs M.L. HekkelmanCreated 28-09-07 11:18:30
*/

#include "MLib.h"

#include <boost/foreach.hpp>
#define foreach BOOST_FOREACH

#include <iostream>
#include <cassert>

#include "MWindow.h"
#include "MError.h"
#include "MUtils.h"
#include "MDevice.h"

using namespace std;

MView::MView(
	uint32			inID,
	MRect			inBounds)
	: mID(inID)
	, mBounds(inBounds)
	, mFrame(inBounds)
	, mParent(nil)
	, mWillDraw(true)
	, mActive(eTriStateOn)
	, mVisible(eTriStateOn)
	, mEnabled(eTriStateOn)
	, mBindLeft(true)
	, mBindTop(true)
	, mBindRight(true)
	, mBindBottom(true)
{
}

MView::~MView()
{
	while (mChildren.size() > 0)
	{
		MView* v = mChildren.back();
		v->mParent = nil;
		mChildren.pop_back();
		delete v;
	}

	if (mParent != nil)
		mParent->RemoveChild(this);
}

MView* MView::GetParent() const
{
	return mParent;
}

void MView::SetParent(
	MView*			inParent)
{
	mParent = inParent;
}

void MView::AddChild(MView* inView)
{
	mChildren.push_back(inView);
	inView->mParent = this;

	if (mEnabled == eTriStateOn)
		inView->SuperEnable();
	else
		inView->SuperDisable();

	if (mActive == eTriStateOn)
		inView->SuperActivate();
	else
		inView->SuperDeactivate();

	if (mVisible == eTriStateOn)
		inView->SuperShow();
	else
		inView->SuperHide();
	
	if (GetWindow() != nil)
		inView->AddedToWindow();
}

void MView::AddedToWindow()
{
	foreach (MView* child, mChildren)
		child->AddedToWindow();
}

void MView::RemoveChild(MView* inView)
{
	MViewList::iterator i = std::find(mChildren.begin(), mChildren.end(), inView);
	if (i != mChildren.end())
	{
		MView* child = *i;
		mChildren.erase(i);
		child->mParent = nil;
	}
}

MWindow* MView::GetWindow() const
{
	MWindow* result = nil;
	if (mParent != nil)
		result = mParent->GetWindow();
	return result;
}

void MView::GetBounds(
	MRect&			outBounds) const
{
	outBounds = mBounds;
}

void MView::SetBounds(
	const MRect&	inBounds)
{
	mBounds = inBounds;
}

void MView::GetFrame(
	MRect&			outFrame) const
{
	outFrame = mFrame;
}

void MView::SetFrame(
	const MRect&	inFrame)
{
	mFrame = inFrame;
}

void MView::ResizeFrame(
	int32			inXDelta,
	int32			inYDelta,
	int32			inWidthDelta,
	int32			inHeightDelta)
{
	if (mWillDraw)
		Invalidate();

	MRect newBounds = mBounds;
	newBounds.width += inWidthDelta;
	newBounds.height += inHeightDelta;

	MRect newFrame = mFrame;
	newFrame.x += inXDelta;
	newFrame.y += inYDelta;
	newFrame.width += inWidthDelta;
	newFrame.height += inHeightDelta;

	mFrame = newFrame;
	mBounds = newBounds;

	foreach (MView* child, mChildren)
	{
		int32 dx = 0, dy = 0, dw = 0, dh = 0;
		
		if (child->mBindLeft)
		{
			if (child->mBindRight)
				dw = inXDelta + inWidthDelta;
		}
		else if (child->mBindRight)
		{
			dx = inWidthDelta;
		}
		
		if (child->mBindTop)
		{
			if (child->mBindBottom)
				dh = inYDelta + inHeightDelta;
		}
		else if (child->mBindBottom)
		{
			dy = inHeightDelta;
		}

		if (dx or dy or dw or dh)
			child->ResizeFrame(dx, dy, dw, dh);
		else
			child->Invalidate();
	}

	if (mWillDraw)
		Invalidate();
}

void MView::Invalidate()
{
	Invalidate(mBounds);
}

void MView::Invalidate(
	MRect		inRect)
{
	if (mParent != nil)
	{
		ConvertToParent(inRect.x, inRect.y);
		inRect &= mFrame;
		if (inRect)
			mParent->Invalidate(inRect);
	}
}

void MView::Scroll(
	int32			inX,
	int32			inY)
{
	Scroll(mBounds, inX, inY);
}

void MView::Scroll(
	MRect			inRect,
	int32			inX,
	int32			inY)
{
	if (mParent != nil)
	{
		ConvertToParent(inRect.x, inRect.y);
		inRect &= mFrame;
		if (inRect)
			mParent->Scroll(inRect, inX, inY);
	}
}

void MView::UpdateNow()
{
	//GtkWidget* w = mGtkWidget; 
	//GtkWidget* toplevel = gtk_widget_get_toplevel(mGtkWidget);

	//if (GTK_WIDGET_TOPLEVEL(toplevel))
	//	w = toplevel;

	//gdk_window_process_all_updates();
}

void MView::SetCursor(
	MCursor			inCursor)
{
	//assert(inCursor < eCursorCount);

	//if (gGdkCursors[inCursor] == nil)
	//{
	//	switch (inCursor)
	//	{
	//		case eNormalCursor:
	//			gGdkCursors[inCursor] = gdk_cursor_new(GDK_LEFT_PTR);
	//			break;
	//		
	//		case eIBeamCursor:	
	//			gGdkCursors[inCursor] = gdk_cursor_new(GDK_XTERM);
	//			break;
	//		
	//		case eRightCursor:
	//			gGdkCursors[inCursor] = gdk_cursor_new(GDK_RIGHT_PTR);
	//			break;
	//		
	//		case eBlankCursor:
	//		{
	//			GdkPixmap* pixmap = gdk_pixmap_new(nil, 1, 1, 1);
	//			GdkColor c = {};
	//			
	//			gGdkCursors[inCursor] = gdk_cursor_new_from_pixmap(
	//				pixmap, pixmap, &c, &c, 0, 0);
	//			gdk_pixmap_unref(pixmap);
	//			break;
	//		}
	//		
	//		default:
	//			assert(false);
	//	}
	//}
	//
	//if (GDK_IS_WINDOW(mGtkWidget->window) and gGdkCursors[inCursor] != nil)
	//	gdk_window_set_cursor(mGtkWidget->window, gGdkCursors[inCursor]);
}

void MView::ObscureCursor()
{
	SetCursor(eBlankCursor);
}

void MView::Add(
	MView*			inSubView)
{
	//assert(GTK_IS_CONTAINER(GetGtkWidget()));
	//gtk_container_add(GTK_CONTAINER(GetGtkWidget()), inSubView->GetGtkWidget());
	//inSubView->Added();
}

bool MView::ActivateOnClick(int32 inX, int32 inY, uint32 inModifiers)
{
	return true;
}

void MView::Click(int32 inX, int32 inY, uint32 inModifiers)
{
	PRINT(("Click at %d,%d", inX, inY));
}

void MView::Show()
{
	if (mVisible == eTriStateOff)
	{
		if (mParent and mParent->IsVisible())
		{
			mVisible = eTriStateOn;
			Invalidate();
			ShowSelf();
		}
		else
			mVisible = eTriStateLatent;
	}

	if (mVisible == eTriStateOn)
	{
		foreach (MView* child, mChildren)
			child->SuperShow();
	}
}

void MView::SuperShow()
{
	if (mVisible == eTriStateLatent)
	{
		mVisible = eTriStateOn;
		ShowSelf();
	}

	if (mVisible == eTriStateOn)
	{
		foreach (MView* child, mChildren)
			child->SuperShow();
	}
}

void MView::ShowSelf()
{
}

void MView::Hide()
{
	if (mVisible == eTriStateOn)
	{
		foreach (MView* child, mChildren)
			child->SuperHide();
	}

	if (mVisible != eTriStateOff)
	{
		Invalidate();

		bool wasVisible = (mVisible == eTriStateOn);
		mVisible = eTriStateOff;
		if (wasVisible)
			HideSelf();
	}
}

void MView::SuperHide()
{
	if (mVisible == eTriStateOn)
	{
		foreach (MView* child, mChildren)
			child->SuperHide();

		mVisible = eTriStateLatent;
		HideSelf();
	}
}

void MView::HideSelf()
{
}

bool MView::IsVisible() const
{
	return mVisible == eTriStateOn;
}

void MView::Enable()
{
	if (mEnabled == eTriStateOff)
	{
		if (mParent != nil and mParent->mEnabled == eTriStateOn)
		{
			mEnabled = eTriStateOn;
			EnableSelf();
		}
		else
			mEnabled = eTriStateLatent;
	}
	
	if (mEnabled == eTriStateOn)
	{
		foreach (MView* child, mChildren)
			child->SuperEnable();
	}
}

void MView::SuperEnable()
{
	if (mEnabled == eTriStateLatent)
	{
		mEnabled = eTriStateOn;
		EnableSelf();
	}
	
	if (mEnabled == eTriStateOn)
	{
		foreach (MView* child, mChildren)
			child->SuperEnable();
	}
}

void MView::EnableSelf()
{
}

void MView::Disable()
{
	if (mEnabled == eTriStateOn)
	{
		foreach (MView* child, mChildren)
			child->SuperDisable();
	}

	bool wasEnabled = (mEnabled == eTriStateOn);
	mEnabled = eTriStateOff;
	if (wasEnabled)
		DisableSelf();
}

void MView::SuperDisable()
{
	if (mEnabled == eTriStateOn)
	{
		foreach (MView* child, mChildren)
			child->SuperDisable();

		mEnabled = eTriStateLatent;
		DisableSelf();
	}
}

void MView::DisableSelf()
{
}

bool MView::IsEnabled() const
{
	return (mEnabled == eTriStateOn) and IsVisible();
}

void MView::Activate()
{
	if (mActive == eTriStateOff)
	{
		if (mParent != nil and mParent->mActive == eTriStateOn)
		{
			mActive = eTriStateOn;
			ActivateSelf();
		}
		else
			mActive = eTriStateLatent;
	}

	if (mActive == eTriStateOn)
	{
		foreach (MView* child, mChildren)
			child->SuperActivate();
	}
}

void MView::SuperActivate()
{	
	if (mActive == eTriStateLatent)
	{
		mActive = eTriStateOn;
		ActivateSelf();
	}
	
	if (mActive == eTriStateOn)
	{
		foreach (MView* child, mChildren)
			child->SuperActivate();
	}
}

void MView::ActivateSelf()
{
}

void MView::Deactivate()
{
	if (mActive == eTriStateOn)
	{
		foreach (MView* child, mChildren)
			child->SuperDeactivate();
	}

	bool wasActive = (mActive == eTriStateOn);
	mActive = eTriStateOff;
	if (wasActive)
		DeactivateSelf();
}

void MView::SuperDeactivate()
{
	if (mActive == eTriStateOn)
	{
		foreach (MView* child, mChildren)
			child->SuperDeactivate();

		mActive = eTriStateLatent;
		DeactivateSelf();
	}
}

void MView::DeactivateSelf()
{
}

bool MView::IsActive() const
{
	return (mActive == eTriStateOn) and IsVisible();
}

//bool MView::OnRealize()
//{
//	int m = gdk_window_get_events(GetGtkWidget()->window);
//
//	m |= GDK_FOCUS_CHANGE_MASK | GDK_STRUCTURE_MASK |
//		GDK_KEY_PRESS_MASK |
//		GDK_BUTTON_PRESS_MASK | GDK_POINTER_MOTION_MASK | GDK_BUTTON_RELEASE_MASK;
//	gdk_window_set_events(GetGtkWidget()->window, (GdkEventMask)m);
//	
//	return false;
//}
//
//bool MView::OnFocusInEvent(
//	GdkEventFocus*	inEvent)
//{
//	return false;
//}
//
//bool MView::OnFocusOutEvent(
//	GdkEventFocus*	inEvent)
//{
//	return false;
//}
//
//bool MView::IsActive() const
//{
//	return GTK_WIDGET_HAS_FOCUS(mGtkWidget);
//}
//
//bool MView::OnButtonPressEvent(
//	GdkEventButton*	inEvent)
//{
//	bool result = false;
//	
//	if (inEvent->button == 3 and inEvent->type == GDK_BUTTON_PRESS)
//	{
//		OnPopupMenu(inEvent);
//		result = true;
//	}
//	
//	return result;
//}
//
//bool MView::OnMotionNotifyEvent(
//	GdkEventMotion*	inEvent)
//{
//	return false;
//}
//
//bool MView::OnKeyPressEvent(
//	GdkEventKey*	inEvent)
//{
//	return false;
//}
//
//bool MView::OnButtonReleaseEvent(
//	GdkEventButton*	inEvent)
//{
//	return false;
//}
//
//bool MView::OnConfigureEvent(
//	GdkEventConfigure*	inEvent)
//{
//	return false;
//}
//
//bool MView::OnScrollEvent(
//	GdkEventScroll*	inEvent)
//{
//	return false;
//}
//
//bool MView::OnExposeEvent(
//	GdkEventExpose*	inEvent)
//{
//	MRect bounds;
//	GetBounds(bounds);
//
//	MRect update(inEvent->area);
//	
//	MDevice dev(this, bounds);
//	Draw(dev, update);
//
//	return true;
//}

// Drag and Drop support

//void MView::SetupDragAndDrop(
//	const GtkTargetEntry	inTargets[],
//	uint32					inTargetCount)
//{
//	gtk_drag_dest_set(mGtkWidget, GTK_DEST_DEFAULT_ALL,
//		inTargets, inTargetCount,
//		GdkDragAction(GDK_ACTION_COPY|GDK_ACTION_MOVE));
//	
//	mDragDataReceived.Connect(mGtkWidget, "drag-data-received");
//	mDragMotion.Connect(mGtkWidget, "drag-motion");
//	mDragLeave.Connect(mGtkWidget, "drag-leave");
//	
//	mDragDataGet.Connect(mGtkWidget, "drag-data-get");
//	mDragDataDelete.Connect(mGtkWidget, "drag-data-delete");
//	
//	mDragWithin = false;
//}
//
//void MView::OnDragDataReceived(
//	GdkDragContext*		inDragContext,
//	gint				inX,
//	gint				inY,
//	GtkSelectionData*	inData,
//	guint				inInfo,
//	guint				inTime)
//{
//	bool ok = false;
//	bool del = false;
//	bool move = inDragContext->action == GDK_ACTION_MOVE;
//	
//	if (inData->length >= 0)
//	{
//		ok = DragAccept(
//			move,
//			inX, inY,
//			reinterpret_cast<const char*>(inData->data), inData->length,
//			inInfo);
//		del = ok and move and gtk_drag_get_source_widget(inDragContext) != mGtkWidget;
//	}
//
//	gtk_drag_finish(inDragContext, ok, del, inTime);
//}
//
//bool MView::OnDragMotion(
//	GdkDragContext*	inDragContext,
//	gint			inX,
//	gint			inY,
//	guint			inTime)
//{
//	if (not mDragWithin)
//	{
//		DragEnter();
//		mDragWithin = true;
//	}
//	
//	if (DragWithin(inX, inY))
//	{
//		bool copy =
//			IsModifierDown(GDK_SHIFT_MASK) or
//			mGtkWidget != gtk_drag_get_source_widget(inDragContext);
//		gdk_drag_status(inDragContext, copy ? GDK_ACTION_COPY : GDK_ACTION_MOVE, inTime);
//	}
//	else
//		gdk_drag_status(inDragContext, GdkDragAction(0), inTime);
//
//	return false;
//}
//
//void MView::OnDragLeave(
//	GdkDragContext*	inDragContext,
//	guint			inTime)
//{
//	mDragWithin = false;
//	DragLeave();
//}
//
//void MView::OnDragDataDelete(
//	GdkDragContext*	inDragContext)
//{
//	DragDeleteData();
//}
//
//void MView::OnDragDataGet(
//	GdkDragContext*		inDragContext,
//	GtkSelectionData*	inData,
//	guint				inInfo,
//	guint				inTime)
//{
//	string data;
//	
//	DragSendData(data);
//	
//	gtk_selection_data_set_text(inData, data.c_str(), data.length());
//}
//
//void MView::DragEnter()
//{
//}
//	
//bool MView::DragWithin(
//	int32			inX,
//	int32			inY)
//{
//	return false;
//}
//
//void MView::DragLeave()
//{
//}
//
//bool MView::DragAccept(
//	bool			inMove,
//	int32			inX,
//	int32			inY,
//	const char*		inData,
//	uint32			inLength,
//	uint32			inType)
//{
//	return false;
//}
//
//void MView::DragBegin(
//	const GtkTargetEntry	inTargets[],
//	uint32					inTargetCount,
//	GdkEventMotion*			inEvent)
//{
//	int button = 1;
//	
//	GtkTargetList* lst = gtk_target_list_new(inTargets, inTargetCount);
//	
////	GdkDragAction action = GDK_ACTION_MOVE;
////	if (inEvent->state & GDK_SHIFT_MASK)
////		action = GDK_ACTION_COPY;
////	
//	GdkDragContext* context = gtk_drag_begin(
//		mGtkWidget, lst, GdkDragAction(GDK_ACTION_MOVE|GDK_ACTION_COPY),
//		button, (GdkEvent*)inEvent);
//
////	gtk_drag_set_icon_default(context);
//	MRect bounds;
//	GetBounds(bounds);
//	MDevice dev(this, bounds, true);
//	
//	GdkPixmap* pm = nil;
//	int32 x, y;
//	GdkModifierType state;
//
//	if (inEvent->is_hint)
//		gdk_window_get_pointer(inEvent->window, &x, &y, &state);
//	else
//	{
//		x = static_cast<int32>(inEvent->x);
//		y = static_cast<int32>(inEvent->y);
//		state = GdkModifierType(inEvent->state);
//	}
//	
//	// only draw a transparent bitmap to drag around
//	// if we're on a composited screen.
//
//	GdkScreen* screen = gtk_widget_get_screen(GetGtkWidget());
//	if (gdk_screen_is_composited(screen))
//		DrawDragImage(pm, x, y);
//	
//	if (pm != nil)
//	{
//		int32 w, h;
//		gdk_drawable_get_size(pm, &w, &h);
//		
//		gtk_drag_set_icon_pixmap(context, gdk_drawable_get_colormap(pm),
//			pm, nil, x, y);
//		
//		g_object_unref(pm);
//	}
//	else
//		gtk_drag_set_icon_default(context);
//
//	gtk_target_list_unref(lst);
//}
//
//void MView::DragSendData(
//	string&		outData)
//{
//}
//
//void MView::DragDeleteData()
//{
//}

void MView::GetMouse(
	int32&			outX,
	int32&			outY) const
{
	if (mParent != nil)
	{
		mParent->GetMouse(outX, outY);
		ConvertFromParent(outX, outY);
	}
}

uint32 MView::GetModifiers() const
{
	uint32 result = 0;

	MWindow* w = GetWindow();
	if (w != nil)
		result = w->GetModifiers();
	
	return result;
}

uint32 MView::CountPages(
	MDevice&		inDevice)
{
	return 1;
}

//void MView::OnPopupMenu(
//	GdkEventButton*	inEvent)
//{
//	PRINT(("Show Popup Menu"));
//}

MView* MView::FindSubView(int32 inX, int32 inY)
{
	MView* result = this;

	foreach (MView* view, mChildren)
	{
		if (view->mVisible and view->mFrame.ContainsPoint(inX, inY))
		{
			ConvertFromParent(inX, inY);
			result = view->FindSubView(inX, inY);
			break;
		}
	}
	
	return result;
}

void MView::ConvertToParent(int32& ioX, int32& ioY) const
{
	assert(mParent);
	ioX += mFrame.x - mBounds.x;
	ioY += mFrame.y - mBounds.y;
}

void MView::ConvertFromParent(int32& ioX, int32& ioY) const
{
	assert(mParent);
	ioX -= mFrame.x - mBounds.x;
	ioY -= mFrame.y - mBounds.y;
}

void MView::ConvertToWindow(int32& ioX, int32& ioY) const
{
	if (mParent != nil)
	{
		ConvertToParent(ioX, ioY);
		mParent->ConvertToWindow(ioX, ioY);
	}
}

void MView::ConvertFromWindow(int32& ioX, int32& ioY) const
{
	if (mParent != nil)
	{
		mParent->ConvertFromWindow(ioX, ioY);
		ConvertFromParent(ioX, ioY);
	}
}

void MView::ConvertToScreen(int32& ioX, int32& ioY) const
{
	ConvertToParent(ioX, ioY);
	if (mParent != nil)
		mParent->ConvertToScreen(ioX, ioY);
}

void MView::ConvertFromScreen(int32& ioX, int32& ioY) const
{
	if (mParent != nil)
		mParent->ConvertFromScreen(ioX, ioY);
	ConvertFromParent(ioX, ioY);
}

void MView::RedrawAll(
	MRect			inUpdate)
{
	inUpdate &= mBounds;

	if (not IsVisible() or not inUpdate)
		return;

	if (mWillDraw)
		Draw(inUpdate);

	foreach (MView* child, mChildren)
	{
		MRect r = inUpdate;

		child->ConvertFromParent(r.x, r.y);
		child->RedrawAll(r);
	}
}

void MView::Draw(
	MRect			inUpdate)
{
	// do nothing
}

