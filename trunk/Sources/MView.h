/*	$Id$
	Copyright Drs M.L. Hekkelman
	Created 28-09-07 11:15:18
*/

#ifndef MVIEW_H
#define MVIEW_H

#include "MCallbacks.h"

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

	PangoContext*	GetPangoContext();
	
	void			SetCursor(
						MCursor			inCursor);

	void			ObscureCursor();

  protected:

					MView(
						GtkWidget*		inWidget,
						bool			inCanActivate);

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

  private:
	GtkWidget*		mGtkWidget;
	PangoContext*	mPangoContext;
};


#endif // MVIEW_H
