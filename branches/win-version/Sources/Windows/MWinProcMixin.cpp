//          Copyright Maarten L. Hekkelman 2006-2010
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <Windows.h>

#include <cassert>

#include "MLib.h"
#include "MWinProcMixin.h"
#include "MError.h"
#include "MWinApplicationImpl.h"
#include "MWinUtils.h"

using namespace std;

MWinProcMixin::MWinProcMixin()
	: mHandle(nil)
	, mOldWinProc(nil)
	, mSuppressNextWMChar(false)
{
	AddHandler(WM_DESTROY,		&MWinProcMixin::WMDestroy);
	AddHandler(WM_CHAR,			&MWinProcMixin::WMChar);
	AddHandler(WM_KEYDOWN,		&MWinProcMixin::WMKeydown);
	AddHandler(WM_SYSKEYDOWN,	&MWinProcMixin::WMKeydown);
}

MWinProcMixin::~MWinProcMixin()
{
	SetHandle(nil);
}

void MWinProcMixin::SetHandle(HWND inHandle)
{
	if (mHandle != inHandle)
	{
		if (mHandle != nil)
			RemovePropW(mHandle, L"m_window_imp");
		mHandle = inHandle;
		if (mHandle != nil)
			SetPropW(mHandle, L"m_window_imp", this);
	}
}

MWinProcMixin* MWinProcMixin::Fetch(HWND inHandle)
{
	return reinterpret_cast<MWinProcMixin*>(::GetPropW(inHandle, L"m_window_imp"));
}

void MWinProcMixin::Create(const MRect& inBounds, const string& inTitle)
{
	DWORD		style = WS_CHILD | WS_CLIPSIBLINGS | WS_OVERLAPPEDWINDOW;
	DWORD		exStyle = 0;
	HWND		parent = nil;
	HMENU		menu = nil;
	wstring		className;

	CreateParams(style, exStyle, className, menu);

	RECT r = { inBounds.x, inBounds.y, inBounds.width - inBounds.x, inBounds.height - inBounds.y };
	::AdjustWindowRect(&r, style, menu != nil);

	HINSTANCE instance = MWinApplicationImpl::GetInstance()->GetHInstance();
	WNDCLASSEXW lWndClass = { sizeof(WNDCLASSEXW) };
	lWndClass.lpszClassName = className.c_str();

	if (not ::GetClassInfoExW(instance, lWndClass.lpszClassName, &lWndClass))
	{
		RegisterParams(lWndClass.style, lWndClass.hCursor,
			lWndClass.hIcon, lWndClass.hIconSm, lWndClass.hbrBackground);

		ATOM a = ::RegisterClassExW(&lWndClass);
		if (a == 0)
			throw MException("Failed to register window class");
			//ThrowIfOSErr((::GetLastError()));
	}

	HWND handle = CreateWindowExW(exStyle,
		lWndClass.lpszClassName, c2w(inTitle).c_str(),
		style,
		r.left, r.top, r.right - r.left, r.bottom - r.top,
		parent, menu, instance, this);
	if (handle == nil)
//		ThrowIfOSErr((::GetLastError()));
		throw MException("Error creating window");

	assert(mHandle == handle);

	SetHandle(handle);
}

void MWinProcMixin::CreateParams(DWORD& outStyle, DWORD& outExStyle,
	wstring& outClassName, HMENU& outMenu)
{
	outStyle = WS_CHILD | WS_CLIPSIBLINGS;
}

void MWinProcMixin::RegisterParams(UINT& outStyle, HCURSOR& outCursor,
	HICON& outIcon, HICON& outSmallIcon, HBRUSH& outBackground)
{
	outStyle = CS_VREDRAW | CS_HREDRAW;
	outCursor = ::LoadCursor(0, IDC_ARROW);
}

bool MWinProcMixin::WMDestroy(HWND inHWnd, UINT inUMsg, WPARAM inWParam, LPARAM inLParam, int& outResult)
{
	SetHandle(nil);
	outResult = 0;
	return true;
}

bool MWinProcMixin::WMChar(HWND inHWnd, UINT inUMsg, WPARAM inWParam, LPARAM inLParam, int& outResult)
{
	bool result = false;

	if (mSuppressNextWMChar)
		result = true;
	else
	{
		//HKeyDown keyDown;
		//
		//char txt[8] = "";
	
		//if (IsWindowUnicode(inHWnd))
		//	keyDown.text_length = static_cast<unsigned short>(
		//		HEncodingTraits<enc_UTF8>::
		//		InsertUnicode(txt, static_cast<HUnicode>(inWParam)));
		//else
		//{
		//	unsigned long s1 = 1, s2 = 8;
		//	char ch = static_cast<char>(inWParam);
		//
		//	HEncoder::FetchEncoder(enc_Native)->
		//		EncodeToUTF8(&ch, s1, txt, s2);
		//	
		//	keyDown.text_length = static_cast<unsigned short>(s2);
		//}
	
		//keyDown.text = txt;
		//keyDown.key_code = static_cast<unsigned short>(inWParam);
		//
		//GetModifierState(keyDown.modifiers, false);
		//if (inLParam & (1 << 24))
		//	keyDown.modifiers |= kNumPad;
		//
		//outResult = 1;
		//if (DispatchKeyDown(keyDown))
		//{
			result = true;
			outResult = 0;
		//}
	}
	
	return result;
}

bool MWinProcMixin::WMKeydown(HWND inHWnd, UINT inUMsg, WPARAM inWParam, LPARAM inLParam, int& outResult)
{
	//HKeyDown keyDown;
	//
	//keyDown.text = "";
	//keyDown.text_length = 0;
	//keyDown.key_code = 0;
	//
	//GetModifierState(keyDown.modifiers, false);
	//if (inLParam & (1 << 24))
	//	keyDown.modifiers |= kNumPad;
	//
	//switch (inWParam)
	//{
	//	case VK_HOME:
	//		keyDown.key_code = kHHomeKeyCode;
	//		keyDown.modifiers ^= kNumPad;
	//		break;
	//	case VK_ESCAPE:
	//		keyDown.key_code = kHEscapeKeyCode;
	//		break;
	//	case VK_END:
	//		keyDown.key_code = kHEndKeyCode;
	//		keyDown.modifiers ^= kNumPad;
	//		break;
	//	case VK_NEXT:
	//		keyDown.key_code = kHPageDownKeyCode;
	//		keyDown.modifiers ^= kNumPad;
	//		break;
	//	case VK_PRIOR:
	//		keyDown.key_code = kHPageUpKeyCode;
	//		keyDown.modifiers ^= kNumPad;
	//		break;
	//	case VK_LEFT:
	//		keyDown.key_code = kHLeftArrowKeyCode;
	//		keyDown.modifiers ^= kNumPad;
	//		break;
	//	case VK_RIGHT:
	//		keyDown.key_code = kHRightArrowKeyCode;
	//		keyDown.modifiers ^= kNumPad;
	//		break;
	//	case VK_UP:
	//		keyDown.key_code = kHUpArrowKeyCode;
	//		keyDown.modifiers ^= kNumPad;
	//		break;
	//	case VK_DOWN:
	//		keyDown.key_code = kHDownArrowKeyCode;
	//		keyDown.modifiers ^= kNumPad;
	//		break;
	//	case VK_DELETE:
	//		keyDown.key_code = kHDeleteKeyCode;
	//		keyDown.modifiers ^= kNumPad;
	//		break;
	//	case VK_INSERT:
	//		keyDown.key_code = kHHelpKeyCode;
	//		keyDown.modifiers ^= kNumPad;
	//		break;
	//	case VK_CONTROL:
	//	case VK_SHIFT:
	//		keyDown.key_code = 0;
	//		break;
	//	case VK_TAB:
	//		if (keyDown.modifiers & kControlKey)
	//			keyDown.key_code = kHTabKeyCode;
	//		break;
	//	case VK_BACK:
	//		if (keyDown.modifiers & kControlKey)
	//			keyDown.key_code = kHBackspaceKeyCode;
	//		break;
	//	case VK_RETURN:
	//		if (keyDown.modifiers & kControlKey)
	//			keyDown.key_code = kHReturnKeyCode;
	//		break;
	//	default:
	//		if (inWParam >= VK_F1 && inWParam <= VK_F24)
	//			keyDown.key_code = static_cast<unsigned short>(
	//				0x0101 + inWParam - VK_F1);
	//		else if (IsWindowUnicode(inHWnd))
	//			keyDown.key_code = static_cast<unsigned short>(
	//				::MapVirtualKeyW(inWParam, 2));
	//		else
	//			keyDown.key_code = static_cast<unsigned short>(
	//				::MapVirtualKeyA(inWParam, 2));
	//		break;
	//}
	
	bool result = false;
	outResult = 1;
	mSuppressNextWMChar = true;
	
	//if (keyDown.key_code != 0 && DispatchKeyDown(keyDown))
	//{
	//	result = true;
	//	outResult = 0;
	//}
	//else
	//	fSuppressNextWMChar = false;
	
	return result;
}

//bool HWinProcMixin::DispatchKeyDown(const HKeyDown& inKeyDown)
//{
//	bool result = false;
//	if (HHandler::GetGrabbingHandler())
//		result = HHandler::GetGrabbingHandler()->KeyDown(inKeyDown);
//	else if (HHandler::GetFocus())
//		result = HHandler::GetFocus()->KeyDown(inKeyDown);
//	return result;
//}

int	MWinProcMixin::WinProc(HWND inHandle, UINT inMsg, WPARAM inWParam, LPARAM inLParam)
{
	int result = 0;
	MHandlerTable::iterator i = mHandlers.find(inMsg);
	if (i == mHandlers.end() or not (this->*(i->second))(inHandle, inMsg, inWParam, inLParam, result))
	{
		if (mOldWinProc != nil and mOldWinProc != &MWinProcMixin::WinProcCallBack)
			result = ::CallWindowProcW(mOldWinProc, inHandle, inMsg, inWParam, inLParam);
		else
			result = DefProc(inHandle, inMsg, inWParam, inLParam);
	}
	return result;
}

int MWinProcMixin::DefProc(HWND inHWnd, UINT inUMsg, WPARAM inWParam, LPARAM inLParam)
{
	return ::DefWindowProcW(inHWnd, inUMsg, inWParam, inLParam);
}

LRESULT CALLBACK MWinProcMixin::WinProcCallBack(HWND hwnd, UINT uMsg,
	WPARAM wParam, LPARAM lParam)
{
	int result = 1;
	
//	if (uMsg == WM_QUERYENDSESSION)
//		return 0;
//
	try
	{
		if (uMsg == WM_CREATE)
		{
			CREATESTRUCTW* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
			MWinProcMixin* impl = reinterpret_cast<MWinProcMixin*>(cs->lpCreateParams);
			impl->SetHandle(hwnd);
			result = 0;
		}
		else
		{
			MWinProcMixin* impl = MWinProcMixin::Fetch(hwnd);
			if (impl != nil and impl->mHandle == hwnd)
				result = impl->WinProc(hwnd, uMsg, wParam, lParam);
			else
				result = ::DefWindowProcW(hwnd, uMsg, wParam, lParam);
		}
	}
	catch (std::exception& e)
	{
		DisplayError(e);
	}
	catch (...)
	{
		DisplayError(MException("ouch"));
	}
	return result;
}

/*

Just a help when debugging! Cannot find it in the MSDN help :)

#define WM_NULL                         0x0000
#define WM_CREATE                       0x0001
#define WM_DESTROY                      0x0002
#define WM_MOVE                         0x0003
#define WM_SIZE                         0x0005
#define WM_ACTIVATE                     0x0006
#define WM_SETFOCUS                     0x0007
#define WM_KILLFOCUS                    0x0008
#define WM_ENABLE                       0x000A
#define WM_SETREDRAW                    0x000B
#define WM_SETTEXT                      0x000C
#define WM_GETTEXT                      0x000D
#define WM_GETTEXTLENGTH                0x000E
#define WM_PAINT                        0x000F
#define WM_CLOSE                        0x0010
#define WM_QUERYENDSESSION              0x0011
#define WM_QUIT                         0x0012
#define WM_QUERYOPEN                    0x0013
#define WM_ERASEBKGND                   0x0014
#define WM_SYSCOLORCHANGE               0x0015
#define WM_ENDSESSION                   0x0016
#define WM_SHOWWINDOW                   0x0018
#define WM_WININICHANGE                 0x001A
#define WM_SETTINGCHANGE                WM_WININICHANGE
#define WM_DEVMODECHANGE                0x001B
#define WM_ACTIVATEAPP                  0x001C
#define WM_FONTCHANGE                   0x001D
#define WM_TIMECHANGE                   0x001E
#define WM_CANCELMODE                   0x001F
#define WM_SETCURSOR                    0x0020
#define WM_MOUSEACTIVATE                0x0021
#define WM_CHILDACTIVATE                0x0022
#define WM_QUEUESYNC                    0x0023
#define WM_GETMINMAXINFO                0x0024
#define WM_PAINTICON                    0x0026
#define WM_ICONERASEBKGND               0x0027
#define WM_NEXTDLGCTL                   0x0028
#define WM_SPOOLERSTATUS                0x002A
#define WM_DRAWITEM                     0x002B
#define WM_MEASUREITEM                  0x002C
#define WM_DELETEITEM                   0x002D
#define WM_VKEYTOITEM                   0x002E
#define WM_CHARTOITEM                   0x002F
#define WM_SETFONT                      0x0030
#define WM_GETFONT                      0x0031
#define WM_SETHOTKEY                    0x0032
#define WM_GETHOTKEY                    0x0033
#define WM_QUERYDRAGICON                0x0037
#define WM_COMPAREITEM                  0x0039
#define WM_GETOBJECT                    0x003D
#define WM_COMPACTING                   0x0041
#define WM_COMMNOTIFY                   0x0044  
#define WM_WINDOWPOSCHANGING            0x0046
#define WM_WINDOWPOSCHANGED             0x0047
#define WM_POWER                        0x0048
#define WM_COPYDATA                     0x004A
#define WM_CANCELJOURNAL                0x004B
#define WM_NOTIFY                       0x004E
#define WM_INPUTLANGCHANGEREQUEST       0x0050
#define WM_INPUTLANGCHANGE              0x0051
#define WM_TCARD                        0x0052
#define WM_HELP                         0x0053
#define WM_USERCHANGED                  0x0054
#define WM_NOTIFYFORMAT                 0x0055
#define WM_CONTEXTMENU                  0x007B
#define WM_STYLECHANGING                0x007C
#define WM_STYLECHANGED                 0x007D
#define WM_DISPLAYCHANGE                0x007E
#define WM_GETICON                      0x007F
#define WM_SETICON                      0x0080
#define WM_NCCREATE                     0x0081
#define WM_NCDESTROY                    0x0082
#define WM_NCCALCSIZE                   0x0083
#define WM_NCHITTEST                    0x0084
#define WM_NCPAINT                      0x0085
#define WM_NCACTIVATE                   0x0086
#define WM_GETDLGCODE                   0x0087
#define WM_SYNCPAINT                    0x0088
#define WM_NCMOUSEMOVE                  0x00A0
#define WM_NCLBUTTONDOWN                0x00A1
#define WM_NCLBUTTONUP                  0x00A2
#define WM_NCLBUTTONDBLCLK              0x00A3
#define WM_NCRBUTTONDOWN                0x00A4
#define WM_NCRBUTTONUP                  0x00A5
#define WM_NCRBUTTONDBLCLK              0x00A6
#define WM_NCMBUTTONDOWN                0x00A7
#define WM_NCMBUTTONUP                  0x00A8
#define WM_NCMBUTTONDBLCLK              0x00A9
#define WM_KEYFIRST                     0x0100
#define WM_KEYDOWN                      0x0100
#define WM_KEYUP                        0x0101
#define WM_CHAR                         0x0102
#define WM_DEADCHAR                     0x0103
#define WM_SYSKEYDOWN                   0x0104
#define WM_SYSKEYUP                     0x0105
#define WM_SYSCHAR                      0x0106
#define WM_SYSDEADCHAR                  0x0107
#define WM_KEYLAST                      0x0108
#define WM_IME_STARTCOMPOSITION         0x010D
#define WM_IME_ENDCOMPOSITION           0x010E
#define WM_IME_COMPOSITION              0x010F
#define WM_IME_KEYLAST                  0x010F
#define WM_INITDIALOG                   0x0110
#define WM_COMMAND                      0x0111
#define WM_SYSCOMMAND                   0x0112
#define WM_TIMER                        0x0113
#define WM_HSCROLL                      0x0114
#define WM_VSCROLL                      0x0115
#define WM_INITMENU                     0x0116
#define WM_INITMENUPOPUP                0x0117
#define WM_MENUSELECT                   0x011F
#define WM_MENUCHAR                     0x0120
#define WM_ENTERIDLE                    0x0121
#define WM_MENURBUTTONUP                0x0122
#define WM_MENUDRAG                     0x0123
#define WM_MENUGETOBJECT                0x0124
#define WM_UNINITMENUPOPUP              0x0125
#define WM_MENUCOMMAND                  0x0126
#define WM_CTLCOLORMSGBOX               0x0132
#define WM_CTLCOLOREDIT                 0x0133
#define WM_CTLCOLORLISTBOX              0x0134
#define WM_CTLCOLORBTN                  0x0135
#define WM_CTLCOLORDLG                  0x0136
#define WM_CTLCOLORSCROLLBAR            0x0137
#define WM_CTLCOLORSTATIC               0x0138
#define WM_MOUSEFIRST                   0x0200
#define WM_MOUSEMOVE                    0x0200
#define WM_LBUTTONDOWN                  0x0201
#define WM_LBUTTONUP                    0x0202
#define WM_LBUTTONDBLCLK                0x0203
#define WM_RBUTTONDOWN                  0x0204
#define WM_RBUTTONUP                    0x0205
#define WM_RBUTTONDBLCLK                0x0206
#define WM_MBUTTONDOWN                  0x0207
#define WM_MBUTTONUP                    0x0208
#define WM_MBUTTONDBLCLK                0x0209
#define WM_MOUSEWHEEL                   0x020A
#define WM_MOUSELAST                    0x020A
#define WM_MOUSELAST                    0x0209
#define WM_PARENTNOTIFY                 0x0210
#define WM_ENTERMENULOOP                0x0211
#define WM_EXITMENULOOP                 0x0212
#define WM_NEXTMENU                     0x0213
#define WM_SIZING                       0x0214
#define WM_CAPTURECHANGED               0x0215
#define WM_MOVING                       0x0216
#define WM_POWERBROADCAST               0x0218      // r_winuser pbt
#define WM_DEVICECHANGE                 0x0219
#define WM_MDICREATE                    0x0220
#define WM_MDIDESTROY                   0x0221
#define WM_MDIACTIVATE                  0x0222
#define WM_MDIRESTORE                   0x0223
#define WM_MDINEXT                      0x0224
#define WM_MDIMAXIMIZE                  0x0225
#define WM_MDITILE                      0x0226
#define WM_MDICASCADE                   0x0227
#define WM_MDIICONARRANGE               0x0228
#define WM_MDIGETACTIVE                 0x0229
#define WM_MDISETMENU                   0x0230
#define WM_ENTERSIZEMOVE                0x0231
#define WM_EXITSIZEMOVE                 0x0232
#define WM_DROPFILES                    0x0233
#define WM_MDIREFRESHMENU               0x0234
#define WM_IME_SETCONTEXT               0x0281
#define WM_IME_NOTIFY                   0x0282
#define WM_IME_CONTROL                  0x0283
#define WM_IME_COMPOSITIONFULL          0x0284
#define WM_IME_SELECT                   0x0285
#define WM_IME_CHAR                     0x0286
#define WM_IME_REQUEST                  0x0288
#define WM_IME_KEYDOWN                  0x0290
#define WM_IME_KEYUP                    0x0291
#define WM_MOUSEHOVER                   0x02A1
#define WM_MOUSELEAVE                   0x02A3
#define WM_CUT                          0x0300
#define WM_COPY                         0x0301
#define WM_PASTE                        0x0302
#define WM_CLEAR                        0x0303
#define WM_UNDO                         0x0304
#define WM_RENDERFORMAT                 0x0305
#define WM_RENDERALLFORMATS             0x0306
#define WM_DESTROYCLIPBOARD             0x0307
#define WM_DRAWCLIPBOARD                0x0308
#define WM_PAINTCLIPBOARD               0x0309
#define WM_VSCROLLCLIPBOARD             0x030A
#define WM_SIZECLIPBOARD                0x030B
#define WM_ASKCBFORMATNAME              0x030C
#define WM_CHANGECBCHAIN                0x030D
#define WM_HSCROLLCLIPBOARD             0x030E
#define WM_QUERYNEWPALETTE              0x030F
#define WM_PALETTEISCHANGING            0x0310
#define WM_PALETTECHANGED               0x0311
#define WM_HOTKEY                       0x0312
#define WM_PRINT                        0x0317
#define WM_PRINTCLIENT                  0x0318
#define WM_HANDHELDFIRST                0x0358
#define WM_HANDHELDLAST                 0x035F
#define WM_AFXFIRST                     0x0360
#define WM_AFXLAST                      0x037F
#define WM_PENWINFIRST                  0x0380
#define WM_PENWINLAST                   0x038F

*/
