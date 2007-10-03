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

#ifndef MTEXTVIEW_H
#define MTEXTVIEW_H
#pragma once

#include "MColor.h"
#include "MView.h"
#include "MP2PEvents.h"
#include "MSelection.h"

class MDocument;
class MDocWindow;
class MController;
class MDevice;

class MTextView : public MView
{
  public:
						MTextView();
						
	virtual				~MTextView();
	
	bool				ScrollToCaret();

	void				ScrollToSelection(
							bool				inForceCenter);

	void				ScrollForDiff();

	void				ScrollToPosition(
							int32				inX,
							int32				inY);

	void				GetScrollPosition(
							int32&				outX,
							int32&				outY);
	
	void				GetVisibleLineSpan(
							uint32&				outFirstLine,
							uint32&				outLastLine) const;

	uint32				GetWrapWidth() const;

	MController*		GetController() const		{ return mController; }

	void				SetController(
							MController*		inController);

	MEventOut<void()>					eBoundsChanged;

	MEventIn<void()>					eLineCountChanged;
	MEventIn<void(MSelection,std::string)>	eSelectionChanged;
	MEventIn<void(MScrollMessage)>		eScroll;
	MEventIn<void(uint32,int32)>		eShiftLines;
	MEventIn<void()>					eInvalidateDirtyLines;
	MEventIn<void()>					eInvalidateAll;
	MEventIn<void()>					eStylesChanged;

	MEventIn<void()>					eDocumentClosed;
	MEventIn<void(MDocument*)>			eDocumentChanged;

	void				SetDocument(
							MDocument*			inDocument);

	void				DocumentClosed();
	
  protected:

//	OSStatus			DoControlDraw(EventRef inEvent);
//	OSStatus			DoControlClick(EventRef inEvent);
//	OSStatus			DoControlHitTest(EventRef inEvent);
//	OSStatus			DoControlTrack(EventRef inEvent);
//	OSStatus			DoScrollableGetInfo(EventRef inEvent);
//	OSStatus			DoScrollableScrollTo(EventRef inEvent);
//	OSStatus			DoControlActivate(EventRef inEvent);
//	OSStatus			DoControlDeactivate(EventRef inEvent);
//	OSStatus			DoControlTrackingAreaEntered(EventRef inEvent);
//	OSStatus			DoControlTrackingAreaExited(EventRef inEvent);
//	OSStatus			DoBoundsChanged(EventRef inEvent);
//	OSStatus			DoDragEnter(EventRef inEvent);
//	OSStatus			DoDragWithin(EventRef inEvent);
//	OSStatus			DoDragLeave(EventRef inEvent);
//	OSStatus			DoDragReceive(EventRef inEvent);
//	OSStatus			DoGetClickActivation(EventRef inEvent);
//	OSStatus			DoContextualMenuClick(EventRef inEvent);
//	OSStatus			DoSetFocusPart(EventRef inEvent);

  private:

	void				DrawLine(
							uint32			inLineNr,
							MDevice&		inDevice);
	
	void				GetLine(
							uint32			inLineNr,
							std::string&	outText);
	
	void				LineCountChanged();

	void				SelectionChanged(
							MSelection		inNewSelection,
							std::string		inRangeName);

	void				Scroll(
							MScrollMessage	inMessage);

	void				ShiftLines(
							uint32			inFromLine,
							int32			inDelta);

	void				InvalidateDirtyLines();

	void				InvalidateAll();
	
	MRect				GetLineRect(
							uint32			inLineNr) const;

	void				InvalidateLine(
							uint32			inLineNr);

	void				InvalidateRect(
							MRect			inRect);

	void				StylesChanged();
	
	void				Tick();
	
	void				ScrollToLine(
							uint32			inLineNr,
							bool			inForceCenter);

//	RgnHandle			SelectionToRegion();
	bool				IsPointInSelection(
							int32			inX,
							int32			inY);

	void				DoScrollTo(
							int32			inX,
							int32			inY);
	
//	void				DoDragSendData(FlavorType theType,
//							DragItemRef theItemRef, DragRef theDrag);
//	
//	bool				InitiateDrag(HIPoint inWhere, EventRef inEvent);
//	void				DrawDragHilite(CGContextRef inContext);
//	
//	static pascal OSErr	DoDragSendDataCallback(FlavorType theType, void *dragSendRefCon,
//							DragItemRef theItemRef, DragRef theDrag);
//	static DragSendDataUPP
//						sDragSendDataUPP;
//	static MTextView*	sDraggingView;

	MController*		mController;
	MDocument*			mDocument;
	int32				mLineHeight;
	int32				mDescent;
	int32				mImageOriginX, mImageOriginY;
	int32				mSavedOriginX, mSavedOriginY;	// for kissing
	bool				mCaretVisible;
	bool				mNeedsDisplay;	// cache this
	bool				mDrawForDragImage;
	bool				mTrackCursorInTick;
	uint32				mCaret;		// our cache of the current caret position
	uint32				mClickCount;
	double				mLastClickTime;
//	TSMDocumentID		mTSMDocument;
//	HIViewTrackingAreaRef
//						mTrackingLeftRef, mTrackingMainRef;
//	EventLoopTimerRef	mTimerRef;
//	DragRef				mDragRef;
//	uint32				mDragCaret;
	static MColor		sCurrentLineColor, sMarkedLineColor, sPCLineColor, sBreakpointColor;

//	// tick support for caret blinking
//	
//	static pascal void	EventLoopTimerProc(EventLoopTimerRef inTimer, void *inUserData);
//
};

#endif
