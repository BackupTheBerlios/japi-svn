//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef MTEXTVIEW_H
#define MTEXTVIEW_H
#pragma once

#include "MColor.h"
#include "MHandler.h"
#include "MP2PEvents.h"
#include "MView.h"
#include "MSelection.h"

class MDocument;
class MTextDocument;
class MDocWindow;
class MController;
class MDevice;

class MTextView : public MView
{
  public:
						MTextView(
							const std::string&	inID,
							MRect				inBounds);
						
	virtual				~MTextView();
	
	bool				ScrollToCaret(
							bool				inScrollForDrag = false);

	void				ScrollToSelection(
							bool				inForceCenter);

	void				ScrollForDiff();

	void				GetVisibleLineSpan(
							uint32&				outFirstLine,
							uint32&				outLastLine) const;

	uint32				GetWrapWidth() const;

	MController*		GetController() const		{ return mController; }

	void				SetController(
							MController*		inController);

	MEventIn<void()>					eLineCountChanged;
	MEventIn<void(MSelection,std::string)>	eSelectionChanged;
	MEventIn<void(MScrollMessage)>		eScroll;
	MEventIn<void(uint32,int32)>		eShiftLines;
	MEventIn<void()>					eInvalidateDirtyLines;
	MEventIn<void()>					eInvalidateAll;
	MEventIn<void()>					eStylesChanged;

	MEventIn<void(MDocument*)>			eDocumentClosed;
	MEventIn<void(MDocument*)>			eDocumentChanged;
	
	MEventIn<void(double)>				eIdle;
	
	MEventOut<void()>					eBoundsChanged;

	void				SetDocument(
							MDocument*		inDocument);

	void				DocumentClosed(
							MDocument*		inDocument);
	
	virtual void		Draw(
							MRect			inUpdate);

	virtual uint32		CountPages(
							MDevice&		inDevice);

	virtual void		AdjustCursor(
							int32			inX,
							int32			inY,
							uint32			inModifiers);

	virtual void		ActivateSelf();
	virtual void		DeactivateSelf();

	virtual void		ResizeFrame(
							int32			inXDelta,
							int32			inYDelta,
							int32			inWidthDelta,
							int32			inHeightDelta);

  public:

	virtual void		MouseDown(
							int32			inX,
							int32			inY,
							uint32			inClickCount,
							uint32			inModifiers);

	virtual void		MouseMove(
							int32			inX,
							int32			inY,
							uint32			inModifiers);

	virtual void		MouseExit();

	virtual void		MouseUp(
							int32			inX,
							int32			inY,
							uint32			inModifiers);

	//virtual void		OnPopupMenu(
	//						GdkEventButton*	inEvent);
	
  private:

	void				DrawLine(
							uint32			inLineNr,
							MDevice&		inDevice,
							MRect			inLineRect);
	
	void				GetLine(
							uint32			inLineNr,
							std::string&	outText);
	
	void				LineCountChanged();

	void				SelectionChanged(
							MSelection		inNewSelection,
							std::string		inRangeName);

	void				ScrollMessage(
							MScrollMessage	inMessage);

	bool				ScrollToPointer(
							int32			inX,
							int32			inY);

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
	
	void				Tick(
							double			inTime);
	
	void				ScrollToLine(
							uint32			inLineNr,
							bool			inForceCenter);

	bool				IsPointInSelection(
							int32			inX,
							int32			inY);

	void				DrawDragHilite(
							MDevice&		inDevice);
	
	void				AdjustScrollBars();

	virtual void		DragEnter();
	
	virtual bool		DragWithin(
							int32			inX,
							int32			inY);
	
	virtual void		DragLeave();

	virtual bool		DragAccept(
							bool			inMove,
							int32			inX,
							int32			inY,
							const char*		inData,
							uint32			inLength,
							uint32			inType);
	
	virtual void		DragSendData(
							std::string&	outData);

	virtual void		DragDeleteData();

	//virtual void		DrawDragImage(
	//						GdkPixmap*&		outPixmap,
	//						int32&			outX,
	//						int32&			outY);
	
	MController*		mController;
	MTextDocument*		mDocument;
	int32				mLineHeight;
	int32				mCharWidth;		// for block selection drawing
	int32				mDescent;
	MRect				mSavedBounds;	// for kissing
	double				mLastCaretBlinkTime;
	bool				mCaretVisible;
	bool				mNeedsDisplay;	// cache this
	bool				mDrawForDragImage;
	bool				mTrackCursorInTick;
	uint32				mCaret;		// our cache of the current caret position
	double				mLastScrollTime;
	double				mLastFocusTime;
	bool				mInTick;
	
	enum
	{
		eSelectNone,
		eSelectStartDrag,
		eSelectRegular,
		eSelectWords,
		eSelectLines,
		eSelectBlock
	}					mClickMode;
	uint32				mClickAnchor, mClickCaret;
	uint32				mMinClickAnchor, mMaxClickAnchor;
	
	uint32				mDragCaret;
	bool				mDragIsAcceptable;
	int32				mClickStartX, mClickStartY;
};

#endif
