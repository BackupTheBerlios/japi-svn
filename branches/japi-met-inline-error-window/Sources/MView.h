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

#include "MCallbacks.h"

class MWindow;
class MDevice;

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
						int32			inWidth,
						int32			inHeight);

	virtual			~MView();

	GtkWidget*		GetGtkWidget() const		{ return mGtkWidget; }

	MWindow*		GetWindow() const;

	void			GetBounds(
						MRect&			outBounds) const;

	void			SetBounds(
						const MRect&	inBounds);

	void			ResizeTo(
						int32			inWidth,
						int32			inHeight);

	void			ConvertToGlobal(
						int32&			ioX,
						int32&			ioY);

	virtual void	Draw(
						MDevice&		inDevice,
						MRect			inUpdate);

	bool			IsActive() const;

	void			Invalidate();

	void			Invalidate(
						const MRect&	inRect);

	void			Scroll(
						const MRect&	inRect,
						int32			inX,
						int32			inY);

	void			Scroll(
						int32			inX,
						int32			inY);

	void			UpdateNow();

	virtual void	Add(
						MView*			inSubView);

	void			SetCursor(
						MCursor			inCursor);

	void			ObscureCursor();

	void			GetMouse(
						int32&			outX,
						int32&			outY) const;

	uint32			GetModifiers() const;

	// called for printing
	virtual uint32	CountPages(
						MDevice&		inDevice);

  protected:

					MView(
						GtkWidget*		inWidget,
						bool			inCanActivate,
						bool			inCanDraw = false);

					MView();

	void			SetWidget(
						GtkWidget*		inWidget,
						bool			inCanActivate,
						bool			inCanDraw);

	virtual void	Added();

	virtual bool	OnFocusInEvent(
						GdkEventFocus*	inEvent);

	virtual bool	OnFocusOutEvent(
						GdkEventFocus*	inEvent);
	
	virtual bool	OnButtonPressEvent(
						GdkEventButton*	inEvent);

	virtual bool	OnMotionNotifyEvent(
						GdkEventMotion*	inEvent);
	
	virtual bool	OnButtonReleaseEvent(
						GdkEventButton*	inEvent);

	virtual bool	OnKeyPressEvent(
						GdkEventKey*	inEvent);

	virtual bool	OnConfigureEvent(
						GdkEventConfigure*
										inEvent);

	virtual bool	OnScrollEvent(
						GdkEventScroll*	inEvent);

	virtual bool	OnRealize();

	virtual bool	OnExposeEvent(
						GdkEventExpose*	inEvent);

	virtual void	OnPopupMenu(
						GdkEventButton*	inEvent);

	// Drag and Drop support

	void			SetupDragAndDrop(
						const GtkTargetEntry
										inTargets[],
						uint32			inTargetCount);

	void			DragBegin(
						const GtkTargetEntry
										inTargets[],
						uint32			inTargetCount,
						GdkEventMotion*	inEvent);

	virtual void	DragEnter();
	
	virtual bool	DragWithin(
						int32			inX,
						int32			inY);
	
	virtual void	DragLeave();
	
	virtual bool	DragAccept(
						bool			inMove,
						int32			inX,
						int32			inY,
						const char*		inData,
						uint32			inLength,
						uint32			inType);
	
	virtual void	DragSendData(
						std::string&	outData);
	
	virtual void	DragDeleteData();
	
	virtual void	OnDragDataReceived(
						GdkDragContext*	inDragContext,
						gint			inX,
						gint			inY,
						GtkSelectionData*
										inData,
						guint			inInfo,
						guint			inTime);

	virtual bool	OnDragMotion(
						GdkDragContext*	inDragContext,
						gint			inX,
						gint			inY,
						guint			inTime);

	virtual void	OnDragLeave(
						GdkDragContext*	inDragContext,
						guint			inTime);

	virtual void	OnDragDataDelete(
						GdkDragContext*	inDragContext);

	virtual void	OnDragDataGet(
						GdkDragContext*	inDragContext,
						GtkSelectionData*
										inData,
						guint			inInfo,
						guint			inTime);

	bool			IsWithinDrag() const	{ return mDragWithin; }

	virtual void	DrawDragImage(
						GdkPixmap*&		outPixmap,
						int32&			outX,
						int32&			outY)	{  }

	MSlot<bool(GdkEventFocus*)>			mFocusInEvent;
	MSlot<bool(GdkEventFocus*)>			mFocusOutEvent;
	MSlot<bool(GdkEventButton*)>		mButtonPressEvent;
	MSlot<bool(GdkEventMotion*)>		mMotionNotifyEvent;
	MSlot<bool(GdkEventButton*)>		mButtonReleaseEvent;
	MSlot<bool(GdkEventKey*)>			mKeyPressEvent;
	MSlot<bool(GdkEventConfigure*)>		mConfigureEvent;
	MSlot<bool(GdkEventScroll*)>		mScrollEvent;
	MSlot<bool()>						mRealize;
	MSlot<bool(GdkEventExpose*)>		mExposeEvent;
	MSlot<void(GdkEventButton*)>		mPopupMenu;
	
	MSlot<void(GdkDragContext*,
               gint,
               gint,
               GtkSelectionData*,
               guint,
               guint)>					mDragDataReceived;

	MSlot<bool(GdkDragContext*,
               gint,
               gint,
               guint)>					mDragMotion;

	MSlot<void(GdkDragContext*,
               guint)>					mDragLeave;

	MSlot<void(GdkDragContext*)>		mDragDataDelete;
	
	MSlot<void(GdkDragContext*,
			   GtkSelectionData*,
			   guint,
			   guint)>					mDragDataGet;
	
  private:
	GtkWidget*		mGtkWidget;
	bool			mDragWithin;
};

#endif // MVIEW_H
