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
					uint32		inWidth,
					uint32		inHeight);
				~MDrawingArea();

  protected:

	virtual bool	OnExposeEvent(
						GdkEventExpose*	inEvent);

	MSlot<bool(GdkEventExpose*)>	mExposeEvent;
};

#endif // MDRAWINGAREA_H
