//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "MLib.h"

#include <cmath>
#include <cstring>

#include "MDevice.h"
#include "MView.h"
#include "MWindow.h"
#include "MError.h"
#include "MUnicode.h"

#include "MDeviceImpl.h"

using namespace std;

// -------------------------------------------------------------------

MDevice::MDevice()
	: mImpl(MDeviceImp::Create())
{
}

MDevice::MDevice(
	MView*		inView,
	MRect		inRect,
	bool		inCreateOffscreen)
	: mImpl(MDeviceImp::Create(inView, inRect, inCreateOffscreen))
{
}

//MDevice::MDevice(
//	GtkPrintContext*	inPrintContext,
//	MRect				inRect,
//	int32				inPage)
//	: mImpl(new MCairoDeviceImp(inPrintContext, inRect, inPage))
//{
//}

MDevice::~MDevice()
{
	delete mImpl;
}

void MDevice::Save()
{
	mImpl->Save();
}

void MDevice::Restore()
{
	mImpl->Restore();
}

bool MDevice::IsPrinting() const
{
	int32 page;
	return mImpl->IsPrinting(page);
}

int32 MDevice::GetPageNr() const
{
	int32 page;
	if (not mImpl->IsPrinting(page))
		page = -1;
	return page;
}

MRect MDevice::GetBounds() const
{
	return mImpl->GetBounds();
}

void MDevice::SetFont(
	const string&	inFont)
{
	mImpl->SetFont(inFont);
}

void MDevice::SetForeColor(
	MColor		inColor)
{
	mImpl->SetForeColor(inColor);
}

MColor MDevice::GetForeColor() const
{
	return mImpl->GetForeColor();
}

void MDevice::SetBackColor(
	MColor		inColor)
{
	mImpl->SetBackColor(inColor);
}

MColor MDevice::GetBackColor() const
{
	return mImpl->GetBackColor();
}

void MDevice::ClipRect(
	MRect		inRect)
{
	mImpl->ClipRect(inRect);
}

//void MDevice::ClipRegion(
//	MRegion		inRegion)
//{
//	mImpl->ClipRegion(inRegion);
//}

void MDevice::EraseRect(
	MRect		inRect)
{
	mImpl->EraseRect(inRect);
}

void MDevice::FillRect(
	MRect		inRect)
{
	mImpl->FillRect(inRect);
}

void MDevice::StrokeRect(
	MRect		inRect,
	uint32		inLineWidth)
{
	mImpl->StrokeRect(inRect, inLineWidth);
}

void MDevice::FillEllipse(
	MRect		inRect)
{
	mImpl->FillEllipse(inRect);
}

void MDevice::CreateAndUsePattern(
	MColor		inColor1,
	MColor		inColor2)
{
	mImpl->CreateAndUsePattern(inColor1, inColor2);
}

float MDevice::GetAscent() const
{
	return mImpl->GetAscent();
}

float MDevice::GetDescent() const
{
	return mImpl->GetDescent();
}

float MDevice::GetLeading() const
{
	return mImpl->GetLeading();
}

int32 MDevice::GetLineHeight() const
{
	return mImpl->GetLineHeight();
}

float MDevice::GetXWidth() const
{
	return mImpl->GetXWidth();
}

void MDevice::DrawString(
	const string&	inText,
	float 			inX,
	float 			inY,
	uint32			inTruncateWidth,
	MAlignment		inAlign)
{
	mImpl->DrawString(inText, inX, inY, inTruncateWidth, inAlign);
}

void MDevice::SetText(
	const string&	inText)
{
	mImpl->SetText(inText);
}

void MDevice::SetTabStops(
	float			inTabWidth)
{
	mImpl->SetTabStops(inTabWidth);
}

void MDevice::SetTextColors(
	uint32				inColorCount,
	uint32				inColorIndices[],
	uint32				inOffsets[],
	MColor				inColors[])
{
	mImpl->SetTextColors(inColorCount, inColorIndices, inOffsets, inColors);
}

void MDevice::SetTextSelection(
	uint32			inStart,
	uint32			inLength,
	MColor			inSelectionColor)
{
	mImpl->SetTextSelection(inStart, inLength, inSelectionColor);
}

void MDevice::IndexToPosition(
	uint32			inIndex,
	bool			inTrailing,
	int32&			outPosition)
{
	mImpl->IndexToPosition(inIndex, inTrailing, outPosition);
}

bool MDevice::PositionToIndex(
	int32			inPosition,
	uint32&			outIndex)
{
	return mImpl->PositionToIndex(inPosition, outIndex);
}

void MDevice::DrawText(
	float			inX,
	float			inY)
{
	mImpl->DrawText(inX, inY);
}

float MDevice::GetTextWidth() const
{
	return mImpl->GetTextWidth();
}

void MDevice::BreakLines(
	uint32				inWidth,
	vector<uint32>&		outBreaks)
{
	mImpl->BreakLines(inWidth, outBreaks);
}

void MDevice::DrawCaret(
	float				inX,
	float				inY,
	uint32				inOffset)
{
	mImpl->DrawCaret(inX, inY, inOffset);
}

void MDevice::MakeTransparent(
	float				inOpacity)
{
	mImpl->MakeTransparent(inOpacity);
}

//GdkPixmap* MDevice::GetPixmap() const
//{
//	return mImpl->GetPixmap();
//}
//
void MDevice::SetDrawWhiteSpace(
	bool				inDrawWhiteSpace)
{
	mImpl->SetDrawWhiteSpace(inDrawWhiteSpace);
}

//void MDevice::DrawImage(
//	cairo_surface_t*	inImage,
//	float				inX,
//	float				inY,
//	float				inShear)
//{
//	mImpl->DrawImage(inImage, inX, inY, inShear);
//}
