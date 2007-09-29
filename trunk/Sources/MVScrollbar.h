/*	$Id$
	Copyright Maarten L. Hekkelman
	Created 28-09-07 21:35:07
*/

#ifndef MVSCROLLBAR_H
#define MVSCROLLBAR_H

#include "MView.h"

class MVScrollbar : public MView
{
  public:
						MVScrollbar(
							GtkObject*		inAdjustment);
};

#endif // MVSCROLLBAR_H
