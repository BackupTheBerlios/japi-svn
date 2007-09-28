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
					MView(
						GtkWidget*	inWidget);
	virtual			~MView();

	

	GtkWidget*		GetGtkWidget() const		{ return mGtkWidget; }

  protected:

	virtual bool	OnExposeEvent(
						GdkEventExpose*	inEvent);

	MSlot<bool(GdkEventExpose*)>	mExposeEvent;

  private:
	GtkWidget*		mGtkWidget;
};


#endif // MVIEW_H
