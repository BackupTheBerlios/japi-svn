/*
	Copyright (c) 2006, Maarten L. Hekkelman
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

#include "MJapieG.h"

#include <cmath>

#include "MTypes.h"
#include "MTextView.h"
#include "MDocument.h"
#include "MGlobals.h"
#include "MPreferences.h"
#include "MDocWindow.h"
#include "MStyles.h"
#include "MUtils.h"
#include "MDevice.h"

#ifndef NDEBUG
#include <iostream>
#endif

using namespace std;

namespace
{

//const uint32	kMTextViewKind = 'MTxt';

const int32
	kLeftMargin = 10;

const MColor
	kPCLineColor = MColor("#cce5ff"),
	kBreakpointColor = MColor("#5ea50c");

}

MColor		MTextView::sCurrentLineColor,
			MTextView::sMarkedLineColor,
			MTextView::sPCLineColor,
			MTextView::sBreakpointColor;

//MTextView* MTextView::sDraggingView = nil;

// ---------------------------------------------------------------------------
//	Default constructor

MTextView::MTextView(HIObjectRef inObject)
	: MView(inObject, kControlSupportsDragAndDrop|kHIViewFeatureGetsFocusOnClick)
	, eLineCountChanged(this, &MTextView::LineCountChanged)
	, eSelectionChanged(this, &MTextView::SelectionChanged)
	, eScroll(this, &MTextView::Scroll)
	, eShiftLines(this, &MTextView::ShiftLines)
	, eInvalidateDirtyLines(this, &MTextView::InvalidateDirtyLines)
	, eInvalidateAll(this, &MTextView::InvalidateAll)
	, eStylesChanged(this, &MTextView::StylesChanged)
	, eDocumentClosed(this, &MTextView::DocumentClosed)
	, eDocumentChanged(this, &MTextView::SetDocument)
	, mController(nil)
	, mDocument(nil)
	, mDrawForDragImage(false)
	, mLastClickTime(0)
	, mDragRef(nil)
{
	// Default constructor should set all data members to default values
	
	mImageOrigin.x = mImageOrigin.y = 0;

//	Install(kEventClassControl, kEventControlDraw,			this, &MTextView::DoControlDraw);
//	Install(kEventClassControl, kEventControlClick,			this, &MTextView::DoControlClick);
//	Install(kEventClassControl, kEventControlHitTest,		this, &MTextView::DoControlHitTest);
//	Install(kEventClassControl, kEventControlTrack,			this, &MTextView::DoControlTrack);
//	Install(kEventClassControl, kEventControlActivate,		this, &MTextView::DoControlActivate);
//	Install(kEventClassControl, kEventControlDeactivate,	this, &MTextView::DoControlDeactivate);
//	Install(kEventClassControl, kEventControlTrackingAreaEntered,
//															this, &MTextView::DoControlTrackingAreaEntered);
//	Install(kEventClassControl, kEventControlTrackingAreaExited,
//															this, &MTextView::DoControlTrackingAreaExited);
//	Install(kEventClassScrollable, kEventScrollableGetInfo,	this, &MTextView::DoScrollableGetInfo);
//	Install(kEventClassScrollable, kEventScrollableScrollTo,
//															this, &MTextView::DoScrollableScrollTo);
//	Install(kEventClassControl, kEventControlBoundsChanged,	this, &MTextView::DoBoundsChanged);
//	Install(kEventClassControl, kEventControlDragEnter,		this, &MTextView::DoDragEnter);
//	Install(kEventClassControl, kEventControlDragWithin,	this, &MTextView::DoDragWithin);
//	Install(kEventClassControl, kEventControlDragLeave,		this, &MTextView::DoDragLeave);
//	Install(kEventClassControl, kEventControlDragReceive,	this, &MTextView::DoDragReceive);
//	Install(kEventClassControl, kEventControlGetClickActivation,
//															this, &MTextView::DoGetClickActivation);
//	Install(kEventClassControl, kEventControlSetFocusPart,	this, &MTextView::DoSetFocusPart);
}

// Destructor

MTextView::~MTextView()
{
	::RemoveEventLoopTimer(mTimerRef);
}

// ---------------------------------------------------------------------------
//	Initialize								Direct Initializer		  [public]

OSStatus MTextView::Initialize(EventRef ioEvent)
{
	OSStatus result = MView::Initialize(ioEvent);

	THROW_IF_OSERROR(::SetControlDragTrackingEnabled(GetSysView(), true));
		
	static EventLoopTimerUPP sTimerUPP = ::NewEventLoopTimerUPP(&MTextView::EventLoopTimerProc);
	THROW_IF_OSERROR(::InstallEventLoopTimer(::GetCurrentEventLoop(),
		::TicksToEventTime(::GetCaretTime()), ::TicksToEventTime(::GetCaretTime()),
		sTimerUPP, this, &mTimerRef));

	InterfaceTypeList intf = { kUnicodeDocumentInterfaceType };
	::NewTSMDocument(1, intf, &mTSMDocument, reinterpret_cast<uint32>(this));
	::ActivateTSMDocument(mTSMDocument);	// why?

	MRect r;
	GetBounds(r);
	r.size.width = kLeftMargin;
	
	MCFRef<HIShapeRef> shape(::HIShapeCreateWithRect(&r), false);
	::HIViewNewTrackingArea(GetSysView(), shape, 1, &mTrackingLeftRef);
	
	GetBounds(r);
	r.origin.x += kLeftMargin;
	r.size.width -= kLeftMargin;

	shape = MCFRef<HIShapeRef>(::HIShapeCreateWithRect(&r), false);
	::HIViewNewTrackingArea(GetSysView(), shape, 2, &mTrackingMainRef);

	sCurrentLineColor = Preferences::GetColor("current line color", kCurrentLineColor);
	sMarkedLineColor = Preferences::GetColor("marked line color", kMarkedLineColor);

	sPCLineColor = kPCLineColor;
	sBreakpointColor = kBreakpointColor;
	
	
	StylesChanged();
	
	return result;
}

void MTextView::SetController(MController* inController)
{
	assert(mController == nil);
	
	mController = inController;
	
	SetDocument(mController->GetDocument());
}

//OSStatus MTextView::DoControlClick(EventRef ioEvent)
//{
//	OSStatus err = eventNotHandledErr;
//	
//	if (mLastClickTime + ::TicksToEventTime(::GetDblTime()) > ::GetEventTime(ioEvent))
//		mClickCount = mClickCount % 3 + 1;
//	else
//		mClickCount = 1;
//	mLastClickTime = ::GetEventTime(ioEvent);
//	
//	err = ::CallNextEventHandler(GetHandlerCallRef(), ioEvent);
//	
//	return err;
//}
//
//OSStatus MTextView::DoControlHitTest(EventRef ioEvent)
//{
//	ControlPartCode partCode = 1;
//	::SetEventParameter(ioEvent, kEventParamControlPart,
//		typeControlPartCode, sizeof(partCode), &partCode);
//	return noErr;
//}
//
//OSStatus MTextView::DoControlTrack(EventRef ioEvent)
//{
//	if (mDocument == nil)
//		return eventNotHandledErr;
//	
//	enum
//	{
//		eSelectRegular,
//		eSelectWords,
//		eSelectLines
//	} mode = eSelectRegular;
//	
//	switch (mClickCount)
//	{
//		case 1: mode = eSelectRegular; break;
//		case 2: mode = eSelectWords; break;
//		case 3: mode = eSelectLines; break;
//	}
//	
//	mDocument->Reset();
//
//	HIViewRef contentView;
//	THROW_IF_OSERROR(::GetRootControl(GetSysWindow(), &contentView));
//	MSelection selection = mDocument->GetSelection();
//
//	HIPoint where;
//	::GetEventParameter(ioEvent, kEventParamMouseLocation,
//		typeHIPoint, nil, sizeof(HIPoint), nil, &where);
//
//	if (InitiateDrag(where, ioEvent))
//		return noErr;
//
//	uint32 keyModifiers;
//	::GetEventParameter(ioEvent, kEventParamKeyModifiers,
//		typeUInt32, nil, sizeof(UInt32), nil, &keyModifiers);
//
//	uint32 line, anchor, caret;
//
//	where.x += mImageOrigin.x - kLeftMargin;
//	where.y += mImageOrigin.y;
//	
//	if (where.x < 0)
//		mode = eSelectLines;
//
//	mDocument->PositionToOffset(where, caret);
//	line = mDocument->OffsetToLine(caret);
//
//	if (keyModifiers & shiftKey)
//		anchor = selection.GetAnchor();
//	else
//		anchor = caret;
//	
//	mCaretVisible = true;
//	bool stillDown = true;
//	
//	if (mode == eSelectRegular)
//	{
//		do
//		{
//			mDocument->Select(anchor, caret);
//			
//			double timeOut = 10.0;
//			
//			if (ScrollToCaret())
//				timeOut = 0.01;
//		
//			Point pt;
//			MouseTrackingResult flags;
//			OSStatus err = ::TrackMouseLocationWithOptions(
//				::GetWindowPort(GetSysWindow()), 0, timeOut, &pt, nil, &flags);
//			
//			if (err != noErr and err != kMouseTrackingTimedOut)
//				THROW_IF_OSERROR(err);
//			
//			where.x = mImageOrigin.x - kLeftMargin + pt.h;
//			where.y = mImageOrigin.y + pt.v;
//			
//			::HIViewConvertPoint(&where, contentView, GetSysView());
//
//			mDocument->PositionToOffset(where, caret);
//	
//			stillDown = not (flags == kMouseTrackingMouseUp);
//		}
//		while (stillDown);
//	}
//	else if (mode == eSelectWords)
//	{
//		uint32 minAnchor, maxAnchor;
//		mDocument->FindWord(caret, minAnchor, maxAnchor);
//
//		do
//		{
//			if (caret < minAnchor)
//				mDocument->Select(maxAnchor, caret);
//			else if (caret > maxAnchor)
//				mDocument->Select(minAnchor, caret);
//			else
//				mDocument->Select(minAnchor, maxAnchor);
//			
//			double timeOut = 10.0;
//			
//			if (ScrollToCaret())
//				timeOut = 0.01;
//		
//			Point pt;
//			MouseTrackingResult flags;
//			OSStatus err = ::TrackMouseLocationWithOptions(
//				::GetWindowPort(GetSysWindow()), 0, timeOut, &pt, nil, &flags);
//			
//			if (err != noErr and err != kMouseTrackingTimedOut)
//				THROW_IF_OSERROR(err);
//			
//			where.x = pt.h;
//			where.y = pt.v;
//			
//			::HIViewConvertPoint(&where, contentView, GetSysView());
//			
//			where.x += mImageOrigin.x - kLeftMargin;
//			where.y += mImageOrigin.y;
//
//			mDocument->PositionToOffset(where, caret);
//			
//			uint32 c1, c2;
//			mDocument->FindWord(caret, c1, c2);
//			if (c1 != c2)
//			{
//				if (c1 < caret and caret < minAnchor)
//					caret = c1;
//				else if (c2 > caret and caret > maxAnchor)
//					caret = c2;
//			}
//	
//			stillDown = not (flags == kMouseTrackingMouseUp);
//		}
//		while (stillDown);
//	}
//	else if (mode == eSelectLines)
//	{
//		uint32 anchorLine = line;
//		uint32 caretLine = line;
//		
//		do
//		{
//			if (caretLine < anchorLine)
//				mDocument->Select(
//					mDocument->LineStart(anchorLine + 1), mDocument->LineStart(caretLine));
//			else
//				mDocument->Select(
//					mDocument->LineStart(anchorLine), mDocument->LineStart(caretLine + 1));
//			
//			double timeOut = 10.0;
//			
//			if (ScrollToCaret())
//				timeOut = 0.01;
//		
//			Point pt;
//			MouseTrackingResult flags;
//			OSStatus err = ::TrackMouseLocationWithOptions(
//				::GetWindowPort(GetSysWindow()), 0, timeOut, &pt, nil, &flags);
//			
//			if (err != noErr and err != kMouseTrackingTimedOut)
//				THROW_IF_OSERROR(err);
//			
//			where.x = pt.h;
//			where.y = pt.v;
//			
//			::HIViewConvertPoint(&where, contentView, GetSysView());
//			
//			where.x += mImageOrigin.x - kLeftMargin;
//			where.y += mImageOrigin.y;
//
//			mDocument->PositionToOffset(where, caret);
//			caretLine = mDocument->OffsetToLine(caret);
//	
//			stillDown = not (flags == kMouseTrackingMouseUp);
//		}
//		while (stillDown);
//	}
//
//	ControlPartCode partCode = 0;
//	::SetEventParameter(ioEvent, kEventParamControlPart,
//		typeControlPartCode, sizeof(partCode), &partCode);
//		
//	return noErr;
//}

bool MTextView::OnExposeEvent(
	GdkEventExpose*		inEvent)
{
	if (mDocument == nil)
		return false;
	
	MRect update(inEvent->area);

	MDevice dev(*this, update);

	try
	{
		mNeedsDisplay = false;
		
		if (not mDrawForDragImage)
			dev.EraseRect(update);
		
		int32 minLine = update.y / mLineHeight;
		if (minLine < 0)
			minLine = 0;
		
		uint32 maxLine = minLine + update.height / mLineHeight;
		if (maxLine >= mDocument->CountLines() and mDocument->CountLines() > 0)
			maxLine = mDocument->CountLines() - 1;
	
		for (uint32 line = minLine; line <= maxLine; ++line)
		{
			MRect lr = GetLineRect(line);

			if (update.Intersects(lr))
			{
//				cout << "drawing line: " << line << endl;
				DrawLine(line, device);
			}
		}
	}
	catch (std::exception& e)
	{
	}

//	if (mDragRef != nil)
//		DrawDragHilite(context);
	
	return true;
}

void MTextView::DrawLine(
	uint32				inLineNr,
	MDevice&			inDevice)
{
	assert(mDocument);
	
	string text;
	
	mDocument->GetStyledText(inLineNr, inDevice, text);
	
	bool trailingNL = false;
	if (inLineNr < mDocument->CountLines())
		trailingNL = mDocument->GetTextBuffer().GetChar(mDocument->LineEnd(inLineNr)) == '\n';
	
	MRect lineRect = GetLineRect(inLineNr);
	float indent = mDocument->GetLineIndentWidth(inLineNr);

	MDeviceContextSaver save(inDevice);
	float y = inLineNr * mLineHeight;
	float x = indent;

	MSelection selection = mDocument->GetSelection();

	if (not mDrawForDragImage)
	{
		MDeviceContextSaver save(inDevice);
		
		bool marked = mDocument->IsLineMarked(inLineNr);
		bool current = mDocument->OffsetToLine(selection.GetCaret()) == inLineNr and IsActive();
		bool fill = true;
		
		if (mDocument->GetPCLine() == inLineNr)
			dev.SetForeColor(sPCLineColor);
		else if (marked and current)
		{
			dev.SetForeColor(sMarkedLineColor);
			dev.SetBackColor(sCurrentLineColor);
			dev.SetUsePattern(true);
		}
		else if (marked)
			dev.SetForeColor(sMarkedLineColor);
		else if (current)
			dev.SetForeColor(sCurrentLineColor);
		else
			fill = false;
		
		if (fill)
		{
			dev.FillRect(lineRect);

			MRect r2(lineRect);
			r2.x -= r2.height / 2;
			r2.width = r2.height;
			
			dev.FillEllipse(r2);
		}
	}

//	// text input area, perhaps?
//	
//	const MTextInputAreaInfo& ii = mDocument->GetTextInputAreaInfo();
//	
//	if (ii.fOffset[kActiveInputArea] >= 0 and
//		mDocument->OffsetToLine(ii.fOffset[kActiveInputArea]) == inLineNr)
//	{
//		for (uint32 i = kSelectedRawText; i <= kSelectedConvertedText; i += 2)
//		{
//			if (ii.fOffset[i] < 0 or ii.fLength[i] <= 0)
//				continue;
//			
//			uint32 offset = ii.fOffset[i] - mDocument->LineStart(inLineNr);
//			uint32 length = ii.fLength[i];
//
//			dev.DrawTextHighLight(x, y, offset, length, false, false, IsActive());
////			
////			(void)::ATSUHighlightText(textLayout, myXLocation, myYLocation,
////				offset, length);
//		}
//	}

	// Highlight selection
	
	bool drawCaret = false;
	uint32 caretLine, caretColumn = 0;
	
	if (selection.IsEmpty() or
		selection.GetMaxLine(*mDocument) < inLineNr or
		selection.GetMinLine(*mDocument) > inLineNr)
	{
		if (not mDrawForDragImage)
			dev.DrawText(x, y);
		
		if (selection.IsEmpty())
		{
			caretLine = mDocument->OffsetToLine(mCaret);
			caretColumn = mCaret - mDocument->LineStart(caretLine);
			
			if (caretLine == inLineNr and mCaretVisible)
				drawCaret = true;
		}
	}
	else
	{
		uint32 selectionStart = selection.GetAnchor();
		uint32 selectionEnd = selection.GetCaret();
		if (selectionStart > selectionEnd)
			std::swap(selectionEnd, selectionStart);

		uint32 startOffset = mDocument->LineStart(inLineNr);
//		uint32 endOffset = mDocument->LineStart(inLineNr + 1);
		
//		if (not mDrawForDragImage)
//		{
//			if (selection.IsBlock())
//			{
//				
//			}
//			else if (selectionStart < endOffset and selectionEnd >= endOffset)
//			{
//				MCGContextSaver save(inContext);
//	
//				(void)::ATSUOffsetToPosition(textLayout,
//					text.length(), false, &mainCaret, &secondCaret, &isSplit);
//	
//				CGRect s = lineRect;
//				s.origin.x = std::min(Fix2X(mainCaret.fX), Fix2X(mainCaret.fDeltaX));
//	
//				if (IsActive() and not mDrawForDragImage)
//					::CGContextSetRGBFillColor(inContext, gHiliteColor.red,
//						gHiliteColor.green, gHiliteColor.blue, gHiliteColor.alpha);
//				else
//					::CGContextSetRGBFillColor(inContext, gInactiveHiliteColor.red,
//						gInactiveHiliteColor.green, gInactiveHiliteColor.blue,
//						gInactiveHiliteColor.alpha);
//					
//				::CGContextFillRect(inContext, s);
//			}
//		}
		
		bool fillBefore = false;
		bool fillAfter = false;
		
		if (selectionStart < startOffset)
		{
			fillBefore = true;
			selectionStart = 0;
		}
		else
			selectionStart -= startOffset;
		
		uint32 length = text.length();
		if (trailingNL)
			++length;
		
		selectionEnd -= startOffset;
		if (selectionEnd >= length)
		{
			fillAfter = true;
			selectionEnd = text.length();
		}
		
		if ((selectionEnd > selectionStart or fillBefore or fillAfter) and not mDrawForDragImage)
		{
			if (IsActive()
				dev.SetTextSelection(selectionStart, selectionEnd, gHiliteColor);
			else
				dev.SetTextSelection(selectionStart, selectionEnd, gInactiveHiliteColor);
//			dev.DrawTextHighLight(x, y, selectionStart, selectionEnd - selectionStart,
//				fillBefore, fillAfter, IsActive());
		}
		dev.DrawText(x, y);
	}
	
//	if (mDragRef != nil)
//	{
//		caretLine = mDocument->OffsetToLine(mDragCaret);
//		caretColumn = mDragCaret - mDocument->LineStart(caretLine);
//		
//		if (caretLine == inLineNr and mCaretVisible)
//			drawCaret = true;
//	}

	if (drawCaret)
	{
		if (caretColumn > text.length())	// duh
			caretColumn = text.length();

		dev.DrawCaret(caretColumn);
	}
}

OSStatus MTextView::DoScrollableGetInfo(EventRef ioEvent)
{
	if (mDocument == nil)
		return eventNotHandledErr;
	
	MRect frame;
	GetBounds(frame);

	HISize		imageSize = { 0 };
	HISize		viewSize = { 0 };
	HISize		lineSize = { 0 };
	HIPoint		origin = { 0 };

	lineSize.height = mLineHeight;
	imageSize.height = mDocument->CountLines() * mLineHeight;
	viewSize.height = frame.size.height;

	origin = mImageOrigin;
	
	::SetEventParameter(ioEvent, kEventParamImageSize, typeHISize, sizeof(HISize), &imageSize);
	::SetEventParameter(ioEvent, kEventParamViewSize, typeHISize, sizeof(HISize), &viewSize);
	::SetEventParameter(ioEvent, kEventParamLineSize, typeHISize, sizeof(HISize), &lineSize);
	::SetEventParameter(ioEvent, kEventParamOrigin, typeHIPoint, sizeof(HIPoint), &origin);
	
	return noErr;
}

OSStatus MTextView::DoScrollableScrollTo(EventRef ioEvent)
{
	HIPoint location;
	::GetEventParameter(ioEvent, kEventParamOrigin, typeHIPoint, nil,
		sizeof(location), nil, &location);
	
	DoScrollTo(location);
	
	return noErr;
}

void MTextView::DoScrollTo(HIPoint inWhere)
{
	HIPoint delta;
	
	if (inWhere.x < 0.0)
	{
		delta.x = -mImageOrigin.x;
		mImageOrigin.x = 0.0;
	}
	else
	{
		delta.x = mImageOrigin.x - inWhere.x;
		mImageOrigin.x = inWhere.x;
	}
	
	if (inWhere.y < 0.0)
	{
		delta.y = -mImageOrigin.y;
		mImageOrigin.y = 0.0;
	}
	else
	{
		delta.y = mImageOrigin.y - inWhere.y;
		mImageOrigin.y = inWhere.y;
	}
	
	MRect frame;
	GetBounds(frame);
	
	if (mNeedsDisplay)	// invalidate everything if a piece is invalid
		SetNeedsDisplay(true);
	else if (std::fabs(delta.x) < frame.size.width and std::fabs(delta.y) < frame.size.height)
		::HIViewScrollRect(GetSysView(), NULL, delta.x, delta.y);
	else
		SetNeedsDisplay(true);
}

void MTextView::ScrollToPosition(const HIPoint& inPosition)
{
	DoScrollTo(inPosition);
	
	MCarbonEvent event;
	event.MakeEvent(kEventClassScrollable, kEventScrollableInfoChanged);
	
	HIViewRef superView = ::HIViewGetSuperview(GetSysView());
	event.SendTo(::HIViewGetEventTarget(superView), kEventTargetDontPropagate);
}

void MTextView::GetScrollPosition(HIPoint& outPosition)
{
	outPosition = mImageOrigin;
}

void MTextView::LineCountChanged()
{
	MCarbonEvent event;
	event.MakeEvent(kEventClassScrollable, kEventScrollableInfoChanged);

	HIViewRef superView = ::HIViewGetSuperview(GetSysView());
	event.SendTo(::HIViewGetEventTarget(superView), kEventTargetDontPropagate);
}

void MTextView::Tick()
{
	if (mDocument == nil)
		return;
		
	MSelection sel = mDocument->GetSelection();

	if (mDragRef != nil)
	{
		InvalidateLine(mDocument->OffsetToLine(mDragCaret));
		mCaretVisible = not mCaretVisible;
	}
	else
	{
		uint32 caret = sel.GetCaret();
		
		if (mCaret != caret)
		{
			InvalidateLine(mDocument->OffsetToLine(mCaret));
			mCaret = caret;
		}

		if ((not IsActive() and mCaretVisible) or
			IsActive() and (sel.IsEmpty() or mCaretVisible))
		{
			if (mDocument->GetFastFindMode())
				mCaretVisible = true;
			else
				mCaretVisible = not mCaretVisible;
	
			InvalidateLine(mDocument->OffsetToLine(mCaret));
		}
	}
}

void MTextView::SelectionChanged(MSelection inNewSelection, string inRangeName)
{
	InvalidateDirtyLines();

	mCaretVisible = true;
	Tick();
	mCaretVisible = true;
	::SetEventLoopTimerNextFireTime(mTimerRef, ::TicksToEventTime(::GetCaretTime()));
	
	MRect bounds;
	GetBounds(bounds);
	bounds.origin.x += kLeftMargin;
	bounds.size.width -= kLeftMargin;
	
	MCFRef<HIShapeRef> shape(::HIShapeCreateWithRect(&bounds), false);

//	if (not mDocument->GetSelection().IsEmpty())
//	{
//		MRegion rgn(SelectionToRegion());
//		MCFRef<HIShapeRef> selectionShape(::HIShapeCreateWithQDRgn(rgn), false);
//		MCFRef<HIShapeRef> newShape(::HIShapeCreateDifference(shape, selectionShape), false);
//		shape = newShape;
//	}

	::HIViewChangeTrackingArea(mTrackingMainRef, shape);
	
//	if (inInvalidate)
//		::HIWindowFlush(GetSysWindow());
}

// ---------------------------------------------------------------------------
//	GetLineRect

MRect MTextView::GetLineRect(
	uint32		inLineNr) const
{
	MRect result;
	
	GetBounds(result);
	result.origin.y = inLineNr * mLineHeight - mImageOrigin.y;
	result.size.height = mLineHeight;
	
	return result;
}

// ---------------------------------------------------------------------------
//	InvalidateDirtyLines

void MTextView::InvalidateDirtyLines()
{
	assert(mDocument);
	
	MRect frame;
	GetBounds(frame);

	uint32 minLine = static_cast<uint32>(mImageOrigin.y / mLineHeight);
	if (minLine > 0)
		--minLine;
	
	uint32 maxLine = static_cast<uint32>(minLine + frame.size.height / mLineHeight + 1);
	if (maxLine >= mDocument->CountLines() and mDocument->CountLines() > 0)
		maxLine = mDocument->CountLines() - 1;
	
	bool invalidated = false;
	
	for (uint32 line = minLine; line <= maxLine; ++line)
	{
		if (mDocument->IsLineDirty(line))
		{
			invalidated = true;
			InvalidateLine(line);
		}
	}
}

// ---------------------------------------------------------------------------
//	InvalidateAll

void MTextView::InvalidateAll()
{
	mNeedsDisplay = true;
	SetNeedsDisplay(true);
}

// ---------------------------------------------------------------------------
//	InvalidateRect

void MTextView::InvalidateRect(
	MRect		inRect)
{
	mNeedsDisplay = true;
	::HIViewSetNeedsDisplayInRect(GetSysView(), &inRect, true);
}

// ---------------------------------------------------------------------------
//	InvalidateLine

void MTextView::InvalidateLine(
	uint32		inLine)
{
	InvalidateRect(GetLineRect(inLine));
}

// ---------------------------------------------------------------------------
//	ScrollToCaret

void MTextView::Scroll(MScrollMessage inCommand)
{
	assert(mDocument);
	
	MCarbonEvent event;
	HIPoint loc = mImageOrigin;
	
	MRect frame;
	GetBounds(frame);
	
	uint32 lineCount = mDocument->CountLines();
	uint32 linesPerPage = static_cast<uint32>(frame.size.height / mLineHeight);
	if (linesPerPage > 1)
		--linesPerPage;
	
	float maxLoc = (lineCount * mLineHeight) - frame.size.height;
	if (maxLoc < 0)
		maxLoc = 0;
	
	switch (inCommand)
	{
		case kScrollToStart:
			loc.y = 0;
			DoScrollTo(loc);
			break;
		
		case kScrollToEnd:
			loc.y = maxLoc;
			DoScrollTo(loc);
			break;
		
		case kScrollToCaret:
			ScrollToCaret();
			break;

		case kScrollToSelection:
			ScrollToSelection(false);
			break;
		
		case kScrollCenterSelection:
			ScrollToSelection(true);
			break;
		
		case kScrollLineUp:
			loc.y += mLineHeight;
			if (loc.y > maxLoc)
				loc.y = maxLoc;
			DoScrollTo(loc);
			break;

		case kScrollLineDown:
			loc.y -= mLineHeight;
			if (loc.y < 0)
				loc.y = 0;
			DoScrollTo(loc);
			break;

		case kScrollPageUp:
			loc.y -= (linesPerPage * mLineHeight);
			if (loc.y < 0)
				loc.y = 0;
			DoScrollTo(loc);
			break;

		case kScrollPageDown:
			loc.y += (linesPerPage * mLineHeight);
			if (loc.y > maxLoc)
				loc.y = maxLoc;
			DoScrollTo(loc);
			break;
		
		case kScrollForKiss:
			mSavedOrigin = mImageOrigin;
			ScrollToSelection(false);
			::HIWindowFlush(GetSysWindow());
			break;
		
		case kScrollReturnAfterKiss:
			DoScrollTo(mSavedOrigin);
//			::HIWindowFlush(GetSysWindow());
			break;
		
		case kScrollForDiff:
			ScrollForDiff();
			break;
		
		case kScrollToPC:
			if (mDocument != nil and mDocument->GetPCLine() < mDocument->CountLines())
				ScrollToLine(mDocument->GetPCLine(), false);
			break;
		
		default:
			break;
	}

	event.MakeEvent(kEventClassScrollable, kEventScrollableInfoChanged);
	HIViewRef superView = ::HIViewGetSuperview(GetSysView());
	event.SendTo(::HIViewGetEventTarget(superView), kEventTargetDontPropagate);
}

// ---------------------------------------------------------------------------
//	ScrollToCaret

bool MTextView::ScrollToCaret()
{
	assert(mDocument);
	
	MRect frame;
	GetBounds(frame);
	
	int32 top, bottom;
	top = static_cast<int32>(mImageOrigin.y);
	bottom = static_cast<int32>(top + frame.size.height);
	
	MSelection selection = mDocument->GetSelection();
	
	uint32 caret;
	
	if (mDragRef)
	{
		caret = mDragCaret;
		top += mLineHeight;
		bottom -= mLineHeight;
	}
	else
		caret = selection.GetCaret();
	
	uint32 line;
	int32 x, y;
	
	mDocument->OffsetToPosition(caret, line, x);
	y = line * mLineHeight;
	
	if (selection.GetAnchor() < caret and
		line < mDocument->CountLines() and
		mDocument->LineStart(line) == caret)
	{
		--line;
		y -= mLineHeight;
	}
	
	HIPoint newOrigin = mImageOrigin;
	
	if (y < top)
	{
		newOrigin.y = line * mLineHeight;
		if (mDragRef)
			newOrigin.y -= mLineHeight;
	}
	else if (y + mLineHeight > bottom)
	{
		newOrigin.y = (line + 1) * mLineHeight - (bottom - top) + 2;
		if (mDragRef)
			newOrigin.y += mLineHeight;
	}
	
	if (x < mImageOrigin.x + 3)
		newOrigin.x = std::max(0, static_cast<int32>(x - 3));
	else if (x > mImageOrigin.x + frame.size.width - 3 - kLeftMargin)
		newOrigin.x = x - frame.size.width + 3 + kLeftMargin;
	
	bool result = false;
	
	if (newOrigin.x != mImageOrigin.x or newOrigin.y != mImageOrigin.y)
	{
		MCarbonEvent er;
		DoScrollTo(newOrigin);

		MCarbonEvent event;
		event.MakeEvent(kEventClassScrollable, kEventScrollableInfoChanged);
		HIViewRef superView = ::HIViewGetSuperview(GetSysView());
		event.SendTo(::HIViewGetEventTarget(superView), kEventTargetDontPropagate);

		result = true;
	}
	
	return result;
}

// ---------------------------------------------------------------------------
//	ScrollToPC

void MTextView::ScrollToLine(
	uint32		inLineNr,
	bool		inForceCenter)
{
	assert(mDocument);
	
	MCarbonEvent event;

	MRect frame;
	GetBounds(frame);
	
	float y;
		
	if (not inForceCenter)
	{
		y = inLineNr * mLineHeight;
		
		if (y < mImageOrigin.y)
			inForceCenter = true;
	}
			
	if (not inForceCenter)
	{
		y = (inLineNr + 1) * mLineHeight;
	 
		if (y > mImageOrigin.y + frame.size.height)
			inForceCenter = true;
	}

	if (inForceCenter)
	{
		uint32 midLine = static_cast<uint32>(frame.size.height / (3 * mLineHeight));

		if (inLineNr < midLine)
			inLineNr = 0;
		else
			inLineNr -= midLine;

		HIPoint loc;
		loc.x = 0;
		loc.y = inLineNr * mLineHeight;
		
		DoScrollTo(loc);

		event.MakeEvent(kEventClassScrollable, kEventScrollableInfoChanged);
		HIViewRef superView = ::HIViewGetSuperview(GetSysView());
		event.SendTo(::HIViewGetEventTarget(superView), kEventTargetDontPropagate);
	}
}

// ---------------------------------------------------------------------------
//	ScrollToSelection

void MTextView::ScrollToSelection(
	bool	inForceCenter)
{
	assert(mDocument);
	
	MCarbonEvent event;
	uint32 line;

	MRect frame;
	GetBounds(frame);
	
	MSelection selection(mDocument->GetSelection());
	float y;
		
	if (not inForceCenter)
	{
		line = selection.GetMinLine(*mDocument);
		y = line * mLineHeight;
		
		if (y < mImageOrigin.y)
			inForceCenter = true;
	}
			
	if (not inForceCenter)
	{
		line = selection.GetMaxLine(*mDocument);
		y = (line + 1) * mLineHeight;
	 
		if (y > mImageOrigin.y + frame.size.height)
			inForceCenter = true;
	}

	if (inForceCenter)
	{
		uint32 midLine = static_cast<uint32>(frame.size.height / (3 * mLineHeight));

		line = selection.GetMinLine(*mDocument);
		if (line < midLine)
			line = 0;
		else
			line -= midLine;

		HIPoint loc;
		loc.x = 0;
		loc.y = line * mLineHeight;
		
		DoScrollTo(loc);
	}
   
	ScrollToCaret();

	if (/*not fDoc->GetSoftWrap() and */not selection.IsEmpty())
	{
		int32 ax, cx;
		uint32 line;
		mDocument->OffsetToPosition(selection.GetMinOffset(*mDocument), line, ax);
		mDocument->OffsetToPosition(selection.GetMaxOffset(*mDocument), line, cx);

		float center = (ax + cx) / 2;

		if (center < mImageOrigin.x + 10 or
			center > mImageOrigin.x + frame.size.width - 10)
		{
			HIPoint loc;
			loc.y = mImageOrigin.y;
			loc.x = mImageOrigin.x + center -
				(mImageOrigin.x + frame.size.width / 2);
			DoScrollTo(loc);
		}
	}

	event.MakeEvent(kEventClassScrollable, kEventScrollableInfoChanged);
	HIViewRef superView = ::HIViewGetSuperview(GetSysView());
	event.SendTo(::HIViewGetEventTarget(superView), kEventTargetDontPropagate);
}

// ---------------------------------------------------------------------------
//	ScrollForDiff

void MTextView::ScrollForDiff()
{
	assert(mDocument);
	
	MRect frame;
	GetBounds(frame);
	
	int32 center = static_cast<int32>(frame.size.height / (4 * mLineHeight));

	uint32 offset = mDocument->GetSelection().GetMinOffset();
	
	uint32 line;
	int32 x;
	
	mDocument->OffsetToPosition(offset, line, x);

	HIPoint newOrigin = mImageOrigin;
	newOrigin.y = (static_cast<int32>(line) - center) * mLineHeight;
	if (newOrigin.y < 0)
		newOrigin.y = 0;
	
	if (newOrigin.y != mImageOrigin.y)
	{
		MCarbonEvent er;
		DoScrollTo(newOrigin);

		MCarbonEvent event;
		event.MakeEvent(kEventClassScrollable, kEventScrollableInfoChanged);
		HIViewRef superView = ::HIViewGetSuperview(GetSysView());
		event.SendTo(::HIViewGetEventTarget(superView), kEventTargetDontPropagate);
	}
}

// -----------------------------------------------------------------------------
// ShiftLines

void MTextView::ShiftLines(uint32 inFromLine, int32 inDelta)
{
	// get our frame
	MRect frame;
	GetBounds(frame);
	
	// calculate the rectangle that needs to be scrolled
	MRect r = frame;

	// the top of the scrolling rect
	r.origin.y = inFromLine * mLineHeight - mImageOrigin.y;
	if (r.origin.y < frame.origin.y)
		r.origin.y = frame.origin.y;
	
	// and the height
	r.size.height = frame.size.height - r.origin.y;

	float dy = inDelta * static_cast<int32>(mLineHeight);
	
	// if the rect is not empty and lies within our frame
	if (r.size.height > 0 and
		r.origin.y <= frame.size.height and
		r.origin.y + r.size.height >= 0 and
		std::fabs(dy) < frame.size.height and
		not mNeedsDisplay)
	{
		::HIViewScrollRect(GetSysView(), &r, 0.f, dy);
	}
	
	// otherwise just invalidate the entire view
	
	else
		SetNeedsDisplay(true);
}

void MTextView::GetVisibleLineSpan(
	uint32&				outFirstLine,
	uint32&				outLastLine) const
{
	MRect frame;
	GetBounds(frame);
	
	outFirstLine = static_cast<uint32>(mImageOrigin.y / mLineHeight);
	
	uint32 cnt = static_cast<uint32>(frame.size.height / mLineHeight);
	if (cnt > 0)
		--cnt;
	
	outLastLine = outFirstLine + cnt;
}

OSStatus MTextView::DoControlActivate(EventRef ioEvent)
{
	::ActivateTSMDocument(mTSMDocument);
	SetNeedsDisplay(true);
	
	if (mDocument != nil)
	{
		mDocument->SetTargetTextView(this);
		mDocument->MakeFirstDocument();
		mDocument->CheckFile();
	}
	
	return noErr;
}

OSStatus MTextView::DoControlDeactivate(EventRef ioEvent)
{
	::DeactivateTSMDocument(mTSMDocument);
	
	mNeedsDisplay = true;
	SetNeedsDisplay(true);
	
	if (mDocument != nil)
		mDocument->Reset();
	
	return noErr;
}

OSStatus MTextView::DoControlTrackingAreaEntered(EventRef ioEvent)
{
	HIPoint where;
	::GetEventParameter(ioEvent, kEventParamMouseLocation, typeHIPoint, nil,
		sizeof(where), nil, &where);
	
	HIViewTrackingAreaRef area;
	::GetEventParameter(ioEvent, kEventParamHIViewTrackingArea, typeHIViewTrackingAreaRef,
		nil, sizeof(area), nil, &area);
	
	if (IsActive())
	{
		if (area == mTrackingLeftRef)
		{
			const Cursor kSelectLineCursor =
			{
#if __BIG_ENDIAN__
				{
					0x0000, 0x0002, 0x0006, 0x000E, 0x001E, 0x003E, 0x007E, 0x00FE,
					0x01FE, 0x003E, 0x0036, 0x0062, 0x0060, 0x00C0, 0x00C0, 0x0000
				},
				{
					0x0003, 0x0007, 0x000F, 0x001F, 0x003F, 0x007F, 0x00FF, 0x01FF,
					0x03FF, 0x07FF, 0x007F, 0x00F7, 0x00F3, 0x01E1, 0x01E0, 0x01C0
				},
#else
				{
					0x0000, 0x0200, 0x0600, 0x0E00, 0x1E00, 0x3E00, 0x7E00, 0xFE00,
					0xFE01, 0x3E00, 0x3600, 0x6200, 0x6000, 0xC000, 0xC000, 0x0000
				},
				{
					0x0300, 0x0700, 0x0F00, 0x1F00, 0x3F00, 0x7F00, 0xFF00, 0xFF01,
					0xFF03, 0xFF07, 0x7F00, 0xF700, 0xF300, 0xE101, 0xE001, 0xC001
				},
#endif
				0x0000, 0x000F
			};

			::SetCursor(&kSelectLineCursor);
			mTrackCursorInTick = false;
		}
		else
		{
			mTrackCursorInTick = true;
			::QDSetNamedPixMapCursor(
				::QDGetCursorNameForSystemCursor(kQDXIBeamXORCursor));
		}
	}
	else
	{
		mTrackCursorInTick = false;
		::QDSetNamedPixMapCursor(
			::QDGetCursorNameForSystemCursor(kQDXArrowCursor));
	}
	return noErr;
}

OSStatus MTextView::DoControlTrackingAreaExited(EventRef ioEvent)
{
	mTrackCursorInTick = false;

	HIPoint where;
	::GetEventParameter(ioEvent, kEventParamMouseLocation, typeHIPoint, nil,
		sizeof(where), nil, &where);
	
	MRect bounds;
	GetBounds(bounds);
	
	if (where.x < bounds.origin.x or where.x > bounds.origin.x + bounds.size.width or
		where.y < bounds.origin.y or where.y > bounds.origin.y + bounds.size.height)
	{
		::QDSetNamedPixMapCursor(
			::QDGetCursorNameForSystemCursor(kQDXArrowCursor));
	}

	return noErr;
}

OSStatus MTextView::DoBoundsChanged(EventRef ioEvent)
{
	MRect r;
	GetBounds(r);
	r.size.width = kLeftMargin;
	
	MCFRef<HIShapeRef> shape(::HIShapeCreateWithRect(&r), false);
	::HIViewChangeTrackingArea(mTrackingLeftRef, shape);
	
	GetBounds(r);
	r.origin.x += kLeftMargin;
	r.size.width -= kLeftMargin;

	shape = MCFRef<HIShapeRef>(::HIShapeCreateWithRect(&r), false);

//	if (mDocument and not mDocument->GetSelection().IsEmpty())
//	{
//		MRegion rgn(SelectionToRegion());
//		MCFRef<HIShapeRef> selectionShape(::HIShapeCreateWithQDRgn(rgn), false);
//		shape = ::HIShapeCreateDifference(shape, selectionShape);
//	}

	::HIViewChangeTrackingArea(mTrackingMainRef, shape);
	
	eBoundsChanged();
	
	return noErr;
}

pascal void	MTextView::EventLoopTimerProc(EventLoopTimerRef inTimer, void *inUserData)
{
	try
	{
		MTextView* self = static_cast<MTextView*>(inUserData);
		self->Tick();
	}
	catch (std::exception& e)
	{
		MError::DisplayError(e);
	}
	catch (...) {}
}

OSStatus MTextView::DoDragEnter(EventRef ioEvent)
{
	OSStatus err = eventNotHandledErr;
	
	try
	{
		DragRef dragRef;
		THROW_IF_OSERROR(::GetEventParameter(ioEvent, kEventParamDragRef, typeDragRef, nil,
			sizeof(dragRef), nil, &dragRef));
		
		UInt16 itemCount;
		THROW_IF_OSERROR(::CountDragItems(dragRef, &itemCount));
		
		Boolean accept = false;
		
		for (UInt16 ix = 1; ix <= itemCount and accept == false; ++ix)
		{
			ItemReference itemRef;
			THROW_IF_OSERROR(::GetDragItemReferenceNumber(dragRef, ix, &itemRef));
			
			UInt16 flavorCount;
			if (::CountDragItemFlavors(dragRef, itemRef, &flavorCount) != noErr)
				return false;
		
			for (UInt16 index = 1; index <= flavorCount and accept == false; ++index)
			{
				FlavorType type;
				if (::GetFlavorType(dragRef, itemRef, index, &type) != noErr)
					continue;
		
				accept =
					type == kScrapFlavorTypeText or
					type == kScrapFlavorTypeUnicode or
					type == kScrapFlavorTypeUTF16External;
		
//				if (type == flavorTypeHFS)
//				{
//					HFSFlavor hfs;
//					long size = sizeof(hfs);
//					
//					if (::GetFlavorData(fDragRef, itemRef, flavorTypeHFS, &hfs, &size, 0) != noErr)
//						return false;
//					
//					FInfo info;
//					if (::FSpGetFInfo(&hfs.fileSpec, &info) != noErr)
//						return false;
//					
//					return info.fdType == 'TEXT';
//				}
//				else if (type == inDataType)
//					return true;
			}
		}
		
		if (accept and mDocument != nil)
		{
			err = noErr;
			THROW_IF_OSERROR(::SetEventParameter(ioEvent, kEventParamControlWouldAcceptDrop,
				typeBoolean, sizeof(accept), &accept));
		}
	}
	catch (...)
	{
		err = eventNotHandledErr;
	}
	
	return err;
}

OSStatus MTextView::DoDragWithin(EventRef ioEvent)
{
	try
	{
		DragRef dragRef;
		THROW_IF_OSERROR(::GetEventParameter(ioEvent, kEventParamDragRef, typeDragRef, nil,
			sizeof(dragRef), nil, &dragRef));
		
		Point where, dummy;
		THROW_IF_OSERROR(::GetDragMouse(dragRef, &where, &dummy));
		
		HIPoint pt = { where.h, where.v };
		ConvertFromGlobal(pt);

		pt.x += mImageOrigin.x - kLeftMargin;
		pt.y += mImageOrigin.y;
		
		uint32 caret;
		mDocument->PositionToOffset(pt, caret);
		
		MRect bounds;
		GetBounds(bounds);
		
		if (mDragCaret != caret)
		{
			InvalidateLine(mDocument->OffsetToLine(mDragCaret));
			mDragCaret = caret;
			InvalidateLine(mDocument->OffsetToLine(mDragCaret));

			double timeOut = 0.01;
			
			while (ScrollToCaret())
			{
				MouseTrackingResult flags;
				OSStatus err = ::TrackMouseLocationWithOptions(
					::GetWindowPort(GetSysWindow()), 0, timeOut, &where, nil, &flags);
				
				if (err != noErr and err != kMouseTrackingTimedOut)
					THROW_IF_OSERROR(err);
				
				pt.x = where.h;
				pt.y = where.v;
				ConvertFromRoot(pt);
				
				if (not ::CGRectContainsPoint(bounds, pt))
					break;
				
				pt.x += mImageOrigin.x - kLeftMargin;
				pt.y += mImageOrigin.y;
	
				mDocument->PositionToOffset(pt, caret);

				if (mDragCaret != caret)
				{
					InvalidateLine(mDocument->OffsetToLine(mDragCaret));
					mDragCaret = caret;
					InvalidateLine(mDocument->OffsetToLine(mDragCaret));
				}
		
				if (flags == kMouseTrackingMouseUp)
					break;
			}
		}
		
		if (mDragRef == nil)
		{
			mDragRef = dragRef;
			InvalidateAll();
		}
	}
	catch (...) {}
	return noErr;
}

OSStatus MTextView::DoDragLeave(EventRef ioEvent)
{
	mDocument->TouchLine(mDocument->OffsetToLine(mDragCaret));
	InvalidateDirtyLines();
	
//	if (mDragRef != nil)
//		::HideDragHilite(mDragRef);

	InvalidateAll();
	
	mDragRef = nil;
	return noErr;
}

OSStatus MTextView::DoDragReceive(EventRef ioEvent)
{
	// short cut, make sure we accept the drag
	MSelection selection = mDocument->GetSelection();

	// use brute force... sigh
	InvalidateAll();

	if (sDraggingView == this and
		mDragCaret >= selection.GetMinOffset(*mDocument) and
		mDragCaret <= selection.GetMaxOffset(*mDocument))
	{
		return dragNotAcceptedErr;
	}
	
	OSStatus err = noErr;
	
	try
	{
		UInt16 itemCount;
		THROW_IF_OSERROR(::CountDragItems(mDragRef, &itemCount));
		
		bool accept = false;
		
		SInt16 modifiers, downModifiers, upModifiers;
		::GetDragModifiers(mDragRef, &modifiers, &downModifiers, &upModifiers);
		
		bool move = (sDraggingView == this) and (modifiers & optionKey) == 0;
		
		for (UInt16 ix = 1; ix <= itemCount and accept == false; ++ix)
		{
			ItemReference itemRef;
			THROW_IF_OSERROR(::GetDragItemReferenceNumber(mDragRef, ix, &itemRef));
			
			Size size;
			if (::GetFlavorDataSize(mDragRef, itemRef, kScrapFlavorTypeUnicode, &size) == noErr)
			{
				auto_array<UniChar> txt(new UniChar[size / 2]);
				THROW_IF_OSERROR(::GetFlavorData(mDragRef, itemRef, kScrapFlavorTypeUnicode,
					txt.get(), &size, 0));
				
				replace(txt.get(), txt.get() + (size / 2), '\r', '\n');
				mDocument->Drop(mDragCaret, txt.get(), size / 2, move);

				accept = true;
			}
			else if (::GetFlavorDataSize(mDragRef, itemRef, kScrapFlavorTypeUTF16External, &size) == noErr)
			{
				auto_array<UniChar> txt(new UniChar[size / 2]);
				THROW_IF_OSERROR(::GetFlavorData(mDragRef, itemRef, kScrapFlavorTypeUTF16External,
					txt.get(), &size, 0));
				
				UniChar kBOM = 0xfeff;
				
				if (txt[0] != kBOM)
				{
					auto_ptr<MDecoder> decoder;
					const char* b = reinterpret_cast<const char*>(txt.get());
					
					if (kEncodingNative == kEncodingUTF16BE)
						decoder.reset(MDecoder::GetDecoder(kEncodingUTF16LE, b, size));
					else
						decoder.reset(MDecoder::GetDecoder(kEncodingUTF16BE, b, size));
					
					ustring s;
					decoder->GetText(s);
					replace(s.begin(), s.end(), '\r', '\n');
					mDocument->Drop(mDragCaret, s.c_str(), s.length(), move);
				}
				else
				{
					replace(txt.get(), txt.get() + (size / 2), '\r', '\n');
					mDocument->Drop(mDragCaret, txt.get(), size / 2, move);
				}

				accept = true;
			}
			else if (::GetFlavorDataSize(mDragRef, itemRef, kScrapFlavorTypeText, &size) == noErr)
			{
				auto_array<char> txt(new char[size]);
				THROW_IF_OSERROR(::GetFlavorData(mDragRef, itemRef, kScrapFlavorTypeText,
					txt.get(), &size, 0));
				
				auto_ptr<MDecoder> decoder(MDecoder::GetDecoder(kEncodingMacOSRoman, txt.get(), size));
				
				ustring s;
				decoder->GetText(s);
				
				replace(s.begin(), s.end(), '\r', '\n');
				mDocument->Drop(mDragCaret, s.c_str(), s.length(), move);

				accept = true;
			}
		}
		
		if (not accept)
			err = dragNotAcceptedErr;
	}
	catch (std::exception& e)
	{
		MError::DisplayError(e);
		err = dragNotAcceptedErr;
	}

	mDragRef = nil;
	return err;
}

OSStatus MTextView::DoGetClickActivation(EventRef ioEvent)
{
	HIPoint where;
	(void)::GetEventParameter(ioEvent, kEventParamMouseLocation, typeHIPoint, nil,
		sizeof(where), nil, &where);

	ClickActivationResult result = kActivateAndIgnoreClick;
	
	if (InitiateDrag(where, ioEvent))
		result = kDoNotActivateAndIgnoreClick;
	
	(void)::SetEventParameter(ioEvent, kEventParamClickActivation, typeClickActivationResult,
		sizeof(result), &result);

	return noErr;
}

// inWhere should be in view local coordinates
bool MTextView::IsPointInSelection(HIPoint inWhere)
{
	bool result = false;
	if (mDocument != nil and inWhere.x > kLeftMargin)
	{
		MSelection selection(mDocument->GetSelection());
		
		if (not selection.IsEmpty())
		{
			uint32 caret;
			inWhere.x += mImageOrigin.x - kLeftMargin;
			inWhere.y += mImageOrigin.y;
			
			mDocument->PositionToOffset(inWhere, caret);
		
			if (caret >= selection.GetMinOffset(*mDocument) and
				caret <= selection.GetMaxOffset(*mDocument))
			{
				// we're not there yet. Make sure we didn't click below the selection
				
				uint32 line = mDocument->OffsetToLine(caret);
				
				uint32 y = line * mLineHeight;
				
				result = inWhere.y >= y and inWhere.y <= y + mLineHeight;
			}
		}
	}
	
	return result;
}

//RgnHandle MTextView::SelectionToRegion()
//{
//	MRegion rgn;
//	
//	if (mDocument != nil)
//	{
//		MSelection selection = mDocument->GetSelection();
//		
//		MRect bounds;
//		GetBounds(bounds);
//		bounds.origin.x += kLeftMargin;
//		
//		MRect r = bounds;
//		r.origin.y += selection.GetMinLine(*mDocument) * mLineHeight - mImageOrigin.y;
//		r.size.height = mLineHeight * selection.CountLines(*mDocument);
//		
//		r = ::CGRectIntersection(r, bounds);
//		
//		HIPoint p = { 0 };
//		ConvertToGlobal(p);
//		
//		r = ::CGRectOffset(r, p.x, p.y);
//		rgn.Set(r);
//	}
//	
//	return rgn.Release();
//}

void MTextView::DoDragSendData(FlavorType theType,
	DragItemRef theItemRef, DragRef theDrag)
{
	ustring s;

	mDocument->GetSelectedText(s);

	if (theType == kScrapFlavorTypeUnicode)
	{
		THROW_IF_OSERROR(::SetDragItemFlavorData(theDrag, theItemRef, theType,
			s.c_str(), s.length() * sizeof(UniChar), 0));
	}
	else if (theType == kScrapFlavorTypeText)
	{
		MEncoder* encoder = MEncoder::GetEncoder(kEncodingMacOSRoman);
		encoder->SetText(s);
		THROW_IF_OSERROR(::SetDragItemFlavorData(theDrag, theItemRef, theType,
			encoder->Peek(), encoder->GetBufferSize(), 0));
	}
	else
		THROW(("invalid flavor requested"));
}
	
pascal OSErr MTextView::DoDragSendDataCallback(FlavorType theType,
	void *dragSendRefCon, DragItemRef theItemRef, DragRef theDrag)
{
	OSErr err = noErr;
	try
	{
		MTextView* textView = static_cast<MTextView*>(dragSendRefCon);
		textView->DoDragSendData(theType, theItemRef, theDrag);
	}
	catch (std::exception& inErr)
	{
		err = cantGetFlavorErr;
	}
	
	return err;
}

bool MTextView::InitiateDrag(HIPoint inWhere, EventRef inEvent)
{
	bool result = false;
	
	if (IsPointInSelection(inWhere))
	{
		ConvertToGlobal(inWhere);

		Point pt;
		pt.h = static_cast<int16>(inWhere.x);
		pt.v = static_cast<int16>(inWhere.y);
	
		if (::WaitMouseMoved(pt))
		{
			CGImageRef imageRef;
			CGRect bounds;
			
			mDrawForDragImage = true;
			::HIViewCreateOffscreenImage(GetSysView(), 0, &bounds, &imageRef);
			mDrawForDragImage = false;

			MCFRef<CGImageRef> image(imageRef, false);
			
			MSelection selection = mDocument->GetSelection();
			
			MRect minLine = GetLineRect(selection.GetMinLine(*mDocument));
			MRect maxLine = GetLineRect(selection.GetMaxLine(*mDocument));
			
			CGRect r = ::CGRectUnion(minLine, maxLine);
			MCFRef<CGImageRef> subImage(::CGImageCreateWithImageInRect(image, r), false);
			
			HIPoint offset;
			offset.x = -inWhere.x;
			offset.y = -inWhere.y + minLine.origin.y;
			ConvertToGlobal(offset);

			try
			{
				DragRef dragRef;
				::NewDrag(&dragRef);
				
				EventRecord er;
				::ConvertEventRefToEventRecord(inEvent, &er);
				
				THROW_IF_OSERROR(::SetDragSendProc(dragRef, sDragSendDataUPP, this));
				THROW_IF_OSERROR(::SetDragImageWithCGImage(dragRef,
					subImage, &offset, 0));
				
				THROW_IF_OSERROR(::AddDragItemFlavor(dragRef, 1, kScrapFlavorTypeUnicode,
					nil, 0, 0));
				THROW_IF_OSERROR(::AddDragItemFlavor(dragRef, 2, kScrapFlavorTypeText,
					nil, 0, 0));
				
				sDraggingView = this;
				result = true;
				
				RgnHandle rgn = ::NewRgn();
				(void)::TrackDrag(dragRef, &er, rgn);
				::DisposeRgn(rgn);
			}
			catch (std::exception& e)
			{
				MError::DisplayError(e);
			}
			
			sDraggingView = nil;
		}
	}
	
	return result;
}

void MTextView::DrawDragHilite(CGContextRef inContext)
{
	MCGContextSaver save(inContext);
	
	RGBColor c;
	if (::GetDragHiliteColor(GetSysWindow(), &c) == noErr)
	{
		::CGContextSetRGBStrokeColor(inContext, c.red / 65536.f,
			c.green / 65536.f, c.blue / 65536.f, 1.0f);
		::CGContextSetLineWidth(inContext, 2.f);
		
		MRect bounds;
		GetBounds(bounds);

		::CGContextClipToRect(inContext, bounds);

		bounds = ::CGRectInset(bounds, 1.f, 1.f);
		::CGContextStrokeRect(inContext, bounds);
	}
}

OSStatus MTextView::DoContextualMenuClick(EventRef inEvent)
{
	return noErr;
}

OSStatus MTextView::DoSetFocusPart(EventRef ioEvent)
{
	ControlPartCode part;
	
	::GetEventParameter(ioEvent, kEventParamMouseLocation,
		typeControlPartCode, nil, sizeof(part), nil, &part);
	
//	if ((part != kControlFocusNoPart) != mHasFocus)
//	{
//		mHasFocus = not mHasFocus;
//		
//		SetNeedsDisplay(true);
//	}

	return noErr;
}

uint32 MTextView::GetWrapWidth() const
{
	MRect r;
	GetBounds(r);
	return static_cast<uint32>(r.size.width) - kLeftMargin;
}

void MTextView::StylesChanged()
{
	MDevice dev;
	mDescent = dev.GetDescent();
	mLineHeight = dev.GetAscent() + mDescent + dev.GetLeading();
}

void MTextView::DocumentClosed()
{
	SetDocument(nil);
}

void MTextView::SetDocument(MDocument* inDocument)
{
	if (mDocument != nil)
	{
		RemoveRoute(eLineCountChanged, mDocument->eLineCountChanged);
		RemoveRoute(eSelectionChanged, mDocument->eSelectionChanged);
		RemoveRoute(eScroll, mDocument->eScroll);
		RemoveRoute(eShiftLines, mDocument->eShiftLines);
		RemoveRoute(eInvalidateDirtyLines, mDocument->eInvalidateDirtyLines);
		RemoveRoute(eBoundsChanged, mDocument->eBoundsChanged);
		RemoveRoute(eDocumentClosed, mDocument->eDocumentClosed);
	}

	mDocument = inDocument;
	
	if (mDocument != nil)
	{
		AddRoute(eLineCountChanged, mDocument->eLineCountChanged);
		AddRoute(eSelectionChanged, mDocument->eSelectionChanged);
		AddRoute(eScroll, mDocument->eScroll);
		AddRoute(eShiftLines, mDocument->eShiftLines);
		AddRoute(eInvalidateDirtyLines, mDocument->eInvalidateDirtyLines);
		AddRoute(eBoundsChanged, mDocument->eBoundsChanged);
		AddRoute(eDocumentClosed, mDocument->eDocumentClosed);
		
		if (IsActive())
			mDocument->SetTargetTextView(this);
		
		LineCountChanged();
	}
}
