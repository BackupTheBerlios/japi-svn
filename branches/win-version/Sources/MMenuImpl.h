//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef MMENUIMPL_H
#define MMENUIMPL_H

class MMenuImpl
{
public:
						MMenuImpl() {}

	virtual				~MMenuImpl() {}

	virtual void		CreateNewItem(
							const std::string&	inLabel,
							uint32				inCommand) = 0;

	virtual void		Popup(
							MHandler*			inHandler,
							int32				inX,
							int32				inY,
							bool				inBottomMenu) = 0;

	static MMenuImpl*	Create();
};

#endif