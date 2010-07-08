//          Copyright Maarten L. Hekkelman 2006-2010
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "MWinLib.h"

#include <cstdarg>

#include "MUtils.h"
#include "MWinUtils.h"
#include "MError.h"
#include "MWinApplicationImpl.h"

using namespace std;

char* __S_FILE = "";
int __S_LINE;

double GetLocalTime()
{
	static double sDiff = -1.0;

	FILETIME tm;
	ULARGE_INTEGER li;
	
	if (sDiff == -1.0)
	{
		SYSTEMTIME st = { 0 };

		st.wDay = 1;
		st.wMonth = 1;
		st.wYear = 1970;

		if (::SystemTimeToFileTime(&st, &tm))
		{
			li.LowPart = tm.dwLowDateTime;
			li.HighPart = tm.dwHighDateTime;
		
			// Prevent Ping Pong comment. VC cannot convert UNSIGNED int64 to double. SIGNED is ok. (No more long)
			sDiff = static_cast<double>(static_cast<int64>(li.QuadPart));
			sDiff /= 1e7;
		}
	}	
	
	::GetSystemTimeAsFileTime(&tm);
	
	li.LowPart = tm.dwLowDateTime;
	li.HighPart = tm.dwHighDateTime;
	
	double result = static_cast<double>(static_cast<__int64> (li.QuadPart));
	result /= 1e7;
	return result - sDiff;
}

wstring c2w(const string& s)
{
	wstring result;
	result.reserve(s.length());

	for (string::const_iterator i = s.begin(); i != s.end(); ++i)
	{
		uint32 ch = static_cast<unsigned char>(*i);

		if (ch & 0x0080)
		{
			if ((ch & 0x0E0) == 0x0C0)
			{
				uint32 ch1 = static_cast<unsigned char>(*(i + 1));
				if ((ch1 & 0x0c0) == 0x080)
				{
					ch = ((ch & 0x01f) << 6) | (ch1 & 0x03f);
					i += 1;
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
				}
			}
		}

		if (ch <= 0x0FFFF)
			result += static_cast<wchar_t>(ch);
		else
		{
			wchar_t h = (ch - 0x010000) / 0x0400 + 0x0D800;
			wchar_t l = (ch - 0x010000) % 0x0400 + 0x0DC00;

			result += h;
			result += l;
		}
	}

	return result;
}

string w2c(const wstring& s)
{
	string result;
	result.reserve(s.length());

	for (wstring::const_iterator i = s.begin(); i != s.end(); ++i)
	{
		uint32 uc = static_cast<uint16>(*i);

		if (uc >= 0x0D800 and uc <= 0x0DBFF)
		{
			wchar_t ch = static_cast<uint16>(*(i + 1));
			if (ch >= 0x0DC00 and ch <= 0x0DFFF)
			{
				uc = (uc << 16) | ch;
				++i;
			}
		}

		if (uc < 0x080)
			result += (static_cast<char>(uc));
		else if (uc < 0x0800)
		{
			char ch[2] = {
				static_cast<char>(0x0c0 | (uc >> 6)),
				static_cast<char>(0x080 | (uc & 0x3f))
			};
			result.append(ch, 2);
		}
		else if (uc < 0x00010000)
		{
			char ch[3] = {
				static_cast<char>(0x0e0 | (uc >> 12)),
				static_cast<char>(0x080 | ((uc >> 6) & 0x3f)),
				static_cast<char>(0x080 | (uc & 0x3f))
			};
			result.append(ch, 3);
		}
		else
		{
			char ch[4] = {
				static_cast<char>(0x0f0 | (uc >> 18)),
				static_cast<char>(0x080 | ((uc >> 12) & 0x3f)),
				static_cast<char>(0x080 | ((uc >> 6) & 0x3f)),
				static_cast<char>(0x080 | (uc & 0x3f))
			};
			result.append(ch, 4);
		}
	}

	return result;
}

#ifndef NDEBUG

void __debug_printf(const char* inMessage, ...)
{
	char msg[1024] = {0};
	
	va_list vl;
	va_start(vl, inMessage);
	int n = vsnprintf(msg, sizeof(msg), inMessage, vl);
	va_end(vl);

	if (n < sizeof(msg) - 1 and msg[n] != '\n')
		msg[n] = '\n';

	_CrtDbgReport (_CRT_WARN, __S_FILE, __S_LINE, NULL, msg);
}

void __signal_throw(
	const char*		inCode,
	const char*		inFunction,
	const char*		inFile,
	int				inLine)
{
	stringstream s;
	s << "Throwing in file " << inFile << " line " << inLine
	  << " \"" << inFunction << "\": " << endl << inCode << endl;

	cerr << s.str();

	if (StOKToThrow::IsOK())
		return;

	wstring msg(c2w(s.str()));

	TaskDialog(nil, MWinApplicationImpl::GetInstance()->GetHInstance(),
		L"Japi - Exception Warning", L"An exception was thrown in Japi",
		msg.c_str(), 0, nil, nil);
}

#endif

MWinException::MWinException(
	int32				inHResult,
	const char*			inMsg,
	...)
{
	va_list vl;
	va_start(vl, inMsg);
	int n = vsnprintf(mMessage, sizeof(mMessage), inMsg, vl);
	va_end(vl);

	if (n < sizeof(mMessage))
		mMessage[n++] = ' ';

	wchar_t buffer[1024];

	int m = ::FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		nil, inHResult, 0, buffer, sizeof(buffer) / sizeof(wchar_t), nil);
	buffer[m] = 0;
	
	string err = w2c(buffer);
	copy(err.begin(), err.end(), mMessage + n);
	mMessage[n + m] = 0;
}

MWinException::MWinException(
	const char*			inMsg,
	...)
{
	DWORD error = ::GetLastError();

	va_list vl;
	va_start(vl, inMsg);
	int n = vsnprintf(mMessage, sizeof(mMessage), inMsg, vl);
	va_end(vl);

	if (n < sizeof(mMessage))
		mMessage[n++] = ' ';

	::FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		nil, error, 0, mMessage + n, sizeof(mMessage) - n, nil);
}

//void DisplayError(const exception& e)
//{
//	assert(false);
//}

string GetHomeDirectory()
{
	string result;
	wchar_t* path;

	HRESULT hr = ::SHGetKnownFolderPath(FOLDERID_Profile, 0, nil, &path);
	if (hr == S_OK)
	{
		result = w2c(path);
		::CoTaskMemFree(path);
	}

	return result;
}

void delay(double inSeconds)
{
	if (inSeconds > 0.0)
		::Sleep(static_cast<unsigned long>(inSeconds * 1000));
}

void GetModifierState(uint32& outModifiers, bool inAsync)
{
	outModifiers = 0;
	if (inAsync)
	{
		if (::GetAsyncKeyState(VK_SHIFT) & 0x8000)
			outModifiers |= kShiftKey;
		if (::GetAsyncKeyState(VK_CONTROL) & 0x8000)
			outModifiers |= kControlKey;
		if (::GetAsyncKeyState(VK_MENU) & 0x8000)
			outModifiers |= kOptionKey;
		if (::GetAsyncKeyState(VK_LWIN) & 0x8000 ||
			::GetAsyncKeyState(VK_RWIN) & 0x8000)
			outModifiers |= kCmdKey;
		if (::GetKeyState(VK_CAPITAL) & 0x0001)
			outModifiers |= kAlphaLock;
	}
	else
	{
		if (::GetKeyState(VK_SHIFT) & 0x8000)
			outModifiers |= kShiftKey;
		if (::GetKeyState(VK_CONTROL) & 0x8000)
			outModifiers |= kControlKey;
		if (::GetKeyState(VK_MENU) & 0x8000)
			outModifiers |= kOptionKey;
		if (::GetKeyState(VK_LWIN) & 0x8000 ||
			::GetKeyState(VK_RWIN) & 0x8000)
			outModifiers |= kCmdKey;
		if (::GetKeyState(VK_CAPITAL) & 0x0001)
			outModifiers |= kAlphaLock;
	}
}

double GetDblClickTime()
{
	return ::GetDoubleClickTime() / 1000.0;
}

#if DEBUG
struct MWinMsgDesc {
	const char*		mnemonic;
	int				msg;
} kWinMsgDesc[] = {

	{ "WM_NULL", WM_NULL },
	{ "WM_CREATE", WM_CREATE },
	{ "WM_DESTROY", WM_DESTROY },
	{ "WM_MOVE", WM_MOVE },
	{ "WM_SIZE", WM_SIZE },
	{ "WM_ACTIVATE", WM_ACTIVATE },
	{ "WM_SETFOCUS", WM_SETFOCUS },
	{ "WM_KILLFOCUS", WM_KILLFOCUS },
	{ "WM_ENABLE", WM_ENABLE },
	{ "WM_SETREDRAW", WM_SETREDRAW },
	{ "WM_SETTEXT", WM_SETTEXT },
	{ "WM_GETTEXT", WM_GETTEXT },
	{ "WM_GETTEXTLENGTH", WM_GETTEXTLENGTH },
	{ "WM_PAINT", WM_PAINT },
	{ "WM_CLOSE", WM_CLOSE },
	{ "WM_QUERYENDSESSION", WM_QUERYENDSESSION },
	{ "WM_QUERYOPEN", WM_QUERYOPEN },
	{ "WM_ENDSESSION", WM_ENDSESSION },
	{ "WM_QUIT", WM_QUIT },
	{ "WM_ERASEBKGND", WM_ERASEBKGND },
	{ "WM_SYSCOLORCHANGE", WM_SYSCOLORCHANGE },
	{ "WM_SHOWWINDOW", WM_SHOWWINDOW },
	{ "WM_WININICHANGE", WM_WININICHANGE },
	{ "WM_SETTINGCHANGE", WM_WININICHANGE },
	{ "WM_DEVMODECHANGE", WM_DEVMODECHANGE },
	{ "WM_ACTIVATEAPP", WM_ACTIVATEAPP },
	{ "WM_FONTCHANGE", WM_FONTCHANGE },
	{ "WM_TIMECHANGE", WM_TIMECHANGE },
	{ "WM_CANCELMODE", WM_CANCELMODE },
	{ "WM_SETCURSOR", WM_SETCURSOR },
	{ "WM_MOUSEACTIVATE", WM_MOUSEACTIVATE },
	{ "WM_CHILDACTIVATE", WM_CHILDACTIVATE },
	{ "WM_QUEUESYNC", WM_QUEUESYNC },
	{ "WM_GETMINMAXINFO", WM_GETMINMAXINFO },
	{ "WM_PAINTICON", WM_PAINTICON },
	{ "WM_ICONERASEBKGND", WM_ICONERASEBKGND },
	{ "WM_NEXTDLGCTL", WM_NEXTDLGCTL },
	{ "WM_SPOOLERSTATUS", WM_SPOOLERSTATUS },
	{ "WM_DRAWITEM", WM_DRAWITEM },
	{ "WM_MEASUREITEM", WM_MEASUREITEM },
	{ "WM_DELETEITEM", WM_DELETEITEM },
	{ "WM_VKEYTOITEM", WM_VKEYTOITEM },
	{ "WM_CHARTOITEM", WM_CHARTOITEM },
	{ "WM_SETFONT", WM_SETFONT },
	{ "WM_GETFONT", WM_GETFONT },
	{ "WM_SETHOTKEY", WM_SETHOTKEY },
	{ "WM_GETHOTKEY", WM_GETHOTKEY },
	{ "WM_QUERYDRAGICON", WM_QUERYDRAGICON },
	{ "WM_COMPAREITEM", WM_COMPAREITEM },
	{ "WM_GETOBJECT", WM_GETOBJECT },
	{ "WM_COMPACTING", WM_COMPACTING },
	{ "WM_COMMNOTIFY", WM_COMMNOTIFY },
	{ "WM_WINDOWPOSCHANGING", WM_WINDOWPOSCHANGING },
	{ "WM_WINDOWPOSCHANGED", WM_WINDOWPOSCHANGED },
	{ "WM_POWER", WM_POWER },
	{ "WM_COPYDATA", WM_COPYDATA },
	{ "WM_CANCELJOURNAL", WM_CANCELJOURNAL },
	{ "WM_NOTIFY", WM_NOTIFY },
	{ "WM_INPUTLANGCHANGEREQUEST", WM_INPUTLANGCHANGEREQUEST },
	{ "WM_INPUTLANGCHANGE", WM_INPUTLANGCHANGE },
	{ "WM_TCARD", WM_TCARD },
	{ "WM_HELP", WM_HELP },
	{ "WM_USERCHANGED", WM_USERCHANGED },
	{ "WM_NOTIFYFORMAT", WM_NOTIFYFORMAT },
	{ "WM_CONTEXTMENU", WM_CONTEXTMENU },
	{ "WM_STYLECHANGING", WM_STYLECHANGING },
	{ "WM_STYLECHANGED", WM_STYLECHANGED },
	{ "WM_DISPLAYCHANGE", WM_DISPLAYCHANGE },
	{ "WM_GETICON", WM_GETICON },
	{ "WM_SETICON", WM_SETICON },
	{ "WM_NCCREATE", WM_NCCREATE },
	{ "WM_NCDESTROY", WM_NCDESTROY },
	{ "WM_NCCALCSIZE", WM_NCCALCSIZE },
	{ "WM_NCHITTEST", WM_NCHITTEST },
	{ "WM_NCPAINT", WM_NCPAINT },
	{ "WM_NCACTIVATE", WM_NCACTIVATE },
	{ "WM_GETDLGCODE", WM_GETDLGCODE },
	{ "WM_SYNCPAINT", WM_SYNCPAINT },
	{ "WM_NCMOUSEMOVE", WM_NCMOUSEMOVE },
	{ "WM_NCLBUTTONDOWN", WM_NCLBUTTONDOWN },
	{ "WM_NCLBUTTONUP", WM_NCLBUTTONUP },
	{ "WM_NCLBUTTONDBLCLK", WM_NCLBUTTONDBLCLK },
	{ "WM_NCRBUTTONDOWN", WM_NCRBUTTONDOWN },
	{ "WM_NCRBUTTONUP", WM_NCRBUTTONUP },
	{ "WM_NCRBUTTONDBLCLK", WM_NCRBUTTONDBLCLK },
	{ "WM_NCMBUTTONDOWN", WM_NCMBUTTONDOWN },
	{ "WM_NCMBUTTONUP", WM_NCMBUTTONUP },
	{ "WM_NCMBUTTONDBLCLK", WM_NCMBUTTONDBLCLK },
	{ "WM_NCXBUTTONDOWN", WM_NCXBUTTONDOWN },
	{ "WM_NCXBUTTONUP", WM_NCXBUTTONUP },
	{ "WM_NCXBUTTONDBLCLK", WM_NCXBUTTONDBLCLK },
	{ "WM_INPUT_DEVICE_CHANGE", WM_INPUT_DEVICE_CHANGE },
	{ "WM_INPUT", WM_INPUT },
	{ "WM_KEYFIRST", WM_KEYFIRST },
	{ "WM_KEYDOWN", WM_KEYDOWN },
	{ "WM_KEYUP", WM_KEYUP },
	{ "WM_CHAR", WM_CHAR },
	{ "WM_DEADCHAR", WM_DEADCHAR },
	{ "WM_SYSKEYDOWN", WM_SYSKEYDOWN },
	{ "WM_SYSKEYUP", WM_SYSKEYUP },
	{ "WM_SYSCHAR", WM_SYSCHAR },
	{ "WM_SYSDEADCHAR", WM_SYSDEADCHAR },
	{ "WM_UNICHAR", WM_UNICHAR },
	{ "WM_KEYLAST", WM_KEYLAST },
	{ "WM_KEYLAST", WM_KEYLAST },
	{ "WM_IME_STARTCOMPOSITION", WM_IME_STARTCOMPOSITION },
	{ "WM_IME_ENDCOMPOSITION", WM_IME_ENDCOMPOSITION },
	{ "WM_IME_COMPOSITION", WM_IME_COMPOSITION },
	{ "WM_IME_KEYLAST", WM_IME_KEYLAST },
	{ "WM_INITDIALOG", WM_INITDIALOG },
	{ "WM_COMMAND", WM_COMMAND },
	{ "WM_SYSCOMMAND", WM_SYSCOMMAND },
	{ "WM_TIMER", WM_TIMER },
	{ "WM_HSCROLL", WM_HSCROLL },
	{ "WM_VSCROLL", WM_VSCROLL },
	{ "WM_INITMENU", WM_INITMENU },
	{ "WM_INITMENUPOPUP", WM_INITMENUPOPUP },
	{ "WM_MENUSELECT", WM_MENUSELECT },
	{ "WM_MENUCHAR", WM_MENUCHAR },
	{ "WM_ENTERIDLE", WM_ENTERIDLE },
	{ "WM_MENURBUTTONUP", WM_MENURBUTTONUP },
	{ "WM_MENUDRAG", WM_MENUDRAG },
	{ "WM_MENUGETOBJECT", WM_MENUGETOBJECT },
	{ "WM_UNINITMENUPOPUP", WM_UNINITMENUPOPUP },
	{ "WM_MENUCOMMAND", WM_MENUCOMMAND },
	{ "WM_CHANGEUISTATE", WM_CHANGEUISTATE },
	{ "WM_UPDATEUISTATE", WM_UPDATEUISTATE },
	{ "WM_QUERYUISTATE", WM_QUERYUISTATE },
	{ "WM_CTLCOLORMSGBOX", WM_CTLCOLORMSGBOX },
	{ "WM_CTLCOLOREDIT", WM_CTLCOLOREDIT },
	{ "WM_CTLCOLORLISTBOX", WM_CTLCOLORLISTBOX },
	{ "WM_CTLCOLORBTN", WM_CTLCOLORBTN },
	{ "WM_CTLCOLORDLG", WM_CTLCOLORDLG },
	{ "WM_CTLCOLORSCROLLBAR", WM_CTLCOLORSCROLLBAR },
	{ "WM_CTLCOLORSTATIC", WM_CTLCOLORSTATIC },
	{ "WM_MOUSEFIRST", WM_MOUSEFIRST },
	{ "WM_MOUSEMOVE", WM_MOUSEMOVE },
	{ "WM_LBUTTONDOWN", WM_LBUTTONDOWN },
	{ "WM_LBUTTONUP", WM_LBUTTONUP },
	{ "WM_LBUTTONDBLCLK", WM_LBUTTONDBLCLK },
	{ "WM_RBUTTONDOWN", WM_RBUTTONDOWN },
	{ "WM_RBUTTONUP", WM_RBUTTONUP },
	{ "WM_RBUTTONDBLCLK", WM_RBUTTONDBLCLK },
	{ "WM_MBUTTONDOWN", WM_MBUTTONDOWN },
	{ "WM_MBUTTONUP", WM_MBUTTONUP },
	{ "WM_MBUTTONDBLCLK", WM_MBUTTONDBLCLK },
	{ "WM_MOUSEWHEEL", WM_MOUSEWHEEL },
	{ "WM_XBUTTONDOWN", WM_XBUTTONDOWN },
	{ "WM_XBUTTONUP", WM_XBUTTONUP },
	{ "WM_XBUTTONDBLCLK", WM_XBUTTONDBLCLK },
	{ "WM_MOUSEHWHEEL", WM_MOUSEHWHEEL },
	{ "WM_MOUSELAST", WM_MOUSELAST },
	{ "WM_MOUSELAST", WM_MOUSELAST },
	{ "WM_MOUSELAST", WM_MOUSELAST },
	{ "WM_MOUSELAST", WM_MOUSELAST },
	{ "WM_PARENTNOTIFY", WM_PARENTNOTIFY },
	{ "WM_ENTERMENULOOP", WM_ENTERMENULOOP },
	{ "WM_EXITMENULOOP", WM_EXITMENULOOP },
	{ "WM_NEXTMENU", WM_NEXTMENU },
	{ "WM_SIZING", WM_SIZING },
	{ "WM_CAPTURECHANGED", WM_CAPTURECHANGED },
	{ "WM_MOVING", WM_MOVING },
	{ "WM_POWERBROADCAST", WM_POWERBROADCAST },
	{ "WM_DEVICECHANGE", WM_DEVICECHANGE },
	{ "WM_MDICREATE", WM_MDICREATE },
	{ "WM_MDIDESTROY", WM_MDIDESTROY },
	{ "WM_MDIACTIVATE", WM_MDIACTIVATE },
	{ "WM_MDIRESTORE", WM_MDIRESTORE },
	{ "WM_MDINEXT", WM_MDINEXT },
	{ "WM_MDIMAXIMIZE", WM_MDIMAXIMIZE },
	{ "WM_MDITILE", WM_MDITILE },
	{ "WM_MDICASCADE", WM_MDICASCADE },
	{ "WM_MDIICONARRANGE", WM_MDIICONARRANGE },
	{ "WM_MDIGETACTIVE", WM_MDIGETACTIVE },
	{ "WM_MDISETMENU", WM_MDISETMENU },
	{ "WM_ENTERSIZEMOVE", WM_ENTERSIZEMOVE },
	{ "WM_EXITSIZEMOVE", WM_EXITSIZEMOVE },
	{ "WM_DROPFILES", WM_DROPFILES },
	{ "WM_MDIREFRESHMENU", WM_MDIREFRESHMENU },
	{ "WM_IME_SETCONTEXT", WM_IME_SETCONTEXT },
	{ "WM_IME_NOTIFY", WM_IME_NOTIFY },
	{ "WM_IME_CONTROL", WM_IME_CONTROL },
	{ "WM_IME_COMPOSITIONFULL", WM_IME_COMPOSITIONFULL },
	{ "WM_IME_SELECT", WM_IME_SELECT },
	{ "WM_IME_CHAR", WM_IME_CHAR },
	{ "WM_IME_REQUEST", WM_IME_REQUEST },
	{ "WM_IME_KEYDOWN", WM_IME_KEYDOWN },
	{ "WM_IME_KEYUP", WM_IME_KEYUP },
	{ "WM_MOUSEHOVER", WM_MOUSEHOVER },
	{ "WM_MOUSELEAVE", WM_MOUSELEAVE },
	{ "WM_NCMOUSEHOVER", WM_NCMOUSEHOVER },
	{ "WM_NCMOUSELEAVE", WM_NCMOUSELEAVE },
	{ "WM_WTSSESSION_CHANGE", WM_WTSSESSION_CHANGE },
	{ "WM_TABLET_FIRST", WM_TABLET_FIRST },
	{ "WM_TABLET_LAST", WM_TABLET_LAST },
	{ "WM_CUT", WM_CUT },
	{ "WM_COPY", WM_COPY },
	{ "WM_PASTE", WM_PASTE },
	{ "WM_CLEAR", WM_CLEAR },
	{ "WM_UNDO", WM_UNDO },
	{ "WM_RENDERFORMAT", WM_RENDERFORMAT },
	{ "WM_RENDERALLFORMATS", WM_RENDERALLFORMATS },
	{ "WM_DESTROYCLIPBOARD", WM_DESTROYCLIPBOARD },
	{ "WM_DRAWCLIPBOARD", WM_DRAWCLIPBOARD },
	{ "WM_PAINTCLIPBOARD", WM_PAINTCLIPBOARD },
	{ "WM_VSCROLLCLIPBOARD", WM_VSCROLLCLIPBOARD },
	{ "WM_SIZECLIPBOARD", WM_SIZECLIPBOARD },
	{ "WM_ASKCBFORMATNAME", WM_ASKCBFORMATNAME },
	{ "WM_CHANGECBCHAIN", WM_CHANGECBCHAIN },
	{ "WM_HSCROLLCLIPBOARD", WM_HSCROLLCLIPBOARD },
	{ "WM_QUERYNEWPALETTE", WM_QUERYNEWPALETTE },
	{ "WM_PALETTEISCHANGING", WM_PALETTEISCHANGING },
	{ "WM_PALETTECHANGED", WM_PALETTECHANGED },
	{ "WM_HOTKEY", WM_HOTKEY },
	{ "WM_PRINT", WM_PRINT },
	{ "WM_PRINTCLIENT", WM_PRINTCLIENT },
	{ "WM_APPCOMMAND", WM_APPCOMMAND },
	{ "WM_THEMECHANGED", WM_THEMECHANGED },
	{ "WM_CLIPBOARDUPDATE", WM_CLIPBOARDUPDATE },
	{ "WM_DWMCOMPOSITIONCHANGED", WM_DWMCOMPOSITIONCHANGED },
	{ "WM_DWMNCRENDERINGCHANGED", WM_DWMNCRENDERINGCHANGED },
	{ "WM_DWMCOLORIZATIONCOLORCHANGED", WM_DWMCOLORIZATIONCOLORCHANGED },
	{ "WM_DWMWINDOWMAXIMIZEDCHANGE", WM_DWMWINDOWMAXIMIZEDCHANGE },
	{ "WM_GETTITLEBARINFOEX", WM_GETTITLEBARINFOEX },
	{ "WM_HANDHELDFIRST", WM_HANDHELDFIRST },
	{ "WM_HANDHELDLAST", WM_HANDHELDLAST },
	{ "WM_AFXFIRST", WM_AFXFIRST },
	{ "WM_AFXLAST", WM_AFXLAST },
	{ "WM_PENWINFIRST", WM_PENWINFIRST },
	{ "WM_PENWINLAST", WM_PENWINLAST },
	{ "WM_APP", WM_APP },
	{ "WM_USER", WM_USER },
};

void LogWinMsg(const char* inWhere, uint32 inMsg)
{
	MWinMsgDesc* d;
	for (d = kWinMsgDesc; d->mnemonic != nil; ++d)
	{
		if (d->msg == inMsg)
			break;
	}
	
	PRINT(("%s => %s, %d, 0x%8.8x", inWhere, d->mnemonic ? d->mnemonic : "undefined", inMsg, inMsg));
}

#endif
