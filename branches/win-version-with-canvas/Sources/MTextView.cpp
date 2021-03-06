//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "MJapi.h"

#include <cmath>
#include <sstream>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>

#include "MTypes.h"
#include "MTextView.h"
#include "MTextDocument.h"
#include "MController.h"
#include "MGlobals.h"
#include "MPreferences.h"
#include "MDocWindow.h"
#include "MStyles.h"
#include "MUtils.h"
#include "MDevice.h"
#include "MJapiApp.h"

#ifndef NDEBUG
#include <iostream>
#endif

using namespace std;

namespace
{

//const uint32	kMTextViewKind = 'MTxt';

enum {
	DND_TARGET_TYPE_URI_LIST = 1,
	DND_TARGET_TYPE_UTF8_STRING,
	DND_TARGET_TYPE_STRING,
};

const int32
	kLeftMargin = 10;

const double
	kCaretBlinkTime = 0.6,
	kScrollDelay = 0.05,
	kIsFocusClickDelay = 0.1;

}

// ---------------------------------------------------------------------------
//	Default constructor

MTextView::MTextView(
	GtkWidget*		inTextViewWidget,
	GtkWidget*		inVScrollBar,
	GtkWidget*		inHScrollBar)
	: MView(inTextViewWidget, true, true)
	, eLineCountChanged(this, &MTextView::LineCountChanged)
	, eSelectionChanged(this, &MTextView::SelectionChanged)
	, eScroll(this, &MTextView::ScrollMessage)
	, eShiftLines(this, &MTextView::ShiftLines)
	, eInvalidateDirtyLines(this, &MTextView::InvalidateDirtyLines)
	, eInvalidateAll(this, &MTextView::InvalidateAll)
	, eStylesChanged(this, &MTextView::StylesChanged)
	, eDocumentClosed(this, &MTextView::DocumentClosed)
	, eDocumentChanged(this, &MTextView::SetDocument)
	, eIdle(this, &MTextView::Tick)

	, slOnCommit(this, &MTextView::OnCommit)
	, slOnDeleteSurrounding(this, &MTextView::OnDeleteSurrounding)
	, slOnPreeditChanged(this, &MTextView::OnPreeditChanged)
	, slOnPreeditStart(this, &MTextView::OnPreeditStart)
	, slOnPreeditEnd(this, &MTextView::OnPreeditEnd)
	, slOnRetrieveSurrounding(this, &MTextView::OnRetrieveSurrounding)
	
	, slOnVScrollBarValueChanged(this, &MTextView::OnVScrollBarValueChanged)
	, slOnHScrollBarValueChanged(this, &MTextView::OnHScrollBarValueChanged)
	
	, mController(nil)
	, mDocument(nil)
	, mVScrollBar(inVScrollBar)
	, mHScrollBar(inHScrollBar)
	, mLineHeight(10)
	, mLastCaretBlinkTime(0)
	, mCaretVisible(false)
	, mDrawForDragImage(false)
	, mCaret(0)
	, mLastClickTime(0)
	, mLastScrollTime(0)
	, mLastFocusTime(0)
	, mInTick(false)
	, mClickMode(eSelectNone)
{
	AddRoute(eIdle, gApp->eIdle);
	
	mIMContext = gtk_im_context_simple_new();
	
	slOnCommit.Connect(G_OBJECT(mIMContext), "commit");
	slOnDeleteSurrounding.Connect(G_OBJECT(mIMContext), "delete-surrounding");
	slOnPreeditChanged.Connect(G_OBJECT(mIMContext), "preedit-changed");
	slOnPreeditStart.Connect(G_OBJECT(mIMContext), "preedit-start");
	slOnPreeditEnd.Connect(G_OBJECT(mIMContext), "preedit-end");
	slOnRetrieveSurrounding.Connect(G_OBJECT(mIMContext), "retrieve-surrounding");
	
//	SetCallback(mVScrollBar->cbValueChanged, this, &MTextView::OnSBValueChanged);
	slOnVScrollBarValueChanged.Connect(mVScrollBar, "value-changed");
	slOnHScrollBarValueChanged.Connect(mHScrollBar, "value-changed");

	mImageOriginX = mImageOriginY = 0;
	
	StylesChanged();
	
	const GtkTargetEntry targets[] =
	{
	    { const_cast<gchar*>("text/uri-list"), 0, DND_TARGET_TYPE_URI_LIST },
	    { const_cast<gchar*>("UTF8_STRING"), 0, DND_TARGET_TYPE_UTF8_STRING },
	    { const_cast<gchar*>("STRING"), 0, DND_TARGET_TYPE_STRING },
//	    { const_cast<gchar*>("text/plain"), 0, DND_TARGET_TYPE_TEXT_PLAIN }
	};

	SetupDragAndDrop(
		targets, sizeof(targets) / sizeof(GtkTargetEntry));
}

// Destructor

MTextView::~MTextView()
{
}

bool MTextView::OnRealize()
{
	bool result = false;
	
	if (MView::OnRealize())
	{
		gtk_im_context_set_client_window(mIMContext, GetGtkWidget()->window);
		
		result = true;
	}
	
	return result;
}

void MTextView::SetController(MController* inController)
{
	assert(mController == nil);
	
	mController = inController;
	
	SetDocument(mController->GetDocument());
}

bool MTextView::OnButtonPressEvent(
	GdkEventButton*		inEvent)
{
	// short cut
	if (mDocument == nil or
		inEvent->button != 1 or
		inEvent->type != GDK_BUTTON_PRESS or
		GetLocalTime() < mLastFocusTime + kIsFocusClickDelay)
	{
		mLastFocusTime = 0;
		return MView::OnButtonPressEvent(inEvent);
	}

	if (mLastClickTime + 250 > inEvent->time)
		mClickCount = mClickCount % 3 + 1;
	else
		mClickCount = 1;
	mLastClickTime = inEvent->time;
		
	switch (mClickCount)
	{
		case 1: mClickMode = eSelectRegular; break;
		case 2: mClickMode = eSelectWords; break;
		case 3: mClickMode = eSelectLines; break;
	}
	
	mDocument->Reset();

	MSelection selection = mDocument->GetSelection();

	int32 x, y;
	x = static_cast<int32>(inEvent->x) + mImageOriginX - kLeftMargin;
	y = static_cast<int32>(inEvent->y) + mImageOriginY;
	
	mClickStartX = x;
	mClickStartY = y;

	uint32 keyModifiers = inEvent->state;
	
	if (x < 0)
		mClickMode = eSelectLines;

	mDocument->PositionToOffset(x, y, mClickCaret);

	if (keyModifiers & GDK_SHIFT_MASK)
		mClickAnchor = selection.GetAnchor();
	else
		mClickAnchor = mClickCaret;

	if (keyModifiers & GDK_CONTROL_MASK)
		mClickMode = eSelectBlock;
	
	mCaretVisible = true;
	
	if (mClickMode == eSelectRegular or mClickMode == eSelectBlock)
	{
		if (IsPointInSelection(x, y) and IsActive())
			mClickMode = eSelectStartDrag;
		else
			mDocument->Select(mClickAnchor, mClickCaret, mClickMode == eSelectBlock);
	}
	else if (mClickMode == eSelectWords)
	{
		mDocument->FindWord(mClickCaret, mMinClickAnchor, mMaxClickAnchor);
		mDocument->Select(mMinClickAnchor, mMaxClickAnchor);
	}
	else if (mClickMode == eSelectLines)
	{
		mClickAnchor = mClickCaret = mDocument->OffsetToLine(mClickCaret);
		
		mDocument->Select(
			mDocument->LineStart(mClickAnchor), mDocument->LineStart(mClickCaret + 1));
	}

	gtk_grab_add(GetGtkWidget());
	
	mLastScrollTime = 0;

	return true;
}

bool MTextView::OnMotionNotifyEvent(
	GdkEventMotion*	inEvent)
{
	int32 x, y;
	GdkModifierType state;
	
	if (inEvent->is_hint)
		gdk_window_get_pointer(inEvent->window, &x, &y, &state);
	else
	{
		x = static_cast<int32>(inEvent->x);
		y = static_cast<int32>(inEvent->y);
		state = GdkModifierType(inEvent->state);
	}

	x += mImageOriginX - kLeftMargin;
	y += mImageOriginY;
	
	if (mClickMode == eSelectStartDrag)
	{
		if (gtk_drag_check_threshold(GetGtkWidget(), mClickStartX, mClickStartY, x, y))
		{
			const GtkTargetEntry sources[] =
			{
			    { const_cast<gchar*>("UTF8_STRING"), 0, DND_TARGET_TYPE_UTF8_STRING },
			};
			
			DragBegin(sources, 1, inEvent);

			mClickMode = eSelectNone;
		}
	}
	else
	{
		if (IsPointInSelection(x, y))
			SetCursor(eNormalCursor);
		else if (x >= 0)
			SetCursor(eIBeamCursor);
		else
			SetCursor(eRightCursor);
	
		if (mClickMode != eSelectNone and IsActive())
		{
			if (ScrollToPointer(x, y))
				mLastScrollTime = GetLocalTime();
		}
	}
	
	return true;
}

bool MTextView::ScrollToPointer(
	int32		inX,
	int32		inY)
{
	bool scrolled = false;
	
	THROW_IF_NIL(mDocument);

	mDocument->PositionToOffset(inX, inY, mClickCaret);

	if (mClickMode == eSelectBlock)
	{
		MSelection selection = mDocument->GetSelection();
		uint32 al, ac, cl, cc;
		selection.GetAnchorLineAndColumn(al, ac);
		mDocument->PositionToLineColumn(inX, inY, cl, cc);
		mDocument->Select(MSelection(mDocument, al, ac, cl, cc));

		scrolled = ScrollToCaret();
	}
	else if (mClickMode == eSelectRegular)
	{
		mDocument->Select(mClickAnchor, mClickCaret);
		scrolled = ScrollToCaret();
	}
	else if (mClickMode == eSelectWords)
	{
		uint32 c1, c2;
		mDocument->FindWord(mClickCaret, c1, c2);
		if (c1 != c2)
		{
			if (c1 < mClickCaret and mClickCaret < mMinClickAnchor)
				mClickCaret = c1;
			else if (c2 > mClickCaret and mClickCaret > mMaxClickAnchor)
				mClickCaret = c2;
		}

		if (mClickCaret < mMinClickAnchor)
			mDocument->Select(mMaxClickAnchor, mClickCaret);
		else if (mClickCaret > mMaxClickAnchor)
			mDocument->Select(mMinClickAnchor, mClickCaret);
		else
			mDocument->Select(mMinClickAnchor, mMaxClickAnchor);

		scrolled = ScrollToCaret();
	}
	else if (mClickMode == eSelectLines)
	{
		mClickCaret = mDocument->OffsetToLine(mClickCaret);

		if (mClickCaret < mClickAnchor)
			mDocument->Select(
				mDocument->LineStart(mClickAnchor + 1), mDocument->LineStart(mClickCaret));
		else
			mDocument->Select(
				mDocument->LineStart(mClickAnchor), mDocument->LineStart(mClickCaret + 1));

		scrolled = ScrollToCaret();
	}
	
	return scrolled;
}
	
bool MTextView::OnButtonReleaseEvent(
	GdkEventButton*	inEvent)
{
	gtk_grab_remove(GetGtkWidget());	

	if (mClickMode == eSelectStartDrag and mDocument != nil)
		mDocument->Select(mClickAnchor, mClickCaret);
	
	mClickMode = eSelectNone;
	mLastScrollTime = 0;

	return true;
}

uint32 MTextView::CountPages(
	MDevice&		inDevice)
{
	THROW_IF_NIL(mDocument);
	
	MRect bounds = inDevice.GetBounds();

	uint32 wrapWidth = mDocument->GetWrapWidth();
	mDocument->SetWrapWidth(bounds.width);
	uint32 lines = mDocument->CountLines();
	mDocument->SetWrapWidth(wrapWidth);

	inDevice.SetFont(mDocument->GetFont());

	uint32 lineHeight = inDevice.GetAscent() + inDevice.GetDescent() + inDevice.GetLeading();
	
	uint32 linesPerPage = bounds.height / lineHeight;
	if (linesPerPage == 0 or lines == 0)
		THROW(("Invalid page height"));
	
	return lines / linesPerPage + 1;
}

void MTextView::Draw(
	MDevice&		inDevice,
	MRect			inUpdate)
{
	if (mDocument == nil)
		return;

	MValueChanger<int32> saveXOrigin(mImageOriginX, mImageOriginX);
	MValueChanger<int32> saveYOrigin(mImageOriginY, mImageOriginY);

	MValueChanger<int32> saveDescent(mDescent, mDescent);
	MValueChanger<int32> saveLineHeight(mLineHeight, mLineHeight);
	MValueChanger<int32> saveCharWidth(mCharWidth, mCharWidth);

	mNeedsDisplay = false;

	MRect bounds = inDevice.GetBounds();

	if (inDevice.IsPrinting())
	{
		inDevice.SetFont(mDocument->GetFont());
		
		mDescent = inDevice.GetDescent();
		mLineHeight = inDevice.GetAscent() + mDescent + inDevice.GetLeading();
		mCharWidth = inDevice.GetStringWidth("          ") / 10;
		
		uint32 linesPerPage = bounds.height / mLineHeight;

		mImageOriginX = 0;
		mImageOriginY = linesPerPage * inDevice.GetPageNr() * mLineHeight;
		
		inDevice.SetOrigin(bounds.x, bounds.y);
		inUpdate.x = 0;
		inUpdate.y = 0;
		
		mDocument->SetWrapWidth(bounds.width);
	}
	else
		inDevice.EraseRect(bounds);
	
	if (mDocument->GetShowWhiteSpace())
		inDevice.SetDrawWhiteSpace(true);
	
	int32 minLine = (mImageOriginY + inUpdate.y) / mLineHeight - 1;
	if (minLine < 0)
		minLine = 0;
	
	uint32 maxLine = minLine + inUpdate.height / mLineHeight + 2;
	if (maxLine >= mDocument->CountLines() and mDocument->CountLines() > 0)
		maxLine = mDocument->CountLines() - 1;

	for (uint32 line = minLine; line <= maxLine; ++line)
	{
		MRect lineRect(0, line * mLineHeight - mImageOriginY, bounds.width + mImageOriginX, mLineHeight);
		if (inUpdate.Intersects(lineRect))
			DrawLine(line, inDevice, lineRect);
	}

	if (IsWithinDrag())
		DrawDragHilite(inDevice);
}

void MTextView::DrawLine(
	uint32				inLineNr,
	MDevice&			inDevice,
	MRect				inLineRect)
{
	string text;
	mDocument->GetStyledText(inLineNr, inDevice, text);
	
	bool trailingNL = false;
	if (inLineNr < mDocument->CountLines() and mDocument->LineEnd(inLineNr) < mDocument->GetTextSize())
		trailingNL = mDocument->GetTextBuffer().GetChar(mDocument->LineEnd(inLineNr)) == '\n';
	
	inLineRect.x += kLeftMargin;
	
	int32 indent = mDocument->GetLineIndentWidth(inLineNr);

	MDeviceContextSaver save(inDevice);

	if (mImageOriginX != 0)
		inDevice.SetOrigin(-mImageOriginX, 0);

	int32 y = inLineRect.y;
	int32 x = inLineRect.x + indent;

	MSelection selection = mDocument->GetSelection();

	if (not (mDrawForDragImage or inDevice.IsPrinting()))
	{
		MDeviceContextSaver save(inDevice);
		
		bool marked = mDocument->IsLineMarked(inLineNr);
		bool current = mDocument->OffsetToLine(selection.GetCaret()) == inLineNr and IsActive();
		bool fill = true;
		
		if (mDocument->GetPCLine() == inLineNr)
			inDevice.SetForeColor(gPCLineColor);
		else if (marked and current)
			inDevice.CreateAndUsePattern(gMarkedLineColor, gCurrentLineColor);
		else if (marked)
			inDevice.SetForeColor(gMarkedLineColor);
		else if (current)
			inDevice.SetForeColor(gCurrentLineColor);
		else
			fill = false;
		
		if (fill)
		{
			inDevice.FillRect(inLineRect);

			MRect r2(inLineRect);
			r2.x -= r2.height / 2;
			r2.width = r2.height;
			
			inDevice.FillEllipse(r2);
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
//			inDevice.DrawTextHighLight(x, y, offset, length, false, false, IsActive());
////			
////			(void)::ATSUHighlightText(textLayout, myXLocation, myYLocation,
////				offset, length);
//		}
//	}

	// Highlight selection
	
	bool drawCaret = false;
	uint32 caretLine, caretColumn = 0;
	
	if (inDevice.IsPrinting() or
		selection.IsEmpty() or
		selection.GetMaxLine() < inLineNr or
		selection.GetMinLine() > inLineNr)
	{
		inDevice.DrawText(x, y);
		
		if (selection.IsEmpty() and not inDevice.IsPrinting())
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
		
		bool fillAfter = false;
		
		if (selection.IsBlock())
		{
			selectionStart = mDocument->LineAndColumnToOffset(inLineNr, selection.GetMinColumn());
			selectionEnd = mDocument->LineAndColumnToOffset(inLineNr, selection.GetMaxColumn());
		}
		else
		{
			selectionStart = selection.GetAnchor();
			selectionEnd = selection.GetCaret();
		}

		if (selectionStart > selectionEnd)
			swap(selectionEnd, selectionStart);
		uint32 startOffset = mDocument->LineStart(inLineNr);

		if (selectionStart < startOffset and not selection.IsBlock())
			selectionStart = 0;
		else
			selectionStart -= startOffset;

		selectionEnd -= startOffset;
		
		uint32 length = text.length();
		if (trailingNL)
			++length;
		
		if (selectionEnd >= length and not selection.IsBlock())
		{
			fillAfter = true;
			selectionEnd = text.length();
		}
		
		if (selection.IsBlock())
		{
			MColor selectionColor;
			
			if (IsActive() or mDrawForDragImage)
				selectionColor = gHiliteColor;
			else
				selectionColor = gInactiveHiliteColor;
		
			MRect r = inLineRect;
			
			uint32 c1, c2;
			c1 = selection.GetMinColumn();
			c2 = selection.GetMaxColumn();
			
			r.x += indent + c1 * mCharWidth;
			r.width = (c2 - c1) * mCharWidth;

			inDevice.Save();
			inDevice.SetForeColor(selectionColor);
			inDevice.FillRect(r);
			inDevice.Restore();
		}
		else if ((selectionEnd > selectionStart or fillAfter))
		{
			MColor selectionColor;
			
			if (IsActive() or mDrawForDragImage)
				selectionColor = gHiliteColor;
			else
				selectionColor = gInactiveHiliteColor;
		
			inDevice.SetTextSelection(selectionStart, selectionEnd - selectionStart, selectionColor);	
			
			if (fillAfter)
			{
				MRect r = inLineRect;
				
				if (text.length() > 0)
					r.x += indent + inDevice.GetTextWidth();

				inDevice.Save();
				inDevice.SetForeColor(selectionColor);
				inDevice.FillRect(r);
				inDevice.Restore();
			}
		}

		inDevice.DrawText(x, y);
	}
	
	if (IsWithinDrag())
	{
		caretLine = mDocument->OffsetToLine(mDragCaret);
		caretColumn = mDragCaret - mDocument->LineStart(caretLine);
		
		if (caretLine == inLineNr and mCaretVisible)
			drawCaret = true;
	}

	if (drawCaret)
	{
		if (caretColumn > text.length())	// duh
			caretColumn = text.length();

		inDevice.DrawCaret(x, y, caretColumn);
	}
}

void MTextView::DrawDragImage(
	GdkPixmap*&		outPixmap,
	int32&			outX,
	int32&			outY)
{
	THROW_IF_NIL(mDocument);

	MRect bounds;
	GetBounds(bounds);
	
	MValueChanger<bool> saveFlag(mDrawForDragImage, true);
	
	MSelection selection = mDocument->GetSelection();
	
	uint32 minLine = (mImageOriginY + bounds.y) / mLineHeight;
	if (minLine < selection.GetMinLine())
		minLine = selection.GetMinLine();

	MRect lr = GetLineRect(minLine);
	outX -= lr.x;
	outY -= lr.y;
	
	uint32 maxLine = minLine + bounds.height / mLineHeight + 2;
	if (maxLine > selection.GetMaxLine())
		maxLine = selection.GetMaxLine();

	bounds = MRect(0, 0, bounds.width, (maxLine - minLine + 1) * mLineHeight);
	MDevice dev(this, bounds, true);
	dev.EraseRect(bounds);
	
	MRegion rgn;
	mDocument->GetSelectionRegion(rgn);
	rgn.OffsetBy(-mImageOriginX + kLeftMargin, -(minLine * mLineHeight));
	dev.ClipRegion(rgn);

	lr = bounds;
	lr.height = mLineHeight;

	for (uint32  line= minLine; line <= maxLine; ++line)
	{
		DrawLine(line, dev, lr);
		lr.y += mLineHeight;
	}
	
	dev.MakeTransparent(0.35);
	outPixmap = dev.GetPixmap();
}

void MTextView::OnVScrollBarValueChanged()
{
	DoScrollTo(mImageOriginX, 
		static_cast<int32>(gtk_range_get_value(GTK_RANGE(mVScrollBar))));
}

void MTextView::OnHScrollBarValueChanged()
{
	DoScrollTo(
		static_cast<int32>(gtk_range_get_value(GTK_RANGE(mHScrollBar))),
		mImageOriginY);
}

void MTextView::AdjustScrollBars()
{
	uint32 width = 10, height = 10;
	
	if (mDocument != nil)
	{
		width = mDocument->GuessMaxWidth();
		height = mDocument->CountLines() * mLineHeight;
	}
	
	MRect bounds;
	GetBounds(bounds);
	
	uint32 linesPerPage = bounds.height / mLineHeight;

	GtkAdjustment* adj = gtk_range_get_adjustment(GTK_RANGE(mVScrollBar));
	
	if (adj != nil)
	{
		adj->lower = 0;
		adj->upper = height;
		adj->step_increment = mLineHeight;
		adj->page_increment = linesPerPage * mLineHeight;
		adj->page_size = bounds.height;
		adj->value = mImageOriginY;
		
		gtk_adjustment_changed(adj);
	}
	
	if (width == 0)
		gtk_widget_hide(mHScrollBar);
	else
	{
		gtk_widget_show(mHScrollBar);
	
		adj = gtk_range_get_adjustment(GTK_RANGE(mHScrollBar));
		
		if (adj != nil)
		{
			adj->lower = 0;
			adj->upper = width + (bounds.width / 2);
			adj->step_increment = mCharWidth;
			adj->page_increment = bounds.width / 2;
			adj->page_size = bounds.width;
			adj->value = mImageOriginX;
			
			gtk_adjustment_changed(adj);
		}
	}
}

bool MTextView::OnConfigureEvent(
	GdkEventConfigure*	inEvent)
{
	AdjustScrollBars();
	
	eBoundsChanged();
	
	return false;
}

void MTextView::DoScrollTo(
	int32			inX,
	int32			inY)
{
	if (mDocument != nil and mDocument->GetSoftwrap())
		inX = 0;
	
	int32 dx = inX - mImageOriginX;
	int32 dy = inY - mImageOriginY;
	
	Scroll(-dx, -dy);
	
	mImageOriginX = inX;
	mImageOriginY = inY;
	
	gtk_range_set_value(GTK_RANGE(mVScrollBar), inY);
	gtk_range_set_value(GTK_RANGE(mHScrollBar), inX);
}

void MTextView::ScrollToPosition(
	int32			inX,
	int32			inY)
{
	DoScrollTo(inX, inY);
}

void MTextView::GetScrollPosition(
	int32&				outX,
	int32&				outY)
{
	outX = mImageOriginX;
	outY = mImageOriginY;
}

bool MTextView::OnScrollEvent(
	GdkEventScroll*		inEvent)
{
	switch (inEvent->direction)
	{
		case GDK_SCROLL_UP:
			for (int i = 0; i < 3; ++i)
				ScrollMessage(kScrollLineDown);
			break; 

		case GDK_SCROLL_DOWN:
			for (int i = 0; i < 3; ++i)
				ScrollMessage(kScrollLineUp);
			break; 

		case GDK_SCROLL_LEFT:
			break;

		case GDK_SCROLL_RIGHT:
			break; 
	}	
		
	return true;
}

void MTextView::LineCountChanged()
{
	AdjustScrollBars();
}

void MTextView::Tick(
	double		inTime)
{
	if (mDocument == nil or mInTick)
		return;
	
	MValueChanger<bool> change(mInTick, true);

	if (mLastScrollTime > 0 and
		mLastScrollTime + kScrollDelay < inTime)
	{
		int32 x, y;
		GdkModifierType state;
		gdk_window_get_pointer(GetGtkWidget()->window, &x, &y, &state);
	
		x += mImageOriginX - kLeftMargin;
		y += mImageOriginY;
		
		if (ScrollToPointer(x, y))
			mLastScrollTime = inTime;
		else
			mLastScrollTime = 0;
		
		return;
	}
	
	if (inTime < mLastCaretBlinkTime + kCaretBlinkTime)
		return;
	
	mLastCaretBlinkTime = inTime;
	
	MSelection sel = mDocument->GetSelection();

	if (IsWithinDrag() and mDragIsAcceptable)
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
			(IsActive() and (sel.IsEmpty() or mCaretVisible)))
		{
			if (mDocument->GetFastFindMode())
				mCaretVisible = true;
			else
				mCaretVisible = not mCaretVisible;
	
			InvalidateLine(mDocument->OffsetToLine(mCaret));
		}
	}
}

void MTextView::SelectionChanged(
	MSelection		inNewSelection,
	string			inRangeName)
{
	InvalidateDirtyLines();

	mCaretVisible = true;
	mLastCaretBlinkTime = 0;
	Tick(GetLocalTime());
	mCaretVisible = true;
}

// ---------------------------------------------------------------------------
//	GetLineRect

MRect MTextView::GetLineRect(
	uint32		inLineNr) const
{
	MRect result;
	
	GetBounds(result);

	result.x = 0;
	result.y = inLineNr * mLineHeight - mImageOriginY;
	result.height = mLineHeight;
	result.width += mImageOriginX;
	
	return result;
}

// ---------------------------------------------------------------------------
//	InvalidateDirtyLines

void MTextView::InvalidateDirtyLines()
{
	assert(mDocument);
	
	MRect frame;
	GetBounds(frame);

	uint32 minLine = static_cast<uint32>((mImageOriginY + frame.y) / mLineHeight);
	if (minLine > 0)
		--minLine;
	
	uint32 maxLine = static_cast<uint32>(minLine + frame.height / mLineHeight + 1);
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
	Invalidate();
}

// ---------------------------------------------------------------------------
//	InvalidateRect

void MTextView::InvalidateRect(
	MRect		inRect)
{
	mNeedsDisplay = true;
	Invalidate(inRect);
}

// ---------------------------------------------------------------------------
//	InvalidateLine

void MTextView::InvalidateLine(
	uint32		inLine)
{
	InvalidateRect(GetLineRect(inLine));
}

// ---------------------------------------------------------------------------
//	Scroll

void MTextView::ScrollMessage(MScrollMessage inCommand)
{
	assert(mDocument);
	
	MRect frame;
	GetBounds(frame);

	int32 x = mImageOriginX, y = mImageOriginY;
	
	uint32 lineCount = mDocument->CountLines();
	uint32 linesPerPage = static_cast<uint32>(frame.height / mLineHeight);
	if (linesPerPage > 1)
		--linesPerPage;
	
	int32 maxLoc = (lineCount * mLineHeight) - frame.height;
	if (maxLoc < 0)
		maxLoc = 0;
	
	switch (inCommand)
	{
		case kScrollToStart:
			y = 0;
			DoScrollTo(x, y);
			break;
		
		case kScrollToEnd:
			y = maxLoc;
			DoScrollTo(x, y);
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
			y += mLineHeight;
			if (y > maxLoc)
				y = maxLoc;
			DoScrollTo(x, y);
			break;

		case kScrollLineDown:
			y -= mLineHeight;
			if (y < 0)
				y = 0;
			DoScrollTo(x, y);
			break;

		case kScrollPageUp:
			y -= (linesPerPage * mLineHeight);
			if (y < 0)
				y = 0;
			DoScrollTo(x, y);
			break;

		case kScrollPageDown:
			y += (linesPerPage * mLineHeight);
			if (y > maxLoc)
				y = maxLoc;
			DoScrollTo(x, y);
			break;
		
		case kScrollForKiss:
			mSavedOriginX = mImageOriginX;
			mSavedOriginY = mImageOriginY;
			ScrollToSelection(false);
			UpdateNow();
			break;
		
		case kScrollReturnAfterKiss:
			DoScrollTo(mSavedOriginX, mSavedOriginY);
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
}

// ---------------------------------------------------------------------------
//	ScrollToCaret

bool MTextView::ScrollToCaret(
	bool			inScrollForDrag)
{
	assert(mDocument);
	
	MRect bounds;
	GetBounds(bounds);
	
	int32 top, bottom;

	top = mImageOriginY;
	bottom = top + bounds.height;
	
	if (inScrollForDrag)
	{
		// scroll when mouse is in the top or bottom visible line
		top += mLineHeight;
		bottom -= mLineHeight;
	}
	
	MSelection selection = mDocument->GetSelection();
	
	uint32 caret;
	
	if (IsWithinDrag())
		caret = mDragCaret;
	else
		caret = selection.GetCaret();
	
	uint32 line;
	int32 x, y;
	
	mDocument->OffsetToPosition(caret, line, x);
	y = line * mLineHeight;
	
	if (not IsWithinDrag())
	{
		if (selection.GetAnchor() < caret and
			line < mDocument->CountLines() and
			mDocument->LineStart(line) == caret)
		{
			--line;
			y -= mLineHeight;
		}
	}
	
	int32 newX, newY;
	newX = mImageOriginX;
	newY = mImageOriginY;
	
	if (y < top)
	{
		newY = line * mLineHeight;
		if (IsWithinDrag())
			newY -= mLineHeight;
	}
	else if (y + mLineHeight > bottom)
	{
		newY = (line + 1) * mLineHeight - (bottom - top) + 2;
		if (IsWithinDrag())
			newY += mLineHeight;
	}
	
	if (x < 3 or (mDocument != nil and mDocument->GetSoftwrap()))
		newX = 0;
	else if (x < mImageOriginX)
		newX = mImageOriginX;
	else if (x > mImageOriginX + bounds.width - 3 - kLeftMargin)
		newX = x - bounds.width + 3 + kLeftMargin;
	
	bool result = false;
	
	if (newX != mImageOriginX or newY != mImageOriginY)
	{
		int32 dx = mImageOriginX - newX;
		int32 dy = mImageOriginY - newY;
		
		if (IsWithinDrag())
		{
			if (dx)
			{
				MRect r(bounds);

				if (dx < 0)
					r.x += r.width - 4;
				r.width = 4;

				InvalidateRect(r);
			}

			if (dy)
			{
				MRect r(bounds);

				if (dy < 0)
					r.y += r.height - 4;
				r.height = 4;

				InvalidateRect(r);
			}
		}

		DoScrollTo(newX, newY);

		if (IsWithinDrag())
		{
			MDevice dev(this, bounds);
			DrawDragHilite(dev);
		}
		
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
	
	MRect bounds;
	GetBounds(bounds);
	
	int32 y;
		
	if (not inForceCenter)
	{
		y = inLineNr * mLineHeight;
		
		if (y < bounds.y)
			inForceCenter = true;
	}
			
	if (not inForceCenter)
	{
		y = (inLineNr + 1) * mLineHeight;
	 
		if (y > bounds.y + bounds.height)
			inForceCenter = true;
	}

	if (inForceCenter)
	{
		uint32 midLine = static_cast<uint32>(bounds.height / (3 * mLineHeight));

		if (inLineNr < midLine)
			inLineNr = 0;
		else
			inLineNr -= midLine;

		y = inLineNr * mLineHeight;
		
		DoScrollTo(0, y);
	}
}

// ---------------------------------------------------------------------------
//	ScrollToSelection

void MTextView::ScrollToSelection(
	bool	inForceCenter)
{
	assert(mDocument);
	
	uint32 line;

	MRect bounds;
	GetBounds(bounds);
	
	MSelection selection(mDocument->GetSelection());
	int32 y;
		
	if (not inForceCenter)
	{
		line = selection.GetMinLine();
		y = line * mLineHeight;
		
		if (y < mImageOriginY)
			inForceCenter = true;
	}
			
	if (not inForceCenter)
	{
		line = selection.GetMaxLine();
		y = (line + 1) * mLineHeight;
	 
		if (y > mImageOriginY + bounds.height)
			inForceCenter = true;
	}

	if (inForceCenter)
	{
		uint32 midLine =
			static_cast<uint32>(bounds.height / (3 * mLineHeight));

		line = selection.GetMinLine();
		if (line < midLine)
			line = 0;
		else
			line -= midLine;

		int32 x, y;
		x = 0;
		y = line * mLineHeight;
		
		DoScrollTo(x, y);
	}
   
	ScrollToCaret();

//	GetBounds(bounds);
//
//	if (not fDoc->GetSoftwrap() and not selection.IsEmpty())
//	{
//		int32 ax, cx;
//		uint32 line;
//		mDocument->OffsetToPosition(selection.GetMinOffset(), line, ax);
//		mDocument->OffsetToPosition(selection.GetMaxOffset(), line, cx);
//
//		int32 center = (ax + cx) / 2;
//
//		if (center < mImageOriginX + 10 or
//			center > mImageOriginX + bounds.width - 10)
//		{
//			DoScrollTo(mImageOriginX + (center - bounds.width) / 2, mImageOriginY);
//		}
//	}
}

// ---------------------------------------------------------------------------
//	ScrollForDiff

void MTextView::ScrollForDiff()
{
	assert(mDocument);
	
	MRect bounds;
	GetBounds(bounds);
	
	int32 center = bounds.height / (4 * mLineHeight);

	uint32 offset = mDocument->GetSelection().GetMinOffset();
	
	uint32 line;
	int32 x;
	
	mDocument->OffsetToPosition(offset, line, x);

	int32 y = (line - center) * mLineHeight;
	if (y < 0)
		y = 0;
	
	if (y != mImageOriginY)
		DoScrollTo(mImageOriginX, y);
}

// -----------------------------------------------------------------------------
// ShiftLines

void MTextView::ShiftLines(uint32 inFromLine, int32 inDelta)
{
	// get our bounds
	MRect bounds;
	GetBounds(bounds);
	
	// calculate the rectangle that needs to be scrolled
	MRect r = bounds;

	// the top of the scrolling rect
	r.y = inFromLine * mLineHeight - mImageOriginY;
	if (r.y < bounds.y)
		r.y = bounds.y;
	
	// and the height
	r.height = bounds.height - r.y;

	int32 dy = inDelta * static_cast<int32>(mLineHeight);

	if (dy < 0)
	{
		r.y -= dy;
		r.height += dy;
	}
	
	// if the rect is not empty and lies within our bounds
	if (r.height > 0 and
		r.y <= bounds.height and
		r.y + r.height >= 0 and
		fabs(dy) < bounds.height and
		not mNeedsDisplay)
	{
		Scroll(r, 0, dy);
		
		if (dy < 0)
		{
			r.y += r.height + dy;
			r.height = bounds.height - r.y;
			InvalidateRect(r);
		}
	}
	else
		Invalidate();
}

void MTextView::GetVisibleLineSpan(
	uint32&				outFirstLine,
	uint32&				outLastLine) const
{
	MRect bounds;
	GetBounds(bounds);
	
	outFirstLine = static_cast<uint32>(mImageOriginY / mLineHeight);
	outLastLine = static_cast<uint32>((mImageOriginY + bounds.height) / mLineHeight);
	if (outLastLine > 0)
		--outLastLine;
}

bool MTextView::OnFocusInEvent(
	GdkEventFocus*	inEvent)
{
	Invalidate();

	if (mDocument != nil)
	{
		mDocument->SetTargetTextView(this);
		mDocument->MakeFirstDocument();
		
		gtk_im_context_focus_in(mIMContext);
	
		mLastFocusTime = GetLocalTime();
	}
	
	if (mController != nil)
		mController->TakeFocus();
	
	return true;
}

bool MTextView::OnFocusOutEvent(
	GdkEventFocus*	inEvent)
{
	mNeedsDisplay = true;
	Invalidate();

	if (mDocument != nil)
	{
		gtk_im_context_focus_out(mIMContext);
		
		mDocument->Reset();
	}

	if (mController != nil)
		mController->ReleaseFocus();

	return true;
}

void MTextView::DragEnter()
{
	MRect bounds;
	GetBounds(bounds);
	
	MDevice dev(this, bounds);
	DrawDragHilite(dev);
}

bool MTextView::DragWithin(
	int32			inX,
	int32			inY)
{
	mDragIsAcceptable = true;
	
	uint32 caret;
	mDocument->PositionToOffset(inX + mImageOriginX, inY + mImageOriginY, caret);

	if (IsPointInSelection(inX + mImageOriginX, inY + mImageOriginY) or
		mDocument->IsReadOnly())
	{
		mDragIsAcceptable = false;
		if (mCaretVisible)
			InvalidateLine(mDocument->OffsetToLine(mDragCaret));
		
		mDragCaret = caret;
	}
	else if (mDragCaret != caret)
	{
		InvalidateLine(mDocument->OffsetToLine(mDragCaret));
		mDragCaret = caret;
		InvalidateLine(mDocument->OffsetToLine(mDragCaret));
	}
	
	while (ScrollToCaret(true))
		;
	
	return mDragIsAcceptable;
}
	
void MTextView::DragLeave()
{
	Invalidate();
}

bool MTextView::DragAccept(
	bool			inMove,
	int32			inX,
	int32			inY,
	const char*		inData,
	uint32			inLength,
	uint32			inType)
{
	bool result = not IsPointInSelection(inX, inY);
	
	switch (inType)
	{
		case DND_TARGET_TYPE_UTF8_STRING:
			if (result)
				mDocument->Drop(mDragCaret, inData, inLength, inMove);
			break;
	
		case DND_TARGET_TYPE_STRING:
			if (result)
			{
				unique_ptr<MDecoder> decoder(MDecoder::GetDecoder(kEncodingISO88591, inData, inLength));
				string text;
				decoder->GetText(text);
				mDocument->Drop(mDragCaret, text.c_str(), text.length(), inMove);
			}
			break;
		
		case DND_TARGET_TYPE_URI_LIST:
		{
			vector<string> files;
			string text(inData, inLength);
			boost::split(files, text, boost::is_any_of("\n\r"));
			
			for (vector<string>::iterator file = files.begin(); file != files.end(); ++file)
			{
				if (file->length() == 0)
					continue;
				
				MFile url(*file, true);
				gApp->OpenOneDocument(url);
			}
			
			result = true;
			break;
		}
	}
	
	return result;
}

void MTextView::DragSendData(
	string&		outData)
{
	mDocument->GetSelectedText(outData);
}

void MTextView::DragDeleteData()
{
//	mDocument->DeleteSelectedText();
}

//OSStatus MTextView::DoGetClickActivation(EventRef ioEvent)
//{
//	HIPoint where;
//	(void)::GetEventParameter(ioEvent, kEventParamMouseLocation, typeHIPoint, nil,
//		sizeof(where), nil, &where);
//
//	ClickActivationResult result = kActivateAndIgnoreClick;
//	
//	if (InitiateDrag(where, ioEvent))
//		result = kDoNotActivateAndIgnoreClick;
//	
//	(void)::SetEventParameter(ioEvent, kEventParamClickActivation, typeClickActivationResult,
//		sizeof(result), &result);
//
//	return noErr;
//}

// inWhere should be in view local coordinates
bool MTextView::IsPointInSelection(
	int32			inX,
	int32			inY)
{
	bool result = false;
	
	if (mDocument != nil)
	{
		mDocument->SetTargetTextView(this);
		
		MRegion rgn;
		mDocument->GetSelectionRegion(rgn);
		result = rgn.ContainsPoint(inX, inY);
	}
	
	return result;
}

void MTextView::DrawDragHilite(
	MDevice&		inDevice)
{
	inDevice.Save();
	
	MRect bounds;
	GetBounds(bounds);

	bounds.InsetBy(1, 1);

	inDevice.SetForeColor(gHiliteColor);
	inDevice.StrokeRect(bounds);
	
	inDevice.Restore();
}

uint32 MTextView::GetWrapWidth() const
{
	MRect r;
	GetBounds(r);
	return static_cast<uint32>(r.width) - kLeftMargin;
}

void MTextView::StylesChanged()
{
	if (mDocument != nil)
	{
		MDevice dev;
		string txt;
		mDocument->GetStyledText(0, dev, txt);
		mDescent = dev.GetDescent();
		mLineHeight = dev.GetAscent() + mDescent + dev.GetLeading();
		mCharWidth = dev.GetStringWidth("          ") / 10;
	}
	else
	{
		mLineHeight = 10;	// reasonable guess
		mCharWidth = 6;
	}
}

void MTextView::DocumentClosed(
	MDocument*		inDocument)
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
//		RemoveRoute(eBoundsChanged, mDocument->eBoundsChanged);
		RemoveRoute(eDocumentClosed, mDocument->eDocumentClosed);
	}

	mDocument = dynamic_cast<MTextDocument*>(inDocument);;
	
	if (mDocument != nil)
	{
		AddRoute(eLineCountChanged, mDocument->eLineCountChanged);
		AddRoute(eSelectionChanged, mDocument->eSelectionChanged);
		AddRoute(eScroll, mDocument->eScroll);
		AddRoute(eShiftLines, mDocument->eShiftLines);
		AddRoute(eInvalidateDirtyLines, mDocument->eInvalidateDirtyLines);
//		AddRoute(eBoundsChanged, mDocument->eBoundsChanged);
		AddRoute(eDocumentClosed, mDocument->eDocumentClosed);
		
		if (IsActive())
			mDocument->SetTargetTextView(this);
		
		StylesChanged();
		LineCountChanged();
	}
}

bool MTextView::OnKeyPressEvent(
	GdkEventKey*	inEvent)
{
	if (gtk_im_context_filter_keypress(mIMContext, inEvent) == false)
		mDocument->OnKeyPressEvent(inEvent);
	
	return true;
}

bool MTextView::OnCommit(
	gchar*			inText)
{
	if (mDocument != nil)
		mDocument->OnCommit(inText, strlen(inText));

	return true;
}

bool MTextView::OnDeleteSurrounding(
	gint			inStart,
	gint			inLength)
{
	cout << "OnDeleteSurrounding" << endl;
	return true;
}

bool MTextView::OnPreeditChanged()
{
	cout << "OnPreeditChanged" << endl;
	return true;
}

bool MTextView::OnPreeditEnd()
{
	cout << "OnPreeditEnd" << endl;
	return true;
}

bool MTextView::OnPreeditStart()
{
	cout << "OnPreeditStart" << endl;
	return true;
}

bool MTextView::OnRetrieveSurrounding()
{
	cout << "OnRetrieveSurrounding" << endl;
	return true;
}

void MTextView::OnPopupMenu(
	GdkEventButton*	inEvent)
{
	int32 x = 0, y = 0;
	
	if (inEvent != nil)
	{
		x = static_cast<int32>(inEvent->x);
		y = static_cast<int32>(inEvent->y);
	}

	ConvertToGlobal(x, y);

	MMenu* popup = MMenu::CreateFromResource("text-view-context-menu");
	
	if (popup != nil)
		popup->Popup(mController, inEvent, x, y, true);
}

