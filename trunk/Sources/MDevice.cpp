//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "MJapi.h"

#include <cmath>
#include <cstring>

#include "MDevice.h"
#include "MView.h"
#include "MWindow.h"
#include "MGlobals.h"
#include "MError.h"
#include "MUnicode.h"

using namespace std;

// --------------------------------------------------------------------
// base class for MDeviceImp
// provides only the basic Pango functionality
// This is needed in measuring text metrics and such

class MDeviceImp
{
  public:
							MDeviceImp();

							MDeviceImp(
								PangoLayout*		inLayout);

	virtual					~MDeviceImp();

	virtual void			Save();
	
	virtual void			Restore();

	virtual bool			IsPrinting(
								int32&				outPage) const	{ return false; }

	virtual MRect			GetBounds() const						{ return MRect(0, 0, 100, 100); }

	virtual void			SetOrigin(
								int32				inX,
								int32				inY);

	virtual void			SetFont(
								const string&		inFont);

	virtual void			SetForeColor(
								MColor				inColor);

	virtual MColor			GetForeColor() const;

	virtual void			SetBackColor(
								MColor				inColor);

	virtual MColor			GetBackColor() const;
	
	virtual void			ClipRect(
								MRect				inRect);

	virtual void			ClipRegion(
								MRegion				inRegion);

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
								uint32&				outIndex);
	
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

	virtual void			MakeTransparent(
								float				inOpacity) {}

	virtual GdkPixmap*		GetPixmap() const		{ return nil; }

	virtual void			SetDrawWhiteSpace(
								bool				inDrawWhiteSpace) {}

  protected:

	PangoItem*				Itemize(
								const char*			inText,
								PangoAttrList*		inAttrs);

	void					GetWhiteSpaceGlyphs(
								uint32&				outSpace,
								uint32&				outTab,
								uint32&				outNL);

	PangoLayout*			mPangoLayout;
	PangoFontDescription*	mFont;
	PangoFontMetrics*		mMetrics;
	bool					mTextEndsWithNewLine;
	uint32					mSpaceGlyph, mTabGlyph, mNewLineGlyph;
};

MDeviceImp::MDeviceImp()
	: mPangoLayout(nil)
	, mFont(nil)
	, mMetrics(nil)
{
	static PangoContext*	sPangoContext;
	static PangoLanguage*	sPangoLanguage;

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

MDeviceImp::MDeviceImp(
	PangoLayout*		inLayout)
	: mPangoLayout(inLayout)
	, mFont(nil)
	, mMetrics(nil)
{
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

void MDeviceImp::SetOrigin(
	int32		inX,
	int32		inY)
{
}

void MDeviceImp::SetFont(
	const string&		inFont)
{
	PangoFontDescription* newFontDesc = pango_font_description_from_string(inFont.c_str());

	if (mFont == nil or newFontDesc == nil or
		not pango_font_description_equal(mFont, newFontDesc))
	{
		if (mMetrics != nil)
			pango_font_metrics_unref(mMetrics);
		
		if (mFont != nil)
			pango_font_description_free(mFont);
		
		mFont = newFontDesc;
	
		if (mFont != nil)
		{
			mMetrics = GetMetrics();
			
			pango_layout_set_font_description(mPangoLayout, mFont);
			GetWhiteSpaceGlyphs(mSpaceGlyph, mTabGlyph, mNewLineGlyph);
		}
	}
	else
		pango_font_description_free(newFontDesc);
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

void MDeviceImp::ClipRect(
	MRect				inRect)
{
}

void MDeviceImp::ClipRegion(
	MRegion				inRegion)
{
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

PangoFontMetrics* MDeviceImp::GetMetrics()
{
	if (mMetrics == nil)
	{
		PangoContext* context = pango_layout_get_context(mPangoLayout);
		
		PangoFontDescription* fontDesc = mFont;
		if (fontDesc == nil)
		{
			fontDesc = pango_context_get_font_description(context);
			
			// there's a bug in pango I guess
			
			int32 x;
			if (IsPrinting(x))
				fontDesc = pango_font_description_copy(fontDesc);
		}
		
		mMetrics = pango_context_get_metrics(context, fontDesc, nil);
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
	mTextEndsWithNewLine = inText.length() > 0 and inText[inText.length() - 1] == '\n';

	// reset attributes
	PangoAttrList* attrs = pango_attr_list_new();
	pango_layout_set_attributes(mPangoLayout, attrs);
	pango_attr_list_unref(attrs);
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
	PangoAttrList* attrs = pango_attr_list_new();

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
	
	pango_layout_set_attributes(mPangoLayout, attrs);
	
	pango_attr_list_unref(attrs);
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
	uint32&				outIndex)
{
	int index, trailing;
	
	bool result = pango_layout_xy_to_index(mPangoLayout,
		inPosition * PANGO_SCALE, 0, &index, &trailing); 

	MEncodingTraits<kEncodingUTF8> enc;
	const char* text = pango_layout_get_text(mPangoLayout);	

	while (trailing-- > 0)
	{
		uint32 n = enc.GetNextCharLength(text); 
		text += n;
		index += n;
	}
	
	outIndex = index;

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

	if (pango_layout_is_wrapped(mPangoLayout))
	{
		PangoLayoutLine* line = pango_layout_get_line_readonly(mPangoLayout, 0);
		
		if (line != nil)
		{
			outBreak = line->length;
			result = true;
		}
	}
	
	return result;
}

PangoItem* MDeviceImp::Itemize(
	const char*			inText,
	PangoAttrList*		inAttrs)
{
	PangoContext* context = pango_layout_get_context(mPangoLayout);
	GList* items = pango_itemize(context, inText, 0, strlen(inText), inAttrs, nil);
	PangoItem* item = static_cast<PangoItem*>(items->data);
	g_list_free(items);
	return item;
}

void MDeviceImp::GetWhiteSpaceGlyphs(
	uint32&				outSpace,
	uint32&				outTab,
	uint32&				outNL)
{
	//	long kMiddleDot = 0x00B7, kRightChevron = 0x00BB, kNotSign = 0x00AC; 
	
	const char
		not_sign[] = "\xc2\xac",		// 0x00AC
		right_chevron[] = "\xc2\xbb",	// 0x00BB
		middle_dot[] = "\xc2\xb7";		// 0x00B7
	
	PangoAttrList* attrs = pango_attr_list_new();
	
	PangoAttribute* attr = pango_attr_font_desc_new(mFont);
	attr->start_index = 0;
	attr->end_index = 2;
	
	pango_attr_list_insert(attrs, attr);
	
	PangoGlyphString* ts = pango_glyph_string_new();

	PangoItem* item = Itemize(middle_dot, attrs);
	assert(item->analysis.font);
	pango_shape(middle_dot, strlen(middle_dot), &item->analysis, ts);
	outSpace = ts->glyphs[0].glyph;

	item = Itemize(right_chevron, attrs);
	assert(item->analysis.font);
	pango_shape(right_chevron, strlen(right_chevron), &item->analysis, ts);
	outTab = ts->glyphs[0].glyph;

	item = Itemize(not_sign, attrs);
	assert(item->analysis.font);
	pango_shape(not_sign, strlen(not_sign), &item->analysis, ts);
	outNL = ts->glyphs[0].glyph;

	pango_glyph_string_free(ts);
	pango_attr_list_unref(attrs);
}

// --------------------------------------------------------------------
// MCairoDeviceImp is derived from MDeviceImp
// It provides the routines for drawing on a cairo surface

class MCairoDeviceImp : public MDeviceImp
{
  public:
							MCairoDeviceImp(
								MView*				inView,
								MRect				inRect,
								bool				inCreateOffscreen);

							MCairoDeviceImp(
								GtkPrintContext*	inContext,
								MRect				inRect,
								int32				inPage);

							~MCairoDeviceImp();

	virtual void			Save();
	
	virtual void			Restore();

	virtual bool			IsPrinting(
								int32&				outPage) const
							{
								outPage = mPage;
								return outPage >= 0;
							}

	virtual MRect			GetBounds() const						{ return mRect; }

	virtual void			SetOrigin(
								int32				inX,
								int32				inY);

	virtual void			SetForeColor(
								MColor				inColor);

	virtual MColor			GetForeColor() const;

	virtual void			SetBackColor(
								MColor				inColor);

	virtual MColor			GetBackColor() const;
	
	virtual void			ClipRect(
								MRect				inRect);

	virtual void			ClipRegion(
								MRegion				inRegion);

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

	virtual void			MakeTransparent(
								float				inOpacity);

	virtual GdkPixmap*		GetPixmap() const;

	virtual void			SetDrawWhiteSpace(
								bool				inDrawWhiteSpace);

  protected:

	void					DrawWhiteSpace(
								float				inX,
								float				inY);

	MRect					mRect;
	MColor					mForeColor;
	MColor					mBackColor;
	MColor					mEvenRowColor;
	cairo_t*				mContext;
	GdkPixmap*				mOffscreenPixmap;
	uint32					mPatternData[8][8];
	int32					mPage;
	bool					mDrawWhiteSpace;
};

MCairoDeviceImp::MCairoDeviceImp(
	MView*		inView,
	MRect		inRect,
	bool		inCreateOffscreen)
	: mRect(inRect)
	, mOffscreenPixmap(nil)
	, mPage(-1)
	, mDrawWhiteSpace(false)
{
	mForeColor = kBlack;
	mBackColor = kWhite;

	if (inCreateOffscreen)
	{
		GdkScreen* screen = gtk_widget_get_screen(inView->GetGtkWidget());
		GdkColormap* colormap = nil;
		
		if (gdk_screen_is_composited(screen))
			colormap = gdk_screen_get_rgba_colormap(screen);
		else
			colormap = gtk_widget_get_colormap(inView->GetGtkWidget());

		mOffscreenPixmap = gdk_pixmap_new(nil, inRect.width, inRect.height,
			gdk_colormap_get_visual(colormap)->depth);
		gdk_drawable_set_colormap(mOffscreenPixmap, colormap);

		mContext = gdk_cairo_create(mOffscreenPixmap);
	}
	else
		mContext = gdk_cairo_create(inView->GetGtkWidget()->window);

	cairo_rectangle(mContext, mRect.x, mRect.y, mRect.width, mRect.height);
	cairo_clip(mContext);
}

MCairoDeviceImp::MCairoDeviceImp(
	GtkPrintContext*	inPrintContext,
	MRect				inRect,
	int32				inPage)
	: MDeviceImp(gtk_print_context_create_pango_layout(inPrintContext))
	, mRect(inRect)
	, mOffscreenPixmap(nil)
	, mPage(inPage)
	, mDrawWhiteSpace(false)
{
	mForeColor = kBlack;
	mBackColor = kWhite;

	mContext = gtk_print_context_get_cairo_context(inPrintContext);

	cairo_rectangle(mContext, mRect.x, mRect.y, mRect.width, mRect.height);
	cairo_clip(mContext);
}

MCairoDeviceImp::~MCairoDeviceImp()
{
	if (mPage == -1)
		cairo_destroy(mContext);
	
	if (mOffscreenPixmap)
		g_object_unref(mOffscreenPixmap);
}

void MCairoDeviceImp::Save()
{
	cairo_save(mContext);
}

void MCairoDeviceImp::Restore()
{
	cairo_restore(mContext);
}

void MCairoDeviceImp::SetOrigin(
	int32				inX,
	int32				inY)
{
	cairo_translate(mContext, inX, inY);
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

void MCairoDeviceImp::ClipRect(
	MRect				inRect)
{
	cairo_rectangle(mContext, inRect.x, inRect.y, inRect.width, inRect.height);
	cairo_clip(mContext);
}

void MCairoDeviceImp::ClipRegion(
	MRegion				inRegion)
{
	gdk_cairo_region(mContext, inRegion);
	cairo_clip(mContext);
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

	if (mOffscreenPixmap != nil)
		cairo_set_operator(mContext, CAIRO_OPERATOR_CLEAR);

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

void MCairoDeviceImp::DrawWhiteSpace(
	float				inX,
	float				inY)
{
	int baseLine = pango_layout_get_baseline(mPangoLayout);
	PangoLayoutLine* line = pango_layout_get_line(mPangoLayout, 0);
	
	cairo_set_source_rgb(mContext,
		gWhiteSpaceColor.red / 255.0,
		gWhiteSpaceColor.green / 255.0,
		gWhiteSpaceColor.blue / 255.0);

	// we're using one font anyway
	PangoFontMap* fontMap = pango_cairo_font_map_get_default();
	PangoContext* context = pango_layout_get_context(mPangoLayout);
	PangoFont* font = pango_font_map_load_font(fontMap, context, mFont);
	cairo_scaled_font_t* scaledFont =
		pango_cairo_font_get_scaled_font(reinterpret_cast<PangoCairoFont*>(font));

	if (scaledFont == nil or cairo_scaled_font_status(scaledFont) != CAIRO_STATUS_SUCCESS)
		return;

	cairo_set_scaled_font(mContext, scaledFont);
	
	int x_position = 0;
	vector<cairo_glyph_t> cairo_glyphs;

	for (GSList* run = line->runs; run != nil; run = run->next)
	{
		PangoGlyphItem* glyphItem = reinterpret_cast<PangoGlyphItem*>(run->data);
		
		PangoGlyphItemIter iter;
		const char* text = pango_layout_get_text(mPangoLayout);
		
		for (bool more = pango_glyph_item_iter_init_start(&iter, glyphItem, text);
				  more;
				  more = pango_glyph_item_iter_next_cluster(&iter))
		{
			PangoGlyphString* gs = iter.glyph_item->glyphs;
			char ch = text[iter.start_index];
	
			for (int i = iter.start_glyph; i < iter.end_glyph; ++i)
			{
				PangoGlyphInfo* gi = &gs->glyphs[i];
				
				if (ch == ' ' or ch == '\t')
				{
					double cx = inX + double(x_position + gi->geometry.x_offset) / PANGO_SCALE;
					double cy = inY + double(baseLine + gi->geometry.y_offset) / PANGO_SCALE;
					
					cairo_glyph_t g;
					if (ch == ' ')
						g.index = mSpaceGlyph;
					else
						g.index = mTabGlyph;
					g.x = cx;
					g.y = cy;
		
					cairo_glyphs.push_back(g);
				}
				
				x_position += gi->geometry.width;
			}
		}
	}
	
	// and a trailing newline perhaps?
	
	if (mTextEndsWithNewLine)
	{
		double cx = inX + double(x_position) / PANGO_SCALE;
		double cy = inY + double(baseLine) / PANGO_SCALE;
		
		cairo_glyph_t g;
		g.index = mNewLineGlyph;
		g.x = cx;
		g.y = cy;

		cairo_glyphs.push_back(g);
	}
	
	cairo_show_glyphs(mContext, &cairo_glyphs[0], cairo_glyphs.size());	
}

void MCairoDeviceImp::DrawText(
	float				inX,
	float				inY)
{
	if (mDrawWhiteSpace)
	{
		Save();
		DrawWhiteSpace(inX, inY);
		Restore();
	}

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

void MCairoDeviceImp::MakeTransparent(
	float				inOpacity)
{
	cairo_set_operator(mContext, CAIRO_OPERATOR_DEST_OUT);
	cairo_set_source_rgba(mContext, 1, 0, 0, inOpacity);
	cairo_paint(mContext);
}

GdkPixmap* MCairoDeviceImp::GetPixmap() const
{
	g_object_ref(mOffscreenPixmap);
	return mOffscreenPixmap;
}

void MCairoDeviceImp::SetDrawWhiteSpace(
	bool				inDrawWhiteSpace)
{
	mDrawWhiteSpace = inDrawWhiteSpace;
}

// -------------------------------------------------------------------

MDevice::MDevice()
	: mImpl(new MDeviceImp())
{
}

MDevice::MDevice(
	MView*		inView,
	MRect		inRect,
	bool		inCreateOffscreen)
	: mImpl(new MCairoDeviceImp(inView, inRect, inCreateOffscreen))
{
}

MDevice::MDevice(
	GtkPrintContext*	inPrintContext,
	MRect				inRect,
	int32				inPage)
	: mImpl(new MCairoDeviceImp(inPrintContext, inRect, inPage))
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

void MDevice::SetOrigin(
	int32				inX,
	int32				inY)
{
	mImpl->SetOrigin(inX, inY);
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

void MDevice::ClipRegion(
	MRegion		inRegion)
{
	mImpl->ClipRegion(inRegion);
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

void MDevice::MakeTransparent(
	float				inOpacity)
{
	mImpl->MakeTransparent(inOpacity);
}

GdkPixmap* MDevice::GetPixmap() const
{
	return mImpl->GetPixmap();
}

void MDevice::SetDrawWhiteSpace(
	bool				inDrawWhiteSpace)
{
	mImpl->SetDrawWhiteSpace(inDrawWhiteSpace);
}
