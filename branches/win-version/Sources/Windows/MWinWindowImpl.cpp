//          Copyright Maarten L. Hekkelman 2006-2010
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <Windows.h>

#undef CreateWindow
#undef GetNextWindow

#include "zeep/xml/document.hpp"
#include <boost/filesystem/fstream.hpp>

#include "MLib.h"
#include "MWinWindowImpl.h"
#include "MWindow.h"
#include "MError.h"
#include "MWinApplicationImpl.h"
#include "MWinUtils.h"
#include "MWinMenu.h"
#include "MDevice.h"

using namespace std;
using namespace zeep;

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
		//mrsrc::rsrc rsrc(string("Menus/") + inResourceName + ".xml");
		//
		//if (not rsrc)
		//	THROW(("Menu resource not found: %s", inResourceName));

		//io::stream<io::array_source> data(rsrc.data(), rsrc.size());
		ifstream data("C:\\Users\\maarten\\projects\\japi\\Resources\\Menus\\" + string(inMenu) + ".xml");
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
	
	outStyle = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
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
//		const int kScrollBarWidth = HScrollBarNode::GetScrollBarWidth();
		const int kScrollBarWidth = 16;
		
		MRect r;
		mWindow->GetBounds(r);
		mSizeBox = ::CreateWindowExW(0, L"SCROLLBAR", nil,
			WS_CHILD | WS_VISIBLE | SBS_SIZEGRIP | SBS_SIZEBOXBOTTOMRIGHTALIGN,
			r.x + r.width - kScrollBarWidth, r.y + r.height - kScrollBarWidth,
			kScrollBarWidth, kScrollBarWidth, GetHandle(),
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
	AddHandler(WM_INITMENU,			boost::bind(&MWinWindowImpl::WMInitMenu, this, _1, _2, _3, _4, _5));
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
	//mWindow->SetFrame(MRect(clientArea));

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

void MWinWindowImpl::Select(){}
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
	
//	virtual void	Invalidate(const HRegion& inRegion){}
//	virtual void	Validate(const HRegion& inRegion){}
void MWinWindowImpl::UpdateIfNeeded(bool inFlush){}

void MWinWindowImpl::ScrollBits(MRect inRect, int32 inDeltaH, int32 inDeltaV){}
	
bool MWinWindowImpl::GetMouse(int32& outX, int32& outY, unsigned long& outModifiers)
{
	return false;
}

bool MWinWindowImpl::WaitMouseMoved(int32 inX, int32 inY)
{
	return false;
}

//virtual void	ConvertToScreen(HPoint& ioPoint) const{}
//virtual void	ConvertFromScreen(HPoint& ioPoint) const{}
//virtual void	ConvertToScreen(HRect& ioRect) const{}
//virtual void	ConvertFromScreen(HRect& ioRect) const{}

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
	//if (LOWORD(inWParam) == WA_INACTIVE)
	//	mWindow->Deactivate ();
	//else if (mWindow->IsEnabled())
	//	mWindow->Activate();
	//else
	//	beep();
	return true;
}

bool MWinWindowImpl::WMMouseActivate(HWND /*inHWnd*/, UINT /*inUMsg*/, WPARAM /*inWParam*/, LPARAM inLParam, int& outResult)
{
	outResult = MA_ACTIVATE;
	
	//if (not mWindow->IsEnabled())
	//{
	//	outResult = MA_NOACTIVATEANDEAT;
	//}
	//else if (LOWORD(inLParam) == HTCLIENT && not mWindow->IsActive())
	//{
		//unsigned long modifiers;
		//GetModifierState(modifiers, false);
		//
		//POINT lPoint;
		//::GetCursorPos (&lPoint);
		//::ScreenToClient (GetHandle(), &lPoint);

		//HPoint where(lPoint.x, lPoint.y);
		//
		//HNode* node;
		//
		//if (HNode::GetGrabbingNode())
		//	node = HNode::GetGrabbingNode();
		//else
		//	node = mWindow->FindSubPane(where);
		//assert(node != nil);
		//
		//node->ConvertFromWindow(where);
		//
		//if (node->ActivateClick(where, modifiers))
		//	outResult = MA_NOACTIVATEANDEAT;
		//else
		//	outResult = MA_ACTIVATEANDEAT;
	//}
	
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
			//int kScrollBarWidth = HScrollBarNode::GetScrollBarWidth();
			int kScrollBarWidth = 16;

			MRect r(newBounds);
			r.x += r.width - kScrollBarWidth;
			r.y += r.height- kScrollBarWidth;
			
			::MoveWindow(mSizeBox, r.x, r.y, r.width, r.height, true);
//			::SetWindowPos(fSizeBox, GetHandle(), lNewBounds.right - kScrollBarWidth,
//				lNewBounds.bottom - kScrollBarWidth, 0, 0,
//				SWP_NOZORDER | SWP_NOZORDER);
		}
		
		//mWindow->ResizeFrame(0, 0, newBounds.right - oldBounds.right,
		//	newBounds.bottom - oldBounds.bottom);
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

bool MWinWindowImpl::WMPaint(HWND inHWnd, UINT /*inUMsg*/, WPARAM /*inWParam*/, LPARAM /*inLParam*/, int& /*outResult*/)
{
	/* Get the 'dirty' rect */
	RECT lUpdateRect;
	if (::GetUpdateRect(inHWnd, &lUpdateRect, FALSE) == TRUE)
	{
		MView view(100, 100);
		view.SetParent(mWindow);

		MRect update(lUpdateRect.left, lUpdateRect.top, lUpdateRect.right - lUpdateRect.left, lUpdateRect.bottom - lUpdateRect.top);
		MDevice dev(&view, update, false);

		MColor c1 = kBlack, c2 = kWhite;

		dev.CreateAndUsePattern(c1, c2);

		dev.FillRect(MRect(10, 10, 50, 50));

		///* Fill a PAINTSTRUCT. No background erase */
		//PAINTSTRUCT lPs;
		//lPs.hdc = ::GetDC (inHWnd);
		//lPs.fErase = FALSE;
		//lPs.rcPaint = lUpdateRect;

		///* Convert the native rect to a HRegion */
		//HRect updateRect(lUpdateRect);
		//HRegion lUpdateRegion(updateRect);
		//
		///* BeginPaint and call the Node redraw */ 
		//::BeginPaint (inHWnd, &lPs);
		//try
		//{
		//	mWindow->RedrawAll(lUpdateRegion);
		//}
		//catch (...)
		//{
		//}
		//::EndPaint (inHWnd, &lPs);
	}

	return true;

	return false;
}

bool MWinWindowImpl::WMInitMenu(HWND /*inHWnd*/, UINT /*inUMsg*/, WPARAM /*inWParam*/, LPARAM /*inLParam*/, int& /*outResult*/)
{
	mMenubar->UpdateCommandStatus();
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
	//unsigned long modifiers;
	//GetModifierState(modifiers, false);
	//
	//HPoint where;
	//where.x = LOWORD(inLParam);
	//where.y = HIWORD(inLParam);
	//
	//HNode* node;
	//
	//if (HNode::GetGrabbingNode())
	//	node = HNode::GetGrabbingNode();
	//else
	//	node = mWindow->FindSubPane(where);
	//assert(node != nil);

	//double when = ::GetMessageTime();
	//if (when >= HNode::sfLastClickTime + 1000 * ::get_dbl_time() ||
	//	HNode::sfLastClickNode != node)
	//{
	//	HNode::sfClickCount = 1;
	//}
	//else
	//	HNode::sfClickCount = (HNode::sfClickCount % 3) + 1;

	//HNode::sfLastClickTime = when;
	//HNode::sfLastClickNode = node;
	//
	//if (node)
	//{
	//	node->ConvertFromWindow(where);
	//	node->Click(where, modifiers);
	//}

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

bool MWinWindowImpl::WMScroll(HWND inHWnd, UINT /*inUMsg*/, WPARAM inWParam, LPARAM inLParam, int& outResult)
{
	//HWinScrollBarImp* scrollBarImp = dynamic_cast<HWinScrollBarImp*>(
	//	HNativeControlNodeImp::FetchControlNodeImp((HWND)inLParam));
	//if (scrollBarImp)
	//	outResult = scrollBarImp->Scroll(inHWnd, WM_VSCROLL, inWParam, inLParam);

	return true;
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
