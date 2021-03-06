//          Copyright Maarten L. Hekkelman 2006-2010
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "MWinLib.h"
#include <comdef.h>

#include "zeep/xml/document.hpp"
#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/filesystem/fstream.hpp>

#include "MWinWindowImpl.h"
#include "MWindow.h"
#include "MError.h"
#include "MWinApplicationImpl.h"
#include "MWinControlsImpl.h"
#include "MUtils.h"
#include "MWinUtils.h"
#include "MWinMenu.h"
#include "MWinDeviceImpl.h"
#include "MResources.h"
#include "MAcceleratorTable.h"

using namespace std;
using namespace zeep;
namespace io = boost::iostreams;

MWinWindowImpl::MWinWindowImpl(MWindowFlags inFlags, const string& inMenu,
		MWindow* inWindow)
	: MWindowImpl(inFlags, inWindow)
	, mSizeBox(nil)
	, mStatus(nil)
	, mMinWidth(100)
	, mMinHeight(100)
	, mMenubar(nil)
//	, mRenderTarget(nil)
	, mMousedView(nil)
	, mClickCount(0)
	, mLastClickTime(0)
{
	if (not inMenu.empty())
	{
		mrsrc::rsrc rsrc(string("Menus/") + inMenu + ".xml");
		
		if (not rsrc)
			THROW(("Menu resource not found: %s", inMenu));

		io::stream<io::array_source> data(rsrc.data(), rsrc.size());
		
		xml::document doc(data);
	
		mMenubar = MMenu::Create(doc.child(), false);
	}
}

MWinWindowImpl::~MWinWindowImpl()
{
	//if (mRenderTarget != nil)
	//	mRenderTarget->Release();
}

void MWinWindowImpl::CreateParams(DWORD& outStyle,
	DWORD& outExStyle, wstring& outClassName, HMENU& outMenu)
{
	MWinProcMixin::CreateParams(outStyle, outExStyle, outClassName, outMenu);

	outClassName = L"MWinWindowImpl";
	outStyle = WS_OVERLAPPEDWINDOW;
	if (mFlags & kMFixedSize)
		outStyle &= ~(WS_SIZEBOX | WS_MAXIMIZEBOX);
	outExStyle = 0;

	if (mMenubar != nil)
		outMenu = static_cast<MWinMenuImpl*>(mMenubar->impl())->GetHandle();
}

void MWinWindowImpl::RegisterParams(UINT& outStyle, int& outWndExtra,
	HCURSOR& outCursor, HICON& outIcon, HICON& outSmallIcon, HBRUSH& outBackground)
{
	MWinProcMixin::RegisterParams(outStyle, outWndExtra,
		outCursor, outIcon, outSmallIcon, outBackground);
	
	HINSTANCE inst = MWinApplicationImpl::GetInstance()->GetHInstance();
	
	outStyle = 0;// CS_HREDRAW | CS_VREDRAW;
	//outIcon = ::LoadIcon(inst, MAKEINTRESOURCE(ID_DEF_DOC_ICON));
	//outSmallIcon = ::LoadIcon(inst, MAKEINTRESOURCE(ID_DEF_DOC_ICON));
	outCursor = ::LoadCursor(NULL, IDC_ARROW);
	
	if (mFlags & kMDialogBackground)
		outBackground = (HBRUSH)(COLOR_BTNFACE + 1);
	else
		outBackground = (HBRUSH)(COLOR_WINDOW + 1);
}

void MWinWindowImpl::Create(MRect inBounds, const wstring& inTitle)
{
	if (mFlags & kMPostionDefault)
		inBounds.x = inBounds.y = CW_USEDEFAULT;

	MWinProcMixin::CreateHandle(nil, inBounds, inTitle);

	if (not (mFlags & kMFixedSize))
	{
		MRect r;
		mWindow->GetBounds(r);
		mSizeBox = ::CreateWindowExW(0, L"SCROLLBAR", nil,
			WS_CHILD | WS_VISIBLE | SBS_SIZEGRIP | SBS_SIZEBOXBOTTOMRIGHTALIGN,
			r.x + r.width - kScrollbarWidth, r.y + r.height - kScrollbarWidth,
			kScrollbarWidth, kScrollbarWidth, GetHandle(),
			nil, MWinApplicationImpl::GetInstance()->GetHInstance(),
			nil);
	}
	
	if (mFlags & kMAcceptFileDrops)
		::DragAcceptFiles(GetHandle(), true);

	AddHandler(WM_CLOSE,			boost::bind(&MWinWindowImpl::WMClose, this, _1, _2, _3, _4, _5));
	AddHandler(WM_ACTIVATE,			boost::bind(&MWinWindowImpl::WMActivate, this, _1, _2, _3, _4, _5));
	AddHandler(WM_MOUSEACTIVATE,	boost::bind(&MWinWindowImpl::WMMouseActivate, this, _1, _2, _3, _4, _5));
	AddHandler(WM_SIZE,				boost::bind(&MWinWindowImpl::WMSize, this, _1, _2, _3, _4, _5));
	AddHandler(WM_SIZING,			boost::bind(&MWinWindowImpl::WMSizing, this, _1, _2, _3, _4, _5));
//	AddHandler(WM_PAINT,			boost::bind(&MWinWindowImpl::WMPaint, this, _1, _2, _3, _4, _5));
	if (mFlags & kMDialogBackground)
		AddHandler(WM_ERASEBKGND,	boost::bind(&MWinWindowImpl::WMEraseBkgnd, this, _1, _2, _3, _4, _5));
	AddHandler(WM_INITMENU,			boost::bind(&MWinWindowImpl::WMInitMenu, this, _1, _2, _3, _4, _5));
	AddHandler(WM_COMMAND,			boost::bind(&MWinWindowImpl::WMCommand, this, _1, _2, _3, _4, _5));
	AddHandler(WM_MENUCOMMAND,		boost::bind(&MWinWindowImpl::WMMenuCommand, this, _1, _2, _3, _4, _5));
	AddHandler(WM_LBUTTONDOWN,		boost::bind(&MWinWindowImpl::WMMouseDown, this, _1, _2, _3, _4, _5));
	AddHandler(WM_LBUTTONDBLCLK,	boost::bind(&MWinWindowImpl::WMMouseDown, this, _1, _2, _3, _4, _5));
	AddHandler(WM_LBUTTONUP,		boost::bind(&MWinWindowImpl::WMMouseUp, this, _1, _2, _3, _4, _5));
	AddHandler(WM_MOUSEMOVE,		boost::bind(&MWinWindowImpl::WMMouseMove, this, _1, _2, _3, _4, _5));
	AddHandler(WM_MOUSELEAVE,		boost::bind(&MWinWindowImpl::WMMouseExit, this, _1, _2, _3, _4, _5));
	AddHandler(WM_CAPTURECHANGED,	boost::bind(&MWinWindowImpl::WMMouseExit, this, _1, _2, _3, _4, _5));
	AddHandler(WM_MOUSEWHEEL,		boost::bind(&MWinWindowImpl::WMMouseWheel, this, _1, _2, _3, _4, _5));
	AddHandler(WM_VSCROLL,			boost::bind(&MWinWindowImpl::WMScroll, this, _1, _2, _3, _4, _5));
	AddHandler(WM_HSCROLL,			boost::bind(&MWinWindowImpl::WMScroll, this, _1, _2, _3, _4, _5));
	AddHandler(WM_CONTEXTMENU,		boost::bind(&MWinWindowImpl::WMContextMenu, this, _1, _2, _3, _4, _5));
	AddHandler(WM_SETCURSOR,		boost::bind(&MWinWindowImpl::WMSetCursor, this, _1, _2, _3, _4, _5));
//	AddHandler(WM_IME_COMPOSITION,	boost::bind(&MWinWindowImpl::WMImeComposition, this, _1, _2, _3, _4, _5));
//	AddHandler(WM_IME_STARTCOMPOSITION,
//									boost::bind(&MWinWindowImpl::WMImeStartComposition, this, _1, _2, _3, _4, _5));
	AddHandler(WM_IME_REQUEST,		boost::bind(&MWinWindowImpl::WMImeRequest, this, _1, _2, _3, _4, _5));
	AddHandler(WM_QUERYENDSESSION,	boost::bind(&MWinWindowImpl::WMQueryEndSession, this, _1, _2, _3, _4, _5));
	AddHandler(WM_DROPFILES,		boost::bind(&MWinWindowImpl::WMDropFiles, this, _1, _2, _3, _4, _5));
	
	RECT clientArea;
	::GetClientRect(GetHandle(), &clientArea);

	mWindow->SetFrame(MRect(clientArea.left, clientArea.top, clientArea.right - clientArea.left, clientArea.bottom - clientArea.top));

	if (mMenubar != nil)
		mMenubar->SetTarget(mWindow);
}

//ID2D1RenderTarget* MWinWindowImpl::GetRenderTarget()
//{
//	if (mRenderTarget == nil)
//	{
//		D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties(
//			D2D1_RENDER_TARGET_TYPE_DEFAULT,
//			D2D1::PixelFormat(
//				DXGI_FORMAT_B8G8R8A8_UNORM,
//				D2D1_ALPHA_MODE_IGNORE),
//			0, 0, D2D1_RENDER_TARGET_USAGE_NONE, D2D1_FEATURE_LEVEL_DEFAULT);
//
////		THROW_IF_HRESULT_ERROR(MWinDeviceImpl::GetD2D1Factory()->CreateDCRenderTarget(&props, &mRenderTarget));
////
//		RECT rc;
//		::GetClientRect(GetHandle(), &rc);
//
//		D2D1_HWND_RENDER_TARGET_PROPERTIES wprops = D2D1::HwndRenderTargetProperties(
//			GetHandle(), D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top)
//		);
//
//		THROW_IF_HRESULT_ERROR(
//			MWinDeviceImpl::GetD2D1Factory()->CreateHwndRenderTarget(&props, &wprops, &mRenderTarget));
//	}
//
//	THROW_IF_NIL(mRenderTarget);
//
//	ID2D1RenderTarget* result = nil;
//	THROW_IF_HRESULT_ERROR(mRenderTarget->QueryInterface(&result));
//	THROW_IF_NIL(result);
//
//	return result;
//}
//
//void MWinWindowImpl::ReleaseRenderTarget()
//{
//	mRenderTarget->Release();
//	mRenderTarget = nil;
//}

bool MWinWindowImpl::IsDialogMessage(MSG& inMessage)
{
	return false;
}

// --------------------------------------------------------------------
// overrides for MWindowImlp

void MWinWindowImpl::SetTitle(string inTitle)
{
	::SetWindowTextW(GetHandle(), c2w(inTitle).c_str());
}

//string MWinWindowImpl::GetTitle() const
//{
//	return mTitle;
//}

void MWinWindowImpl::Show()
{
	::ShowWindow(GetHandle(), SW_RESTORE);
}

void MWinWindowImpl::Hide()
{
	::ShowWindow(GetHandle(), SW_HIDE);
}

bool MWinWindowImpl::Visible() const
{
	return ::IsWindowVisible(GetHandle()) != 0;
}

void MWinWindowImpl::Select()
{
	WINDOWPLACEMENT pl = { sizeof(WINDOWPLACEMENT) };

	if (not Visible())
		Show();

	if (::GetWindowPlacement(GetHandle(), &pl) and ::IsIconic(GetHandle()))
	{
		pl.showCmd = SW_RESTORE;
		::SetWindowPlacement(GetHandle(), &pl);
	}

	::SetActiveWindow(GetHandle());
	::SetForegroundWindow(GetHandle());
	mWindow->Activate();
}

void MWinWindowImpl::Close()
{
	if (GetHandle() != nil)
	{
		if (not ::DestroyWindow(GetHandle()))
			THROW_WIN_ERROR(("Error destroying window"));
	}
}

//virtual void	ActivateSelf();
//virtual void	DeactivateSelf();
	
void MWinWindowImpl::SetWindowPosition(MRect inBounds, bool inTransition)
{
	RECT area = {};

	if (not ::SystemParametersInfo(SPI_GETWORKAREA, 0, (LPVOID)&area, 0))
	{
		/* Get the root DC */
		HDC lDC = ::GetDC(NULL);
	
		/* Set the outBounds */
		area.right = ::GetDeviceCaps(lDC, HORZRES);
		area.bottom = ::GetDeviceCaps(lDC, VERTRES);
	
		/* Release the DC. Don't know if thats needed with the screen DC */
		::ReleaseDC (NULL, lDC);
	}
	
	MRect screenArea(area.left, area.top, area.right - area.left, area.bottom - area.top);
	
	MRect wr = screenArea & inBounds;
	
	// only sensible sizes are allowed
	if (wr and wr.width > 100 and wr.height > 100)
		::MoveWindow(GetHandle(), wr.x, wr.y, wr.width, wr.height, TRUE);
}

void MWinWindowImpl::GetWindowPosition(MRect& outBounds) const
{
	RECT r;
	::GetWindowRect(GetHandle(), &r);
	outBounds = MRect(r.left, r.top, r.right - r.left, r.bottom - r.top);
}
	
void MWinWindowImpl::Invalidate(MRect inRect)
{
	RECT r = { inRect.x, inRect.y, inRect.x + inRect.width, inRect.y + inRect.height };
	::InvalidateRect(GetHandle(), &r, false);
}

void MWinWindowImpl::Validate(MRect inRect)
{
	RECT r = { inRect.x, inRect.y, inRect.x + inRect.width, inRect.y + inRect.height };
	::ValidateRect(GetHandle(), &r);
}

void MWinWindowImpl::UpdateNow()
{
	::UpdateWindow(GetHandle());
}

void MWinWindowImpl::ScrollRect(MRect inRect, int32 inDeltaH, int32 inDeltaV)
{
	RECT r = { inRect.x, inRect.y, inRect.x + inRect.width, inRect.y + inRect.height };
//	::ScrollWindowEx(GetHandle(), inDeltaH, inDeltaV, &r, &r, nil, nil, SW_INVALIDATE);
	::InvalidateRect(GetHandle(), &r, false);
}
	
bool MWinWindowImpl::GetMouse(int32& outX, int32& outY, uint32& outModifiers)
{
	POINT lPoint;
	::GetCursorPos(&lPoint);
	::ScreenToClient(GetHandle(), &lPoint);

	int button = VK_LBUTTON;
	if (::GetSystemMetrics(SM_SWAPBUTTON))
		button = VK_RBUTTON;

	bool result = (::GetAsyncKeyState(button) & 0x8000) != 0;
	
	if (result and
		mLastGetMouseX == lPoint.x and
		mLastGetMouseY == lPoint.y)
	{
		::delay(0.02);
		::GetCursorPos(&lPoint);
		::ScreenToClient(GetHandle(), &lPoint);
		
		result = (::GetAsyncKeyState(button) & 0x8000) != 0;
	}
	
	outX = lPoint.x;
	outY = lPoint.y;
	
	mLastGetMouseX = lPoint.x;
	mLastGetMouseY = lPoint.y;

	::GetModifierState(outModifiers, true);

	return result;
}

bool MWinWindowImpl::WaitMouseMoved(int32 inX, int32 inY)
{
	bool result = false;

	if (mWindow->IsActive())
	{
		POINT w = { inX, inY };
		result = ::DragDetect(GetHandle(), w) != 0;
	}
	else if (MWindow::GetFirstWindow() and MWindow::GetFirstWindow()->IsActive())
	{
		double test = GetLocalTime() + 0.5;
		
		for (;;)
		{
			if (GetLocalTime() > test)
			{
				result = true;
				break;
			}
			
			int32 x, y;
			uint32 mod;
			
			if (not GetMouse(x, y, mod))
				break;
			
			if (std::abs(x - inX) > 2 or
				std::abs(y - inY) > 2)
			{
				result = true;
				break;
			}
		}
	}

	return result;
}

void MWinWindowImpl::SetCursor(MCursor inCursor)
{
	static HCURSOR sArrow = ::LoadCursor(NULL, IDC_ARROW);
	static HCURSOR sIBeam = ::LoadCursor(NULL, IDC_IBEAM);
	static HCURSOR sRightArrow = nil;

	switch (inCursor)
	{
		case eNormalCursor:
			if (sArrow != nil)
				::SetCursor(sArrow);
			break;

		case eIBeamCursor:
			if (sIBeam != nil)
				::SetCursor(sIBeam);
			break;

		case eRightCursor:
		{
			if (sRightArrow == nil)
			{
				ICONINFO info;
				if (::GetIconInfo(sArrow, &info))
				{
					BITMAP color = {}, mask = {};

					HDC dc = ::GetDC(GetHandle());

					::GetObjectW(info.hbmColor, sizeof(BITMAP), &color);
					::GetObjectW(info.hbmMask, sizeof(BITMAP), &mask);

					BITMAPINFOHEADER bi = { sizeof(BITMAPINFOHEADER) };
					bi.biWidth = color.bmWidth;    
					bi.biHeight = color.bmHeight;  
					bi.biPlanes = 1;    
					bi.biBitCount = 32;    
					bi.biCompression = BI_RGB;

					vector<uint32> rgba(bi.biWidth);

					for (int row = 0; row < bi.biHeight; ++row)
					{
						if (::GetDIBits(dc, info.hbmColor, row, 1, &rgba[0], (BITMAPINFO*)&bi, DIB_RGB_COLORS) == 1)
						{
							reverse(rgba.begin(), rgba.end());
							(void)::SetDIBits(dc, info.hbmColor, row, 1, &rgba[0], (BITMAPINFO*)&bi, DIB_RGB_COLORS);
						}

						if (::GetDIBits(dc, info.hbmMask, row, 1, &rgba[0], (BITMAPINFO*)&bi, DIB_RGB_COLORS) == 1)
						{
							reverse(rgba.begin(), rgba.end());
							(void)::SetDIBits(dc, info.hbmMask, row, 1, &rgba[0], (BITMAPINFO*)&bi, DIB_RGB_COLORS);
						}
					}
					
					info.xHotspot = 32 - info.xHotspot;
					sRightArrow = ::CreateIconIndirect(&info);
				}
			}

			if (sRightArrow != nil)
				::SetCursor(sRightArrow);
			break;
		}

		default:
			break;
	}
}

void MWinWindowImpl::ObscureCursor()
{
	::SetCursor(nil);
}

void MWinWindowImpl::ConvertToScreen(int32& ioX, int32& ioY) const
{
	POINT p = { ioX, ioY };
	::ClientToScreen(GetHandle(), &p);
	ioX = p.x;
	ioY = p.y;
}

void MWinWindowImpl::ConvertFromScreen(int32& ioX, int32& ioY) const
{
	POINT p = { ioX, ioY };
	::ScreenToClient(GetHandle(), &p);
	ioX = p.x;
	ioY = p.y;
}

bool MWinWindowImpl::DispatchKeyDown(uint32 inKeyCode,
	uint32 inModifiers, const string& inText)
{
	bool result = false;
	uint32 cmd;

	MHandler* focus = mWindow->GetFocus();

	if (MAcceleratorTable::Instance().IsAcceleratorKey(inKeyCode, inModifiers, cmd))
	{
		result = true;
		
		bool enabled = false, checked = false;

		if (focus->UpdateCommandStatus(cmd, nil, 0, enabled, checked) and enabled)
			focus->ProcessCommand(cmd, nil, 0, inModifiers);
	}
	else
		result = focus->HandleKeydown(inKeyCode, inModifiers, inText);
	
	return result;
}

// --------------------------------------------------------------------
// Windows Message handling

bool MWinWindowImpl::WMClose(HWND /*inHWnd*/, UINT /*inUMsg*/, WPARAM /*inWParam*/, LPARAM /*inLParam*/, int& /*outResult*/)
{
	mWindow->Close();
	return true;
}

// Destroy (delete) myself and notify some others,
bool MWinWindowImpl::WMDestroy(HWND inHWnd, UINT inUMsg, WPARAM inWParam, LPARAM inLParam, int& outResult)
{
	bool result = MWinProcMixin::WMDestroy(inHWnd, inUMsg, inWParam, inLParam, outResult);
	if (result)
	{
		delete mWindow;
		mWindow = nil;
	}
	return result;
}

bool MWinWindowImpl::WMActivate(HWND /*inHWnd*/, UINT /*inUMsg*/, WPARAM inWParam, LPARAM /*inLParam*/, int& /*outResult*/)
{
	if (LOWORD(inWParam) == WA_INACTIVE)
		mWindow->Deactivate ();
	else if (mWindow->IsEnabled())
		mWindow->Activate();
	else
		PRINT(("Duh"));
	return false;
}

bool MWinWindowImpl::WMMouseActivate(HWND /*inHWnd*/, UINT /*inUMsg*/, WPARAM /*inWParam*/, LPARAM inLParam, int& outResult)
{
	outResult = MA_ACTIVATE;
	
	if (not mWindow->IsEnabled())
	{
		outResult = MA_NOACTIVATEANDEAT;
	}
	else if (LOWORD(inLParam) == HTCLIENT and not mWindow->IsActive())
	{
		uint32 modifiers;
		GetModifierState(modifiers, false);
		
		POINT lPoint;
		::GetCursorPos(&lPoint);
		::ScreenToClient(GetHandle(), &lPoint);

		MView* view;
		
		//if (MView::GetGrabbingNode())
		//	node = HNode::GetGrabbingNode();
		//else
			view = mWindow->FindSubView(lPoint.x, lPoint.y);
		assert(view != nil);
		
		int32 x = lPoint.x;
		int32 y = lPoint.y;
		
		view->ConvertFromWindow(x, y);
		
		if (view->ActivateOnClick(x, y, modifiers))
			outResult = MA_ACTIVATEANDEAT;
		else
			outResult = MA_NOACTIVATEANDEAT;
	}
	
	return true;
}

bool MWinWindowImpl::WMSize(HWND /*inHWnd*/, UINT /*inUMsg*/, WPARAM inWParam, LPARAM inLParam, int& outResult)
{
	if (inWParam != SIZE_MINIMIZED)
	{
		MRect newBounds(0, 0, LOWORD(inLParam), HIWORD(inLParam));
		MRect oldBounds;
		mWindow->GetBounds(oldBounds);

		if (mSizeBox != nil)
		{
			//int kScrollbarWidth = HScrollBarNode::GetScrollBarWidth();
			int kScrollbarWidth = 16;

			MRect r(newBounds);
			r.x += r.width - kScrollbarWidth;
			r.y += r.height- kScrollbarWidth;
			
			::MoveWindow(mSizeBox, r.x, r.y, r.width, r.height, true);
//			::SetWindowPos(fSizeBox, GetHandle(), lNewBounds.right - kScrollbarWidth,
//				lNewBounds.bottom - kScrollbarWidth, 0, 0,
//				SWP_NOZORDER | SWP_NOZORDER);
		}

		//if (mRenderTarget != nil)
		//{
		//	HRESULT hr = mRenderTarget->Resize(D2D1::SizeU(newBounds.width, newBounds.height));
		//	if (hr == D2DERR_RECREATE_TARGET)
		//	{
		//		mRenderTarget->Release();
		//		mRenderTarget = nil;
		//	}
		//}
		
		mWindow->ResizeFrame(0, 0, newBounds.width - oldBounds.width,
			newBounds.height - oldBounds.height);
	}

//	return true;
	return false;
}

bool MWinWindowImpl::WMSizing(HWND inHWnd, UINT inUMsg, WPARAM inWParam, LPARAM inLParam, int& outResult)
{
	bool result = true;

	RECT& r = *reinterpret_cast<RECT*>(inLParam);

	switch (inWParam)
	{
		case WMSZ_LEFT:
			if (r.right - r.left < mMinWidth)
				r.left = r.right - mMinWidth;
			break;
		case WMSZ_RIGHT:
			if (r.right - r.left < mMinWidth)
				r.right = r.left + mMinWidth;
			break;
		case WMSZ_TOP:
			if (r.bottom - r.top < mMinHeight)
				r.top = r.bottom - mMinHeight;
			break;
		case WMSZ_TOPLEFT:
			if (r.right - r.left < mMinWidth)
				r.left = r.right - mMinWidth;
			if (r.bottom - r.top < mMinHeight)
				r.top = r.bottom - mMinHeight;
			break;
		case WMSZ_TOPRIGHT:
			if (r.right - r.left < mMinWidth)
				r.right = r.left + mMinWidth;
			if (r.bottom - r.top < mMinHeight)
				r.top = r.bottom - mMinHeight;
			break;
		case WMSZ_BOTTOM:
			if (r.bottom - r.top < mMinHeight)
				r.bottom = r.top + mMinHeight;
			break;
		case WMSZ_BOTTOMLEFT:
			if (r.right - r.left < mMinWidth)
				r.left = r.right - mMinWidth;
			if (r.bottom - r.top < mMinHeight)
				r.bottom = r.top + mMinHeight;
			break;
		case WMSZ_BOTTOMRIGHT:
			if (r.right - r.left < mMinWidth)
				r.right = r.left + mMinWidth;
			if (r.bottom - r.top < mMinHeight)
				r.bottom = r.top + mMinHeight;
			break;
	}

	outResult = 1;
	return result;
}

bool MWinWindowImpl::WMPaint(HWND inHWnd, UINT /*inUMsg*/, WPARAM /*inWParam*/, LPARAM /*inLParam*/, int& outResult)
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

		//ID2D1RenderTarget* renderTarget = GetRenderTarget();

		try
		{
			//renderTarget->BeginDraw();
			//
			mWindow->RedrawAll(update);
			::ValidateRect(GetHandle(), &lUpdateRect);
			
			//HRESULT hr = renderTarget->EndDraw();
			//renderTarget->Release();

			//if (hr == D2DERR_RECREATE_TARGET)
			//{
			//	mRenderTarget->Release();
			//	mRenderTarget = nil;
			//}
		}
		catch (_com_error&)
		{
			//ID2D1DCRenderTarget* renderTarget = mRenderTarget;
			//mRenderTarget = nil;
			//
			//try
			//{
			//	renderTarget->Release();
			//}
			//catch (...) {}
		}
		catch (...)
		{
		}

		::EndPaint(inHWnd, &lPs);
	}

	outResult = 0;
	return true;
}

bool MWinWindowImpl::WMEraseBkgnd(HWND inHWnd, UINT /*inUMsg*/, WPARAM /*inWParam*/, LPARAM /*inLParam*/, int& outResult)
{
	outResult = 0;
	bool result = false;
	
	/* Get the 'dirty' rect */
	RECT lUpdateRect;
	if ((mFlags & kMDialogBackground) and ::GetUpdateRect(inHWnd, &lUpdateRect, FALSE) == TRUE)
	{
		// Fill a PAINTSTRUCT. No background erase
		PAINTSTRUCT lPs;
		lPs.hdc = ::GetDC(inHWnd);
		lPs.fErase = FALSE;
		lPs.rcPaint = lUpdateRect;

		// BeginPaint and call the Node redraw 
		::BeginPaint(inHWnd, &lPs);
	
		::FillRect(lPs.hdc, &lUpdateRect, ::GetSysColorBrush(COLOR_BTNFACE));
	
		::EndPaint(inHWnd, &lPs);
	
		outResult = 1;
		result = true;
	}
	
	return result;
}

bool MWinWindowImpl::WMInitMenu(HWND /*inHWnd*/, UINT /*inUMsg*/, WPARAM /*inWParam*/, LPARAM /*inLParam*/, int& /*outResult*/)
{
	if (mMenubar != nil)
		mMenubar->UpdateCommandStatus();
	return false;
}

bool MWinWindowImpl::WMCommand(HWND inHWnd, UINT inUMsg, WPARAM inWParam, LPARAM inLParam, int& outResult)
{
	outResult = 1;
	bool result = false;

	if (inLParam != nil)
	{
		MWinProcMixin* imp = MWinProcMixin::Fetch((HWND)inLParam);

		if (imp != nil)
			result = imp->WMCommand(inHWnd, HIWORD(inWParam), inWParam, inLParam, outResult);
	}
	else
	{
		uint32 modifiers;
		GetModifierState(modifiers, false);
		result = mWindow->GetFocus()->ProcessCommand(inWParam, nil, 0, modifiers);
	}

	return result;
}

bool MWinWindowImpl::WMMenuCommand(HWND inHWnd, UINT /*inUMsg*/, WPARAM inWParam, LPARAM inLParam, int& outResult)
{
	outResult = 1;
	bool result = false;

	uint32 index = inWParam;
	MMenu* menu = MWinMenuImpl::Lookup((HMENU)inLParam);

	mWindow->GetFocus()->ProcessCommand(menu->GetItemCommand(index), menu, index, 0);

	return result;
}

bool MWinWindowImpl::WMMouseDown(HWND inHWnd, UINT inUMsg, WPARAM inWParam, LPARAM inLParam, int& outResult)
{
	::SetFocus(inHWnd);
	::SetCapture(inHWnd);

	uint32 modifiers;
	::GetModifierState(modifiers, false);
	
	int32 x = static_cast<int16>(LOWORD(inLParam));
	int32 y = static_cast<int16>(HIWORD(inLParam));

	MView* mousedView = mWindow->FindSubView(x, y);
	if (mousedView == mMousedView)
	{
		if (mLastClickTime + GetDblClickTime() > GetLocalTime())
			mClickCount = mClickCount % 3 + 1;
		else
			mClickCount = 1;
	}
	else
	{
		mClickCount = 1;
		mMousedView = mousedView;
	}

	mLastClickTime = GetLocalTime();

	if (mMousedView != nil)
	{
		mMousedView->ConvertFromWindow(x, y);
		mMousedView->MouseDown(x, y, mClickCount, modifiers);
	}

	return true;
}

bool MWinWindowImpl::WMMouseMove(HWND inHWnd, UINT inUMsg, WPARAM inWParam, LPARAM inLParam, int& outResult)
{
	int32 x = static_cast<int16>(LOWORD(inLParam));
	int32 y = static_cast<int16>(HIWORD(inLParam));

	MView* mousedView = mWindow->FindSubView(x, y);
	if (mousedView == mMousedView)
	{
		uint32 modifiers;
		::GetModifierState(modifiers, false);

		mMousedView->ConvertFromWindow(x, y);
		mMousedView->MouseMove(x, y, modifiers);
	}

	return true;
}

bool MWinWindowImpl::WMMouseExit(HWND inHWnd, UINT inUMsg, WPARAM inWParam, LPARAM inLParam, int& outResult)
{
	if (mMousedView)
		mMousedView->MouseExit();

	return true;
}

bool MWinWindowImpl::WMMouseUp(HWND inHWnd, UINT inUMsg, WPARAM inWParam, LPARAM inLParam, int& outResult)
{
	::ReleaseCapture();

	int32 x = static_cast<int16>(LOWORD(inLParam));
	int32 y = static_cast<int16>(HIWORD(inLParam));

	MView* mousedView = mWindow->FindSubView(x, y);
	if (mousedView == mMousedView)
	{
		uint32 modifiers;
		::GetModifierState(modifiers, false);
	
		mMousedView->ConvertFromWindow(x, y);
		mMousedView->MouseUp(x, y, modifiers);
	}

	return true;
}

bool MWinWindowImpl::WMMouseWheel(HWND /*inHWnd*/, UINT /*inUMsg*/, WPARAM inWParam, LPARAM inLParam, int& outResult)
{
	short deltay = static_cast<short>(HIWORD(inWParam));
	deltay /= WHEEL_DELTA;
	short deltax = static_cast<short>(LOWORD(inWParam));
	deltax /= WHEEL_DELTA;
	
	uint32 modifiers;
	::GetModifierState(modifiers, false);
	
	int32 x = LOWORD(inLParam);
	int32 y = HIWORD(inLParam);
	mWindow->ConvertFromScreen(x, y);
	
	MView* view = mWindow->FindSubView(x, y);
	
	if (view != nil)
	{
		view->ConvertFromWindow(x, y);
		view->MouseWheel(x, y, deltax, deltay, modifiers);
	}

	outResult = 0;
	return true;
}

bool MWinWindowImpl::WMScroll(HWND inHWnd, UINT inUMsg, WPARAM inWParam, LPARAM inLParam, int& outResult)
{
	MWinScrollbarImpl* scrollbarImpl = dynamic_cast<MWinScrollbarImpl*>(MWinProcMixin::Fetch((HWND)inLParam));

	bool result = false;

	if (scrollbarImpl != nil)
		result = scrollbarImpl->WMScroll(inHWnd, inUMsg, inWParam, inLParam, outResult);

	return result;
}

bool MWinWindowImpl::WMContextMenu(HWND /*inHWnd*/, UINT /*inUMsg*/, WPARAM /*inWParam*/, LPARAM inLParam, int& /*outResult*/)
{
	try
	{
		int32 x = LOWORD(inLParam);
		int32 y = HIWORD(inLParam);

		ConvertFromScreen(x, y);
		
		MView* view = mWindow->FindSubView(x, y);
		if (view != nil)
		{
			view->ConvertFromWindow(x, y);
			view->ShowContextMenu(x, y);
		}
	}
	catch (...)
	{
	}

	return true;
}

bool MWinWindowImpl::WMSetCursor(HWND /*inHWnd*/, UINT /*inUMsg*/, WPARAM /*inWParam*/, LPARAM /*inLParam*/, int& /*outResult*/)
{
	bool handled = false;
	try
	{
		int32 x, y;
		uint32 modifiers;
		
		GetMouse(x, y, modifiers);
		
		MView* view;
		//if (HNode::GetGrabbingNode())
		//	node = HNode::GetGrabbingNode();
		//else
			view = mWindow->FindSubView(x, y);

			// if node == mWindow defproc should handle setcursor
		if (view != nil and view != mWindow and view->IsActive())
		{
			view->ConvertFromWindow(x, y);
			view->AdjustCursor(x, y, modifiers);
			handled = true;
		}
	}
	catch (...)
	{
	}

	return handled;
}

bool MWinWindowImpl::WMImeRequest(HWND /*inHWnd*/, UINT /*inUMsg*/, WPARAM inWParam, LPARAM inLParam, int& outResult)
{
	bool result = false;
	//switch (inWParam)
	//{
	//	case IMR_COMPOSITIONWINDOW:
	//	{
	//		COMPOSITIONFORM* cf = reinterpret_cast<COMPOSITIONFORM*>(inLParam);
	//		cf->dwStyle = CFS_POINT;

	//		HPoint pt;
	//		HHandler::GetFocus()->
	//			OffsetToPosition(-1, pt);

	//		cf->ptCurrentPos = HNativePoint(pt);
	//		outResult = 1;
	//		break;
	//	}
	//	
	//	default:
	//		break;
	//}
	
	return result;
}

//bool MWinWindowImpl::WMImeStartComposition(HWND inHWnd, UINT inUMsg, WPARAM inWParam, LPARAM inLParam, int& outResult)
//{
//	return true;
//}
//
//bool MWinWindowImpl::WMImeComposition(HWND inHWnd, UINT inUMsg, WPARAM inWParam, LPARAM inLParam, int& outResult)
//{//
//	bool handled = false;
//	
//	HTextInputAreaInfo info = { 0 };
//	
//	if (inLParam == 0)
//	{
//		HHandler::GetFocus()->
//			UpdateActiveInputArea(nil, 0, 0, info);
//		handled = true;
//	}
//	else
//	{
//		HAutoBuf<char> text(nil);
//		unsigned long size = 0;
//		
//		if (inLParam & CS_INSERTCHAR)
//		{
//			beep();
//		}
//		if (inLParam & GCS_RESULTSTR) 	
//		{
//			HIMC hIMC = ::ImmGetContext(GetHandle());
//			ThrowIfNil((void*)hIMC);
//	
//			// Get the size of the result string.
//			DWORD dwSize = ::ImmGetCompositionString(hIMC, GCS_RESULTSTR, NULL, 0);
//	
//			// increase buffer size for NULL terminator, 
//			//	 maybe it is in UNICODE
//			dwSize += sizeof(WCHAR);
//	
//			HANDLE hstr = ::GlobalAlloc(GHND,dwSize);
//			ThrowIfNil(hstr);
//	
//			void* lpstr = ::GlobalLock(hstr);
//			ThrowIfNil(lpstr);
//	
//			// Get the result strings that is generated by IME into lpstr.
//			::ImmGetCompositionString(hIMC, GCS_RESULTSTR, lpstr, dwSize);
//			::ImmReleaseContext(GetHandle(), hIMC);
//	
//			unsigned long s1 = dwSize;
//			unsigned long s2 = 2 * s1;
//			
//			text.reset(new char[s2]);
//	
//			HEncoder::FetchEncoder(enc_UTF16LE)->
//				EncodeToUTF8((char*)lpstr, s1, text.get(), s2);
//			size = std::strlen(text.get());
//	
//			::GlobalUnlock(hstr);
//			::GlobalFree(hstr);	
//			handled = true;
//		}
//		
//		HHandler::GetFocus()->
//			UpdateActiveInputArea(text.get(), size, 0, info);
//	}
//	return handled;
//}

bool MWinWindowImpl::WMQueryEndSession(HWND /*inHWnd*/, UINT /*inUMsg*/, WPARAM /*inWParam*/, LPARAM /*inLParam*/, int& outResult)
{
	PRINT(("QueryEndSession"));

	outResult = gApp->CloseAll(kSaveChangesQuittingApplication);
	return true;
}

bool MWinWindowImpl::WMDropFiles(HWND inHWnd, UINT inUMsg, WPARAM inWParam, LPARAM inLParam, int& outResult)
{
	//HDROP drop = (HDROP)inWParam;
	//
	//unsigned int cnt = ::DragQueryFile(drop, 0xFFFFFFFF, nil, 0);
	//HAutoBuf<HUrl>	urls(new HUrl[cnt]);
	//HUrl* url = urls.get();
	//
	//for (unsigned int i = 0; i < cnt; ++i)
	//{
	//	wchar_t path[MAX_PATH];
	//	::DragQueryFileW(drop, i, path, MAX_PATH);
	//	
	//	HFileSpec sp(path);
	//	url[i].SetSpecifier(sp);
	//}

	//::DragFinish(drop);
	//
	//HDragThing drag(static_cast<long>(cnt), url);
	//
	//unsigned long modifiers;
	//GetModifierState(modifiers, false);
	//
	//HPoint where;
	//HNativePoint pt(where);
	//if (::DragQueryPoint(drop, &pt))
	//	where.Set(pt.x, pt.y);
	//
	//HNode* node = mWindow->FindSubPane(where);
	//assert(node != nil);
	//if (node->CanAccept(drag))
	//{
	//	node->ConvertFromWindow(where);
	//	node->Receive(drag, where, modifiers);
	//}
	//
	//outResult = 0;
	return true;
}

// --------------------------------------------------------------------

MWindowImpl* MWindowImpl::Create(const string& inTitle, MRect inBounds,
	MWindowFlags inFlags, const string& inMenu, MWindow* inWindow)
{
	MWinWindowImpl* result = new MWinWindowImpl(inFlags, inMenu, inWindow);
	result->Create(inBounds, c2w(inTitle));
	return result;
}
