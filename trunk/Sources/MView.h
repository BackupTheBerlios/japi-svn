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

	bool			IsActive() const;

	void			Invalidate();

	void			Invalidate(
						const MRect&	inRect);

  protected:

					MView(
						GtkWidget*	inWidget);

  private:
	GtkWidget*		mGtkWidget;
};


#endif // MVIEW_H
