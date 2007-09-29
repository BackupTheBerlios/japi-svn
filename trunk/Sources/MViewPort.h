/*	$Id$
	Copyright Maarten L. Hekkelman
	Created 28-09-07 21:24:39
*/

#ifndef MVIEWPORT_H
#define MVIEWPORT_H

#include "MView.h"

class MViewPort : public MView
{
  public:
				MViewPort(
					GtkObject*		inHAdjustment,
					GtkObject*		inVAdjustment);

	void		SetShadowType(
					GtkShadowType	inShadowType);
	
	void		Add(
					MView*			inView);
};

#endif // MVIEWPORT_H
