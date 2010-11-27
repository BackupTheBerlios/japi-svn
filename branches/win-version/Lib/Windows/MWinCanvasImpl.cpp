//          Copyright Maarten L. Hekkelman 2006-2010
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "MWinLib.h"

#include "MWinCanvasImpl.h"
#include "MWinWindowImpl.h"
#include "MWinDeviceImpl.h"
#include "MWinUtils.h"
#include "MUtils.h"
#include "MError.h"

using namespace std;

// --------------------------------------------------------------------

MWinCanvasImpl::MWinCanvasImpl(
	MCanvas*		inCanvas)
	: MCanvasImpl(inCanvas)
	, mRenderTarget(nil)
	, mInBeginEndDrawSection(false)
	, mLastClickTime(0)
{
	AddHandler(WM_PAINT,			boost::bind(&MWinCanvasImpl::WMPaint, this, _1, _2, _3, _4, _5));
	AddHandler(WM_DISPLAYCHANGE,	boost::bind(&MWinCanvasImpl::WMPaint, this, _1, _2, _3, _4, _5));
	AddHandler(WM_SIZE,				boost::bind(&MWinCanvasImpl::WMSize, this, _1, _2, _3, _4, _5));
	AddHandler(WM_LBUTTONDOWN,		boost::bind(&MWinCanvasImpl::WMMouseDown, this, _1, _2, _3, _4, _5));
	AddHandler(WM_LBUTTONDBLCLK,	boost::bind(&MWinCanvasImpl::WMMouseDown, this, _1, _2, _3, _4, _5));
	AddHandler(WM_LBUTTONUP,		boost::bind(&MWinCanvasImpl::WMMouseUp, this, _1, _2, _3, _4, _5));
	AddHandler(WM_MOUSEMOVE,		boost::bind(&MWinCanvasImpl::WMMouseMove, this, _1, _2, _3, _4, _5));
	AddHandler(WM_MOUSELEAVE,		boost::bind(&MWinCanvasImpl::WMMouseExit, this, _1, _2, _3, _4, _5));
	AddHandler(WM_CAPTURECHANGED,	boost::bind(&MWinCanvasImpl::WMMouseExit, this, _1, _2, _3, _4, _5));
	AddHandler(WM_SETCURSOR,		boost::bind(&MWinCanvasImpl::WMSetCursor, this, _1, _2, _3, _4, _5));
}

MWinCanvasImpl::~MWinCanvasImpl()
{
	if (mRenderTarget != nil)
		mRenderTarget->Release();
}

ID2D1RenderTarget* MWinCanvasImpl::GetRenderTarget()
{
	return mRenderTarget;
}
	
void MWinCanvasImpl::ResizeFrame(
	int32			inXDelta,
	int32			inYDelta,
	int32			inWidthDelta,
	int32			inHeightDelta)
{
	mCanvas->MView::ResizeFrame(inXDelta, inYDelta, inWidthDelta, inHeightDelta);

	if (GetHandle() != nil)
	{
		MRect bounds;

		MView* view = mCanvas;
		MView* parent = view->GetParent();
	
		view->GetBounds(bounds);
	
		while (parent != nil)
		{
			view->ConvertToParent(bounds.x, bounds.y);
		
			// for now we don't support embedding of canvasses in canvasses...
			MWindow* window = dynamic_cast<MWindow*>(parent);
			if (window != nil)
				break;
		
			view = parent;
			parent = parent->GetParent();
		}

		::MoveWindow(GetHandle(), bounds.x, bounds.y,
			bounds.width, bounds.height, false);
	}
}

void MWinCanvasImpl::AddedToWindow()
{
	MRect bounds;

	MView* view = mCanvas;
	MView* parent = view->GetParent();
	MWinProcMixin* windowImpl = nil;
	
	view->GetBounds(bounds);
	
	while (parent != nil)
	{
		view->ConvertToParent(bounds.x, bounds.y);
		
		// for now we don't support embedding of canvasses in canvasses...
		MWindow* window = dynamic_cast<MWindow*>(parent);
		if (window != nil)
		{
			windowImpl = static_cast<MWinWindowImpl*>(window->GetImpl());
			break;
		}
		
		view = parent;
		parent = parent->GetParent();
	}

	CreateHandle(windowImpl, bounds, L"");
	SubClass();
	
	RECT r;
	::GetClientRect(GetHandle(), &r);
	if (r.right - r.left != bounds.width or
		r.bottom - r.top != bounds.height)
	{
		::MapWindowPoints(GetHandle(), windowImpl->GetHandle(), (LPPOINT)&r, 2);

		mCanvas->ResizeFrame(
			r.left - bounds.x, r.top - bounds.y,
			(r.right - r.left) - bounds.width,
			(r.bottom - r.top) - bounds.height);
	}

	mCanvas->MView::AddedToWindow();
}

void MWinCanvasImpl::CreateParams(DWORD& outStyle, DWORD& outExStyle,
	wstring& outClassName, HMENU& outMenu)
{
	MWinProcMixin::CreateParams(outStyle, outExStyle, outClassName, outMenu);
	
	outStyle = WS_CHILDWINDOW | WS_VISIBLE;
	outExStyle = 0;
	outClassName = L"MWinCanvasImpl";
}

void MWinCanvasImpl::RegisterParams(UINT& outStyle, int& outWndExtra, HCURSOR& outCursor,
	HICON& outIcon, HICON& outSmallIcon, HBRUSH& outBackground)
{
	MWinProcMixin::RegisterParams(outStyle, outWndExtra, outCursor, outIcon, outSmallIcon, outBackground);
	
	outStyle = CS_HREDRAW | CS_VREDRAW;
}

bool MWinCanvasImpl::WMPaint(HWND inHWnd, UINT inUMsg, WPARAM inWParam, LPARAM inLParam, int& outResult)
{
	/* Get the 'dirty' rect */
	RECT lUpdateRect;
	if (::GetUpdateRect(inHWnd, &lUpdateRect, FALSE) == TRUE)
	{
		MRect update(lUpdateRect.left, lUpdateRect.top,
			lUpdateRect.right - lUpdateRect.left, lUpdateRect.bottom - lUpdateRect.top);

		mCanvas->ConvertFromWindow(update.x, update.y);

		// Fill a PAINTSTRUCT. No background erase
		PAINTSTRUCT lPs;
		lPs.hdc = ::GetDC(inHWnd);
		lPs.fErase = FALSE;
		lPs.rcPaint = lUpdateRect;

		// BeginPaint and call the Node redraw 
		::BeginPaint(inHWnd, &lPs);
		
		// Somehow, we sometimes crash out of the EndDraw
		if (mInBeginEndDrawSection)
		{
			ID2D1RenderTarget* rt = mRenderTarget;
			mRenderTarget = nil;
			rt->Release();
		}

		if (mRenderTarget == nil)
		{
			D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties(
				D2D1_RENDER_TARGET_TYPE_DEFAULT,
				D2D1::PixelFormat(
					DXGI_FORMAT_B8G8R8A8_UNORM,
					D2D1_ALPHA_MODE_IGNORE),
				0, 0, D2D1_RENDER_TARGET_USAGE_NONE, D2D1_FEATURE_LEVEL_DEFAULT);

			RECT rc;
			::GetClientRect(GetHandle(), &rc);

			D2D1_HWND_RENDER_TARGET_PROPERTIES wprops = D2D1::HwndRenderTargetProperties(
				GetHandle(), D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top)
			);

			THROW_IF_HRESULT_ERROR(
				MWinDeviceImpl::GetD2D1Factory()->CreateHwndRenderTarget(&props, &wprops, &mRenderTarget));

			mRenderTarget->SetDpi(96.f, 96.f);
		}

		HRESULT hr = S_OK;

		try
		{
			mRenderTarget->BeginDraw();
			mInBeginEndDrawSection = true;
			
			mCanvas->RedrawAll(update);
			::ValidateRect(GetHandle(), &lUpdateRect);
			
			hr = mRenderTarget->EndDraw();
			mInBeginEndDrawSection = false;
		}
		catch (...)
		{
			hr = D2DERR_RECREATE_TARGET;
		}

		if (hr != S_OK)
			PRINT(("EndDraw returned %lx", hr));

		if (hr == D2DERR_RECREATE_TARGET)
		{
			mRenderTarget->Release();
			mRenderTarget = nil;
		}

		::EndPaint(inHWnd, &lPs);
	}

	outResult = 0;
	return true;
}

bool MWinCanvasImpl::WMSize(HWND inHWnd, UINT inUMsg, WPARAM inWParam, LPARAM inLParam, int& outResult)
{
	if (inWParam != SIZE_MINIMIZED)
	{
		MRect newBounds(0, 0, LOWORD(inLParam), HIWORD(inLParam));
		MRect oldBounds;
		mCanvas->GetBounds(oldBounds);

		if (mRenderTarget != nil)
		{
			HRESULT hr = mRenderTarget->Resize(D2D1::SizeU(newBounds.width, newBounds.height));
			if (hr == D2DERR_RECREATE_TARGET)
			{
				mRenderTarget->Release();
				mRenderTarget = nil;
			}
		}
		
		mCanvas->ResizeFrame(0, 0, newBounds.width - oldBounds.width,
			newBounds.height - oldBounds.height);
	}

	return false;
}

bool MWinCanvasImpl::WMMouseDown(HWND inHWnd, UINT inUMsg, WPARAM inWParam, LPARAM inLParam, int& outResult)
{
//	::SetFocus(inHWnd);
	::SetCapture(inHWnd);

	uint32 modifiers;
	::GetModifierState(modifiers, false);
	
	int32 x = static_cast<int16>(LOWORD(inLParam));
	int32 y = static_cast<int16>(HIWORD(inLParam));

	if (mLastClickTime + GetDblClickTime() > GetLocalTime())
		mClickCount = mClickCount % 3 + 1;
	else
		mClickCount = 1;

	mLastClickTime = GetLocalTime();
	mCanvas->ConvertFromWindow(x, y);
	mCanvas->MouseDown(x, y, mClickCount, modifiers);

	return true;
}

bool MWinCanvasImpl::WMMouseMove(HWND inHWnd, UINT inUMsg, WPARAM inWParam, LPARAM inLParam, int& outResult)
{
	int32 x = static_cast<int16>(LOWORD(inLParam));
	int32 y = static_cast<int16>(HIWORD(inLParam));

	uint32 modifiers;
	::GetModifierState(modifiers, false);

	mCanvas->ConvertFromWindow(x, y);
	mCanvas->MouseMove(x, y, modifiers);

	return true;
}

bool MWinCanvasImpl::WMMouseExit(HWND inHWnd, UINT inUMsg, WPARAM inWParam, LPARAM inLParam, int& outResult)
{
	mCanvas->MouseExit();

	return true;
}

bool MWinCanvasImpl::WMMouseUp(HWND inHWnd, UINT inUMsg, WPARAM inWParam, LPARAM inLParam, int& outResult)
{
	::ReleaseCapture();

	int32 x = static_cast<int16>(LOWORD(inLParam));
	int32 y = static_cast<int16>(HIWORD(inLParam));

	uint32 modifiers;
	::GetModifierState(modifiers, false);

	mCanvas->ConvertFromWindow(x, y);
	mCanvas->MouseUp(x, y, modifiers);

	return true;
}

bool MWinCanvasImpl::WMSetCursor(HWND inHWnd, UINT inUMsg, WPARAM inWParam, LPARAM inLParam, int& outResult)
{
	bool handled = false;
	try
	{
		if (mCanvas->IsActive())
		{
			int32 x, y;
			uint32 modifiers = 0;

			POINT p;
			::GetCursorPos(&p);
			::ScreenToClient(GetHandle(), &p);
			
			x = p.x;
			y = p.y;
			
			mCanvas->ConvertFromWindow(x, y);
			mCanvas->AdjustCursor(x, y, modifiers);
			handled = true;
		}
	}
	catch (...)
	{
	}

	return handled;
}

// --------------------------------------------------------------------

MCanvasImpl* MCanvasImpl::Create(
	MCanvas*		inCanvas)
{
	return new MWinCanvasImpl(inCanvas);
}
