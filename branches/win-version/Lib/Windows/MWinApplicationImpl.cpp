//          Copyright Maarten L. Hekkelman 2006-2010
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "MWinLib.h"

#include "MUtils.h"

#include "MWinApplicationImpl.h"
#include "MWinUtils.h"
#include "MWinWindowImpl.h"
#include "MDialog.h"
#include "MError.h"
#include "MPreferences.h"

using namespace std;
namespace fs = boost::filesystem;

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

MWinApplicationImpl::MWinApplicationImpl(
	HINSTANCE			inInstance)
	: mInstance(inInstance)
{
	sInstance = this;
}

void MWinApplicationImpl::Initialise()
{
	wchar_t path[MAX_PATH] = {};

	if (::GetModuleFileName(NULL, path, MAX_PATH) > 0)
		gExecutablePath = w2c(path);

	try
	{
		PWSTR prefsPath;
		if (::SHGetKnownFolderPath(FOLDERID_LocalAppData, KF_FLAG_CREATE, NULL, &prefsPath) == S_OK and
			prefsPath != NULL)
		{
			string path = w2c(prefsPath);
			gPrefsDir = fs::path(path) / "Japi";
			::CoTaskMemFree(prefsPath);
		}

		if (gPrefsDir.is_complete() and not fs::exists(gPrefsDir))
		{
			fs::create_directories(gPrefsDir);
		}
	}
	catch (...) {}

    HRESULT hr = ::CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

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
	::SetTimer(NULL, 0, 100, &MWinApplicationImpl::Timer);

	MSG message;
	int result = 0;

	// Main message loop:
	for (;;)
	{
		result = ::GetMessage(&message, NULL, 0, 0);

//#if DEBUG
//		LogWinMsg("GetMessageW", message.message);
//#endif

		if (result <= 0)
		{
			if (result < 0)
				result = message.wParam;
			break;
		}

		HWND front = ::GetActiveWindow();
		if (front != nil)
		{
			MWinWindowImpl* impl = dynamic_cast<MWinWindowImpl*>(MWinProcMixin::Fetch(front));
			if (impl != nil and impl->IsDialogMessage(message))
				continue;
		}

		::TranslateMessage(&message);
		::DispatchMessage(&message);
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
			if (gApp != nil)
				gApp->Pulse();
		}
		catch (...) {}
		sInTimer = false;
	}
}
