//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "MWinLib.h"
#include <comdef.h>

#include <boost/foreach.hpp>
#define foreach BOOST_FOREACH

#include <cmath>
#include <cstring>
#include <stack>

#include "MWinDeviceImpl.h"
#include "MView.h"
#include "MWinCanvasImpl.h"
#include "MError.h"
#include "MUnicode.h"
#include "MError.h"
#include "MWinUtils.h"
#include "MPreferences.h"

using namespace std;

// Add our own conversion routine to namespace D2D1
namespace D2D1
{

D2D1FORCEINLINE D2D1_RECT_F RectF(const MRect& r)
{
    return Rect<FLOAT>(
    	static_cast<float>(r.x),
    	static_cast<float>(r.y),
    	static_cast<float>(r.x + r.width),
    	static_cast<float>(r.y + r.height));
}

}

namespace
{

class DECLSPEC_UUID("e91eb6be-6cb7-11df-b6a6-001b21124f0d") MTextColor : public IUnknown
{
public:
						MTextColor();
						MTextColor(const MColor& inColor);
						~MTextColor();

    STDMETHOD(GetColor)(D2D1_COLOR_F* inColor);
    STDMETHOD(SetColor)(const D2D1_COLOR_F& outColor);

public:
    unsigned long STDMETHODCALLTYPE AddRef();
    unsigned long STDMETHODCALLTYPE Release();

	HRESULT STDMETHODCALLTYPE QueryInterface(
        IID const& riid,
        void** ppvObject
    );

private:
    unsigned long	mRefCount;
    D2D1_COLOR_F	mColor;
};

MTextColor::MTextColor()
	: mRefCount(1)
{
}

MTextColor::MTextColor(
		const MColor& inColor)
	: mRefCount(1)
{
	mColor = D2D1::ColorF(inColor.red / 255.f, inColor.green / 255.f, inColor.blue / 255.f);
}

MTextColor::~MTextColor()
{
}

STDMETHODIMP MTextColor::GetColor(
	D2D1_COLOR_F*		outColor)
{
	*outColor = mColor;
	return S_OK;
}

STDMETHODIMP MTextColor::SetColor(
	const D2D1_COLOR_F&	inColor)
{
	mColor = inColor;
	return S_OK;
}

STDMETHODIMP_(unsigned long) MTextColor::AddRef()
{
	return InterlockedIncrement(&mRefCount);
}

STDMETHODIMP_(unsigned long) MTextColor::Release()
{
	unsigned long newCount = InterlockedDecrement(&mRefCount);
    
    if (newCount == 0)
    {
        delete this;
        return 0;
    }

    return newCount;
}

STDMETHODIMP_(HRESULT) MTextColor::QueryInterface(
	IID const&	riid,
	void**		ppvObject)
{
    if (__uuidof(MTextColor) == riid)
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


class MTextRenderer : public IDWriteTextRenderer
{
public:
    MTextRenderer(
        ID2D1Factory* pD2DFactory, 
        ID2D1RenderTarget* pRT
        );

    ~MTextRenderer();

    STDMETHOD(IsPixelSnappingDisabled)(
        __maybenull void* clientDrawingContext,
        __out BOOL* isDisabled
        );

    STDMETHOD(GetCurrentTransform)(
        __maybenull void* clientDrawingContext,
        __out DWRITE_MATRIX* transform
        );

    STDMETHOD(GetPixelsPerDip)(
        __maybenull void* clientDrawingContext,
        __out FLOAT* pixelsPerDip
        );

    STDMETHOD(DrawGlyphRun)(
        __maybenull void* clientDrawingContext,
        FLOAT baselineOriginX,
        FLOAT baselineOriginY,
        DWRITE_MEASURING_MODE measuringMode,
        __in DWRITE_GLYPH_RUN const* glyphRun,
        __in DWRITE_GLYPH_RUN_DESCRIPTION const* glyphRunDescription,
        IUnknown* clientDrawingEffect
        );

    STDMETHOD(DrawUnderline)(
        __maybenull void* clientDrawingContext,
        FLOAT baselineOriginX,
        FLOAT baselineOriginY,
        __in DWRITE_UNDERLINE const* underline,
        IUnknown* clientDrawingEffect
        );

    STDMETHOD(DrawStrikethrough)(
        __maybenull void* clientDrawingContext,
        FLOAT baselineOriginX,
        FLOAT baselineOriginY,
        __in DWRITE_STRIKETHROUGH const* strikethrough,
        IUnknown* clientDrawingEffect
        );

    STDMETHOD(DrawInlineObject)(
        __maybenull void* clientDrawingContext,
        FLOAT originX,
        FLOAT originY,
        IDWriteInlineObject* inlineObject,
        BOOL isSideways,
        BOOL isRightToLeft,
        IUnknown* clientDrawingEffect
        );

public:
    unsigned long STDMETHODCALLTYPE AddRef();
    unsigned long STDMETHODCALLTYPE Release();
    HRESULT STDMETHODCALLTYPE QueryInterface(
        IID const& riid,
        void** ppvObject
    );

private:
    uint32					mRefCount;
    ID2D1Factory*			mD2DFactory;
    ID2D1RenderTarget*		mRenderTarget;
};

MTextRenderer::MTextRenderer(
    ID2D1Factory*		inD2DFactory, 
    ID2D1RenderTarget*	inRenderTarget)
	: mRefCount(0)
	, mD2DFactory(inD2DFactory)
	, mRenderTarget(inRenderTarget)
{
    mD2DFactory->AddRef();
    mRenderTarget->AddRef();
}

MTextRenderer::~MTextRenderer()
{
	if (mD2DFactory != nil)
		mD2DFactory->Release();

	if (mRenderTarget != nil)
		mRenderTarget->Release();
}

STDMETHODIMP MTextRenderer::DrawGlyphRun(
    void*					clientDrawingContext,
    FLOAT					baselineOriginX,
    FLOAT					baselineOriginY,
    DWRITE_MEASURING_MODE	measuringMode,
    DWRITE_GLYPH_RUN const* glyphRun,
    DWRITE_GLYPH_RUN_DESCRIPTION const*
							glyphRunDescription,
    IUnknown*				clientDrawingEffect)
{
	ID2D1SolidColorBrush* pBrush = NULL;

	D2D1::ColorF color = D2D1::ColorF::Black;

	MTextColor* textColor = nil;
	if (clientDrawingEffect != nil)
		clientDrawingEffect->QueryInterface(&textColor);

	if (textColor != nil)
	{
		textColor->GetColor(&color);
		textColor->Release();
	}

    HRESULT hr = mRenderTarget->CreateSolidColorBrush(color, &pBrush);
	mRenderTarget->DrawGlyphRun(D2D1::Point2(baselineOriginX, baselineOriginY),
        glyphRun,
        pBrush,
        measuringMode);
	pBrush->Release();

	return S_OK;
}

STDMETHODIMP MTextRenderer::DrawUnderline(
    __maybenull void* clientDrawingContext,
    FLOAT baselineOriginX,
    FLOAT baselineOriginY,
    __in DWRITE_UNDERLINE const* underline,
    IUnknown* clientDrawingEffect
    )
{
    //HRESULT hr;

    //D2D1_RECT_F rect = D2D1::RectF(
    //    0,
    //    underline->offset,
    //    underline->width,
    //    underline->offset + underline->thickness
    //    );

    //ID2D1RectangleGeometry* pRectangleGeometry = NULL;
    //hr = mD2DFactory->CreateRectangleGeometry(
    //        &rect, 
    //        &pRectangleGeometry
    //        );

    //// Initialize a matrix to translate the origin of the underline
    //D2D1::Matrix3x2F const matrix = D2D1::Matrix3x2F(
    //    1.0f, 0.0f,
    //    0.0f, 1.0f,
    //    baselineOriginX, baselineOriginY
    //    );

    //ID2D1TransformedGeometry* pTransformedGeometry = NULL;
    //if (SUCCEEDED(hr))
    //{
    //    hr = mD2DFactory->CreateTransformedGeometry(
    //        pRectangleGeometry,
    //        &matrix,
    //        &pTransformedGeometry
    //        );
    //}

    //ID2D1SolidColorBrush* pBrush = NULL;

    //// If there is a drawing effect create a color brush using it, otherwise create a black brush.
    //if (clientDrawingEffect != NULL)
    //{
    //    // Go from IUnknown to ColorDrawingEffect.
    //    ColorDrawingEffect *colorDrawingEffect;

    //    clientDrawingEffect->QueryInterface(__uuidof(ColorDrawingEffect), reinterpret_cast<void**>(&colorDrawingEffect));

    //    // Get the color from the ColorDrawingEffect object.
    //    D2D1_COLOR_F color;

    //    colorDrawingEffect->GetColor(&color);

    //    // Create the brush using the color pecified by our ColorDrawingEffect object.
    //    if (SUCCEEDED(hr))
    //    {
    //        hr = mRenderTarget->CreateSolidColorBrush(
    //            color,
    //            &pBrush);
    //    }

    //    SafeRelease(&colorDrawingEffect);
    //}
    //else
    //{
    //    // Create a black brush.
    //    if (SUCCEEDED(hr))
    //    {
    //        hr = mRenderTarget->CreateSolidColorBrush(
    //            D2D1::ColorF(
    //            D2D1::ColorF::Black
    //            ),
    //            &pBrush);
    //    }
    //}

    //// Draw the outline of the rectangle
    //mRenderTarget->DrawGeometry(
    //    pTransformedGeometry,
    //    pBrush
    //    );

    //// Fill in the rectangle
    //mRenderTarget->FillGeometry(
    //    pTransformedGeometry,
    //    pBrush
    //    );

    //SafeRelease(&pRectangleGeometry);
    //SafeRelease(&pTransformedGeometry);
    //SafeRelease(&pBrush);

    return S_OK;
}

STDMETHODIMP MTextRenderer::DrawStrikethrough(
    __maybenull void* clientDrawingContext,
    FLOAT baselineOriginX,
    FLOAT baselineOriginY,
    __in DWRITE_STRIKETHROUGH const* strikethrough,
    IUnknown* clientDrawingEffect
    )
{
//    HRESULT hr;
//
//    D2D1_RECT_F rect = D2D1::RectF(
//        0,
//        strikethrough->offset,
//        strikethrough->width,
//        strikethrough->offset + strikethrough->thickness
//        );
//
//    ID2D1RectangleGeometry* pRectangleGeometry = NULL;
//    hr = mD2DFactory->CreateRectangleGeometry(
//            &rect, 
//            &pRectangleGeometry
//            );
//
//    // Initialize a matrix to translate the origin of the strikethrough
//    D2D1::Matrix3x2F const matrix = D2D1::Matrix3x2F(
//        1.0f, 0.0f,
//        0.0f, 1.0f,
//        baselineOriginX, baselineOriginY
//        );
//
//    ID2D1TransformedGeometry* pTransformedGeometry = NULL;
//    if (SUCCEEDED(hr))
//    {
//        hr = mD2DFactory->CreateTransformedGeometry(
//            pRectangleGeometry,
//            &matrix,
//            &pTransformedGeometry
//            );
//    }
//
//ID2D1SolidColorBrush* pBrush = NULL;
//
//    // If there is a drawing effect create a color brush using it, otherwise create a black brush.
//    if (clientDrawingEffect != NULL)
//    {
//        // Go from IUnknown to ColorDrawingEffect.
//        ColorDrawingEffect *colorDrawingEffect;
//
//        clientDrawingEffect->QueryInterface(__uuidof(ColorDrawingEffect), reinterpret_cast<void**>(&colorDrawingEffect));
//
//        // Get the color from the ColorDrawingEffect object.
//        D2D1_COLOR_F color;
//
//        colorDrawingEffect->GetColor(&color);
//
//        // Create the brush using the color pecified by our ColorDrawingEffect object.
//        if (SUCCEEDED(hr))
//        {
//            hr = mRenderTarget->CreateSolidColorBrush(
//                color,
//                &pBrush);
//        }
//
//        SafeRelease(&colorDrawingEffect);
//    }
//    else
//    {
//        // Create a black brush.
//        if (SUCCEEDED(hr))
//        {
//            hr = mRenderTarget->CreateSolidColorBrush(
//                D2D1::ColorF(
//                D2D1::ColorF::Black
//                ),
//                &pBrush);
//        }
//    }
//
//    // Draw the outline of the rectangle
//    mRenderTarget->DrawGeometry(
//        pTransformedGeometry,
//        pBrush
//        );
//
//    // Fill in the rectangle
//    mRenderTarget->FillGeometry(
//        pTransformedGeometry,
//        pBrush
//        );
//
//    SafeRelease(&pRectangleGeometry);
//    SafeRelease(&pTransformedGeometry);
//    SafeRelease(&pBrush);

    return S_OK;
}

STDMETHODIMP MTextRenderer::DrawInlineObject(
    __maybenull void* clientDrawingContext,
    FLOAT originX,
    FLOAT originY,
    IDWriteInlineObject* inlineObject,
    BOOL isSideways,
    BOOL isRightToLeft,
    IUnknown* clientDrawingEffect
    )
{
    // Not implemented
    return E_NOTIMPL;
}

STDMETHODIMP_(unsigned long) MTextRenderer::AddRef()
{
    return InterlockedIncrement(&mRefCount);
}

STDMETHODIMP_(unsigned long) MTextRenderer::Release()
{
    unsigned long newCount = InterlockedDecrement(&mRefCount);

    if (newCount == 0)
    {
        delete this;
        return 0;
    }

    return newCount;
}

STDMETHODIMP MTextRenderer::IsPixelSnappingDisabled(
    __maybenull void* clientDrawingContext,
    __out BOOL* isDisabled
    )
{
    *isDisabled = FALSE;
    return S_OK;
}

STDMETHODIMP MTextRenderer::GetCurrentTransform(
    __maybenull void* clientDrawingContext,
    __out DWRITE_MATRIX* transform
    )
{
    //forward the render target's transform
    mRenderTarget->GetTransform(reinterpret_cast<D2D1_MATRIX_3X2_F*>(transform));
    return S_OK;
}

STDMETHODIMP MTextRenderer::GetPixelsPerDip(
    __maybenull void* clientDrawingContext,
    __out FLOAT* pixelsPerDip
    )
{
	*pixelsPerDip = 1;
    return S_OK;
}

STDMETHODIMP MTextRenderer::QueryInterface(
    IID const& riid,
    void** ppvObject
    )
{
    if (__uuidof(IDWriteTextRenderer) == riid)
    {
        *ppvObject = this;
    }
    else if (__uuidof(IDWritePixelSnapping) == riid)
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

ID2D1Factory*	MWinDeviceImpl::GetD2D1Factory()
{
	static ID2D1Factory* sD2DFactory = nil;

	if (sD2DFactory == nil)
	{
#if DEBUG
		D2D1_FACTORY_OPTIONS options = {};
		options.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;
		THROW_IF_HRESULT_ERROR(::D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, options, &sD2DFactory));
#else
		THROW_IF_HRESULT_ERROR(::D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &sD2DFactory));
#endif
	}

	return sD2DFactory;
}

IDWriteFactory*	MWinDeviceImpl::GetDWFactory()
{
	static IDWriteFactory* sDWFactory = nil;

	if (sDWFactory == nil)
	{
		THROW_IF_HRESULT_ERROR(::DWriteCreateFactory(
	        DWRITE_FACTORY_TYPE_SHARED,
	        __uuidof(IDWriteFactory),
			reinterpret_cast<IUnknown**>(&sDWFactory)));
	}
	
	return sDWFactory;
}

wstring MWinDeviceImpl::GetLocale()
{
	static wstring sLocale;
	
	if (sLocale.empty())
	{
        wchar_t localeName[LOCALE_NAME_MAX_LENGTH];
        if (::GetUserDefaultLocaleName(localeName, LOCALE_NAME_MAX_LENGTH))
			sLocale = localeName;
		else
			sLocale = L"en-us";
	}

	return sLocale;
}

MWinDeviceImpl::MWinDeviceImpl()
	: mView(nil)
	, mRenderTarget(nil)
	, mClipLayer(nil)
	, mTextFormat(nil)
	, mTextLayout(nil)
	, mForeBrush(nil)
	, mBackBrush(nil)
	, mFont(nil)
	, mSelectionLength(0)
{
    HDC hdc = ::GetDC(NULL);
    if (hdc)
    {
        mDpiScaleX = 96.f / ::GetDeviceCaps(hdc, LOGPIXELSX);
        mDpiScaleY = 96.f / ::GetDeviceCaps(hdc, LOGPIXELSY);
        ::ReleaseDC(NULL, hdc);
    }
    else
    	mDpiScaleX = mDpiScaleY = 1.0f;
}

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
	, mFont(nil)
	, mSelectionLength(0)
{
    HDC hdc = ::GetDC(NULL);
    if (hdc)
    {
        mDpiScaleX = 96.f / ::GetDeviceCaps(hdc, LOGPIXELSX);
        mDpiScaleY = 96.f / ::GetDeviceCaps(hdc, LOGPIXELSY);
        ::ReleaseDC(NULL, hdc);
    }
    else
    	mDpiScaleX = mDpiScaleY = 1.0f;

	MRect bounds;
	inView->GetBounds(bounds);
	inView->ConvertToWindow(bounds.x, bounds.y);

	int32 x = 0, y = 0;
	inView->ConvertToWindow(x, y);

	MCanvas* canvas = dynamic_cast<MCanvas*>(inView);
	mRenderTarget = static_cast<MWinCanvasImpl*>(canvas->GetImpl())->GetRenderTarget();

	D2D1::Matrix3x2F translate(
		D2D1::Matrix3x2F::Translation(
			static_cast<float>(x - bounds.x),
			static_cast<float>(y - bounds.y)));
	
	mRenderTarget->SetTransform(translate);
	mRenderTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);

	if (inUpdate)
		ClipRect(inUpdate);

	SetForeColor(kBlack);
	SetBackColor(kWhite);
}

MWinDeviceImpl::~MWinDeviceImpl()
{
	try
	{
		if (mFont != nil)
			mFont->Release();
	
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
	}
	catch (...)
	{
		PRINT(("Oeps"));
	}
}

void MWinDeviceImpl::Save()
{
	ID2D1DrawingStateBlock* state;
	THROW_IF_HRESULT_ERROR(GetD2D1Factory()->CreateDrawingStateBlock(&state));
	mRenderTarget->SaveDrawingState(state);
	mState.push(state);
}

void MWinDeviceImpl::Restore()
{
	assert(not mState.empty());
	if (not mState.empty())
	{
		mRenderTarget->RestoreDrawingState(mState.top());
		mState.top()->Release();
		mState.pop();
	}
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

	ID2D1SolidColorBrush* brush;
	mForeBrush->QueryInterface(&brush);

	if (brush != nil)
	{
		D2D1_COLOR_F color = brush->GetColor();
		result = MColor(color.r, color.g, color.b);
		brush->Release();
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

	ID2D1SolidColorBrush* brush;
	mBackBrush->QueryInterface(&brush);

	if (brush != nil)
	{
		D2D1_COLOR_F color = brush->GetColor();
		result = MColor(color.r, color.g, color.b);
		brush->Release();
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
		D2D1::LayerParameters(D2D1::RectF(inRect)),
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

	mRenderTarget->FillRectangle(D2D1::RectF(inRect), mBackBrush);
}

void MWinDeviceImpl::FillRect(
	MRect				inRect)
{
	assert(mForeBrush);
	assert(mRenderTarget);

	mRenderTarget->FillRectangle(D2D1::RectF(inRect), mForeBrush);
}

void MWinDeviceImpl::StrokeRect(
	MRect				inRect,
	uint32				inLineWidth)
{
	assert(mForeBrush);
	assert(mRenderTarget);

	mRenderTarget->DrawRectangle(D2D1::RectF(inRect), mForeBrush);
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

	D2D1_ROUNDED_RECT r = D2D1::RoundedRect(D2D1::RectF(inRect), radius, radius);
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

void MWinDeviceImpl::SetFont(
	const string&		inFont)
{
	string::const_iterator e = inFont.end() - 1;

	int size = 0, n = 1;
	while (e != inFont.begin() and isdigit(*e))
	{
		size += n * (*e - '0');
		n *= 10;
		--e;
	}

	if (e == inFont.end() or e == inFont.begin() or *e != ' ')
		THROW(("Error in specified font"));

	mFontFamily = c2w(inFont.substr(0, e - inFont.begin()));
	mFontSize = (size * 96.f) / (72.f * mDpiScaleY);

	// OK, so that's what the user requested, now find something suitable
	LookupFont(mFontFamily);
}

void MWinDeviceImpl::LookupFont(const wstring& inFamily)
{
	if (mFont != nil)
		mFont->Release();
	
	mFont = nil;

	IDWriteFontCollection* pFontCollection = nil;
	THROW_IF_HRESULT_ERROR(GetDWFactory()->GetSystemFontCollection(&pFontCollection));
	uint32 familyCount = pFontCollection->GetFontFamilyCount();

    for (uint32 i = 0; i < familyCount; ++i)
    {
        IDWriteFontFamily* pFontFamily = nil;
		THROW_IF_HRESULT_ERROR(pFontCollection->GetFontFamily(i, &pFontFamily));

		IDWriteLocalizedStrings* pFamilyNames = nil;
		THROW_IF_HRESULT_ERROR(pFontFamily->GetFamilyNames(&pFamilyNames));

        uint32 index = 0;
		BOOL exists = false;
        
		THROW_IF_HRESULT_ERROR(pFamilyNames->FindLocaleName(GetLocale().c_str(), &index, &exists));
        
        // If the specified locale doesn't exist, select the first on the list.
        if (not exists)
            index = 0;

        UINT32 length = 0;
		THROW_IF_HRESULT_ERROR(pFamilyNames->GetStringLength(index, &length));
		
		vector<wchar_t> name(length + 1);
		THROW_IF_HRESULT_ERROR(pFamilyNames->GetString(index, &name[0], length+1));

		pFamilyNames->Release();

		if (inFamily == &name[0])
			pFontFamily->GetFont(index, &mFont);

		pFontFamily->Release();
    }

	pFontCollection->Release();
}

void MWinDeviceImpl::CreateTextFormat()
{
	if (mTextFormat == nil)
	{
		if (mFontFamily.empty())
		{
			mFontFamily = L"Consolas";
			mFontSize = 10 * 96.f / 72.f;
		}

		THROW_IF_HRESULT_ERROR(
			GetDWFactory()->CreateTextFormat(
				mFontFamily.c_str(),                // Font family name.
				NULL,		                       // Font collection (NULL sets it to use the system font collection).
				DWRITE_FONT_WEIGHT_REGULAR,
				DWRITE_FONT_STYLE_NORMAL,
				DWRITE_FONT_STRETCH_NORMAL,
				mFontSize,
				GetLocale().c_str(),
				&mTextFormat
			));
	}
}

float MWinDeviceImpl::GetAscent()
{
	if (not mFont)
		SetFont("Consolas 10");

	DWRITE_FONT_METRICS metrics;
	mFont->GetMetrics(&metrics);
	return metrics.ascent * mFontSize / metrics.designUnitsPerEm;

	//CreateTextFormat();
	//
	//DWRITE_LINE_SPACING_METHOD lineSpacingMethod;
	//float lineSpacing, baseline;
	//
	//mTextFormat->GetLineSpacing(&lineSpacingMethod, &lineSpacing, &baseline);
	//
	//return baseline;
}

float MWinDeviceImpl::GetDescent()
{
	if (not mFont)
		SetFont("Consolas 10");

	DWRITE_FONT_METRICS metrics;
	mFont->GetMetrics(&metrics);
	return metrics.descent * mFontSize / metrics.designUnitsPerEm;
}

int32 MWinDeviceImpl::GetLineHeight()
{
	if (not mFont)
		SetFont("Consolas 10");

	DWRITE_FONT_METRICS metrics;
	mFont->GetMetrics(&metrics);
	return static_cast<int32>(
		ceil((metrics.ascent + metrics.descent + metrics.lineGap) * mFontSize / metrics.designUnitsPerEm));

	//CreateTextFormat();
	//
	//DWRITE_LINE_SPACING_METHOD lineSpacingMethod;
	//float lineSpacing, baseline;
	//
	//THROW_IF_HRESULT_ERROR(mTextFormat->GetLineSpacing(&lineSpacingMethod, &lineSpacing, &baseline));
	//
	//return static_cast<int32>(floor(lineSpacing));


	//if (mTextLayout == nil)
	//	SetText(" ");
	//
	//DWRITE_LINE_SPACING_METHOD lineSpacingMethod = DWRITE_LINE_SPACING_METHOD_DEFAULT;
	//float lineSpacing, baseline;
	//
	//THROW_IF_HRESULT_ERROR(mTextLayout->GetLineSpacing(&lineSpacingMethod, &lineSpacing, &baseline));
	//
	//return static_cast<int32>(floor(lineSpacing));
}

float MWinDeviceImpl::GetXWidth()
{
	if (not mFont)
		SetFont("Consolas 10");

	CreateTextFormat();

	IDWriteTextLayout* layout = nil;

	THROW_IF_HRESULT_ERROR(
		GetDWFactory()->CreateTextLayout(L"xxxxxxxxxx", 10, mTextFormat, 99999.0f, 99999.0f, &layout));

	DWRITE_TEXT_METRICS metrics;
	THROW_IF_HRESULT_ERROR(layout->GetMetrics(&metrics));

	layout->Release();

	return metrics.width / 10;
}

void MWinDeviceImpl::DrawString(
	const string&		inText,
	float				inX,
	float				inY,
	uint32				inTruncateWidth,
	MAlignment			inAlign)
{
	CreateTextFormat();

	wstring s(c2w(inText));
	mRenderTarget->DrawTextW(s.c_str(), s.length(),
		mTextFormat, D2D1::RectF(inX, inY, 200.f, 14.f), mForeBrush);
}

void MWinDeviceImpl::SetText(
	const string&		inText)
{
	CreateTextFormat();

	mText.clear();
	mText.reserve(inText.length());

	mTextIndex.clear();
	mTextIndex.reserve(inText.length());

	mSelectionLength = 0;

	for (string::const_iterator i = inText.begin(); i != inText.end(); ++i)
	{
		uint32 ch = static_cast<unsigned char>(*i);
		mTextIndex.push_back(mText.length());

		if (ch & 0x0080)
		{
			if ((ch & 0x0E0) == 0x0C0)
			{
				uint32 ch1 = static_cast<unsigned char>(*(i + 1));
				if ((ch1 & 0x0c0) == 0x080)
				{
					ch = ((ch & 0x01f) << 6) | (ch1 & 0x03f);
					i += 1;
					mTextIndex.push_back(mText.length());
				}
			}
			else if ((ch & 0x0F0) == 0x0E0)
			{
				uint32 ch1 = static_cast<unsigned char>(*(i + 1));
				uint32 ch2 = static_cast<unsigned char>(*(i + 2));
				if ((ch1 & 0x0c0) == 0x080 and (ch2 & 0x0c0) == 0x080)
				{
					ch = ((ch & 0x00F) << 12) | ((ch1 & 0x03F) << 6) | (ch2 & 0x03F);
					i += 2;
					mTextIndex.push_back(mText.length());
					mTextIndex.push_back(mText.length());
				}
			}
			else if ((ch & 0x0F8) == 0x0F0)
			{
				uint32 ch1 = static_cast<unsigned char>(*(i + 1));
				uint32 ch2 = static_cast<unsigned char>(*(i + 2));
				uint32 ch3 = static_cast<unsigned char>(*(i + 3));
				if ((ch1 & 0x0c0) == 0x080 and (ch2 & 0x0c0) == 0x080 and (ch3 & 0x0c0) == 0x080)
				{
					ch = ((ch & 0x007) << 18) | ((ch1 & 0x03F) << 12) | ((ch2 & 0x03F) << 6) | (ch3 & 0x03F);
					i += 3;
					mTextIndex.push_back(mText.length());
					mTextIndex.push_back(mText.length());
					mTextIndex.push_back(mText.length());
				}
			}
		}

		if (ch <= 0x0FFFF)
			mText += static_cast<wchar_t>(ch);
		else
		{
			wchar_t h = (ch - 0x010000) / 0x0400 + 0x0D800;
			wchar_t l = (ch - 0x010000) % 0x0400 + 0x0DC00;

			mText += h;
			mText += l;
		}
	}
	mTextIndex.push_back(mText.length());

	if (mTextLayout != nil)
		mTextLayout->Release();

	mTextLayout = nil;

	THROW_IF_HRESULT_ERROR(
		GetDWFactory()->CreateTextLayout(
			mText.c_str(),
			mText.length(),
			mTextFormat,
			99999.0f,
			99999.0f,
			&mTextLayout
		));
}

void MWinDeviceImpl::SetTabStops(
	float				inTabWidth)
{
	if (mTextLayout == nil)
		THROW(("SetText must be called first!"));
	mTextLayout->SetIncrementalTabStop(inTabWidth /** 96.f / 72.f*/);
}

void MWinDeviceImpl::SetTextColors(
	uint32				inColorCount,
	uint32				inColorIndices[],
	uint32				inOffsets[],
	MColor				inColors[])
{
	for (uint32 ix = 0; ix < inColorCount; ++ix)
	{
		MColor c = inColors[inColorIndices[ix]];
		
		MTextColor* color = new MTextColor(c);

		DWRITE_TEXT_RANGE range;
		range.startPosition = mTextIndex[inOffsets[ix]];
		if (ix == inColorCount - 1)
			range.length = mText.length() - range.startPosition;
		else
			range.length = mTextIndex[inOffsets[ix + 1]] - range.startPosition;
		
		mTextLayout->SetDrawingEffect(color, range);
		color->Release();
	}
}

void MWinDeviceImpl::SetTextSelection(
	uint32				inStart,
	uint32				inLength,
	MColor				inSelectionColor)
{
	mSelectionColor = inSelectionColor;
	mSelectionStart = mTextIndex[inStart];
	mSelectionLength = mTextIndex[inStart + inLength] - mSelectionStart;
}

void MWinDeviceImpl::IndexToPosition(
	uint32				inIndex,
	bool				inTrailing,
	int32&				outPosition)
{
    // Translate text character offset to point x,y.
    DWRITE_HIT_TEST_METRICS caretMetrics;
    float caretX = 0, caretY = 0;

	if (mTextLayout != nil and inIndex < mTextIndex.size())
	{
		int32 offset = mTextIndex[inIndex];

		mTextLayout->HitTestTextPosition(
			offset, inTrailing, &caretX, &caretY, &caretMetrics);

		outPosition = static_cast<uint32>(caretX + 0.5f);
	}
	else
		outPosition = 0;
}

bool MWinDeviceImpl::PositionToIndex(
	int32				inPosition,
	uint32&				outIndex)
{
	if (mTextLayout == nil)
		outIndex = 0;
	else
	{
		BOOL isTrailingHit, isInside;
		DWRITE_HIT_TEST_METRICS caretMetrics;

		float x = static_cast<float>(inPosition);
		float y = GetAscent();

		mTextLayout->HitTestPoint(x, y, &isTrailingHit, &isInside, &caretMetrics);
		
		if (isTrailingHit)
			++caretMetrics.textPosition;
		
		// remap the wchar_t index into our UTF-8 string
		outIndex = MapBack(caretMetrics.textPosition);
	}

	return true;
}

float MWinDeviceImpl::GetTextWidth()
{
	DWRITE_TEXT_METRICS metrics;
	THROW_IF_HRESULT_ERROR(mTextLayout->GetMetrics(&metrics));

	return metrics.widthIncludingTrailingWhitespace;
}

void MWinDeviceImpl::DrawText(
	float				inX,
	float				inY)
{
	if (mTextLayout != nil)
	{
		if (mSelectionLength > 0)
		{
			ID2D1SolidColorBrush* selectionColorBrush;
			THROW_IF_HRESULT_ERROR(mRenderTarget->CreateSolidColorBrush(
				D2D1::ColorF(mSelectionColor.red / 255.f, mSelectionColor.green / 255.f, mSelectionColor.blue / 255.f),
				&selectionColorBrush));

			DWRITE_TEXT_RANGE caretRange = { mSelectionStart, mSelectionLength };
			UINT32 actualHitTestCount = 0;

			// Determine actual number of hit-test ranges
			mTextLayout->HitTestTextRange(caretRange.startPosition, caretRange.length,
				inX, inY, NULL, 0, &actualHitTestCount);

			// Allocate enough room to return all hit-test metrics.
			std::vector<DWRITE_HIT_TEST_METRICS> hitTestMetrics(actualHitTestCount);

			mTextLayout->HitTestTextRange(caretRange.startPosition, caretRange.length,
				inX, inY, &hitTestMetrics[0], static_cast<UINT32>(hitTestMetrics.size()),
				&actualHitTestCount);

			// Draw the selection ranges behind the text.
			if (actualHitTestCount > 0)
			{
				// Note that an ideal layout will return fractional values,
				// so you may see slivers between the selection ranges due
				// to the per-primitive antialiasing of the edges unless
				// it is disabled (better for performance anyway).

				D2D1_ANTIALIAS_MODE savedMode = mRenderTarget->GetAntialiasMode();
				mRenderTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);

				for (size_t i = 0; i < actualHitTestCount; ++i)
				{
					const DWRITE_HIT_TEST_METRICS& htm = hitTestMetrics[i];
					D2D1_RECT_F highlightRect = {
						htm.left,
						htm.top,
						(htm.left + htm.width),
						(htm.top  + htm.height)
					};
            
					mRenderTarget->FillRectangle(highlightRect, selectionColorBrush);
				}

				mRenderTarget->SetAntialiasMode(savedMode);
			}

			selectionColorBrush->Release();
		}

		MTextRenderer renderer(GetD2D1Factory(), mRenderTarget);
		mTextLayout->Draw(nil, &renderer, inX, inY);
	}
}

void MWinDeviceImpl::DrawCaret(
	float				inX,
	float				inY,
	uint32				inOffset)
{
    // Translate text character offset to point x,y.
    DWRITE_HIT_TEST_METRICS caretMetrics = {};
    float caretX = 0, caretY = 0;

	if (mTextLayout != nil)
	{
		int32 offset;

		if (inOffset < mTextIndex.size())
			offset = mTextIndex[inOffset];
		else
			offset = mText.length() + 1;

		mTextLayout->HitTestTextPosition(
			offset, false, &caretX, &caretY, &caretMetrics);
	}
	else
		caretMetrics.height = static_cast<float>(GetLineHeight());

    // The default thickness of 1 pixel is almost _too_ thin on modern large monitors,
    // but we'll use it.
    DWORD caretIntThickness = 2 / mDpiScaleX;
	::SystemParametersInfo(SPI_GETCARETWIDTH, 0, &caretIntThickness, FALSE);
    const float caretThickness = float(caretIntThickness);

	SetForeColor(kBlack);

	D2D1_ANTIALIAS_MODE savedMode = mRenderTarget->GetAntialiasMode();
	mRenderTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);
	mRenderTarget->FillRectangle(
		D2D1::RectF(inX + caretX - caretThickness / 2, inY + caretY,
					inX + caretX + caretThickness / 2, floor(inY + caretY + caretMetrics.height)
		),
		mForeBrush);
	mRenderTarget->SetAntialiasMode(savedMode);
}

void MWinDeviceImpl::BreakLines(
	uint32				inWidth,
	vector<uint32>&		outBreaks)
{
	mTextLayout->SetMaxWidth(static_cast<float>(inWidth));
	
	uint32 lineCount = 0;
	mTextLayout->GetLineMetrics(nil, 0, &lineCount);
	if (lineCount > 0)
	{
		vector<DWRITE_LINE_METRICS> lineMetrics(lineCount);
		THROW_IF_HRESULT_ERROR(
			mTextLayout->GetLineMetrics(&lineMetrics[0], lineCount, &lineCount));
		
		uint32 offset = 0;
		foreach (DWRITE_LINE_METRICS& m, lineMetrics)
		{
			offset += m.length;
			outBreaks.push_back(MapBack(offset));
		}
	}
}

uint32 MWinDeviceImpl::MapBack(
	uint32				inOffset)
{
	vector<uint16>::iterator ix =
		find(mTextIndex.begin(), mTextIndex.end(), inOffset);

	uint32 result;

	if (ix == mTextIndex.end())
		result = mText.length();
	else
		result = ix - mTextIndex.begin();

	return result;
}

// --------------------------------------------------------------------

MDeviceImp* MDeviceImp::Create()
{
	return new MWinDeviceImpl();
}

MDeviceImp* MDeviceImp::Create(
	MView*				inView,
	MRect				inRect,
	bool				inCreateOffscreen)
{
	return new MWinDeviceImpl(inView, inRect, inCreateOffscreen);
}

// --------------------------------------------------------------------

//#include <AeroStyle.xml>

void MDevice::GetSysSelectionColor(
	MColor&				outColor)
{
	// just like Visual Studio does, we take a 2/3 mix of
	// the system highlight color and white. This way we
	// can still use the syntax highlighted colors and don't
	// have to fall back to some recalculated colors.
	
	COLORREF clr = ::GetSysColor(COLOR_HIGHLIGHT);
	if (clr != 0)
	{
		uint32 red = (clr & 0x000000FF);
		red = (2 * red + 3 * 255) / 5;
		if (red > 255)
			red = 255;
		outColor.red = red;

		clr >>= 8;

		uint32 green = (clr & 0x000000FF);
		green = (2 * green + 3 * 255) / 5;
		if (green > 255)
			green = 255;
		outColor.green = green;
		
		clr >>= 8;

		uint32 blue = (clr & 0x000000FF);
		blue = (2 * blue + 3 * 255) / 5;
		if (blue > 255)
			blue = 255;
		outColor.blue = blue;
	}
}

