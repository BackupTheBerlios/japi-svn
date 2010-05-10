//          Copyright Maarten L. Hekkelman 2006-2010
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef MWINAPPLICATION_IMPL_H
#define MWINAPPLICATION_IMPL_H

#include "MApplicationImpl.h"

class MWinApplicationImpl : public MApplicationImpl
{
  public:
					MWinApplicationImpl(
						MApplication*		inApp);

	virtual			~MWinApplicationImpl();

	virtual int		RunEventLoop();
	virtual void	Quit();

  private:

	static MWinApplicationImpl*
					sInstance;

	static void CALLBACK
					Timer(HWND /*hwnd*/, UINT /*uMsg*/, UINT /*idEvent*/, DWORD /*dwTime*/);
};

#endif