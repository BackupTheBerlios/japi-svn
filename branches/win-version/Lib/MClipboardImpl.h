//          Copyright Maarten L. Hekkelman 2006-2010
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

/*	$Id: MClipboard.h 158 2007-05-28 10:08:18Z maarten $
	Copyright Maarten L. Hekkelman
	Created Wednesday July 21 2004 18:18:19
*/

#ifndef MCLIPBOARDIMPL_H
#define MCLIPBOARDIMPL_H

#include "MClipboard.h"

class MClipboardImpl
{
  public:
	
					MClipboardImpl(
						MClipboard*		inClipboard)
						: mClipboard(inClipboard) {}
	virtual			~MClipboardImpl() {}

	virtual void	LoadClipboardIfNeeded() = 0;
	virtual void	Reset() = 0;
	virtual void	Commit() = 0;

	static MClipboardImpl*
					Create(MClipboard* inClipboard);

  protected:
	MClipboard*		mClipboard;				
};

#endif
