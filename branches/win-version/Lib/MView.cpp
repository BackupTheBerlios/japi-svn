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
#include "MControls.h"

using namespace std;

MView::MView(
	const string&	inID,
	MRect			inBounds)
	: mID(inID)
	, mBounds(0, 0, inBounds.width, inBounds.height)
	, mFrame(inBounds)
	, mViewWidth(inBounds.width)
	, mViewHeight(inBounds.height)
	, mParent(nil)
	, mWillDraw(false)
	, mActive(eTriStateOn)
	, mVisible(eTriStateOn)
	, mEnabled(eTriStateOn)
	, mBindLeft(true)
	, mBindTop(true)
	, mBindRight(false)
	, mBindBottom(false)
	, mScroller(nil)
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

void MView::SetViewScroller(
	MViewScroller*	inScroller)
{
	mScroller = inScroller;
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
	mBounds.width = inFrame.width;
	mBounds.height = inFrame.height;
}

void MView::GetViewSize(
	int32&			outWidth,
	int32&			outHeight) const
{
	outWidth = mViewWidth;
	outHeight = mViewHeight;
}

void MView::SetViewSize(
	int32			inWidth,
	int32			inHeight)
{
	mViewWidth = inWidth;
	mViewHeight = inHeight;
	if (mScroller != nil)
		mScroller->AdjustScrollbars();
}

void MView::GetBindings(
	bool&			outFollowLeft,
	bool&			outFollowTop,
	bool&			outFollowRight,
	bool&			outFollowBottom) const
{
	outFollowLeft = mBindLeft;
	outFollowTop = mBindTop;
	outFollowRight = mBindRight;
	outFollowBottom = mBindBottom;
}

void MView::SetBindings(
	bool			inFollowLeft,
	bool			inFollowTop,
	bool			inFollowRight,
	bool			inFollowBottom)
{
	mBindLeft = inFollowLeft;
	mBindTop = inFollowTop;
	mBindRight = inFollowRight;
	mBindBottom = inFollowBottom;
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

	MRect newFrame = mFrame;
	newFrame.x += inXDelta;
	newFrame.y += inYDelta;

	newFrame.width += inWidthDelta;
	newBounds.width += inWidthDelta;

	newBounds.height += inHeightDelta;
	newFrame.height += inHeightDelta;

	mFrame = newFrame;
	mBounds = newBounds;

	if (mScroller != nil)
		mScroller->AdjustScrollbars();

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

//		if (dx or dy or dw or dh)
			child->ResizeFrame(dx, dy, dw, dh);
		//else
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

void MView::ScrollBy(int32 inDeltaX, int32 inDeltaY)
{
	int32 x, y;
	GetScrollPosition(x, y);
	
	MScrollbar* scrollbar = mScroller ? mScroller->GetHScrollbar() : nil;
	if (inDeltaX and scrollbar != nil)
	{
		if (x + inDeltaX < scrollbar->GetMinValue())
			inDeltaX = scrollbar->GetMinValue() - x;
		if (x + inDeltaX > scrollbar->GetMaxValue())
			inDeltaX = scrollbar->GetMaxValue() - x;
	}

	scrollbar = mScroller ? mScroller->GetVScrollbar() : nil;
	if (inDeltaY and scrollbar != nil)
	{
		if (y + inDeltaY < scrollbar->GetMinValue())
			inDeltaY = scrollbar->GetMinValue() - y;
		if (y + inDeltaY > scrollbar->GetMaxValue())
			inDeltaY = scrollbar->GetMaxValue() - y;
	}
	
	if (inDeltaX != 0 or inDeltaY != 0)
	{
		ScrollRect(mBounds, -inDeltaX, -inDeltaY);

		mBounds.x += inDeltaX;
		mBounds.y += inDeltaY;

		if (mScroller != nil)
			mScroller->AdjustScrollbars();
	}
}

void MView::ScrollTo(int32 inX, int32 inY)
{
	ScrollBy(inX - mBounds.x, inY - mBounds.y);
}

void MView::GetScrollPosition(int32& outX, int32& outY) const
{
	outX = mBounds.x;
	outY = mBounds.y;
}

void MView::GetScrollUnit(
	int32&			outScrollUnitX,
	int32&			outScrollUnitY) const
{
	if (mScroller != nil)
		mScroller->GetTargetScrollUnit(outScrollUnitX, outScrollUnitY);
	else if (mParent != nil)
		mParent->GetScrollUnit(outScrollUnitX, outScrollUnitY);
	else
		outScrollUnitX = outScrollUnitY = 1;
}

void MView::SetScrollUnit(
	int32			inScrollUnitX,
	int32			inScrollUnitY)
{
	if (mScroller != nil)
		mScroller->SetTargetScrollUnit(inScrollUnitX, inScrollUnitY);
	else if (mParent != nil)
		mParent->SetScrollUnit(inScrollUnitX, inScrollUnitY);
}

void MView::ScrollRect(
	MRect			inRect,
	int32			inX,
	int32			inY)
{
	if (mParent != nil)
	{
		ConvertToParent(inRect.x, inRect.y);
		inRect &= mFrame;
		if (inRect)
			mParent->ScrollRect(inRect, inX, inY);
	}
}

void MView::UpdateNow()
{
	if (mParent != nil)
		mParent->UpdateNow();
}

void MView::SetCursor(
	MCursor			inCursor)
{
	if (mParent != nil)
		mParent->SetCursor(inCursor);
}

void MView::AdjustCursor(
	int32			inX,
	int32			inY,
	uint32			inModifiers)
{
	if (mParent != nil)
	{
		ConvertToParent(inX, inY);
		mParent->AdjustCursor(inX, inY, inModifiers);
	}
}

void MView::ObscureCursor()
{
	SetCursor(eBlankCursor);
}

bool MView::ActivateOnClick(int32 inX, int32 inY, uint32 inModifiers)
{
	return true;
}

void MView::MouseDown(
	int32			inX,
	int32			inY,
	uint32			inClickCount,
	uint32			inModifiers)
{
}

void MView::MouseMove(
	int32			inX,
	int32			inY,
	uint32			inModifiers)
{
}

void MView::MouseExit()
{
}

void MView::MouseUp(
	int32			inX,
	int32			inY,
	uint32			inModifiers)
{
}

//void MView::Click(int32 inX, int32 inY, uint32 inModifiers)
//{
//	PRINT(("Click at %d,%d", inX, inY));
//}

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

MView* MView::FindSubView(int32 inX, int32 inY) const
{
	const MView* result = this;

	foreach (MView* view, mChildren)
	{
		if (view->IsVisible() and view->mFrame.ContainsPoint(inX, inY))
		{
			ConvertFromParent(inX, inY);
			result = view->FindSubView(inX, inY);
			break;
		}
	}
	
	return const_cast<MView*>(result);
}

MView* MView::FindSubViewByID(const string& inID) const
{
	const MView* result = nil;
	
	if (mID == inID)
		result = this;
	else
	{
		foreach (MView* view, mChildren)
		{
			result = view->FindSubViewByID(inID);
			if (result != nil)
				break;
		}
	}
	
	return const_cast<MView*>(result);
}

void MView::ConvertToParent(int32& ioX, int32& ioY) const
{
	ioX += mFrame.x - mBounds.x;
	ioY += mFrame.y - mBounds.y;
}

void MView::ConvertFromParent(int32& ioX, int32& ioY) const
{
	ioX -= mFrame.x - mBounds.x;
	ioY -= mFrame.y - mBounds.y;
}

void MView::ConvertToWindow(int32& ioX, int32& ioY) const
{
	ConvertToParent(ioX, ioY);
	if (mParent != nil)
		mParent->ConvertToWindow(ioX, ioY);
}

void MView::ConvertFromWindow(int32& ioX, int32& ioY) const
{
	if (mParent != nil)
		mParent->ConvertFromWindow(ioX, ioY);
	ConvertFromParent(ioX, ioY);
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

// --------------------------------------------------------------------

void MHBox::AddChild(
	MView*			inChild)
{
	MRect frame;
	inChild->GetFrame(frame);
	
	// adjust for new height, if needed
	if (mViewHeight < frame.y + frame.height)
		ResizeFrame(0, 0, 0, frame.y + frame.height - mViewHeight);
		//mViewHeight = frame.y + frame.height;
	else
		inChild->ResizeFrame(0, 0, 0, mViewHeight - frame.height - frame.y);
	
	if (not mChildren.empty())
	{
		MRect lastFrame;
		mChildren.back()->GetFrame(lastFrame);
		
		frame.x += lastFrame.x + lastFrame.width + mSpacing;
		inChild->SetFrame(frame);
	}

	mViewWidth = frame.x + frame.width;

	MView::AddChild(inChild);
}

void MHBox::ResizeFrame(
	int32			inXDelta,
	int32			inYDelta,
	int32			inWidthDelta,
	int32			inHeightDelta)
{
	mFrame.x += inXDelta;
	mFrame.y += inYDelta;
	mFrame.width += inWidthDelta;
	mFrame.height += inHeightDelta;

	mBounds.width += inWidthDelta;
	mBounds.height += inHeightDelta;
	
	mViewWidth += inWidthDelta;
	mViewHeight += inHeightDelta;

	uint32 n = 0;	// count the resizing children
	foreach (MView* child, mChildren)
	{
		if (inHeightDelta != 0)
			child->ResizeFrame(0, 0, 0, inHeightDelta);

		if (child->WidthResizable())
			++n;
	}
	
	if (n > 0 and inWidthDelta != 0)
	{
		int32 delta = inWidthDelta / n;
		int32 dx = 0;
		
		foreach (MView* child, mChildren)
		{
			if (child->WidthResizable())
			{
				child->ResizeFrame(dx, 0, delta, 0);
				dx += delta;
			}
			else
				child->ResizeFrame(dx, 0, 0, 0);
		}
	}
}

void MVBox::AddChild(
	MView*			inChild)
{
	MRect frame;
	inChild->GetFrame(frame);
	
	// adjust for new width, if needed
	if (mViewWidth < frame.x + frame.width)
		ResizeFrame(0, 0, frame.x + frame.width - mViewWidth, 0);
	else
		inChild->ResizeFrame(0, 0, mViewWidth - frame.width - frame.x, 0);
	
	// append the child as last in the row
	if (not mChildren.empty())
	{
		MRect lastFrame;
		mChildren.back()->GetFrame(lastFrame);
		
		frame.y += lastFrame.y + lastFrame.height + mSpacing;
		inChild->SetFrame(frame);
	}

	// our height will increase
	mViewHeight = frame.y + frame.height;
	
	MView::AddChild(inChild);
}

void MVBox::ResizeFrame(
	int32			inXDelta,
	int32			inYDelta,
	int32			inWidthDelta,
	int32			inHeightDelta)
{
	mFrame.x += inXDelta;
	mFrame.y += inYDelta;
	mFrame.width += inWidthDelta;
	mFrame.height += inHeightDelta;

	mBounds.width += inWidthDelta;
	mBounds.height += inHeightDelta;
	
	mViewWidth += inWidthDelta;
	mViewHeight += inHeightDelta;

	uint32 n = 0;	// count the resizing children
	foreach (MView* child, mChildren)
	{
		if (inWidthDelta != 0)
			child->ResizeFrame(0, 0, inWidthDelta, 0);
		
		if (child->HeightResizable())
			++n;
	}
	
	if (n > 0 and inHeightDelta != 0)
	{
		int32 delta = inHeightDelta / n;
		int32 dy = 0;
		
		foreach (MView* child, mChildren)
		{
			if (child->HeightResizable())
			{
				child->ResizeFrame(0, dy, 0, dy);
				dy += delta;
			}
			else
				child->ResizeFrame(0, dy, 0, 0);
		}
	}
}

MTable::MTable(const std::string& inID, MRect inBounds, MView* inChildren[],
	uint32 inColumns, uint32 inRows, int32 inHSpacing, int32 inVSpacing)
	: MView(inID, inBounds)
	, mColumns(inColumns)
	, mRows(inRows)
	, mHSpacing(inHSpacing)
	, mVSpacing(inVSpacing)
	, mGrid(mRows * mColumns)
{
	vector<int32> widths(mColumns);
	vector<int32> heights(mRows);
	
	for (int32 r = 0; r < mRows; ++r)
	{
		for (int32 c = 0; c < mColumns; ++c)
		{
			int ix = r * mColumns + c;
			
			if (inChildren[ix] == nil)
				continue;

			mGrid[ix] = inChildren[ix];
			MView::AddChild(mGrid[ix]);
			
			MRect bounds;
			mGrid[ix]->GetBounds(bounds);
			
			if (widths[c] < bounds.width)
				widths[c] = bounds.width;
			
			if (heights[r] < bounds.height)
				heights[r] = bounds.height;
		}
	}
	
	int32 x, y = 0;
	
	for (int32 r = 0; r < mRows; ++r)
	{
		x = 0;

		for (int32 c = 0; c < mColumns; ++c)
		{
			int ix = r * mColumns + c;

			if (inChildren[ix] == nil)
				continue;

			MRect frame;
			mGrid[ix]->GetFrame(frame);
			
			mGrid[ix]->ResizeFrame(x, y,
				widths[c] - frame.width, heights[r] - frame.height);
			
			x += widths[c] + mHSpacing;
		}
		
		y += heights[r] + mVSpacing;
	}
	
	mBounds.width = mFrame.width = mViewWidth = x - mHSpacing;
	mBounds.height = mFrame.height = mViewHeight = y - mVSpacing;
}

void MTable::ResizeFrame(
	int32			inXDelta,
	int32			inYDelta,
	int32			inWidthDelta,
	int32			inHeightDelta)
{
	mFrame.x += inXDelta;
	mFrame.y += inYDelta;
	mFrame.width += inWidthDelta;
	mFrame.height += inHeightDelta;

	mBounds.width += inWidthDelta;
	mBounds.height += inHeightDelta;
	
	mViewWidth += inWidthDelta;
	mViewHeight += inHeightDelta;
	
	// resize rows
	for (uint32 rx = 0; rx < mRows; ++rx)
	{
		int32 height = 0;
		
		for (uint32 cx = 0; cx < mColumns; ++cx)
		{
			MView* v = mGrid[rx * mColumns + cx];
			if (v == nil)
				continue;
			
			MRect f;
			v->GetFrame(f);
			
			if (height < f.y + f.height)
				height = f.y + f.height;
		}

		for (uint32 cx = 0; cx < mColumns; ++cx)
		{
			MView* v = mGrid[rx * mColumns + cx];
			if (v == nil or v->HeightResizable() == false)
				continue;
			
			MRect f;
			v->GetFrame(f);
			
			if (f.y + f.height < height)
				v->ResizeFrame(0, 0, 0, height - f.y - f.height);
		}
	}

	// resize columns
	// first count the number of resizing columns
	
	if (inWidthDelta != 0)
	{
		uint32 n = 0;
		vector<bool> resizable(mColumns);

		for (uint32 cx = 0; cx < mColumns; ++cx)
		{
			for (uint32 rx = 0; rx < mRows; ++rx)
			{
				MView* child = mGrid[rx * mColumns + cx];

				if (child != nil and child->WidthResizable())
				{
					resizable[cx] = true;
					++n;
					break;
				}
			}
		}
		
		if (n > 0)
		{
			for (uint32 cx = 0; cx < mColumns; ++cx)
			{
				int32 delta = inWidthDelta / n;
				int32 dx = 0;
				
				for (uint32 rx = 0; rx < mRows; ++rx)
				{
					MView* child = mGrid[rx * mColumns + cx];
					
					if (child == nil)
						continue;
					
					if (resizable[cx])
						child->ResizeFrame(dx, 0, delta, 0);
					else
						child->ResizeFrame(dx, 0, 0, 0);
				}

				if (resizable[cx])
					dx += delta;
			}
		}
	}
}

void MTable::AddChild(
	MView*			inView,
	int32			inColumn,
	int32			inRow,
	int32			inColumnSpan,
	int32			inRowSpan)
{
	MView::AddChild(inView);
	
	if (inColumn > mColumns or inRow > mRows)
		THROW(("table index out of bounds"));
	
	mGrid[inRow * mColumns + inColumn] = inView;
	
	ResizeFrame(0, 0, 0, 0);
}

// --------------------------------------------------------------------

MViewScroller::MViewScroller(const string& inID,
		MView* inTarget, bool inHScrollbar, bool inVScrollbar)
	: MView(inID, MRect(0, 0, 0, 0))
	, mTarget(inTarget)
	, mHScrollbar(nil)
	, mVScrollbar(nil)
	, eVScroll(this, &MViewScroller::VScroll)
	, eHScroll(this, &MViewScroller::HScroll)
	, mScrollUnitX(1)
	, mScrollUnitY(1)
{
	MRect frame;
	mTarget->GetFrame(frame);	// our bounds will be the frame of the target
	SetFrame(frame);
	
	MRect bounds;
	mTarget->GetBounds(bounds);
	SetBounds(bounds);

	if (inVScrollbar)
	{
		MRect r(mBounds.x + mBounds.width - kScrollbarWidth, mBounds.y,
			kScrollbarWidth, mBounds.height);
		if (inHScrollbar)
			r.height -= kScrollbarWidth;

		mVScrollbar = new MScrollbar("vscrollbar", r);
		mVScrollbar->SetBindings(false, true, true, true);
		mVScrollbar->SetValue(0);
		AddChild(mVScrollbar);
		AddRoute(mVScrollbar->eScroll, eVScroll);

		frame.width -= kScrollbarWidth;
	}

	if (inHScrollbar)
	{
		MRect r(mBounds.x, mBounds.y + mBounds.height - kScrollbarWidth,
			mBounds.width, kScrollbarWidth);
		
		if (inVScrollbar)
			r.width -= kScrollbarWidth;

		mHScrollbar = new MScrollbar("hscrollbar", r);
		AddChild(mHScrollbar);
		mHScrollbar->SetBindings(true, false, true, true);
		mHScrollbar->SetValue(0);
		AddChild(mHScrollbar);
		AddRoute(mHScrollbar->eScroll, eHScroll);

		frame.height -= kScrollbarWidth;
	}

	frame.x = frame.y = 0;
	mTarget->SetFrame(frame);
	mTarget->SetBounds(frame);

	mTarget->SetViewScroller(this);
	mTarget->SetBindings(true, true, true, true);

	AddChild(mTarget);
}

void MViewScroller::ResizeFrame(
	int32			inXDelta,
	int32			inYDelta,
	int32			inWidthDelta,
	int32			inHeightDelta)
{
	MView::ResizeFrame(inXDelta, inYDelta, inWidthDelta, inHeightDelta);
	AdjustScrollbars();
}

void MViewScroller::AdjustScrollbars()
{
	int32 dx = 0, dy = 0;
	
	int32 viewWidth, viewHeight;
	mTarget->GetViewSize(viewWidth, viewHeight);

	MRect bounds;
	mTarget->GetBounds(bounds);

	if (mHScrollbar != nil)
	{
		mHScrollbar->SetMinValue(0);
		mHScrollbar->SetMaxValue(viewWidth);
		mHScrollbar->SetViewSize(bounds.width);
		mHScrollbar->SetValue(bounds.x);
		
		dx = mHScrollbar->GetValue() - bounds.x;
	}

	if (mVScrollbar != nil)
	{
		mVScrollbar->SetMinValue(0);
		mVScrollbar->SetMaxValue(viewHeight);
		mVScrollbar->SetViewSize(bounds.height);
		mVScrollbar->SetValue(bounds.y);

		dy = mVScrollbar->GetValue() - bounds.y;
	}
	
	if (dx != 0 or dy != 0)
		mTarget->ScrollBy(dx, dy);
}

void MViewScroller::SetTargetScrollUnit(
	int32			inScrollUnitX,
	int32			inScrollUnitY)
{
	if (inScrollUnitX < 1 or inScrollUnitY < 1)
		THROW(("Scroll unit should be larger than one"));
	mScrollUnitX = inScrollUnitX;
	mScrollUnitY = inScrollUnitY;
}

void MViewScroller::GetTargetScrollUnit(
	int32&			outScrollUnitX,
	int32&			outScrollUnitY) const
{
	outScrollUnitX = mScrollUnitX;
	outScrollUnitY = mScrollUnitY;
}

void MViewScroller::VScroll(MScrollMessage inScrollMsg)
{
	int32 value = mVScrollbar->GetValue();

	MRect frame;
	mTarget->GetFrame(frame);

	int32 x, y;
	mTarget->GetScrollPosition(x, y);

	int32 dy = 0;

	switch (inScrollMsg)
	{
		case kScrollLineUp:
			dy = -mScrollUnitY;
			break;

		case kScrollLineDown:
			dy = mScrollUnitY;
			break;

		case kScrollPageUp:
			dy = -frame.height;
			break;

		case kScrollPageDown:
			dy = frame.height;
			break;

		case kScrollToThumb:
			dy = value - y;
			break;
	}

	if (dy != 0)
		mTarget->ScrollBy(0, dy);
}

void MViewScroller::HScroll(MScrollMessage inScrollMsg)
{
	int32 value = mVScrollbar->GetValue();

	MRect frame;
	mTarget->GetFrame(frame);

	int32 x, y;
	mTarget->GetScrollPosition(x, y);

	int32 dx = 0;

	switch (inScrollMsg)
	{
		case kScrollLineUp:
			dx = -mScrollUnitX;
			break;

		case kScrollLineDown:
			dx = mScrollUnitX;
			break;

		case kScrollPageUp:
			dx = -frame.height;
			break;

		case kScrollPageDown:
			dx = frame.height;
			break;

		case kScrollToThumb:
			dx = value - x;
			break;
	}

	if (dx != 0)
		mTarget->ScrollBy(dx, 0);
}

