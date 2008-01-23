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

#include "MJapi.h"

#include <cmath>

#include "MDevice.h"
#include "MView.h"
#include "MWindow.h"
#include "MGlobals.h"

using namespace std;

// --------------------------------------------------------------------
// base class for MDeviceImp
// provides only the basic Pango functionality
// This is needed in measuring text metrics and such

class MDeviceImp
{
  public:
							MDeviceImp();
	virtual					~MDeviceImp();

	virtual void			Save();
	
	virtual void			Restore();

	virtual void			SetFont(
								const string&		inFont);

	virtual void			SetForeColor(
								MColor				inColor);

	virtual MColor			GetForeColor() const;

	virtual void			SetBackColor(
								MColor				inColor);

	virtual MColor			GetBackColor() const;
	
	virtual void			EraseRect(
								MRect				inRect);

	virtual void			FillRect(
								MRect				inRect);

	virtual void			StrokeRect(
								MRect				inRect,
								uint32				inLineWidth = 1);

	virtual void			FillEllipse(
								MRect				inRect);
	
	virtual void			CreateAndUsePattern(
								MColor				inColor1,
								MColor				inColor2);
	
	virtual void			DrawListItemBackground(
								MRect				inRect,
								bool				inSelected,
								bool				inActive,
								bool				inOdd,
								bool				inRoundEdges);

	PangoFontMetrics*		GetMetrics();

	virtual uint32			GetAscent();
	
	virtual uint32			GetDescent();
	
	virtual uint32			GetLeading();
	
	virtual uint32			GetLineHeight();

	virtual void			DrawString(
								const string&		inText,
								float				inX,
								float				inY,
								uint32				inTruncateWidth = 0,
								MAlignment			inAlign = eAlignNone);

	virtual uint32			GetStringWidth(
								const string&		inText);

	// Text Layout options
	
	virtual void			SetText(
								const string&		inText);
	
	virtual void			SetTabStops(
								uint32				inTabWidth);
	
	virtual void			SetTextColors(
								uint32				inColorCount,
								uint32				inColors[],
								uint32				inOffsets[]);

	virtual void			SetTextSelection(
								uint32				inStart,
								uint32				inLength,
								MColor				inSelectionColor);
	
	virtual void			IndexToPosition(
								uint32				inIndex,
								bool				inTrailing,
								int32&				outPosition);

	virtual bool			PositionToIndex(
								int32				inPosition,
								uint32&				outIndex,
								bool&				outTrailing);
	
	virtual uint32			GetTextWidth();
	
	virtual void			DrawText(
								float				inX,
								float				inY);

	virtual void			DrawCaret(
								float				inX,
								float				inY,
								uint32				inOffset);
	
	virtual bool			BreakLine(
								uint32				inWidth,
								uint32&				outBreak);

  protected:

	static PangoContext*	sPangoContext;
	static PangoLanguage*	sPangoLanguage;

	PangoLayout*			mPangoLayout;
	PangoFontDescription*	mFont;
	PangoFontMetrics*		mMetrics;
};

PangoContext* MDeviceImp::sPangoContext;
PangoLanguage* MDeviceImp::sPangoLanguage;

MDeviceImp::MDeviceImp()
	: mPangoLayout(nil)
	, mFont(nil)
	, mMetrics(nil)
{
	if (sPangoContext == nil)
	{
		sPangoContext = pango_cairo_font_map_create_context(
			PANGO_CAIRO_FONT_MAP(pango_cairo_font_map_get_default()));
	}
	
	if (sPangoLanguage == nil)
	{
		const char* LANG = getenv("LANG");
		
		if (LANG != nil)
			sPangoLanguage = pango_language_from_string(LANG);
		else
			sPangoLanguage = gtk_get_default_language();
	}

	mPangoLayout = pango_layout_new(sPangoContext);
}

MDeviceImp::~MDeviceImp()
{
	if (mMetrics != nil)
		pango_font_metrics_unref(mMetrics);
	
	if (mFont != nil)
		pango_font_description_free(mFont);
	
	if (mPangoLayout != nil)
		g_object_unref(mPangoLayout);
}

void MDeviceImp::Save()
{
}

void MDeviceImp::Restore()
{
}

void MDeviceImp::SetFont(
	const string&		inFont)
{
	if (mMetrics != nil)
		pango_font_metrics_unref(mMetrics);
	
	if (mFont != nil)
		pango_font_description_free(mFont);
	
	mFont = pango_font_description_from_string(inFont.c_str());

	if (mFont != nil)
		pango_layout_set_font_description(mPangoLayout, mFont);
}

void MDeviceImp::SetForeColor(
	MColor				inColor)
{
}

MColor MDeviceImp::GetForeColor() const
{
	return kBlack;
}

void MDeviceImp::SetBackColor(
	MColor				inColor)
{
}

MColor MDeviceImp::GetBackColor() const
{
	return kWhite;
}

void MDeviceImp::EraseRect(
	MRect				inRect)
{
}

void MDeviceImp::FillRect(
	MRect				inRect)
{
}

void MDeviceImp::StrokeRect(
	MRect				inRect,
	uint32				inLineWidth)
{
}

void MDeviceImp::FillEllipse(
	MRect				inRect)
{
}

void MDeviceImp::CreateAndUsePattern(
	MColor				inColor1,
	MColor				inColor2)
{
}

void MDeviceImp::DrawListItemBackground(
	MRect				inRect,
	bool				inSelected,
	bool				inActive,
	bool				inOdd,
	bool				inRoundEdges)
{
}

PangoFontMetrics* MDeviceImp::GetMetrics()
{
	if (mMetrics == nil)
	{
		if (mFont == nil)
			mMetrics = pango_context_get_metrics(sPangoContext,
				pango_context_get_font_description(sPangoContext), sPangoLanguage);
		else
			mMetrics = pango_context_get_metrics(sPangoContext, mFont, sPangoLanguage);
	}
	
	return mMetrics;
}

uint32 MDeviceImp::GetAscent()
{
	uint32 result = 10;

	PangoFontMetrics* metrics = GetMetrics();
	if (metrics != nil)
		result = pango_font_metrics_get_ascent(metrics) / PANGO_SCALE;

	return result;
}

uint32 MDeviceImp::GetDescent()
{
	uint32 result = 10;

	PangoFontMetrics* metrics = GetMetrics();
	if (metrics != nil)
		result = pango_font_metrics_get_descent(metrics) / PANGO_SCALE;

	return result;
}

uint32 MDeviceImp::GetLeading()
{
	return 0;
}

uint32 MDeviceImp::GetLineHeight()
{
	uint32 result = 10;

	PangoFontMetrics* metrics = GetMetrics();
	if (metrics != nil)
	{
		uint32 ascent = pango_font_metrics_get_ascent(metrics);
		uint32 descent = pango_font_metrics_get_descent(metrics);

		result = (ascent + descent) / PANGO_SCALE;
	}

	return result;
}

void MDeviceImp::DrawString(
	const string&		inText,
	float				inX,
	float				inY,
	uint32				inTruncateWidth,
	MAlignment			inAlign)
{
}

uint32 MDeviceImp::GetStringWidth(
	const string&		inText)
{
	pango_layout_set_text(mPangoLayout, inText.c_str(), inText.length());
	
	PangoRectangle r;
	pango_layout_get_pixel_extents(mPangoLayout, nil, &r);
	
	return r.width;
}

void MDeviceImp::SetText(
	const string&		inText)
{
	pango_layout_set_text(mPangoLayout, inText.c_str(), inText.length());

	// reset attributes
	PangoAttrList* attrs = pango_attr_list_new ();
	pango_layout_set_attributes (mPangoLayout, attrs);
	pango_attr_list_unref (attrs);
}

void MDeviceImp::SetTabStops(
	uint32				inTabWidth)
{
	PangoTabArray* tabs = pango_tab_array_new(2, false);
	
	uint32 next = inTabWidth;
	
	for (uint32 x = 0; x < 2; ++x)
	{
		pango_tab_array_set_tab(tabs, x, PANGO_TAB_LEFT, next * PANGO_SCALE);
		next += inTabWidth;
	}
	
	pango_layout_set_tabs(mPangoLayout, tabs);
	
	pango_tab_array_free(tabs);
}

void MDeviceImp::SetTextColors(
	uint32				inColorCount,
	uint32				inColors[],
	uint32				inOffsets[])
{
	PangoAttrList* attrs = pango_attr_list_new ();

	for (uint32 ix = 0; ix < inColorCount; ++ix)
	{
		MColor c = gLanguageColors[inColors[ix]];
		
		uint16 red = c.red << 8 | c.red;
		uint16 green = c.green << 8 | c.green;
		uint16 blue = c.blue << 8 | c.blue;
		
		PangoAttribute* attr = pango_attr_foreground_new(red, green, blue);
		attr->start_index = inOffsets[ix];
		
		if (ix == inColorCount - 1)
			attr->end_index = -1;
		else
			attr->end_index = inOffsets[ix + 1];
		
		pango_attr_list_insert(attrs, attr);
	}
	
	pango_layout_set_attributes (mPangoLayout, attrs);
	
	pango_attr_list_unref (attrs);
}

void MDeviceImp::SetTextSelection(
	uint32				inStart,
	uint32				inLength,
	MColor				inSelectionColor)
{
	uint16 red = inSelectionColor.red << 8 | inSelectionColor.red;
	uint16 green = inSelectionColor.green << 8 | inSelectionColor.green;
	uint16 blue = inSelectionColor.blue << 8 | inSelectionColor.blue;
		
	PangoAttribute* attr = pango_attr_background_new(red, green, blue);
	attr->start_index = inStart;
	attr->end_index = inStart + inLength;
	
	PangoAttrList* attrs = pango_layout_get_attributes(mPangoLayout);
	
	if (attrs == nil)
	{
		attrs = pango_attr_list_new();
		pango_attr_list_insert(attrs, attr);
	}
	else
	{
		attrs = pango_attr_list_copy(attrs);
		pango_attr_list_change(attrs, attr);
	}
	
	pango_layout_set_attributes(mPangoLayout, attrs);
	
	pango_attr_list_unref(attrs);
}

void MDeviceImp::IndexToPosition(
	uint32				inIndex,
	bool				inTrailing,
	int32&				outPosition)
{
	PangoRectangle r;
	pango_layout_index_to_pos(mPangoLayout, inIndex, &r);
	outPosition = r.x / PANGO_SCALE;
}

bool MDeviceImp::PositionToIndex(
	int32				inPosition,
	uint32&				outIndex,
	bool&				outTrailing)
{
	int index, trailing;
	
	bool result = pango_layout_xy_to_index(mPangoLayout,
		inPosition * PANGO_SCALE, 0, &index, &trailing); 

	outIndex = index;
	outTrailing = trailing;
	
	if (not result)
	{
		int w;
		pango_layout_get_pixel_size(mPangoLayout, &w, nil);
		if (inPosition >= w)
			outIndex = strlen(pango_layout_get_text(mPangoLayout));		
	}
	
	return result;
}

uint32 MDeviceImp::GetTextWidth()
{
	PangoRectangle r;
	
	pango_layout_get_pixel_extents(mPangoLayout, nil, &r);
	
	return r.width;
}

void MDeviceImp::DrawText(
	float				inX,
	float				inY)
{
}

void MDeviceImp::DrawCaret(
	float				inX,
	float				inY,
	uint32				inOffset)
{
}

bool MDeviceImp::BreakLine(
	uint32				inWidth,
	uint32&				outBreak)
{
	bool result = false;
	
	pango_layout_set_width(mPangoLayout, inWidth * PANGO_SCALE);
	pango_layout_set_wrap(mPangoLayout, PANGO_WRAP_WORD_CHAR);

#if defined(PANGO_VERSION_CHECK)
#if PANGO_VERSION_CHECK(1,15,2)
#define PANGO_OK	1
#endif
#endif

#if defined(PANGO_OK)
	if (pango_layout_is_wrapped(mPangoLayout))
	{
		PangoLayoutLine* line = pango_layout_get_line_readonly(mPangoLayout, 0);
		
		if (line != nil)
		{
			outBreak = line->length;
			result = true;
		}
	}
#else
	PangoLayoutLine* line = pango_layout_get_line(mPangoLayout, 0);
	
	if (line != nil)
	{
		outBreak = line->length;
		result = true;
	}
#endif
	
	return result;
}

// --------------------------------------------------------------------
// MCairoDeviceImp is derived from MDeviceImp
// It provides the routines for drawing on a cairo surface

class MCairoDeviceImp : public MDeviceImp
{
  public:
							MCairoDeviceImp(
								MView*		inView,
								MRect		inRect);
						
							~MCairoDeviceImp();

	virtual void			Save();
	
	virtual void			Restore();

	virtual void			SetForeColor(
								MColor				inColor);

	virtual MColor			GetForeColor() const;

	virtual void			SetBackColor(
								MColor				inColor);

	virtual MColor			GetBackColor() const;
	
	virtual void			EraseRect(
								MRect				inRect);

	virtual void			FillRect(
								MRect				inRect);

	virtual void			StrokeRect(
								MRect				inRect,
								uint32				inLineWidth = 1);

	virtual void			FillEllipse(
								MRect				inRect);
	
	virtual void			CreateAndUsePattern(
								MColor				inColor1,
								MColor				inColor2);
	
	void					SetRowBackColor(
								bool				inOdd);
	
	virtual void			DrawListItemBackground(
								MRect				inRect,
								bool				inSelected,
								bool				inActive,
								bool				inOdd,
								bool				inRoundEdges);

	virtual void			DrawString(
								const string&		inText,
								float				inX,
								float				inY,
								uint32				inTruncateWidth,
								MAlignment			inAlign);

	virtual void			DrawText(
								float				inX,
								float				inY);

	virtual void			DrawCaret(
								float				inX,
								float				inY,
								uint32				inOffset);

  protected:
	
	MView*					mView;
	MRect					mRect;
	MColor					mForeColor;
	MColor					mBackColor;
	MColor					mEvenRowColor;
	cairo_t*				mContext;
	uint32					mPatternData[8][8];
};

MCairoDeviceImp::MCairoDeviceImp(
	MView*		inView,
	MRect		inRect)
	: mView(inView)
	, mRect(inRect)
{
	mForeColor = kBlack;
	mBackColor = kWhite;

	mContext = gdk_cairo_create(mView->GetGtkWidget()->window);

	cairo_rectangle(mContext, mRect.x, mRect.y, mRect.width, mRect.height);
	cairo_clip(mContext);
}

MCairoDeviceImp::~MCairoDeviceImp()
{
	cairo_destroy(mContext);
}

void MCairoDeviceImp::Save()
{
	cairo_save(mContext);
}

void MCairoDeviceImp::Restore()
{
	cairo_restore(mContext);
}

void MCairoDeviceImp::SetForeColor(
	MColor				inColor)
{
	mForeColor = inColor;

	cairo_set_source_rgb(mContext,
		mForeColor.red / 255.0,
		mForeColor.green / 255.0,
		mForeColor.blue / 255.0);
}

MColor MCairoDeviceImp::GetForeColor() const
{
	return mForeColor;
}

void MCairoDeviceImp::SetBackColor(
	MColor				inColor)
{
	mBackColor = inColor;
}

MColor MCairoDeviceImp::GetBackColor() const
{
	return mBackColor;
}

void MCairoDeviceImp::EraseRect(
	MRect				inRect)
{
	cairo_save(mContext);
	
	cairo_rectangle(mContext, inRect.x, inRect.y, inRect.width, inRect.height);

	cairo_set_source_rgb(mContext,
		mBackColor.red / 255.0,
		mBackColor.green / 255.0,
		mBackColor.blue / 255.0);
	cairo_fill(mContext);
	
	cairo_restore(mContext);
}

void MCairoDeviceImp::FillRect(
	MRect				inRect)
{
	cairo_rectangle(mContext, inRect.x, inRect.y, inRect.width, inRect.height);
	cairo_fill(mContext);
}

void MCairoDeviceImp::StrokeRect(
	MRect				inRect,
	uint32				inLineWidth)
{
	cairo_rectangle(mContext, inRect.x, inRect.y, inRect.width, inRect.height);
	cairo_stroke(mContext);
}

void MCairoDeviceImp::FillEllipse(
	MRect				inRect)
{
	cairo_save(mContext);
	cairo_translate(mContext, inRect.x + inRect.width / 2., inRect.y + inRect.height / 2.);
	cairo_scale(mContext, inRect.width / 2., inRect.height / 2.);
	cairo_arc(mContext, 0., 0., 1., 0., 2 * M_PI);
	cairo_fill(mContext);
	cairo_restore(mContext);
}

void MCairoDeviceImp::CreateAndUsePattern(
	MColor				inColor1,
	MColor				inColor2)
{
	uint32 c1 = 0, c2 = 0;
	
	c1 |= inColor1.red << 16;
	c1 |= inColor1.green << 8;
	c1 |= inColor1.blue << 0;
	
	c2 |= inColor2.red << 16;
	c2 |= inColor2.green << 8;
	c2 |= inColor2.blue << 0;
	
	for (uint32 y = 0; y < 8; ++y)
	{
		for (uint32 x = 0; x < 4; ++x)
			mPatternData[y][x] = c1;
		for (uint32 x = 4; x < 8; ++x)
			mPatternData[y][x] = c2;
	}
	
	cairo_surface_t* s = cairo_image_surface_create_for_data(
		reinterpret_cast<uint8*>(mPatternData), CAIRO_FORMAT_RGB24, 8, 8, 0);

	if (s != nil)
	{
		cairo_pattern_t* p = cairo_pattern_create_for_surface(s);
		cairo_pattern_set_extend(p, CAIRO_EXTEND_REPEAT);
		
		if (p != nil)
		{
			cairo_matrix_t m;
			cairo_matrix_init_rotate(&m, 2.356);
			cairo_pattern_set_matrix(p, &m);
			
			cairo_set_source(mContext, p);
			
			cairo_pattern_destroy(p);
		}
		
		cairo_surface_destroy(s);
	}
}

void MCairoDeviceImp::DrawListItemBackground(
	MRect				inRect,
	bool				inSelected,
	bool				inActive,
	bool				inOdd,
	bool				inRoundEdges)
{
	if (inSelected and not inRoundEdges)
	{
		if (inActive)
			SetBackColor(gHiliteColor);
		else
			SetBackColor(gInactiveHiliteColor);
	}
	else
		SetRowBackColor(inOdd);
	
	EraseRect(inRect);

	if (inSelected and inRoundEdges)
	{
		MRect selectionRect = inRect;
		selectionRect.InsetBy(2, 0);

		if (inActive)
			SetForeColor(gHiliteColor);
		else
			SetForeColor(gInactiveHiliteColor);
		
		MRect sr = selectionRect;
		sr.InsetBy(sr.height / 2, 0);
		FillRect(sr);
		
		sr = selectionRect;
		sr.width = sr.height;
		
		FillEllipse(sr);
		
		sr = selectionRect;
		sr.x += sr.width - sr.height;
		sr.width = sr.height;
		FillEllipse(sr);
	}
}

void MCairoDeviceImp::DrawString(
	const string&		inText,
	float				inX,
	float				inY,
	uint32				inTruncateWidth,
	MAlignment			inAlign)
{
	pango_layout_set_text(mPangoLayout, inText.c_str(), inText.length());
	
	if (inTruncateWidth != 0)
	{
		pango_layout_set_ellipsize(mPangoLayout, PANGO_ELLIPSIZE_END);
		pango_layout_set_width(mPangoLayout, inTruncateWidth * PANGO_SCALE);
	
		if (inAlign != eAlignNone and inAlign != eAlignLeft)
		{
			PangoRectangle r;
			pango_layout_get_pixel_extents(mPangoLayout, nil, &r);
		
			if (static_cast<uint32>(r.width) < inTruncateWidth)
			{
				if (inAlign == eAlignCenter)
					inX += (inTruncateWidth - r.width) / 2;
				else
					inX += inTruncateWidth - r.width;
			}
		}
	}
	else
	{
		pango_layout_set_ellipsize(mPangoLayout, PANGO_ELLIPSIZE_NONE);
		pango_layout_set_width(mPangoLayout, mRect.width * PANGO_SCALE);
	}

	cairo_move_to(mContext, inX, inY);	

	pango_cairo_show_layout(mContext, mPangoLayout);
}

void MCairoDeviceImp::DrawText(
	float				inX,
	float				inY)
{
	cairo_move_to(mContext, inX, inY);
	pango_cairo_show_layout(mContext, mPangoLayout);
}

void MCairoDeviceImp::DrawCaret(
	float				inX,
	float				inY,
	uint32				inOffset)
{
	PangoRectangle sp, wp;

	pango_layout_get_cursor_pos(mPangoLayout, inOffset, &sp, &wp);
	
	Save();
	
	cairo_set_line_width(mContext, 1.0);
	cairo_set_source_rgb(mContext, 0, 0, 0);
	cairo_move_to(mContext, inX + sp.x / PANGO_SCALE, inY + sp.y / PANGO_SCALE);
	cairo_rel_line_to(mContext, sp.width / PANGO_SCALE, sp.height / PANGO_SCALE);
	cairo_stroke(mContext);
	
	Restore();
}

void MCairoDeviceImp::SetRowBackColor(
	bool		inOdd)
{
	const char* styleName = "even-row-color";
	if (inOdd)
		styleName = "odd-row-color";

	mBackColor = kWhite;
	
	if (inOdd)
	{
		mBackColor.red = static_cast<uint16>(0.93 * mBackColor.red);
		mBackColor.green = static_cast<uint16>(0.93 * mBackColor.green);
		mBackColor.blue = static_cast<uint16>(0.93 * mBackColor.blue);
	}
}

// -------------------------------------------------------------------

MDevice::MDevice()
	: mImpl(new MDeviceImp())
{
}

MDevice::MDevice(
	MView*		inView,
	MRect		inRect)
	: mImpl(new MCairoDeviceImp(inView, inRect))
{
}

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

void MDevice::DrawListItemBackground(
	MRect				inRect,
	bool				inSelected,
	bool				inActive,
	bool				inOdd,
	bool				inRoundEdges)
{
	mImpl->DrawListItemBackground(inRect, inSelected, inActive, inOdd, inRoundEdges);
}

uint32 MDevice::GetAscent() const
{
	return mImpl->GetAscent();
}

uint32 MDevice::GetDescent() const
{
	return mImpl->GetDescent();
}

uint32 MDevice::GetLeading() const
{
	return mImpl->GetLeading();
}

uint32 MDevice::GetLineHeight() const
{
	return mImpl->GetLineHeight();
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

uint32 MDevice::GetStringWidth(
	const string&		inText)
{
	return mImpl->GetStringWidth(inText);
}

void MDevice::SetText(
	const string&	inText)
{
	mImpl->SetText(inText);
}

void MDevice::SetTabStops(
	uint32			inTabWidth)
{
	mImpl->SetTabStops(inTabWidth);
}

void MDevice::SetTextColors(
	uint32			inColorCount,
	uint32			inColors[],
	uint32			inOffsets[])
{
	mImpl->SetTextColors(inColorCount, inColors, inOffsets);
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
	uint32&			outIndex,
	bool&			outTrailing)
{
	return mImpl->PositionToIndex(inPosition, outIndex, outTrailing);
}

void MDevice::DrawText(
	float			inX,
	float			inY)
{
	mImpl->DrawText(inX, inY);
}

uint32 MDevice::GetTextWidth()
{
	return mImpl->GetTextWidth();
}

bool MDevice::BreakLine(
	uint32				inWidth,
	uint32&				outBreak)
{
	return mImpl->BreakLine(inWidth, outBreak);
}

void MDevice::DrawCaret(
	float				inX,
	float				inY,
	uint32				inOffset)
{
	mImpl->DrawCaret(inX, inY, inOffset);
}
