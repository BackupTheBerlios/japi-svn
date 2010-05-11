//          Copyright Maarten L. Hekkelman 2006-2010
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <Windows.h>

#include "MLib.h"
#include "MUtils.h"

#include "MWinApplicationImpl.h"

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

	if (sInstance != nil)
		sInstance->Pulse();
}
