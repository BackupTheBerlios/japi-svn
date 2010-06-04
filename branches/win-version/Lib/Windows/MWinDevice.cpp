//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "MWinLib.h"

#include <cmath>
#include <cstring>
#include <stack>

#include "MDevice.h"
#include "MDeviceImpl.h"
#include "MView.h"
#include "MWinWindowImpl.h"
#include "MError.h"
#include "MUnicode.h"
#include "MError.h"
#include "MWinUtils.h"
#include "MPreferences.h"

using namespace std;

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
	: mRefCount(0)
{
}

MTextColor::MTextColor(
		const MColor& inColor)
	: mRefCount(0)
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
    float x, yUnused;

    mRenderTarget->GetDpi(&x, &yUnused);
    *pixelsPerDip = x / 96;

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

// --------------------------------------------------------------------
// base class for MWinDeviceImpl
// provides only the basic Pango functionality
// This is needed in measuring text metrics and such

class MWinDeviceImpl : public MDeviceImp
{
  public:
							MWinDeviceImpl();

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
	
	virtual float			GetAscent();
	
	virtual float			GetDescent();

	virtual float			GetLineHeight();

	virtual float			GetXWidth();

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
								float				inTabWidth);
	
	virtual void			SetTextColors(
								uint32				inColorCount,
								uint32				inColorIndices[],
								uint32				inOffsets[],
								MColor				inColors[]);

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
	
	virtual float			GetTextWidth();
	
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

	void					InitGlobals();
	void					CreateTextFormat();
	void					LookupFont(const wstring& inFamily);

	MView*					mView;
	HDC						mDC;
	//HBITMAP					mOffscreenBitmap;
	int						mDCState;
	//HDC						mWindowDC;
	HPEN					mForePen;
	//bool					mPrinting;

	static ID2D1Factory*	sD2DFactory;
	static IDWriteFactory*	sDWFactory;
	static wstring			sLocale;

	ID2D1DCRenderTarget*	mRenderTarget;
	ID2D1Layer*				mClipLayer;
	IDWriteTextFormat*		mTextFormat;
	IDWriteTextLayout*		mTextLayout;
	ID2D1Brush*				mForeBrush;
	ID2D1Brush*				mBackBrush;

	wstring					mFontFamily;
	float					mFontSize;
	IDWriteFont*			mFont;

	// converted text (from UTF8 to UTF16)
	wstring					mText;
	vector<uint16>			mTextIndex;		// from string to wstring
	MColor					mSelectionColor;
	uint32					mSelectionStart, mSelectionLength;

	//float					mDpiScaleX, mDpiScaleY;

	stack<ID2D1DrawingStateBlock*>
							mState;
};

ID2D1Factory*	MWinDeviceImpl::sD2DFactory;
IDWriteFactory*	MWinDeviceImpl::sDWFactory;
wstring			MWinDeviceImpl::sLocale;

void MWinDeviceImpl::InitGlobals()
{
	if (sLocale.empty())
	{
        wchar_t localeName[LOCALE_NAME_MAX_LENGTH];
        if (::GetUserDefaultLocaleName(localeName, LOCALE_NAME_MAX_LENGTH))
			sLocale = localeName;
		else
			sLocale = L"en-us";
	}

	if (sD2DFactory == nil)
		THROW_IF_HRESULT_ERROR(::D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &sD2DFactory));

	if (sDWFactory == nil)
		THROW_IF_HRESULT_ERROR(::DWriteCreateFactory(
	        DWRITE_FACTORY_TYPE_SHARED,
	        __uuidof(IDWriteFactory),
			reinterpret_cast<IUnknown**>(&sDWFactory)));
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
	InitGlobals();
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
	InitGlobals();
	
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
		result = MColor(color.r * 255, color.g * 255, color.b * 255 /*, color.a * 255*/);
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
		result = MColor(color.r * 255, color.g * 255, color.b * 255 /*, color.a * 255*/);
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
	mFontSize = size * 96.f / 72.f;

	// OK, so that's what the user requested, now find something suitable
	LookupFont(mFontFamily);
}

void MWinDeviceImpl::LookupFont(const wstring& inFamily)
{
	if (mFont != nil)
		mFont->Release();
	
	mFont = nil;

	IDWriteFontCollection* pFontCollection = nil;
	THROW_IF_HRESULT_ERROR(sDWFactory->GetSystemFontCollection(&pFontCollection));
	uint32 familyCount = pFontCollection->GetFontFamilyCount();

    for (uint32 i = 0; i < familyCount; ++i)
    {
        IDWriteFontFamily* pFontFamily = nil;
		THROW_IF_HRESULT_ERROR(pFontCollection->GetFontFamily(i, &pFontFamily));

		IDWriteLocalizedStrings* pFamilyNames = nil;
		THROW_IF_HRESULT_ERROR(pFontFamily->GetFamilyNames(&pFamilyNames));

        uint32 index = 0;
		BOOL exists = false;
        
		THROW_IF_HRESULT_ERROR(pFamilyNames->FindLocaleName(sLocale.c_str(), &index, &exists));
        
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
			sDWFactory->CreateTextFormat(
				mFontFamily.c_str(),                // Font family name.
				NULL,		                       // Font collection (NULL sets it to use the system font collection).
				DWRITE_FONT_WEIGHT_REGULAR,
				DWRITE_FONT_STYLE_NORMAL,
				DWRITE_FONT_STRETCH_NORMAL,
				mFontSize,
				sLocale.c_str(),
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
}

float MWinDeviceImpl::GetDescent()
{
	if (not mFont)
		SetFont("Consolas 10");

	DWRITE_FONT_METRICS metrics;
	mFont->GetMetrics(&metrics);
	return metrics.descent * mFontSize / metrics.designUnitsPerEm;
}

float MWinDeviceImpl::GetLineHeight()
{
	if (not mFont)
		SetFont("Consolas 10");

	DWRITE_FONT_METRICS metrics;
	mFont->GetMetrics(&metrics);
	return (metrics.ascent + metrics.descent + metrics.lineGap) * mFontSize / metrics.designUnitsPerEm;
}

float MWinDeviceImpl::GetXWidth()
{
	if (not mFont)
		SetFont("Consolas 10");

	CreateTextFormat();

	IDWriteTextLayout* layout = nil;

	THROW_IF_HRESULT_ERROR(
		sDWFactory->CreateTextLayout(L"x", 1, mTextFormat, 99999.0f, 99999.0f, &layout));

	DWRITE_TEXT_METRICS metrics;
	THROW_IF_HRESULT_ERROR(layout->GetMetrics(&metrics));

	layout->Release();

	return metrics.width;
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
		mTextFormat,
		D2D1::RectF(inX, inY, 200, 14),
		mForeBrush);
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

	if (mTextLayout != nil)
		mTextLayout->Release();

	mTextLayout = nil;

	THROW_IF_HRESULT_ERROR(
		sDWFactory->CreateTextLayout(
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
	BOOL isTrailingHit, isInside;
    DWRITE_HIT_TEST_METRICS caretMetrics;

    //// Remap display coordinates to actual.
    //DWRITE_MATRIX matrix;
    //GetInverseViewMatrix(&matrix);

    //float transformedX = (x * matrix.m11 + y * matrix.m21 + matrix.dx);
    //float transformedY = (x * matrix.m12 + y * matrix.m22 + matrix.dy);

	float x = inPosition;
	float y = GetAscent();

	mTextLayout->HitTestPoint(x, y, &isTrailingHit, &isInside, &caretMetrics);
	outIndex = caretMetrics.textPosition;

    //// Update current selection according to click or mouse drag.
    //SetSelection(
    //    isTrailingHit ? SetSelectionModeAbsoluteTrailing : SetSelectionModeAbsoluteLeading,
    //    caretMetrics.textPosition,
    //    extendSelection
    //    );

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

		MTextRenderer renderer(sD2DFactory, mRenderTarget);
		mTextLayout->Draw(nil, &renderer, inX, inY);
	}
}

void MWinDeviceImpl::DrawCaret(
	float				inX,
	float				inY,
	uint32				inOffset)
{
    // Translate text character offset to point x,y.
    DWRITE_HIT_TEST_METRICS caretMetrics;
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
		caretMetrics.height = GetLineHeight();

    // The default thickness of 1 pixel is almost _too_ thin on modern large monitors,
    // but we'll use it.
    DWORD caretIntThickness = 2;
	::SystemParametersInfo(SPI_GETCARETWIDTH, 0, &caretIntThickness, FALSE);
    const float caretThickness = float(caretIntThickness);

	SetForeColor(kBlack);

	D2D1_ANTIALIAS_MODE savedMode = mRenderTarget->GetAntialiasMode();
	mRenderTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);
	mRenderTarget->FillRectangle(
		D2D1::RectF(inX + caretX - caretThickness / 2, inY + caretY,
					inX + caretX + caretThickness / 2, inY + caretY + caretMetrics.height),
		mForeBrush);
	mRenderTarget->SetAntialiasMode(savedMode);
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

