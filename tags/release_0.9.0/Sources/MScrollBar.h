/*
	Copyright (c) 2007, Maarten L. Hekkelman
	All rights reserved.

	Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions
	are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in
      the documentation and/or other materials provided with the
      distribution.
    * Neither the name of the Maarten L. Hekkelman nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
	"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
	LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
	FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
	COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
	INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
	(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
	SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
	HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
	STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
	ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
	OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/*	$Id$
	Copyright Maarten L. Hekkelman
	Created 28-09-07 21:35:07
*/

#ifndef MSCROLLBAR_H
#define MSCROLLBAR_H

#include "MView.h"
#include "MCallbacks.h"

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

	MCallBack<void(uint32)>	cbValueChanged;

  private:

	void				OnValueChanged();

	MSlot<void()>		slOnValueChanged;

	GtkAdjustment*		mAdjustment;
	uint32				mValue;
};

#endif // MSCROLLBAR_H
