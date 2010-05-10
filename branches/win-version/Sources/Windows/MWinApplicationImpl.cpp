//          Copyright Maarten L. Hekkelman 2006-2010
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <Windows.h>

#include "MLib.h"

#include "MWinApplicationImpl.h"

MWinApplicationImpl::MWinApplicationImpl(
	MApplication*		inApp)
	: MApplicationImpl(inApp)
{
}

MWinApplicationImpl::~MWinApplicationImpl()
{
}

int MWinApplicationImpl::RunEventLoop()
{
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
		
//		eEndOfEvent(true, this);
	}

	return result;
}

void MWinApplicationImpl::Quit()
{
	::PostQuitMessage(0);
}