//          Copyright Maarten L. Hekkelman 2006-2010
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <Windows.h>
#include <Commctrl.h>

#include "MLib.h"
#include "MUtils.h"

#include "MWinApplicationImpl.h"

#ifdef UNICODE
#if defined _M_IX86
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_IA64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='ia64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif
#endif

MWinApplicationImpl* MWinApplicationImpl::sInstance;


MApplicationImpl* MApplicationImpl::Create(
	MApplication* inApp)
{
	return new MWinApplicationImpl(::GetModuleHandle(nil), inApp);
}

MWinApplicationImpl::MWinApplicationImpl(
	HINSTANCE			inInstance,
	MApplication*		inApp)
	: MApplicationImpl(inApp)
	, mInstance(inInstance)
{
	INITCOMMONCONTROLSEX info = {
		sizeof(INITCOMMONCONTROLSEX),

		//ICC_ANIMATE_CLASS |
		ICC_BAR_CLASSES |
		ICC_COOL_CLASSES |
		//ICC_DATE_CLASSES |
		//ICC_HOTKEY_CLASS |
		//ICC_INTERNET_CLASSES |
		//ICC_LINK_CLASS |
		ICC_LISTVIEW_CLASSES |
		ICC_NATIVEFNTCTL_CLASS |
		ICC_PAGESCROLLER_CLASS |
		ICC_PROGRESS_CLASS |
		ICC_STANDARD_CLASSES |
		ICC_TAB_CLASSES |
		ICC_TREEVIEW_CLASSES |
		ICC_UPDOWN_CLASS |
		ICC_USEREX_CLASSES |
		ICC_WIN95_CLASSES
	};

	::InitCommonControlsEx(&info);
}

MWinApplicationImpl::~MWinApplicationImpl()
{
}

int MWinApplicationImpl::RunEventLoop()
{
	sInstance = this;
	::SetTimer(NULL, 0, 100, &MWinApplicationImpl::Timer);

	MSG message;
	int result = 0;

	// Main message loop:
	for (;;)
	{
		result = ::GetMessageW (&message, NULL, 0, 0);
		if (result <= 0)
		{
			if (result < 0)
				result = message.wParam;
			break;
		}
		
		::TranslateMessage(&message);
		::DispatchMessageW(&message);
	}

	sInstance = nil;

	return result;
}

void MWinApplicationImpl::Quit()
{
	::PostQuitMessage(0);
}

void CALLBACK MWinApplicationImpl::Timer(
	HWND	hwnd,
	UINT	uMsg,
	UINT	idEvent,
	DWORD	dwTime)
{
	// dwTime is based on GetTickCount and GetTickCount
	// can wrap. So use our own system_time instead

	static bool sInTimer = false;
	if (not sInTimer)
	{
		sInTimer = true;
		try
		{
			if (sInstance != nil)
				sInstance->Pulse();
		}
		catch (...) {}
		sInTimer = false;
	}
}
