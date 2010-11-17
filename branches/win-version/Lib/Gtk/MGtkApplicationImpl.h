//          Copyright Maarten L. Hekkelman 2006-2010
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef MGTKAPPLICATION_IMPL_H
#define MGTKAPPLICATION_IMPL_H

#include "MApplicationImpl.h"

class MGtkApplicationImpl : public MApplicationImpl
{
  public:
					MGtkApplicationImpl();
	virtual			~MGtkApplicationImpl();

	static MGtkApplicationImpl*
					GetInstance()				{ return sInstance; }

	virtual int		RunEventLoop();
	virtual void	Quit();

	void			Initialise();

  private:

	HINSTANCE		mInstance;
	static MGtkApplicationImpl*
					sInstance;
};

#endif
