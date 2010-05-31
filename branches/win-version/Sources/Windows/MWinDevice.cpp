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

namespace
{

class DECLSPEC_UUID("e91eb6be-6cb7-11df-b6a6-001b21124f0d") MColouredText : public IUnknown
{
public:
						MColouredText();
						MColouredText(const D2D1_COLOR_F& inColour);
						~MColouredText();

    STDMETHOD(GetColour)(D2D1_COLOR_F* inColour);
    STDMETHOD(SetColour)(const D2D1_COLOR_F& outColour);

public:
    unsigned long STDMETHODCALLTYPE AddRef();
    unsigned long STDMETHODCALLTYPE Release();

	HRESULT STDMETHODCALLTYPE QueryInterface(
        IID const& riid,
        void** ppvObject
    );

private:
    unsigned long	mRefCount;
    D2D1_COLOR_F	mColour;
};

MColouredText::MColouredText()
	: mRefCount(0)
{
}

MColouredText::MColouredText(
		const D2D1_COLOR_F& inColour)
	: mRefCount(0)
	, mColour(inColour)
{
}

MColouredText::~MColouredText()
{
}

STDMETHODIMP MColouredText::GetColour(
	D2D1_COLOR_F*		outColour)
{
	*outColour = mColour;
	return S_OK;
}

STDMETHODIMP MColouredText::SetColour(
	const D2D1_COLOR_F&	inColour)
{
	mColour = inColour;
	return S_OK;
}

STDMETHODIMP_(unsigned long) MColouredText::AddRef()
{
	return InterlockedIncrement(&mRefCount);
}

STDMETHODIMP_(unsigned long) MColouredText::Release()
{
	unsigned long newCount = InterlockedDecrement(&mRefCount);
    
    if (newCount == 0)
    {
        delete this;
        return 0;
    }

    return newCount;
}

STDMETHODIMP_(HRESULT) MColouredText::QueryInterface(
	IID const&	riid,
	void**		ppvObject)
{
    if (__uuidof(MColouredText) == riid)
    {
        *ppvObject = this;
    }
    else if (__uuidof(IUnknown) == riid)
    {
        *ppvObject = this;
    }
    else
    {
        *ppvObject = NULL;
        return E_FAIL;
    }

    AddRef();

    return S_OK;
}

}

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
	
	virtual uint32			GetAscent();
	
	virtual uint32			GetDescent();
	
	virtual void			DrawString(
								const string&		inText,
								float				inX,
								float				inY,
								uint32				inTruncateWidth = 0,
								MAlignment			inAlign = eAlignNone);

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

	IDWriteTextFormat*		GetTextFormat();

	MView*					mView;
	HDC						mDC;
	//HBITMAP					mOffscreenBitmap;
	int						mDCState;
	//HDC						mWindowDC;
	HPEN					mForePen;
	//bool					mPrinting;

	static ID2D1Factory*	sD2DFactory;
	static IDWriteFactory*	sDWFactory;

	//ID2D1HwndRenderTarget*	mRenderTarget;
	ID2D1DCRenderTarget*	mRenderTarget;
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
	MRect		inUpdate,
	bool		inOffscreen)
	: mView(inView)
	, mRenderTarget(nil)
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

	D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties(
		D2D1_RENDER_TARGET_TYPE_DEFAULT,
		D2D1::PixelFormat(
			DXGI_FORMAT_B8G8R8A8_UNORM,
			D2D1_ALPHA_MODE_IGNORE),
		0, 0, D2D1_RENDER_TARGET_USAGE_NONE, D2D1_FEATURE_LEVEL_DEFAULT);

	THROW_IF_HRESULT_ERROR(sD2DFactory->CreateDCRenderTarget(&props, &mRenderTarget));

	MRect bounds;
	inView->GetBounds(bounds);
	inView->ConvertToWindow(bounds.x, bounds.y);

	HDC dc = ::GetDC(windowImpl->GetHandle());
	RECT r = { bounds.x, bounds.y, bounds.x + bounds.width, bounds.y + bounds.height };
	mRenderTarget->BindDC(dc, &r);

	mRenderTarget->BeginDraw();

	int32 x = 0, y = 0;
	inView->ConvertToWindow(x, y);

	mRenderTarget->SetTransform(
		D2D1::Matrix3x2F::Translation(x - bounds.x, y - bounds.y));

	if (inUpdate)
		ClipRect(inUpdate);

	SetForeColor(kBlack);
	SetBackColor(kWhite);
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
		//if (e == D2DERR_RECREATE_TARGET)
		//{
		//	MWinWindowImpl* wi = dynamic_cast<MWinWindowImpl*>(
		//		MWinWindowImpl::Fetch(mRenderTarget->GetHwnd()));
		//	wi->SetRenderTarget(nil);
		//	throw 0;
		//}
		mRenderTarget->Release();
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

MDeviceImp* MDeviceImp::Create(
	MView*				inView,
	MRect				inRect,
	bool				inCreateOffscreen)
{
	return new MWinDeviceImpl(inView, inRect, inCreateOffscreen);
}
