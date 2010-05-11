//          Copyright Maarten L. Hekkelman 2006-2010
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef MWINDOWIMPL_H
#define MWINDOWIMPL_H

class MWindow;

class MWindowImpl
{
  public:
				MWindowImpl(MWindow* inWindow)
					: mWindow(inWindow) {}

	virtual		~MWindowImpl();

  protected:
	MWindow*	mWindow;
};

#endif