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

	void				GetAdjustmentValues(
							uint32&			outLower,
							uint32&			outUpper,
							uint32&			outStepIncrement,
							uint32&			outPageIncrement,
							uint32&			outPageSize,
							uint32&			outValue) const;

	void				SetAdjustmentValues(
							uint32			inLower,
							uint32			inUpper,
							uint32			inStepIncrement,
							uint32			inPageIncrement,
							uint32			inPageSize,
							uint32			inValue);
		
};

#endif // MVSCROLLBAR_H
