/*	$Id$
	Copyright Maarten L. Hekkelman
	Created 28-09-07 21:05:58
*/

#ifndef MDRAWINGAREA_H
#define MDRAWINGAREA_H

#include "MView.h"

class MDrawingArea : public MView
{
  public:
					MDrawingArea(
						int32		inWidth,
						int32		inHeight);
					~MDrawingArea();
	
	PangoContext*	GetPangoContext()	{ return mPangoContext; }

  protected:

	virtual bool	OnExposeEvent(
						GdkEventExpose*	inEvent);

	MSlot<bool(GdkEventExpose*)>	mExposeEvent;
	PangoContext*					mPangoContext;
};

#endif // MDRAWINGAREA_H
