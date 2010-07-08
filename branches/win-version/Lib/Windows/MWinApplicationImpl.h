//          Copyright Maarten L. Hekkelman 2006-2010
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef MWINAPPLICATION_IMPL_H
#define MWINAPPLICATION_IMPL_H

#include "MApplicationImpl.h"
#include "MWinProcMixin.h"

class MWinApplicationImpl : public MApplicationImpl
{
  public:
					MWinApplicationImpl(
						HINSTANCE			inInstance);
	virtual			~MWinApplicationImpl();

	static MWinApplicationImpl*
					GetInstance()				{ return sInstance; }
	HINSTANCE		GetHInstance()				{ return mInstance; }

	virtual int		RunEventLoop();
	virtual void	Quit();

	void			Initialise();

  private:

	HINSTANCE		mInstance;
	static MWinApplicationImpl*
					sInstance;

	static void CALLBACK
					Timer(HWND hwnd, UINT uMsg, UINT idEvent, DWORD dwTime);
};

#endif