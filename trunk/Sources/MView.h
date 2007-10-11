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
					MView();
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

	virtual bool	OnFocusInEvent(
						GdkEventFocus*	inEvent);

	virtual bool	OnFocusOutEvent(
						GdkEventFocus*	inEvent);


	MSlot<bool(GdkEventFocus*)>			mFocusInEvent;
	MSlot<bool(GdkEventFocus*)>			mFocusOutEvent;

  private:
	GtkWidget*		mGtkWidget;
};


#endif // MVIEW_H
