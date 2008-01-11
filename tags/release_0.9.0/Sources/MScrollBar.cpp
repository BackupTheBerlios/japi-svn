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
	Created 28-09-07 22:05:43
*/

#include "MJapieG.h"

#include <iostream>

#include "MScrollBar.h"

using namespace std;

MScrollBar::MScrollBar(
	bool			inVertical)
	: slOnValueChanged(this, &MScrollBar::OnValueChanged)
	, mAdjustment(GTK_ADJUSTMENT(gtk_adjustment_new(0, 0, 0, 0, 0, 0)))
	, mValue(0)
{
	if (inVertical)
		SetWidget(gtk_vscrollbar_new(mAdjustment), false, false);
	else
		SetWidget(gtk_hscrollbar_new(mAdjustment), false, false);
	
	slOnValueChanged.Connect(GetGtkWidget(), "value-changed");
}

uint32 MScrollBar::GetValue() const
{
	assert(gtk_adjustment_get_value(mAdjustment) == mValue);
	return mValue;
}

void MScrollBar::SetValue(
	uint32			inValue)
{
	mValue = inValue;
	gtk_adjustment_set_value(mAdjustment, inValue);
}

void MScrollBar::GetAdjustmentValues(
	uint32&			outLower,
	uint32&			outUpper,
	uint32&			outStepIncrement,
	uint32&			outPageIncrement,
	uint32&			outPageSize,
	uint32&			outValue) const
{
	outLower = static_cast<uint32>(mAdjustment->lower);
	outUpper = static_cast<uint32>(mAdjustment->upper);
	outStepIncrement = static_cast<uint32>(mAdjustment->step_increment);
	outPageIncrement = static_cast<uint32>(mAdjustment->page_increment);
	outPageSize = static_cast<uint32>(mAdjustment->page_size);
	outValue = static_cast<uint32>(mAdjustment->value);
}

void MScrollBar::SetAdjustmentValues(
	uint32			inLower,
	uint32			inUpper,
	uint32			inStepIncrement,
	uint32			inPageIncrement,
	uint32			inPageSize,
	uint32			inValue)
{
	assert(inValue == mValue);
	
	mAdjustment->lower = inLower;
	mAdjustment->upper = inUpper;
	mAdjustment->step_increment = inStepIncrement;
	mAdjustment->page_increment = inPageIncrement;
	mAdjustment->page_size = inPageSize;
	mAdjustment->value = inValue;
	
	gtk_adjustment_changed(mAdjustment);
}

void MScrollBar::OnValueChanged()
{
	uint32 value = static_cast<uint32>(gtk_adjustment_get_value(mAdjustment));
	
	if (value != mValue)
	{
		mValue = value;
		cbValueChanged(value);
	}
}
