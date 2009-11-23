//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef MDOCCLOSEDNOTIFIER_H
#define MDOCCLOSEDNOTIFIER_H

class MDocClosedNotifier
{
  public:
						MDocClosedNotifier(
							int							inFD);
						
						MDocClosedNotifier(
							const MDocClosedNotifier&	inRHS);
	
	MDocClosedNotifier&	operator=(
							const MDocClosedNotifier&	inRHS);

						~MDocClosedNotifier();

	int					GetFD() const;

  private:

	struct MDocClosedNotifierImp*						mImpl;
};

#endif
