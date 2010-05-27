//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <windows.h>
#include <d2d1.h>
#include <dwrite.h>

#undef GetNextWindow
#undef ClipRegion

#include "MLib.h"

#include <cmath>
#include <cstring>

#include "MDevice.h"
#include "MDeviceImpl.h"
#include "MView.h"
#include "MWinWindowImpl.h"
#include "MGlobals.h"
#include "MError.h"
#include "MUnicode.h"
#include "MError.h"
#include "MWinUtils.h"

using namespace std;

// --------------------------------------------------------------------
// base class for MWinDeviceImpl
// provides only the basic Pango functionality
// This is needed in measuring text metrics and such

class MWinDeviceImpl : public MDeviceImp
{
  public:
							MWinDeviceImpl(
								MView*		inView,
								MRect		inBounds,
								bool		inOffscreen);

	virtual					~MWinDeviceImpl();

	virtual void			Save();
	
	virtual void			Restore();

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

	//virtual void			ClipRegion(
	//							MRegion				inRegion);

	virtual void			EraseRect(
								MRect				inRect);

	virtual void			FillRect(
								MRect				inRect);

	virtual void			StrokeRect(
								MRect				inRect,
								uint32				inLineWidth = 1);

	virtual void			FillEllipse(
								MRect				inRect);

	//virtual void			DrawImage(
	//							cairo_surface_t*	inImage,
	//							float				inX,
	//							float				inY,
	//							float				inShear);
	
	virtual void			CreateAndUsePattern(
								MColor				inColor1,
								MColor				inColor2);
	
	//PangoFontMetrics*		GetMetrics();

	virtual uint32			GetAscent();
	
	virtual uint32			GetDescent();
	
	//virtual uint32			GetLeading();
	//
	//virtual uint32			GetLineHeight();

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
	
	virtual void			BreakLines(
								uint32				inWidth,
								vector<uint32>&		outBreaks);

	virtual void			MakeTransparent(
								float				inOpacity) {}

	//virtual GdkPixmap*		GetPixmap() const		{ return nil; }

	virtual void			SetDrawWhiteSpace(
								bool				inDrawWhiteSpace) {}

  protected:
	
	///* Should be a member of HDevice. Not of the imp! */
	//HPoint			fBackOffset;

	IDWriteTextFormat*		GetTextFormat();
	  
	/* Device context */
	HDC						mDC;
	//HBITMAP					mOffscreenBitmap;
	int						mDCState;
	//HDC						mWindowDC;
	HPEN					mForePen;
	//bool					mPrinting;

	static ID2D1Factory*	sD2DFactory;
	static IDWriteFactory*	sDWFactory;

	ID2D1HwndRenderTarget*	mRenderTarget;
	ID2D1Layer*				mClipLayer;

	IDWriteTextFormat*		mTextFormat;
	IDWriteTextLayout*		mTextLayout;

	ID2D1Brush*				mForeBrush;
	ID2D1Brush*				mBackBrush;
	wstring					mFont;
	float					mFontSize;

	float					mDpiScaleX, mDpiScaleY;

	stack<ID2D1DrawingStateBlock*>
							mState;
};

ID2D1Factory*	MWinDeviceImpl::sD2DFactory;
IDWriteFactory*	MWinDeviceImpl::sDWFactory;

MWinDeviceImpl::MWinDeviceImpl(
	MView*		inView,
	MRect		inBounds,
	bool		inOffscreen)
	: mRenderTarget(nil)
	, mClipLayer(nil)
	, mTextFormat(nil)
	, mTextLayout(nil)
	, mForeBrush(nil)
	, mBackBrush(nil)
	, mFont(L"Consolas")
	, mFontSize(10.f * 96.f / 72.f)
{
	if (sD2DFactory == nil)
		THROW_IF_HRESULT_ERROR(::D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &sD2DFactory));

	if (sDWFactory == nil)
		THROW_IF_HRESULT_ERROR(::DWriteCreateFactory(
	        DWRITE_FACTORY_TYPE_SHARED,
	        __uuidof(IDWriteFactory),
			reinterpret_cast<IUnknown**>(&sDWFactory)));
	
	MWindow* window = inView->GetWindow();
	THROW_IF_NIL(window);

	MWinWindowImpl* windowImpl = static_cast<MWinWindowImpl*>(window->GetImpl());
	mRenderTarget = windowImpl->GetRenderTarget();
	if (mRenderTarget == nil)
	{
		HWND hwnd = windowImpl->GetHandle();

		RECT r;
		::GetClientRect(hwnd, &r);

		HDC dc = ::GetDC(hwnd);
		mDpiScaleX = ::GetDeviceCaps(dc, LOGPIXELSX) / 96.0f;
		mDpiScaleY = ::GetDeviceCaps(dc, LOGPIXELSY) / 96.0f;
		::ReleaseDC(0, dc);

		THROW_IF_HRESULT_ERROR(sD2DFactory->CreateHwndRenderTarget(
			::D2D1::RenderTargetProperties(),
			::D2D1::HwndRenderTargetProperties(hwnd, D2D1::SizeU(r.right - r.left, r.bottom - r.top)),
			&mRenderTarget));

		windowImpl->SetRenderTarget(mRenderTarget);
	}

	MRect bounds;
	inView->GetBounds(bounds);
	inView->ConvertToParent(bounds.x, bounds.y);
	mRenderTarget->SetTransform(D2D1::Matrix3x2F::Translation(bounds.x, bounds.y));

	mRenderTarget->BeginDraw();

	if (inBounds)
		ClipRect(inBounds);

	SetForeColor(kBlack);
	SetBackColor(kWhite);

	EraseRect(inBounds);
}

MWinDeviceImpl::~MWinDeviceImpl()
{
	if (mForeBrush != nil)
		mForeBrush->Release();

	if (mBackBrush != nil)
		mBackBrush->Release();

	while (not mState.empty())
	{
		mState.top()->Release();
		mState.pop();
	}

	if (mTextLayout != nil)
		mTextLayout->Release();

	if (mTextFormat != nil)
		mTextFormat->Release();

	if (mClipLayer != nil)
	{
		mRenderTarget->PopLayer();
		mClipLayer->Release();
	}

	if (mRenderTarget != nil)
	{
		HRESULT e = mRenderTarget->EndDraw();
		if (e == D2DERR_RECREATE_TARGET)
		{
			MWinWindowImpl* wi = dynamic_cast<MWinWindowImpl*>(
				MWinWindowImpl::Fetch(mRenderTarget->GetHwnd()));
			wi->SetRenderTarget(nil);
			throw 0;
		}
	}
}

void MWinDeviceImpl::Save()
{
	ID2D1DrawingStateBlock* state;
	THROW_IF_HRESULT_ERROR(sD2DFactory->CreateDrawingStateBlock(&state));
	mRenderTarget->SaveDrawingState(state);
	mState.push(state);
}

void MWinDeviceImpl::Restore()
{
	assert(not mState.empty());
	mRenderTarget->RestoreDrawingState(mState.top());
	mState.pop();
}

void MWinDeviceImpl::SetOrigin(
	int32		inX,
	int32		inY)
{
	mRenderTarget->SetTransform(D2D1::Matrix3x2F::Translation(inX, inY));
}

void MWinDeviceImpl::SetFont(
	const string&		inFont)
{
	string::const_iterator e = inFont.end();

	int size = 0, n = 1;
	while (e != inFont.begin() and isdigit(*--e))
	{
		--e;
		size += n * (*e - '0');
		n *= 10;
	}

	if (e == inFont.end() or e == inFont.begin() or *e != ' ')
		THROW(("Error in specified font"));

	--e;
	
	mFont = c2w(inFont.substr(0, e - inFont.begin()));
	mFontSize = mDpiScaleY * size * 96.f / 72.f;
}

void MWinDeviceImpl::SetForeColor(
	MColor				inColor)
{
	if (mForeBrush != nil)
		mForeBrush->Release();

	ID2D1SolidColorBrush* brush;
	THROW_IF_HRESULT_ERROR(
		mRenderTarget->CreateSolidColorBrush(D2D1::ColorF(inColor.red / 255.f, inColor.green / 255.f, inColor.blue / 255.f), &brush));

	mForeBrush = brush;
}

MColor MWinDeviceImpl::GetForeColor() const
{
	MColor result;

	ID2D1SolidColorBrush* brush = dynamic_cast<ID2D1SolidColorBrush*>(mForeBrush);

	if (brush != nil)
	{
		D2D1_COLOR_F color = brush->GetColor();
		result = MColor(color.r * 255, color.g * 255, color.b * 255 /*, color.a * 255*/);
	}

	return result;
}

void MWinDeviceImpl::SetBackColor(
	MColor				inColor)
{
	if (mBackBrush != nil)
		mBackBrush->Release();

	ID2D1SolidColorBrush* brush;
	THROW_IF_HRESULT_ERROR(
		mRenderTarget->CreateSolidColorBrush(D2D1::ColorF(inColor.red / 255.f, inColor.green / 255.f, inColor.blue / 255.f), &brush));

	mBackBrush = brush;
}

MColor MWinDeviceImpl::GetBackColor() const
{
	MColor result;

	ID2D1SolidColorBrush* brush = dynamic_cast<ID2D1SolidColorBrush*>(mBackBrush);

	if (brush != nil)
	{
		D2D1_COLOR_F color = brush->GetColor();
		result = MColor(color.r * 255, color.g * 255, color.b * 255 /*, color.a * 255*/);
	}

	return result;
}

void MWinDeviceImpl::ClipRect(
	MRect				inRect)
{
	if (mClipLayer != nil)
	{
		mRenderTarget->PopLayer();
		mClipLayer->Release();
	}

	THROW_IF_HRESULT_ERROR(mRenderTarget->CreateLayer(&mClipLayer));

    // Push the layer with the geometric mask.
	mRenderTarget->PushLayer(
		D2D1::LayerParameters(D2D1::RectF(inRect.x, inRect.y,
			inRect.x + inRect.width, inRect.y + inRect.height)),
		mClipLayer);
}

//void MWinDeviceImpl::ClipRegion(
//	MRegion				inRegion)
//{
//}

void MWinDeviceImpl::EraseRect(
	MRect				inRect)
{
	assert(mBackBrush);
	assert(mRenderTarget);

	mRenderTarget->FillRectangle(
		D2D1::RectF(inRect.x, inRect.y, inRect.x + inRect.width, inRect.y + inRect.height), mBackBrush);
}

void MWinDeviceImpl::FillRect(
	MRect				inRect)
{
	assert(mForeBrush);
	assert(mRenderTarget);

	mRenderTarget->FillRectangle(
		D2D1::RectF(inRect.x, inRect.y, inRect.x + inRect.width, inRect.y + inRect.height), mForeBrush);
}

void MWinDeviceImpl::StrokeRect(
	MRect				inRect,
	uint32				inLineWidth)
{
	assert(mForeBrush);
	assert(mRenderTarget);

	mRenderTarget->DrawRectangle(
		D2D1::RectF(inRect.x, inRect.y, inRect.x + inRect.width, inRect.y + inRect.height), mForeBrush);
}

void MWinDeviceImpl::FillEllipse(
	MRect				inRect)
{
	assert(mForeBrush);
	assert(mRenderTarget);

	float radius;
	if (inRect.height < inRect.width)
		radius = inRect.height / 2.f;
	else
		radius = inRect.width / 2.f;

	D2D1_ROUNDED_RECT r =
		D2D1::RoundedRect(D2D1::RectF(inRect.x, inRect.y, inRect.x + inRect.width, inRect.y + inRect.height),
		radius, radius);

	mRenderTarget->FillRoundedRectangle(r, mForeBrush);
}

//void MWinDeviceImpl::DrawImage(
//	cairo_surface_t*	inImage,
//	float				inX,
//	float				inY,
//	float				inShear)
//{	
//}

void MWinDeviceImpl::CreateAndUsePattern(
	MColor				inColor1,
	MColor				inColor2)
{
	uint32 data[8][8];

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
			data[y][x] = c1;
		for (uint32 x = 4; x < 8; ++x)
			data[y][x] = c2;
	}

	ID2D1BitmapBrush* brush;

	ID2D1Bitmap* bitmap;
	THROW_IF_HRESULT_ERROR(mRenderTarget->CreateBitmap(D2D1::SizeU(8, 8), data, 32,
		D2D1::BitmapProperties(
			D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE)
		), &bitmap));

	THROW_IF_HRESULT_ERROR(mRenderTarget->CreateBitmapBrush(
		bitmap,
		D2D1::BitmapBrushProperties(D2D1_EXTEND_MODE_WRAP, D2D1_EXTEND_MODE_WRAP),
		D2D1::BrushProperties(1.0f, D2D1::Matrix3x2F::Rotation(45.f)),
		&brush));

	bitmap->Release();

	if (mForeBrush != nil)
		mForeBrush->Release();

	mForeBrush = brush;
}

//PangoFontMetrics* MWinDeviceImpl::GetMetrics()
//{
//	if (mMetrics == nil)
//	{
//		PangoContext* context = pango_layout_get_context(mPangoLayout);
//		
//		PangoFontDescription* fontDesc = mFont;
//		if (fontDesc == nil)
//		{
//			fontDesc = pango_context_get_font_description(context);
//			
//			// there's a bug in pango I guess
//			
//			int32 x;
//			if (IsPrinting(x))
//				fontDesc = pango_font_description_copy(fontDesc);
//		}
//		
//		mMetrics = pango_context_get_metrics(context, fontDesc, nil);
//	}
//	
//	return mMetrics;
//}

IDWriteTextFormat* MWinDeviceImpl::GetTextFormat()
{
	if (mTextFormat == nil)
	{
		THROW_IF_HRESULT_ERROR(
			sDWFactory->CreateTextFormat(
				mFont.c_str(),                // Font family name.
				NULL,                       // Font collection (NULL sets it to use the system font collection).
				DWRITE_FONT_WEIGHT_REGULAR,
				DWRITE_FONT_STYLE_NORMAL,
				DWRITE_FONT_STRETCH_NORMAL,
				mFontSize,
				L"en-us",
				&mTextFormat
			));
	}
	
	return mTextFormat;
}

uint32 MWinDeviceImpl::GetAscent()
{

	uint32 result = 10;

	//PangoFontMetrics* metrics = GetMetrics();
	//if (metrics != nil)
	//	result = pango_font_metrics_get_ascent(metrics) / PANGO_SCALE;

	return result;
}

uint32 MWinDeviceImpl::GetDescent()
{
	uint32 result = 2;

	//PangoFontMetrics* metrics = GetMetrics();
	//if (metrics != nil)
	//	result = pango_font_metrics_get_descent(metrics) / PANGO_SCALE;

	return result;
}

//uint32 MWinDeviceImpl::GetLeading()
//{
//	return 0;
//}
//
//uint32 MWinDeviceImpl::GetLineHeight()
//{
//	uint32 result = 10;
//
//	PangoFontMetrics* metrics = GetMetrics();
//	if (metrics != nil)
//	{
//		uint32 ascent = pango_font_metrics_get_ascent(metrics);
//		uint32 descent = pango_font_metrics_get_descent(metrics);
//
//		result = (ascent + descent) / PANGO_SCALE;
//	}
//
//	return result;
//}

void MWinDeviceImpl::DrawString(
	const string&		inText,
	float				inX,
	float				inY,
	uint32				inTruncateWidth,
	MAlignment			inAlign)
{
	wstring s(c2w(inText));
	mRenderTarget->DrawTextW(s.c_str(), s.length(),
		GetTextFormat(),
		D2D1::RectF(inX, inY, 200, 14),
		mForeBrush);
}

uint32 MWinDeviceImpl::GetStringWidth(
	const string&		inText)
{
	//pango_layout_set_text(mPangoLayout, inText.c_str(), inText.length());
	//
	//PangoRectangle r;
	//pango_layout_get_pixel_extents(mPangoLayout, nil, &r);
	//
	//return r.width;

	return 0;
}

void MWinDeviceImpl::SetText(
	const string&		inText)
{
	if (mTextLayout != nil)
		mTextLayout->Release();

	wstring s(c2w(inText));

	THROW_IF_HRESULT_ERROR(
		sDWFactory->CreateTextLayout(
			s.c_str(),
			s.length(),
			GetTextFormat(),
			99999.0f,
			99999.0f,
			&mTextLayout
		));
}

void MWinDeviceImpl::SetTabStops(
	uint32				inTabWidth)
{
	if (mTextLayout == nil)
		THROW(("SetText must be called first!"));
	mTextLayout->SetIncrementalTabStop(inTabWidth * 96.f / 72.f);
}

void MWinDeviceImpl::SetTextColors(
	uint32				inColorCount,
	uint32				inColors[],
	uint32				inOffsets[])
{

	//PangoAttrList* attrs = pango_attr_list_new();

	//for (uint32 ix = 0; ix < inColorCount; ++ix)
	//{
	//	MColor c = gLanguageColors[inColors[ix]];
	//	
	//	uint16 red = c.red << 8 | c.red;
	//	uint16 green = c.green << 8 | c.green;
	//	uint16 blue = c.blue << 8 | c.blue;
	//	
	//	PangoAttribute* attr = pango_attr_foreground_new(red, green, blue);
	//	attr->start_index = inOffsets[ix];
	//	
	//	if (ix == inColorCount - 1)
	//		attr->end_index = -1;
	//	else
	//		attr->end_index = inOffsets[ix + 1];
	//	
	//	pango_attr_list_insert(attrs, attr);
	//}
	//
	//pango_layout_set_attributes(mPangoLayout, attrs);
	//
	//pango_attr_list_unref(attrs);
}

void MWinDeviceImpl::SetTextSelection(
	uint32				inStart,
	uint32				inLength,
	MColor				inSelectionColor)
{
	//uint16 red = inSelectionColor.red << 8 | inSelectionColor.red;
	//uint16 green = inSelectionColor.green << 8 | inSelectionColor.green;
	//uint16 blue = inSelectionColor.blue << 8 | inSelectionColor.blue;
	//
	//PangoAttribute* attr = pango_attr_background_new(red, green, blue);
	//attr->start_index = inStart;
	//attr->end_index = inStart + inLength;
	//
	//PangoAttrList* attrs = pango_layout_get_attributes(mPangoLayout);
	//
	//if (attrs == nil)
	//{
	//	attrs = pango_attr_list_new();
	//	pango_attr_list_insert(attrs, attr);
	//}
	//else
	//{
	//	attrs = pango_attr_list_copy(attrs);
	//	pango_attr_list_change(attrs, attr);
	//}
	//
	//pango_layout_set_attributes(mPangoLayout, attrs);
	//
	//pango_attr_list_unref(attrs);
}

void MWinDeviceImpl::IndexToPosition(
	uint32				inIndex,
	bool				inTrailing,
	int32&				outPosition)
{
	//PangoRectangle r;
	//pango_layout_index_to_pos(mPangoLayout, inIndex, &r);
	//outPosition = r.x / PANGO_SCALE;
}

bool MWinDeviceImpl::PositionToIndex(
	int32				inPosition,
	uint32&				outIndex)
{
	//int index, trailing;
	//
	//bool result = pango_layout_xy_to_index(mPangoLayout,
	//	inPosition * PANGO_SCALE, 0, &index, &trailing); 

	//MEncodingTraits<kEncodingUTF8> enc;
	//const char* text = pango_layout_get_text(mPangoLayout);	

	//while (trailing-- > 0)
	//{
	//	uint32 n = enc.GetNextCharLength(text); 
	//	text += n;
	//	index += n;
	//}
	//
	//outIndex = index;

	//return result;

	return false;
}

uint32 MWinDeviceImpl::GetTextWidth()
{
	//PangoRectangle r;
	//
	//pango_layout_get_pixel_extents(mPangoLayout, nil, &r);
	//
	//return r.width;

	return 0;
}

void MWinDeviceImpl::DrawText(
	float				inX,
	float				inY)
{
}

void MWinDeviceImpl::DrawCaret(
	float				inX,
	float				inY,
	uint32				inOffset)
{
}

void MWinDeviceImpl::BreakLines(
	uint32				inWidth,
	vector<uint32>&		outBreaks)
{
	//pango_layout_set_width(mPangoLayout, inWidth * PANGO_SCALE);
	//pango_layout_set_wrap(mPangoLayout, PANGO_WRAP_WORD_CHAR);

	//if (pango_layout_is_wrapped(mPangoLayout))
	//{
	//	uint32 line = 0;
	//	for (;;)
	//	{
	//		PangoLayoutLine* pangoLine = pango_layout_get_line_readonly(mPangoLayout, line);
	//		++line;
	//		
	//		if (pangoLine == nil)
	//			break;
	//		
	//		outBreaks.push_back(pangoLine->start_index + pangoLine->length);
	//	}
	//}
}

//// --------------------------------------------------------------------
//// MCairoDeviceImp is derived from MWinDeviceImpl
//// It provides the routines for drawing on a cairo surface
//
//class MCairoDeviceImp : public MWinDeviceImpl
//{
//  public:
//							MCairoDeviceImp(
//								MView*				inView,
//								MRect				inRect,
//								bool				inCreateOffscreen);
//
//							MCairoDeviceImp(
//								GtkPrintContext*	inContext,
//								MRect				inRect,
//								int32				inPage);
//
//							~MCairoDeviceImp();
//
//	virtual void			Save();
//	
//	virtual void			Restore();
//
//	virtual bool			IsPrinting(
//								int32&				outPage) const
//							{
//								outPage = mPage;
//								return outPage >= 0;
//							}
//
//	virtual MRect			GetBounds() const						{ return mRect; }
//
//	virtual void			SetOrigin(
//								int32				inX,
//								int32				inY);
//
//	virtual void			SetForeColor(
//								MColor				inColor);
//
//	virtual MColor			GetForeColor() const;
//
//	virtual void			SetBackColor(
//								MColor				inColor);
//
//	virtual MColor			GetBackColor() const;
//	
//	virtual void			ClipRect(
//								MRect				inRect);
//
//	virtual void			ClipRegion(
//								MRegion				inRegion);
//
//	virtual void			EraseRect(
//								MRect				inRect);
//
//	virtual void			FillRect(
//								MRect				inRect);
//
//	virtual void			StrokeRect(
//								MRect				inRect,
//								uint32				inLineWidth = 1);
//
//	virtual void			FillEllipse(
//								MRect				inRect);
//	
//	virtual void			DrawImage(
//								cairo_surface_t*	inImage,
//								float				inX,
//								float				inY,
//								float				inShear);
//	
//	virtual void			CreateAndUsePattern(
//								MColor				inColor1,
//								MColor				inColor2);
//	
//	virtual void			DrawString(
//								const string&		inText,
//								float				inX,
//								float				inY,
//								uint32				inTruncateWidth,
//								MAlignment			inAlign);
//
//	virtual void			DrawText(
//								float				inX,
//								float				inY);
//
//	virtual void			DrawCaret(
//								float				inX,
//								float				inY,
//								uint32				inOffset);
//
//	virtual void			MakeTransparent(
//								float				inOpacity);
//
//	virtual GdkPixmap*		GetPixmap() const;
//
//	virtual void			SetDrawWhiteSpace(
//								bool				inDrawWhiteSpace);
//
//  protected:
//
//	void					DrawWhiteSpace(
//								float				inX,
//								float				inY);
//
//	MRect					mRect;
//	MColor					mForeColor;
//	MColor					mBackColor;
//	MColor					mEvenRowColor;
//	cairo_t*				mContext;
//	GdkPixmap*				mOffscreenPixmap;
//	uint32					mPatternData[8][8];
//	int32					mPage;
//	bool					mDrawWhiteSpace;
//};
//
//MCairoDeviceImp::MCairoDeviceImp(
//	MView*		inView,
//	MRect		inRect,
//	bool		inCreateOffscreen)
//	: mRect(inRect)
//	, mOffscreenPixmap(nil)
//	, mPage(-1)
//	, mDrawWhiteSpace(false)
//{
//	mForeColor = kBlack;
//	mBackColor = kWhite;
//
//	if (inCreateOffscreen)
//	{
//		GdkScreen* screen = gtk_widget_get_screen(inView->GetGtkWidget());
//		GdkColormap* colormap = nil;
//		
//		if (gdk_screen_is_composited(screen))
//			colormap = gdk_screen_get_rgba_colormap(screen);
//		else
//			colormap = gtk_widget_get_colormap(inView->GetGtkWidget());
//
//		mOffscreenPixmap = gdk_pixmap_new(nil, inRect.width, inRect.height,
//			gdk_colormap_get_visual(colormap)->depth);
//		gdk_drawable_set_colormap(mOffscreenPixmap, colormap);
//
//		mContext = gdk_cairo_create(mOffscreenPixmap);
//	}
//	else
//		mContext = gdk_cairo_create(inView->GetGtkWidget()->window);
//
//	cairo_rectangle(mContext, mRect.x, mRect.y, mRect.width, mRect.height);
//	cairo_clip(mContext);
//}
//
//MCairoDeviceImp::MCairoDeviceImp(
//	GtkPrintContext*	inPrintContext,
//	MRect				inRect,
//	int32				inPage)
//	: MWinDeviceImpl(gtk_print_context_create_pango_layout(inPrintContext))
//	, mRect(inRect)
//	, mOffscreenPixmap(nil)
//	, mPage(inPage)
//	, mDrawWhiteSpace(false)
//{
//	mForeColor = kBlack;
//	mBackColor = kWhite;
//
//	mContext = gtk_print_context_get_cairo_context(inPrintContext);
//
//	cairo_rectangle(mContext, mRect.x, mRect.y, mRect.width, mRect.height);
//	cairo_clip(mContext);
//}
//
//MCairoDeviceImp::~MCairoDeviceImp()
//{
//	if (mPage == -1)
//		cairo_destroy(mContext);
//	
//	if (mOffscreenPixmap)
//		g_object_unref(mOffscreenPixmap);
//}
//
//void MCairoDeviceImp::Save()
//{
//	cairo_save(mContext);
//}
//
//void MCairoDeviceImp::Restore()
//{
//	cairo_restore(mContext);
//}
//
//void MCairoDeviceImp::SetOrigin(
//	int32				inX,
//	int32				inY)
//{
//	cairo_translate(mContext, inX, inY);
//}
//
//void MCairoDeviceImp::SetForeColor(
//	MColor				inColor)
//{
//	mForeColor = inColor;
//
//	cairo_set_source_rgb(mContext,
//		mForeColor.red / 255.0,
//		mForeColor.green / 255.0,
//		mForeColor.blue / 255.0);
//}
//
//MColor MCairoDeviceImp::GetForeColor() const
//{
//	return mForeColor;
//}
//
//void MCairoDeviceImp::SetBackColor(
//	MColor				inColor)
//{
//	mBackColor = inColor;
//}
//
//MColor MCairoDeviceImp::GetBackColor() const
//{
//	return mBackColor;
//}
//
//void MCairoDeviceImp::ClipRect(
//	MRect				inRect)
//{
//	cairo_rectangle(mContext, inRect.x, inRect.y, inRect.width, inRect.height);
//	cairo_clip(mContext);
//}
//
//void MCairoDeviceImp::ClipRegion(
//	MRegion				inRegion)
//{
//	GdkRegion* gdkRegion = const_cast<GdkRegion*>(inRegion.operator const GdkRegion*());
//	gdk_cairo_region(mContext, gdkRegion);
//	cairo_clip(mContext);
//}
//
//void MCairoDeviceImp::EraseRect(
//	MRect				inRect)
//{
//	cairo_save(mContext);
//	
//	cairo_rectangle(mContext, inRect.x, inRect.y, inRect.width, inRect.height);
//
//	cairo_set_source_rgb(mContext,
//		mBackColor.red / 255.0,
//		mBackColor.green / 255.0,
//		mBackColor.blue / 255.0);
//
//	if (mOffscreenPixmap != nil)
//		cairo_set_operator(mContext, CAIRO_OPERATOR_CLEAR);
//
//	cairo_fill(mContext);
//	
//	cairo_restore(mContext);
//}
//
//void MCairoDeviceImp::FillRect(
//	MRect				inRect)
//{
//	cairo_rectangle(mContext, inRect.x, inRect.y, inRect.width, inRect.height);
//	cairo_fill(mContext);
//}
//
//void MCairoDeviceImp::StrokeRect(
//	MRect				inRect,
//	uint32				inLineWidth)
//{
//	cairo_rectangle(mContext, inRect.x, inRect.y, inRect.width, inRect.height);
//	cairo_stroke(mContext);
//}
//
//void MCairoDeviceImp::FillEllipse(
//	MRect				inRect)
//{
//	cairo_save(mContext);
//	cairo_translate(mContext, inRect.x + inRect.width / 2., inRect.y + inRect.height / 2.);
//	cairo_scale(mContext, inRect.width / 2., inRect.height / 2.);
//	cairo_arc(mContext, 0., 0., 1., 0., 2 * M_PI);
//	cairo_fill(mContext);
//	cairo_restore(mContext);
//}
//
//void MCairoDeviceImp::DrawImage(
//	cairo_surface_t*	inImage,
//	float				inX,
//	float				inY,
//	float				inShear)
//{
//	cairo_save(mContext);
//
////	cairo_set_source_surface(mContext, inImage, inX, inY);
////
////	int w = cairo_image_surface_get_width(inImage);
////	int h = cairo_image_surface_get_height(inImage);
////
////	cairo_rectangle(mContext, inX, inY, w, h);
////	cairo_fill(mContext);
//
//	cairo_surface_set_device_offset(inImage, -inX, -inY);
//
//	cairo_pattern_t* p = cairo_pattern_create_for_surface(inImage);
//	
//	if (p != nil)
//	{
//		cairo_matrix_t m;
//		cairo_matrix_init_translate(&m, -inX, -inY);
////		cairo_matrix_init_rotate(&m, 2.356);
//		cairo_matrix_init(&m, 1, inShear, inShear, 1, 0, 0);
//		cairo_pattern_set_matrix(p, &m);
//		
//		cairo_set_source(mContext, p);
//		
//		cairo_pattern_destroy(p);
//		
//		int w = cairo_image_surface_get_width(inImage);
//		int h = cairo_image_surface_get_height(inImage);
//		
//		cairo_rectangle(mContext, inX, inY, w, h);
//		cairo_fill(mContext);
//	}
//
//	cairo_restore(mContext);
//}
//
//void MCairoDeviceImp::CreateAndUsePattern(
//	MColor				inColor1,
//	MColor				inColor2)
//{
//	uint32 c1 = 0, c2 = 0;
//	
//	assert(cairo_format_stride_for_width(CAIRO_FORMAT_RGB24, 8) == 32);
//	
//	c1 |= inColor1.red << 16;
//	c1 |= inColor1.green << 8;
//	c1 |= inColor1.blue << 0;
//	
//	c2 |= inColor2.red << 16;
//	c2 |= inColor2.green << 8;
//	c2 |= inColor2.blue << 0;
//	
//	for (uint32 y = 0; y < 8; ++y)
//	{
//		for (uint32 x = 0; x < 4; ++x)
//			mPatternData[y][x] = c1;
//		for (uint32 x = 4; x < 8; ++x)
//			mPatternData[y][x] = c2;
//	}
//	
//	cairo_surface_t* s = cairo_image_surface_create_for_data(
//		reinterpret_cast<uint8*>(mPatternData), CAIRO_FORMAT_RGB24, 8, 8, 32);
//
//	if (s != nil)
//	{
//		cairo_pattern_t* p = cairo_pattern_create_for_surface(s);
//		cairo_pattern_set_extend(p, CAIRO_EXTEND_REPEAT);
//		
//		if (p != nil)
//		{
//			cairo_matrix_t m;
//			cairo_matrix_init_rotate(&m, 2.356);
//			cairo_pattern_set_matrix(p, &m);
//			
//			cairo_set_source(mContext, p);
//			
//			cairo_pattern_destroy(p);
//		}
//		
//		cairo_surface_destroy(s);
//	}
//}
//
//void MCairoDeviceImp::DrawString(
//	const string&		inText,
//	float				inX,
//	float				inY,
//	uint32				inTruncateWidth,
//	MAlignment			inAlign)
//{
//	pango_layout_set_text(mPangoLayout, inText.c_str(), inText.length());
//	
//	if (inTruncateWidth != 0)
//	{
//		pango_layout_set_ellipsize(mPangoLayout, PANGO_ELLIPSIZE_END);
//		pango_layout_set_width(mPangoLayout, inTruncateWidth * PANGO_SCALE);
//	
//		if (inAlign != eAlignNone and inAlign != eAlignLeft)
//		{
//			PangoRectangle r;
//			pango_layout_get_pixel_extents(mPangoLayout, nil, &r);
//		
//			if (static_cast<uint32>(r.width) < inTruncateWidth)
//			{
//				if (inAlign == eAlignCenter)
//					inX += (inTruncateWidth - r.width) / 2;
//				else
//					inX += inTruncateWidth - r.width;
//			}
//		}
//	}
//	else
//	{
//		pango_layout_set_ellipsize(mPangoLayout, PANGO_ELLIPSIZE_NONE);
//		pango_layout_set_width(mPangoLayout, mRect.width * PANGO_SCALE);
//	}
//
//	cairo_move_to(mContext, inX, inY);	
//
//	pango_cairo_show_layout(mContext, mPangoLayout);
//}
//
//void MCairoDeviceImp::DrawWhiteSpace(
//	float				inX,
//	float				inY)
//{
//#if PANGO_VERSION_CHECK(1, 22, 0)
//	int baseLine = pango_layout_get_baseline(mPangoLayout);
//	PangoLayoutLine* line = pango_layout_get_line(mPangoLayout, 0);
//	
//	cairo_set_source_rgb(mContext,
//		gWhiteSpaceColor.red / 255.0,
//		gWhiteSpaceColor.green / 255.0,
//		gWhiteSpaceColor.blue / 255.0);
//
//	// we're using one font anyway
//	PangoFontMap* fontMap = pango_cairo_font_map_get_default();
//	PangoContext* context = pango_layout_get_context(mPangoLayout);
//	PangoFont* font = pango_font_map_load_font(fontMap, context, mFont);
//	cairo_scaled_font_t* scaledFont =
//		pango_cairo_font_get_scaled_font(reinterpret_cast<PangoCairoFont*>(font));
//
//	if (scaledFont == nil or cairo_scaled_font_status(scaledFont) != CAIRO_STATUS_SUCCESS)
//		return;
//
//	cairo_set_scaled_font(mContext, scaledFont);
//	
//	int x_position = 0;
//	vector<cairo_glyph_t> cairo_glyphs;
//
//	for (GSList* run = line->runs; run != nil; run = run->next)
//	{
//		PangoGlyphItem* glyphItem = reinterpret_cast<PangoGlyphItem*>(run->data);
//		
//		PangoGlyphItemIter iter;
//		const char* text = pango_layout_get_text(mPangoLayout);
//		
//		for (bool more = pango_glyph_item_iter_init_start(&iter, glyphItem, text);
//				  more;
//				  more = pango_glyph_item_iter_next_cluster(&iter))
//		{
//			PangoGlyphString* gs = iter.glyph_item->glyphs;
//			char ch = text[iter.start_index];
//	
//			for (int i = iter.start_glyph; i < iter.end_glyph; ++i)
//			{
//				PangoGlyphInfo* gi = &gs->glyphs[i];
//				
//				if (ch == ' ' or ch == '\t')
//				{
//					double cx = inX + double(x_position + gi->geometry.x_offset) / PANGO_SCALE;
//					double cy = inY + double(baseLine + gi->geometry.y_offset) / PANGO_SCALE;
//					
//					cairo_glyph_t g;
//					if (ch == ' ')
//						g.index = mSpaceGlyph;
//					else
//						g.index = mTabGlyph;
//					g.x = cx;
//					g.y = cy;
//		
//					cairo_glyphs.push_back(g);
//				}
//				
//				x_position += gi->geometry.width;
//			}
//		}
//	}
//	
//	// and a trailing newline perhaps?
//	
//	if (mTextEndsWithNewLine)
//	{
//		double cx = inX + double(x_position) / PANGO_SCALE;
//		double cy = inY + double(baseLine) / PANGO_SCALE;
//		
//		cairo_glyph_t g;
//		g.index = mNewLineGlyph;
//		g.x = cx;
//		g.y = cy;
//
//		cairo_glyphs.push_back(g);
//	}
//	
//	cairo_show_glyphs(mContext, &cairo_glyphs[0], cairo_glyphs.size());	
//#endif
//}
//
//void MCairoDeviceImp::DrawText(
//	float				inX,
//	float				inY)
//{
//	if (mDrawWhiteSpace)
//	{
//		Save();
//		DrawWhiteSpace(inX, inY);
//		Restore();
//	}
//
//	cairo_move_to(mContext, inX, inY);
//	pango_cairo_show_layout(mContext, mPangoLayout);
//}
//
//void MCairoDeviceImp::DrawCaret(
//	float				inX,
//	float				inY,
//	uint32				inOffset)
//{
//	PangoRectangle sp, wp;
//
//	pango_layout_get_cursor_pos(mPangoLayout, inOffset, &sp, &wp);
//	
//	Save();
//	
//	cairo_set_line_width(mContext, 1.0);
//	cairo_set_source_rgb(mContext, 0, 0, 0);
//	cairo_move_to(mContext, inX + sp.x / PANGO_SCALE, inY + sp.y / PANGO_SCALE);
//	cairo_rel_line_to(mContext, sp.width / PANGO_SCALE, sp.height / PANGO_SCALE);
//	cairo_stroke(mContext);
//	
//	Restore();
//}
//
//void MCairoDeviceImp::MakeTransparent(
//	float				inOpacity)
//{
//	cairo_set_operator(mContext, CAIRO_OPERATOR_DEST_OUT);
//	cairo_set_source_rgba(mContext, 1, 0, 0, inOpacity);
//	cairo_paint(mContext);
//}
//
//GdkPixmap* MCairoDeviceImp::GetPixmap() const
//{
//	g_object_ref(mOffscreenPixmap);
//	return mOffscreenPixmap;
//}
//
//void MCairoDeviceImp::SetDrawWhiteSpace(
//	bool				inDrawWhiteSpace)
//{
//	mDrawWhiteSpace = inDrawWhiteSpace;
//}


MDeviceImp* MDeviceImp::Create(
	MView*				inView,
	MRect				inRect,
	bool				inCreateOffscreen)
{
	return new MWinDeviceImpl(inView, inRect, inCreateOffscreen);
}
