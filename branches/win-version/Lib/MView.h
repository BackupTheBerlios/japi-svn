//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

/*	$Id$
	Copyright Drs M.L. Hekkelman
	Created 28-09-07 11:15:18
*/

#ifndef MVIEW_H
#define MVIEW_H

#include <list>
#include "MCallbacks.h"
#include "MP2PEvents.h"

class MWindow;
class MDevice;
class MView;
class MViewScroller;
class MScrollbar;

typedef std::list<MView*> MViewList;

enum MCursor
{
	eNormalCursor,
	eIBeamCursor,
	eRightCursor,
	
	eBlankCursor,
	
	eCursorCount
};

class MView
{
  public:
					MView(
						const std::string&
										inID,
						MRect			inBounds);

	virtual			~MView();

	std::string		GetID() const						{ return mID; }

	virtual MView*	GetParent() const;

	virtual void	SetParent(
						MView*			inParent);

	virtual void	AddChild(
						MView*			inChild);

	virtual void	RemoveChild(
						MView*			inChild);

	virtual void	AddedToWindow();

	virtual MWindow*
					GetWindow() const;

	virtual void	SetViewScroller(
						MViewScroller*	inScroller);

	void			GetBounds(
						MRect&			outBounds) const;

	void			SetBounds(
						const MRect&	inBounds);

	void			GetFrame(
						MRect&			outFrame) const;

	void			SetFrame(
						const MRect&	inFrame);
	
	virtual void	ResizeFrame(
						int32			inXDelta,
						int32			inYDelta,
						int32			inWidthDelta,
						int32			inHeightDelta);

	virtual void	GetViewSize(
						int32&			outWidth,
						int32&			outHeight) const;

	virtual void	SetViewSize(
						int32			inWidth,
						int32			inHeight);

	bool			WidthResizable() const				{ return mBindLeft and mBindRight; }
	bool			HeightResizable() const				{ return mBindTop and mBindBottom; }

	virtual void	GetScrollUnit(
						int32&			outScrollUnitX,
						int32&			outScrollUnitY) const;

	virtual void	SetScrollUnit(
						int32			inScrollUnitX,
						int32			inScrollUnitY);

	void			SetBindings(bool inFollowLeft, bool inFollowTop,
						bool inFollowRight, bool inFollowBottom);

	virtual bool	ActivateOnClick(
						int32			inX,
						int32			inY,
						uint32			inModifiers);

	virtual void	MouseDown(
						int32			inX,
						int32			inY,
						uint32			inClickCount,
						uint32			inModifiers);

	virtual void	MouseMove(
						int32			inX,
						int32			inY,
						uint32			inModifiers);

	virtual void	MouseExit();

	virtual void	MouseUp(
						int32			inX,
						int32			inY,
						uint32			inModifiers);

	virtual void	RedrawAll(
						MRect			inUpdate);

	virtual void	Draw(
						MRect			inUpdate);

	virtual void	Activate();
	virtual void	Deactivate();
	bool			IsActive() const;

	virtual void	Enable();
	virtual void	Disable();
	bool			IsEnabled() const;

	virtual void	Show();
	virtual void	Hide();
	bool			IsVisible() const;

	void			Invalidate();

	virtual void	Invalidate(
						MRect			inRect);

	virtual void	ScrollBy(
						int32			inDeltaX,
						int32			inDeltaY);

	virtual void	ScrollTo(
						int32			inX,
						int32			inY);

	virtual void	GetScrollPosition(
						int32&			outX,
						int32&			outY) const;

	virtual void	ScrollRect(
						MRect			inRect,
						int32			inX,
						int32			inY);

	virtual void	UpdateNow();

	virtual void	AdjustCursor(
						int32			inX,
						int32			inY,
						uint32			inModifiers);

	virtual void	SetCursor(
						MCursor			inCursor);

	void			ObscureCursor();

	void			GetMouse(
						int32&			outX,
						int32&			outY) const;

	uint32			GetModifiers() const;

	// called for printing
	virtual uint32	CountPages(
						MDevice&		inDevice);

	MView*			FindSubView(
						int32			inX,
						int32			inY) const;

	virtual MView*	FindSubViewByID(
						const std::string&
										inID) const;

	virtual void	ConvertToParent(int32& ioX, int32& ioY) const;
	virtual void	ConvertFromParent(int32& ioX, int32& ioY) const;
	virtual void	ConvertToWindow(int32& ioX, int32& ioY) const;
	virtual void	ConvertFromWindow(int32& ioX, int32& ioY) const;
	virtual void	ConvertToScreen(int32& ioX, int32& ioY) const;
	virtual void	ConvertFromScreen(int32& ioX, int32& ioY) const;

  protected:

	void			SuperActivate();
	virtual void	ActivateSelf();
	void			SuperDeactivate();
	virtual void	DeactivateSelf();

	void			SuperEnable();
	virtual void	EnableSelf();
	void			SuperDisable();
	virtual void	DisableSelf();

	void			SuperShow();
	virtual void	ShowSelf();
	void			SuperHide();
	virtual void	HideSelf();

	std::string		mID;
	MRect			mBounds;
	MRect			mFrame;
	int32			mViewWidth, mViewHeight;
//	int32			mMarginLeft, mMarginTop, mMarginRight, mMarginBottom;
	bool			mBindLeft, mBindTop, mBindRight, mBindBottom;
	MView*			mParent;
	MViewScroller*	mScroller;
	MViewList		mChildren;
	bool			mWillDraw;
	MTriState		mActive;
	MTriState		mVisible;
	MTriState		mEnabled;
};

class MHBox : public MView
{
  public:
					MHBox(const std::string& inID, MRect inBounds, uint32 inSpacing)
						: MView(inID, inBounds)
						, mSpacing(inSpacing) {}

	virtual void	AddChild(
						MView*			inChild);
	
	virtual void	ResizeFrame(
						int32			inXDelta,
						int32			inYDelta,
						int32			inWidthDelta,
						int32			inHeightDelta);

  protected:
	uint32			mSpacing;
};

class MVBox : public MView
{
  public:
					MVBox(const std::string& inID, MRect inBounds, uint32 inSpacing)
						: MView(inID, inBounds)
						, mSpacing(inSpacing) {}
	
	virtual void	AddChild(
						MView*			inChild);

	virtual void	ResizeFrame(
						int32			inXDelta,
						int32			inYDelta,
						int32			inWidthDelta,
						int32			inHeightDelta);

  protected:
	uint32			mSpacing;
};

class MTable : public MView
{
  public:
					MTable(const std::string& inID, MRect inBounds,
						MView* inChildren[],
						uint32 inColumns, uint32 inRows,
						int32 inHSpacing, int32 inVSpacing);
	
	virtual void	ResizeFrame(
						int32			inXDelta,
						int32			inYDelta,
						int32			inWidthDelta,
						int32			inHeightDelta);

	virtual void	AddChild(
						MView*			inView,
						int32			inColumn,
						int32			inRow,
						int32			inColumnSpan = 1,
						int32			inRowSpan = 1);

  private:
	uint32			mColumns, mRows;
	int32			mHSpacing, mVSpacing;
	std::vector<MView*>
					mGrid;
};

class MViewScroller : public MView
{
public:
					MViewScroller(const std::string& inID, MView* inTarget,
						bool inHScrollbar, bool inVScrollbar);

	virtual void	AdjustScrollbars();

	MScrollbar*		GetHScrollbar() const					{ return mHScrollbar; }
	MScrollbar*		GetVScrollbar() const					{ return mVScrollbar; }

	virtual void	ResizeFrame(
						int32			inXDelta,
						int32			inYDelta,
						int32			inWidthDelta,
						int32			inHeightDelta);

	void			SetTargetScrollUnit(
						int32			inScrollUnitX,
						int32			inScrollUnitY);

	void			GetTargetScrollUnit(
						int32&			outScrollUnitX,
						int32&			outScrollUnitY) const;

protected:

	MView*			mTarget;
	MScrollbar*		mHScrollbar;
	MScrollbar*		mVScrollbar;
	int32			mScrollUnitX, mScrollUnitY;

	virtual void	VScroll(MScrollMessage inScrollMsg);
	virtual void	HScroll(MScrollMessage inScrollMsg);

	MEventIn<void(MScrollMessage)> eVScroll;
	MEventIn<void(MScrollMessage)> eHScroll;
};

#endif // MVIEW_H
