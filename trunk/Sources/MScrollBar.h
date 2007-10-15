/*	$Id$
	Copyright Maarten L. Hekkelman
	Created 28-09-07 21:35:07
*/

#ifndef MSCROLLBAR_H
#define MSCROLLBAR_H

#include "MView.h"

class MScrollBar : public MView
{
  public:
						MScrollBar(
							bool			inVertical);

	GtkAdjustment*		GetAdjustment() const		{ return mAdjustment; }

	uint32				GetValue() const;

	void				SetValue(
							uint32			inValue);

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

  private:
	GtkAdjustment*		mAdjustment;
};

#endif // MSCROLLBAR_H
