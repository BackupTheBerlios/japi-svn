//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <Windows.h>
#include <d2d1.h>
#include <dwrite.h>

#undef CreateWindow
#undef GetNextWindow
#undef GetTopWindow

#include "MLib.h"
#include "MWinCanvasImpl.h"
#include "MWinWindowImpl.h"
#include "MWinApplicationImpl.h"
#include "MError.h"

using namespace std;

MWinCanvasImpl::MWinCanvasImpl(MCanvas* inCanvas)
	: MCanvasImpl(inCanvas)
	, MWinControlImpl(inCanvas, "")
	, mRenderTarget(nil)
{
}

MWinCanvasImpl::~MWinCanvasImpl()
{
	if (mRenderTarget != nil)
		mRenderTarget->Release();
}

ID2D1HwndRenderTarget* MWinCanvasImpl::GetRenderTarget(
	ID2D1Factory*	inD2DFactory)
{
	if (mRenderTarget == nil)
	{
		MWindow* window = mCanvas->GetWindow();

		MWinWindowImpl* windowImpl = static_cast<MWinWindowImpl*>(window->GetImpl());
		HWND hwnd = windowImpl->GetHandle();

		RECT r;
		::GetClientRect(hwnd, &r);

		//HDC dc = ::GetDC(hwnd);
		//mDpiScaleX = ::GetDeviceCaps(dc, LOGPIXELSX) / 96.0f;
		//mDpiScaleY = ::GetDeviceCaps(dc, LOGPIXELSY) / 96.0f;
		//::ReleaseDC(0, dc);

		THROW_IF_HRESULT_ERROR(inD2DFactory->CreateHwndRenderTarget(
			::D2D1::RenderTargetProperties(),
			::D2D1::HwndRenderTargetProperties(hwnd, D2D1::SizeU(r.right - r.left, r.bottom - r.top)),
			&mRenderTarget));
	}

	mRenderTarget->BeginDraw();

	return mRenderTarget;
}

void MWinCanvasImpl::CreateParams(DWORD& outStyle, DWORD& outExStyle,
	wstring& outClassName, HMENU& outMenu)
{
	MWinControlImpl::CreateParams(outStyle, outExStyle, outClassName, outMenu);

	outClassName = L"MWinCanvasImpl";
	outStyle = WS_CHILD;
	outExStyle = 0;
}

void MWinCanvasImpl::RegisterParams(UINT& outStyle, HCURSOR& outCursor,
	HICON& outIcon, HICON& outSmallIcon, HBRUSH& outBackground)
{
	MWinProcMixin::RegisterParams(outStyle, outCursor, outIcon, outSmallIcon, outBackground);
	
	HINSTANCE inst = MWinApplicationImpl::GetInstance()->GetHInstance();
	
	outStyle = CS_HREDRAW | CS_VREDRAW;
	//outIcon = ::LoadIcon(inst, MAKEINTRESOURCE(ID_DEF_DOC_ICON));
	//outSmallIcon = ::LoadIcon(inst, MAKEINTRESOURCE(ID_DEF_DOC_ICON));
	outCursor = ::LoadCursor(NULL, IDC_ARROW);
	outBackground = (HBRUSH)(COLOR_WINDOW + 1);
}

void MWinCanvasImpl::Create()
{
	AddedToWindow();

	AddHandler(WM_PAINT,			boost::bind(&MWinCanvasImpl::WMPaint, this, _1, _2, _3, _4, _5));
}

bool MWinCanvasImpl::WMPaint(HWND inHWnd, UINT inUMsg, WPARAM inWParam, LPARAM inLParam, int& outResult)
{
	/* Get the 'dirty' rect */
	RECT lUpdateRect;
	if (::GetUpdateRect(inHWnd, &lUpdateRect, FALSE) == TRUE)
	{
		MRect update(lUpdateRect.left, lUpdateRect.top,
			lUpdateRect.right - lUpdateRect.left, lUpdateRect.bottom - lUpdateRect.top);

		// Fill a PAINTSTRUCT. No background erase
		PAINTSTRUCT lPs;
		lPs.hdc = ::GetDC(inHWnd);
		lPs.fErase = FALSE;
		lPs.rcPaint = lUpdateRect;

		// BeginPaint and call the Node redraw 
		::BeginPaint(inHWnd, &lPs);

		try
		{
			mCanvas->RedrawAll(update);

			HRESULT e = mRenderTarget->EndDraw();
			if (e == D2DERR_RECREATE_TARGET)
			{
				mRenderTarget->Release();
				mRenderTarget = nil;
			}
		}
		catch (...)
		{
		}

		::EndPaint(inHWnd, &lPs);
	}

	outResult = 0;
	return true;
}

void MWinCanvasImpl::FrameResized()
{
	if (mRenderTarget != nil)
	{
		MRect bounds;
		mCanvas->GetBounds(bounds);

		mRenderTarget->Resize(D2D1::SizeU(bounds.width, bounds.height));
	}
}

// --------------------------------------------------------------------

MCanvasImpl* MCanvasImpl::Create(MCanvas* inCanvas)
{
	MWinCanvasImpl* result = new MWinCanvasImpl(inCanvas);
	result->Create();
	return result;
}
