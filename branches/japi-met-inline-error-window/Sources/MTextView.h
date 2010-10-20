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
							GtkWidget*			inTextViewWidget,
							GtkWidget*			inVScrollBar,
							GtkWidget*			inHScrollBar);
						
	virtual				~MTextView();
	
	bool				ScrollToCaret(
							bool				inScrollForDrag = false);

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
	
  protected:

	virtual void		Draw(
							MDevice&		inDevice,
							MRect			inUpdate);

	virtual uint32		CountPages(
							MDevice&		inDevice);

	virtual bool		OnFocusInEvent(
							GdkEventFocus*	inEvent);

	virtual bool		OnFocusOutEvent(
							GdkEventFocus*	inEvent);

	virtual bool		OnButtonPressEvent(
							GdkEventButton*	inEvent);

	virtual bool		OnMotionNotifyEvent(
							GdkEventMotion*	inEvent);
	
	virtual bool		OnButtonReleaseEvent(
							GdkEventButton*	inEvent);

	virtual bool		OnScrollEvent(
							GdkEventScroll*	inEvent);
	
  public:

	virtual void		OnPopupMenu(
							GdkEventButton*	inEvent);
	
  private:

	virtual bool		OnRealize();

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

	void				DoScrollTo(
							int32			inX,
							int32			inY);
	
	void				DrawDragHilite(
							MDevice&		inDevice);
	
	void				AdjustScrollBars();

	virtual bool		OnKeyPressEvent(
							GdkEventKey*	inEvent);
	
	bool				OnCommit(
							gchar*			inText);
	
	bool				OnDeleteSurrounding(
							gint			inStart,
							gint			inLength);

	bool				OnConfigureEvent(
							GdkEventConfigure*
											inEvent);

	bool				OnPreeditChanged();
	
	bool				OnPreeditEnd();

	bool				OnPreeditStart();
	
	bool				OnRetrieveSurrounding();

	void				OnVScrollBarValueChanged();

	void				OnHScrollBarValueChanged();
	
	MSlot<bool(gchar*)>						slOnCommit;
	MSlot<bool(gint,gint)>					slOnDeleteSurrounding;
	MSlot<bool()>							slOnPreeditChanged;
	MSlot<bool()>							slOnPreeditStart;
	MSlot<bool()>							slOnPreeditEnd;
	MSlot<bool()>							slOnRetrieveSurrounding;
	MSlot<void()>							slOnVScrollBarValueChanged;
	MSlot<void()>							slOnHScrollBarValueChanged;

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

	virtual void		DrawDragImage(
							GdkPixmap*&		outPixmap,
							int32&			outX,
							int32&			outY);
	
	MController*		mController;
	MTextDocument*		mDocument;
	GtkWidget*			mVScrollBar;
	GtkWidget*			mHScrollBar;
	int32				mLineHeight;
	int32				mCharWidth;		// for block selection drawing
	int32				mDescent;
	int32				mImageOriginX, mImageOriginY;
	int32				mSavedOriginX, mSavedOriginY;	// for kissing
	double				mLastCaretBlinkTime;
	bool				mCaretVisible;
	bool				mNeedsDisplay;	// cache this
	bool				mDrawForDragImage;
	bool				mTrackCursorInTick;
	uint32				mCaret;		// our cache of the current caret position
	uint32				mClickCount;
	uint32				mLastClickTime;
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
	
	GtkIMContext*		mIMContext;
	
	uint32				mDragCaret;
	bool				mDragIsAcceptable;
	int32				mClickStartX, mClickStartY;
};

#endif
