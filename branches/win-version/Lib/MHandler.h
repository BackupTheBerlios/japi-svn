//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef MHANDLER_H
#define MHANDLER_H

#include <list>
#include <string>

class MMenu;

class MHandler
{
  public:
						MHandler(
							MHandler*		inSuper);

	virtual				~MHandler();

	virtual bool		UpdateCommandStatus(
							uint32			inCommand,
							MMenu*			inMenu,
							uint32			inItemIndex,
							bool&			outEnabled,
							bool&			outChecked);

	virtual bool		ProcessCommand(
							uint32			inCommand,
							const MMenu*	inMenu,
							uint32			inItemIndex,
							uint32			inModifiers);

	virtual bool		HandleKeydown(
							uint32			inKeyCode,
							uint32			inModifiers,
							const std::string&
											inText);

	void				SetSuper(
							MHandler*		inSuper);

  protected:
	
	MHandler*			mSuper;
};

#endif
