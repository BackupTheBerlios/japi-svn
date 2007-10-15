/*	$Id$
	Copyright Maarten L. Hekkelman
	Created 28-09-07 21:24:39
*/

#ifndef MVIEWPORT_H
#define MVIEWPORT_H

#include "MView.h"

class MScrollBar;

class MViewPort : public MView
{
  public:
					MViewPort(
						MScrollBar*		inHScrollBar,
						MScrollBar*		inVScrollBar);

	void			SetShadowType(
						GtkShadowType	inShadowType);

	MScrollBar*	GetVScrollBar() const		{ return mVScrollBar; }

	MEventOut<void()>						eBoundsChanged;

  protected:

	virtual bool	OnConfigure(
						GdkEventConfigure*
										inEvent);
	
  private:
	MScrollBar*		mHScrollBar;
	MScrollBar*		mVScrollBar;
};

#endif // MVIEWPORT_H
