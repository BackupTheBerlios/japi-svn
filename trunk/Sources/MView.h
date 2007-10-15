/*	$Id$
	Copyright Drs M.L. Hekkelman
	Created 28-09-07 11:15:18
*/

#ifndef MVIEW_H
#define MVIEW_H

#include "MCallbacks.h"

class MView
{
  public:
	virtual			~MView();

	GtkWidget*		GetGtkWidget() const		{ return mGtkWidget; }

	MWindow*		GetWindow() const;

	void			GetBounds(
						MRect&			outBounds) const;

	void			SetBounds(
						const MRect&	inBounds);

	void			ResizeTo(
						uint32			inWidth,
						uint32			inHeight);

	bool			IsActive() const;

	void			Invalidate();

	void			Invalidate(
						const MRect&	inRect);

	virtual void	Add(
						MView*			inSubView);

  protected:

					MView(
						GtkWidget*		inWidget,
						bool			inCanActivate);

					MView();

	void			SetWidget(
						GtkWidget*		inWidget,
						bool			inCanActivate);

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

	virtual bool	OnConfigure(
						GdkEventConfigure*
										inEvent);

	virtual bool	OnRealize();

	MSlot<bool(GdkEventFocus*)>			mFocusInEvent;
	MSlot<bool(GdkEventFocus*)>			mFocusOutEvent;
	MSlot<bool(GdkEventButton*)>		mButtonPressEvent;
	MSlot<bool(GdkEventMotion*)>		mMotionNotifyEvent;
	MSlot<bool(GdkEventButton*)>		mButtonReleaseEvent;
	MSlot<bool(GdkEventKey*)>			mKeyPressEvent;
	MSlot<bool(GdkEventConfigure*)>		mConfigure;
	MSlot<bool()>						mRealize;

  private:
	GtkWidget*		mGtkWidget;
};


#endif // MVIEW_H
