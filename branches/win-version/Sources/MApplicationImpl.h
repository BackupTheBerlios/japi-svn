//          Copyright Maarten L. Hekkelman 2006-2010
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef MAPPLICATION_IMPL_H
#define MAPPLICATION_IMPL_H

#include "MApplication.h"

class MApplicationImpl
{
  public:
					MApplicationImpl(
						MApplication*		inApp);

	virtual			~MApplicationImpl();

	virtual int		RunEventLoop() = 0;

	virtual void	Quit() = 0;

	static MApplicationImpl*
					Create(
						MApplication* inApp);

  protected:
	MApplication*	mApp;
};

#endif