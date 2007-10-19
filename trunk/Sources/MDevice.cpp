#include "MJapieG.h"

#include <cmath>

#include "MDevice.h"
#include "MDrawingArea.h"
#include "MFont.h"
#include "MDrawingArea.h"
#include "MWindow.h"
#include "MGlobals.h"

using namespace std;

namespace {

#pragma message("GdkPixmap gaan gebruiken")
	
class MDummyWindow : public MWindow
{
  public:
					MDummyWindow();
//					~MDummyWindow();

	static MDummyWindow*	Instance();
	
	MDrawingArea*	GetDrawingArea()			{ return mDrawingArea ; }

  private:
	MDrawingArea*	mDrawingArea;
};

MDummyWindow::MDummyWindow()
{
	mDrawingArea = new MDrawingArea(10, 10);
	Add(mDrawingArea);
	
//	gtk_widget_realize(GetGtkWidget());
	Show();
	Hide();
}

MDummyWindow* MDummyWindow::Instance()
{
	static MDummyWindow* sInstance = nil;
	
	if (sInstance == nil)
		sInstance = new MDummyWindow();
	
	return sInstance;
}

}

struct MDeviceImp
{
					MDeviceImp();
		
					MDeviceImp(
						MView*		inView,
						MRect		inRect);
		
					~MDeviceImp();
	
	void			Init();
	
	MDrawingArea*	mView;
	MRect			mRect;
	MColor			mForeColor;
	MColor			mBackColor;
//	MWindow*		mWindow;
	cairo_t*		mContext;
	PangoLayout*	mLayout;
	PangoLanguage*	mLanguage;
	MFont			mFont;
	uint32			mPatternData[8][8];
};

MDeviceImp::MDeviceImp()
	: mView(MDummyWindow::Instance()->GetDrawingArea())
	, mRect(0, 0, 10000, 10)
	, mFont(kFixedFont)
{
	Init();
}

MDeviceImp::MDeviceImp(
	MView*		inView,
	MRect		inRect)
	: mView(dynamic_cast<MDrawingArea*>(inView))
	, mRect(inRect)
	, mFont(kFixedFont)
{
	Init();
}

void MDeviceImp::Init()
{
	mForeColor = kBlack;
	mBackColor = kWhite;
	mLayout = nil;
	mLanguage = nil;

	mContext = gdk_cairo_create(mView->GetGtkWidget()->window);

	cairo_rectangle(mContext, mRect.x, mRect.y, mRect.width, mRect.height);
	cairo_clip(mContext);

	const char* LANG = getenv("LANG");
	
	if (LANG != nil)
		mLanguage = pango_language_from_string(LANG);
	else
		mLanguage = gtk_get_default_language();

	PangoContext* pc = mView->GetPangoContext();
	mLayout = pango_layout_new(pc);
	pango_layout_set_font_description(mLayout, mFont);
}

MDeviceImp::~MDeviceImp()
{
	if (mLayout != nil)
		g_object_unref(mLayout);
	
	cairo_destroy(mContext);
}

// -------------------------------------------------------------------

MDevice::MDevice()
	: mImpl(new MDeviceImp())
{
}

MDevice::MDevice(
	MView*		inView,
	MRect		inRect)
	: mImpl(new MDeviceImp(inView, inRect))
{
}

MDevice::~MDevice()
{
	delete mImpl;
}

void MDevice::Save()
{
	cairo_save(mImpl->mContext);
}

void MDevice::Restore()
{
	cairo_restore(mImpl->mContext);
}

void MDevice::SetForeColor(
	MColor		inColor)
{
	mImpl->mForeColor = inColor;

	cairo_set_source_rgb(mImpl->mContext,
		mImpl->mForeColor.red / 255.0,
		mImpl->mForeColor.green / 255.0,
		mImpl->mForeColor.blue / 255.0);
}

MColor MDevice::GetForeColor() const
{
	return mImpl->mForeColor;
}

void MDevice::SetBackColor(
	MColor		inColor)
{
	mImpl->mBackColor = inColor;
}

MColor MDevice::GetBackColor() const
{
	return mImpl->mBackColor;
}

void MDevice::EraseRect(
	MRect		inRect)
{
	cairo_save(mImpl->mContext);
	
	cairo_rectangle(mImpl->mContext, inRect.x, inRect.y, inRect.width, inRect.height);

	cairo_set_source_rgb(mImpl->mContext,
		mImpl->mBackColor.red / 255.0,
		mImpl->mBackColor.green / 255.0,
		mImpl->mBackColor.blue / 255.0);
	cairo_fill(mImpl->mContext);
	
	cairo_restore(mImpl->mContext);
}

void MDevice::FillRect(
	MRect		inRect)
{
	cairo_rectangle(mImpl->mContext, inRect.x, inRect.y, inRect.width, inRect.height);
	cairo_fill(mImpl->mContext);
}

void MDevice::FillEllipse(
	MRect		inRect)
{
	cairo_save(mImpl->mContext);
	cairo_translate(mImpl->mContext, inRect.x + inRect.width / 2., inRect.y + inRect.height / 2.);
	cairo_scale(mImpl->mContext, inRect.width / 2., inRect.height / 2.);
	cairo_arc(mImpl->mContext, 0., 0., 1., 0., 2 * M_PI);
	cairo_fill(mImpl->mContext);
	cairo_restore(mImpl->mContext);
}

void MDevice::CreateAndUsePattern(
	MColor		inColor1,
	MColor		inColor2)
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
			mImpl->mPatternData[y][x] = c1;
		for (uint32 x = 4; x < 8; ++x)
			mImpl->mPatternData[y][x] = c2;
	}
	
	cairo_surface_t* s = cairo_image_surface_create_for_data(
		reinterpret_cast<uint8*>(mImpl->mPatternData), CAIRO_FORMAT_RGB24, 8, 8, 0);

	if (s != nil)
	{
		cairo_pattern_t* p = cairo_pattern_create_for_surface(s);
		cairo_pattern_set_extend(p, CAIRO_EXTEND_REPEAT);
		
		if (p != nil)
		{
			cairo_matrix_t m;
			cairo_matrix_init_rotate(&m, 2.356);
			cairo_pattern_set_matrix(p, &m);
			
			cairo_set_source(mImpl->mContext, p);
			
			cairo_pattern_destroy(p);
		}
		
		cairo_surface_destroy(s);
	}
}

void MDevice::DrawListItemBackground(
	MRect				inRect,
	bool				inSelected,
	bool				inActive,
	bool				inOdd)
{
	if (inSelected)
	{
		if (inActive)
			SetBackColor(gHiliteColor);
		else
			SetBackColor(gInactiveHiliteColor);
	}
	else if (inOdd)
		SetBackColor(gOddRowColor);
	else
		SetBackColor(kWhite);
	
	EraseRect(inRect);
}

uint32 MDevice::GetAscent() const
{
	uint32 result = 10;

	PangoContext* pc = pango_layout_get_context(mImpl->mLayout);
	PangoFontMetrics* metrics = pango_context_get_metrics(pc, mImpl->mFont, mImpl->mLanguage);

	if (metrics != nil)
	{
		result = pango_font_metrics_get_ascent(metrics) / PANGO_SCALE;
		pango_font_metrics_unref(metrics);
	}

	return result;
}

uint32 MDevice::GetDescent() const
{
	uint32 result = 10;

	PangoContext* pc = pango_layout_get_context(mImpl->mLayout);
	PangoFontMetrics* metrics = pango_context_get_metrics(pc, mImpl->mFont, mImpl->mLanguage);

	if (metrics != nil)
	{
		result = pango_font_metrics_get_descent(metrics) / PANGO_SCALE;
		pango_font_metrics_unref(metrics);
	}

	return result;
}

uint32 MDevice::GetLeading() const
{
	return 0;
}

uint32 MDevice::GetLineHeight() const
{
	uint32 result = 10;

	PangoContext* pc = pango_layout_get_context(mImpl->mLayout);
	PangoFontMetrics* metrics = pango_context_get_metrics(pc, mImpl->mFont, mImpl->mLanguage);

	if (metrics != nil)
	{
		uint32 ascent = pango_font_metrics_get_ascent(metrics);
		uint32 descent = pango_font_metrics_get_descent(metrics);
		
		result = (ascent + descent) / PANGO_SCALE;
		pango_font_metrics_unref(metrics);
	}

	return result;
}

void MDevice::DrawString(
	const string&	inText,
	float 			inX,
	float 			inY,
	bool			inTruncate)
{
	if (inTruncate)
	{
		pango_layout_set_ellipsize(mImpl->mLayout, PANGO_ELLIPSIZE_END);
		pango_layout_set_width(mImpl->mLayout, mImpl->mRect.width * PANGO_SCALE);
	}

	pango_layout_set_text(mImpl->mLayout, inText.c_str(), inText.length());
	
	cairo_move_to(mImpl->mContext, inX, inY);	

	pango_cairo_show_layout(mImpl->mContext, mImpl->mLayout);
}

uint32 MDevice::GetStringWidth(
	const std::string&	inText)
{
	pango_layout_set_text(mImpl->mLayout, inText.c_str(), inText.length());
	
	PangoRectangle r;
	pango_layout_get_pixel_extents(mImpl->mLayout, nil, &r);
	
	return r.width;
}

// --------------------------------------------------------------------

void MDevice::SetText(
	const string&	inText)
{
	pango_layout_set_text(mImpl->mLayout, inText.c_str(), inText.length());
}

void MDevice::SetTabStops(
	uint32			inTabWidth)
{
	uint32 count = mImpl->mRect.width / inTabWidth;

	PangoTabArray* tabs = pango_tab_array_new(count, false);
	
	uint32 next = inTabWidth;
	
	for (uint32 x = 0; x < count; ++x)
	{
		pango_tab_array_set_tab(tabs, x, PANGO_TAB_LEFT, next * PANGO_SCALE);
		next += inTabWidth;
	}
	
	pango_layout_set_tabs(mImpl->mLayout, tabs);
	
	pango_tab_array_free(tabs);
}

void MDevice::SetTextColors(
	uint32			inColorCount,
	uint32			inColors[],
	uint32			inOffsets[])
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
	
	pango_layout_set_attributes (mImpl->mLayout, attrs);
	
	pango_attr_list_unref (attrs);
}

void MDevice::SetTextSelection(
	uint32			inStart,
	uint32			inLength,
	MColor			inSelectionColor)
{
	uint16 red = inSelectionColor.red << 8 | inSelectionColor.red;
	uint16 green = inSelectionColor.green << 8 | inSelectionColor.green;
	uint16 blue = inSelectionColor.blue << 8 | inSelectionColor.blue;
		
	PangoAttribute* attr = pango_attr_background_new(red, green, blue);
	attr->start_index = inStart;
	attr->end_index = inStart + inLength;
	
	PangoAttrList* attrs = pango_layout_get_attributes(mImpl->mLayout);
	
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
	
	pango_layout_set_attributes (mImpl->mLayout, attrs);
	
	pango_attr_list_unref (attrs);
}

void MDevice::IndexToPosition(
	uint32			inIndex,
	bool			inTrailing,
	int32&			outPosition)
{
	PangoRectangle r;
	pango_layout_index_to_pos(mImpl->mLayout, inIndex, &r);
	outPosition = r.x / PANGO_SCALE;
}

bool MDevice::PositionToIndex(
	int32			inPosition,
	uint32&			outIndex,
	bool&			outTrailing)
{
	int index, trailing;
	
	bool result = pango_layout_xy_to_index(mImpl->mLayout,
		inPosition * PANGO_SCALE, 0, &index, &trailing); 

	outIndex = index;
	outTrailing = trailing;
	
	return result;
}

void MDevice::DrawText(
	float			inX,
	float			inY)
{
	cairo_move_to(mImpl->mContext, inX, inY);	
	pango_cairo_show_layout(mImpl->mContext, mImpl->mLayout);
}

uint32 MDevice::GetTextWidth()
{
	PangoRectangle r;
	
	pango_layout_get_pixel_extents(mImpl->mLayout, nil, &r);
	
	return r.width;
}

void MDevice::DrawCaret(
	float				inX,
	float				inY,
	uint32				inOffset)
{
	PangoRectangle sp, wp;

	pango_layout_get_cursor_pos(mImpl->mLayout, inOffset, &sp, &wp);
	
	Save();
	
	cairo_set_line_width(mImpl->mContext, 1.0);
	cairo_set_source_rgb(mImpl->mContext, 0, 0, 0);
	cairo_move_to(mImpl->mContext, inX + sp.x / PANGO_SCALE, inY + sp.y / PANGO_SCALE);
	cairo_rel_line_to(mImpl->mContext, sp.width / PANGO_SCALE, sp.height / PANGO_SCALE);
	cairo_stroke(mImpl->mContext);
	
	Restore();
}
