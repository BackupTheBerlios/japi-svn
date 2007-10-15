/*	$Id$
	Copyright Maarten L. Hekkelman
	Created 28-09-07 22:05:43
*/

#include "MJapieG.h"

#include "MScrollBar.h"

using namespace std;

MScrollBar::MScrollBar(
	GtkObject*		inAdjustment,
	bool			inVertical)
	: MView(
		inVertical ? 
			gtk_vscrollbar_new(GTK_ADJUSTMENT(inAdjustment) :
			gtk_hscrollbar_new(GTK_ADJUSTMENT(inAdjustment)
		), false)
	, mAdjustment(GTK_ADJUSTMENT(inAdjustment))
{
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
	mAdjustment->lower = inLower;
	mAdjustment->upper = inUpper;
	mAdjustment->step_increment = inStepIncrement;
	mAdjustment->page_increment = inPageIncrement;
	mAdjustment->page_size = inPageSize;
	mAdjustment->value = inValue;
	
	gtk_adjustment_changed(mAdjustment);
}
