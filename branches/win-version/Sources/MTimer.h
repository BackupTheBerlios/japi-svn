//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef MTIMER_H
#define MTIMER_H

#include <boost/noncopyable.hpp>

#include "MCallbacks.h"

class MTimeOut : boost::noncopyable
{
  public:
						MTimeOut();
						~MTimeOut();

	void				Start(
							double	inDurationInSeconds);

	void				Stop();

	MCallback<void()>	eTimedOut;

  private:
	struct MTimeOutImp*	mImpl;
};

#endif
