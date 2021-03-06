//          Copyright Maarten L. Hekkelman 2006-2010
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <Windows.h>
#include <d2d1.h>
#include <dwrite.h>

#undef CreateWindow
#undef GetNextWindow
#undef GetTopWindow

#include "zeep/xml/document.hpp"
#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/filesystem/fstream.hpp>

#include "MLib.h"
#include "MWinWindowImpl.h"
#include "MWindow.h"
#include "MError.h"
#include "MWinApplicationImpl.h"
#include "MWinControlsImpl.h"
#include "MUtils.h"
#include "MWinUtils.h"
#include "MWinMenu.h"
#include "MDevice.h"
#include "MResources.h"
#include "MCanvasImpl.h"

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
{
	if (not inMenu.empty())
	{
		mrsrc::rsrc rsrc(string("Menus/") + inMenu + ".xml");
		
		if (not rsrc)
			THROW(("Menu resource not found: %s", inMenu));

		io::stream<io::array_source> data(rsrc.data(), rsrc.size());
		
		xml::document doc(data);
	
		mMenubar = MMenu::Create(doc.child());
	}
}

MWinWindowImpl::~MWinWindowImpl()
{
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

void MWinWindowImpl::RegisterParams(UINT& outStyle, HCURSOR& outCursor,
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

void MWinWindowImpl::Create(MRect inBounds, const wstring& inTitle)
{
	if (mFlags & kMPostionDefault)
		inBounds.x = inBounds.y = CW_USEDEFAULT;

	MWinProcMixin::Create(nil, inBounds, inTitle);

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
	//AddHandler(WM_MOUSEACTIVATE,	boost::bind(&MWinWindowImpl::WMMouseActivate, this, _1, _2, _3, _4, _5));
	AddHandler(WM_SIZE,				boost::bind(&MWinWindowImpl::WMSize, this, _1, _2, _3, _4, _5));
	AddHandler(WM_SIZING,			boost::bind(&MWinWindowImpl::WMSizing, this, _1, _2, _3, _4, _5));
	AddHandler(WM_PAINT,			boost::bind(&MWinWindowImpl::WMPaint, this, _1, _2, _3, _4, _5));
	//AddHandler(WM_ERASEBKGND,		boost::bind(&MWinWindowImpl::WMEraseBkgnd, this, _1, _2, _3, _4, _5));
	AddHandler(WM_INITMENU,			boost::bind(&MWinWindowImpl::WMInitMenu, this, _1, _2, _3, _4, _5));
	AddHandler(WM_COMMAND,			boost::bind(&MWinWindowImpl::WMCommand, this, _1, _2, _3, _4, _5));
	AddHandler(WM_MENUCOMMAND,		boost::bind(&MWinWindowImpl::WMMenuCommand, this, _1, _2, _3, _4, _5));
	AddHandler(WM_LBUTTONDOWN,		boost::bind(&MWinWindowImpl::WMMouseDown, this, _1, _2, _3, _4, _5));
	AddHandler(WM_MOUSEWHEEL,		boost::bind(&MWinWindowImpl::WMMouseWheel, this, _1, _2, _3, _4, _5));
	AddHandler(WM_VSCROLL,			boost::bind(&MWinWindowImpl::WMScroll, this, _1, _2, _3, _4, _5));
	AddHandler(WM_HSCROLL,			boost::bind(&MWinWindowImpl::WMScroll, this, _1, _2, _3, _4, _5));
	AddHandler(WM_SETFOCUS,			boost::bind(&MWinWindowImpl::WMSetFocus, this, _1, _2, _3, _4, _5));
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

// --------------------------------------------------------------------
// overrides for MWindowImpl

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
	WINDOWPLACEMENT pl = { 0 };
	pl.length = sizeof(pl);
	if (::GetWindowPlacement(GetHandle(), &pl) and ::IsIconic(GetHandle()))
	{
		pl.showCmd = SW_RESTORE;
		::SetWindowPlacement(GetHandle(), &pl);
	}

	::SetActiveWindow(GetHandle());
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
//virtual void	BeFocus();
//virtual void	SubFocusChanged();
	
void MWinWindowImpl::SetWindowPosition(MRect inBounds, bool inTransition)
{
	//HRect sb;
	//HScreen::GetBounds(sb);
	//
	//if (sb.Intersects(inBounds))

	::MoveWindow(GetHandle(), inBounds.x, inBounds.y,
		inBounds.width, inBounds.height, TRUE);
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
	/* Force a direct WM_PAINT */
	::UpdateWindow(GetHandle());
}

void MWinWindowImpl::ScrollRect(MRect inRect, int32 inDeltaH, int32 inDeltaV)
{
	RECT r = { inRect.x, inRect.y, inRect.x + inRect.width, inRect.y + inRect.height };
	::ScrollWindowEx(GetHandle(), inDeltaH, inDeltaV, &r, &r, nil, nil, SW_INVALIDATE);
}
	
bool MWinWindowImpl::GetMouse(int32& outX, int32& outY, unsigned long& outModifiers)
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
			unsigned long mod;
			
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
	MWinProcMixin::WMDestroy(inHWnd, inUMsg, inWParam, inLParam, outResult);

	MWindow::RemoveWindowFromList(mWindow);
	delete mWindow;
	return true;
}

bool MWinWindowImpl::WMActivate(HWND /*inHWnd*/, UINT /*inUMsg*/, WPARAM inWParam, LPARAM /*inLParam*/, int& /*outResult*/)
{
	if (LOWORD(inWParam) == WA_INACTIVE)
		mWindow->Deactivate ();
	else if (mWindow->IsEnabled())
		mWindow->Activate();
	//else
	//	beep();
	return true;
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
		unsigned long modifiers;
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
		}

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

		try
		{
			mWindow->RedrawAll(update);
		}
		catch (...)
		{
		}

		::EndPaint(inHWnd, &lPs);
	}

	outResult = 0;
	return true;
}

bool MWinWindowImpl::WMEraseBkgnd(HWND /*inHWnd*/, UINT /*inUMsg*/, WPARAM /*inWParam*/, LPARAM /*inLParam*/, int& outResult)
{
	bool result = false;
	//if (mRenderTarget != nil)
	//{
	//	outResult = 1;
	//	result = true;
	//}
	return result;
}


bool MWinWindowImpl::WMInitMenu(HWND /*inHWnd*/, UINT /*inUMsg*/, WPARAM /*inWParam*/, LPARAM /*inLParam*/, int& /*outResult*/)
{
	mMenubar->UpdateCommandStatus();
	return false;
}

bool MWinWindowImpl::WMCommand(HWND inHWnd, UINT inUMsg, WPARAM inWParam, LPARAM inLParam, int& outResult)
{
	outResult = 1;
	bool result = false;

	if (inLParam != nil)
	{
		MWinControlImpl* imp =
			MWinControlImpl::FetchControlImpl((HWND)inLParam);

		if (imp != nil)
			result = imp->WMCommand(inHWnd, HIWORD(inWParam), inWParam, inLParam, outResult);
	}
	//else
	//{
	//	HMessage msg (inWParam);
	//	
	//	if (HHandler::GetFocus())
	//		result = HHandler::GetFocus()->HandleMessage(msg);
	//	else
	//		result = HHandler::GetTopHandler()->HandleMessage(msg);
	//}

	return result;
	
	return false;
}

bool MWinWindowImpl::WMMenuCommand(HWND inHWnd, UINT /*inUMsg*/, WPARAM inWParam, LPARAM inLParam, int& outResult)
{
	outResult = 1;
	bool result = false;

	uint32 index = inWParam;
	MMenu* menu = MWinMenuImpl::Lookup((HMENU)inLParam);

	mWindow->ProcessCommand(menu->GetItemCommand(index), menu, index, 0);

	return result;
}

bool MWinWindowImpl::WMMouseDown(HWND /*inHWnd*/, UINT /*inUMsg*/, WPARAM /*inWParam*/, LPARAM inLParam, int& /*outResult*/)
{
	unsigned long modifiers;
	::GetModifierState(modifiers, false);
	
	int32 x = LOWORD(inLParam);
	int32 y = HIWORD(inLParam);
	
	MView* view;
	
	//if (HNode::GetGrabbingNode())
	//	node = HNode::GetGrabbingNode();
	//else
		view = mWindow->FindSubView(x, y);
	assert(view != nil);

	static MView* sLastClickView = nil;
	static double sLastClickTime = 0;
	static uint32 sClickCount = 0;

	double when = ::GetMessageTime();
	if (when >= sLastClickTime + ::GetDblClickTime() or sLastClickView != view)
		sClickCount = 1;
	else
		sClickCount = (sClickCount % 3) + 1;

	sLastClickTime = when;
	sLastClickView = view;
	
	if (view)
	{
		view->ConvertFromWindow(x, y);
		view->Click(x, y, modifiers);
	}

	return true;
}

bool MWinWindowImpl::WMMouseWheel(HWND /*inHWnd*/, UINT /*inUMsg*/, WPARAM inWParam, LPARAM inLParam, int& /*outResult*/)
{
	//short delta = static_cast<short>(HIWORD(inWParam));
	//delta /= WHEEL_DELTA;

	//unsigned long modifiers;
	//GetModifierState(modifiers, false);
	//
	//HPoint where;
	//where.x = LOWORD(inLParam);
	//where.y = HIWORD(inLParam);
	//mWindow->ConvertFromScreen(where);
	//
	//HNode* node;
	//
	//if (HNode::GetGrabbingNode())
	//	node = HNode::GetGrabbingNode();
	//else
	//	node = mWindow->FindSubPane(where);
	//assert(node != nil);
	//
	//if (node)
	//{
	//	node->ConvertFromWindow(where);
	//	node->WheelScroll(where, delta, modifiers);
	//}

	return true;
}

bool MWinWindowImpl::WMScroll(HWND inHWnd, UINT inUMsg, WPARAM inWParam, LPARAM inLParam, int& outResult)
{
	MScrollbarImpl* scrollbarImpl = dynamic_cast<MScrollbarImpl*>(
		MWinControlImpl::FetchControlImpl((HWND)inLParam));

	bool result = false;

	if (scrollbarImpl != nil)
		result = scrollbarImpl->WMScroll(inHWnd, inUMsg, inWParam, inLParam, outResult);

	return result;
}

bool MWinWindowImpl::WMSetFocus(HWND /*inHWnd*/, UINT /*inUMsg*/, WPARAM /*inWParam*/, LPARAM /*inLParam*/, int& /*outResult*/)
{
	//if (not fCalledFromSubFocusChanged)
	//	mWindow->RestoreFocus();

	return true;
}

bool MWinWindowImpl::WMContextMenu(HWND /*inHWnd*/, UINT /*inUMsg*/, WPARAM /*inWParam*/, LPARAM inLParam, int& /*outResult*/)
{
	//try
	//{
	//	HPoint where(LOWORD(inLParam), HIWORD(inLParam));
	//	HPoint local(where);
	//	ConvertFromScreen(local);
	//	
	//	HNode* node = mWindow->FindSubPane(local);
	//	assert(node != nil);

	//	HMenu contextMenu(true);
	//	contextMenu.SetParentWindow(mWindow);
	//	contextMenu.SetIsContextMenu();
	//	
	//	node->PopulateContextMenu(contextMenu);

	//	HMenu* selectedMenu;
	//	unsigned int selectedItem;

	//	if (contextMenu.Popup(where, selectedMenu, selectedItem) &&
	//		selectedMenu != nil && selectedItem >= 0)
	//	{
	//		HMessage msg(selectedMenu->GetItemCommand(selectedItem));
	//		msg.menu = selectedMenu;
	//		msg.itemNr = static_cast<int>(selectedItem);
	//		if (HHandler::GetFocus())
	//			HHandler::GetFocus()->HandleMessage(msg);
	//	}
	//}
	//catch (...)
	//{
	//}

	return true;
}

bool MWinWindowImpl::WMSetCursor(HWND /*inHWnd*/, UINT /*inUMsg*/, WPARAM /*inWParam*/, LPARAM /*inLParam*/, int& /*outResult*/)
{
	bool handled = false;
	//try
	//{
	//	HPoint where;
	//	unsigned long modifiers;
	//	
	//	GetMouse(where, modifiers);
	//	
	//	HNode* node;
	//	if (HNode::GetGrabbingNode())
	//		node = HNode::GetGrabbingNode();
	//	else
	//		node = mWindow->FindSubPane(where);

	//		// if node == mWindow defproc should handle setcursor
	//	if (node && node != mWindow && node->IsActive())
	//	{
	//		node->ConvertFromWindow(where);
	//		node->AdjustCursor(where, modifiers);
	//		handled = true;
	//	}
	//}
	//catch (...)
	//{
	//}

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
	//if (gApp->Quit())
	//	outResult = 1;
	//else
	//	outResult = 0;
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

