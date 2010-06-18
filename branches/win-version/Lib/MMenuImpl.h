//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef MMENUIMPL_H
#define MMENUIMPL_H

#include "MMenu.h"

class MMenuImpl
{
public:
						MMenuImpl(
							MMenu*				inMenu)
							: mMenu(inMenu) {}

	virtual				~MMenuImpl() {}

	virtual void		SetTarget(
							MHandler*			inHandler) = 0;

	virtual void		SetItemState(
							uint32				inItem,
							bool				inEnabled,
							bool				inChecked) = 0;

	virtual void		AppendItem(
							const std::string&	inLabel,
							uint32				inCommand) = 0;

	virtual void		AppendSubmenu(
							MMenu*				inSubmenu) = 0;

	virtual void		AppendSeparator() = 0;

	virtual void		AppendCheckbox(
							const std::string&	inLabel,
							uint32				inCommand) = 0;

	virtual void		AppendRadiobutton(
							const std::string&	inLabel,
							uint32				inCommand) = 0;

	virtual uint32		CountItems() const = 0;

	virtual void		RemoveItems(
							uint32				inFirstIndex,
							uint32				inCount) = 0;

	virtual std::string	GetItemLabel(
							uint32				inIndex) const = 0;

	virtual uint32		GetItemCommand(
							uint32				inIndex) const = 0;

	virtual MMenu*		GetSubmenu(
							uint32				inIndex) const = 0;

	virtual void		Popup(
							MWindow*			inHandler,
							int32				inX,
							int32				inY,
							bool				inBottomMenu) = 0;

	static MMenuImpl*	Create(
							MMenu*				inMenu,
							bool				inPopup);

protected:
	MMenu*				mMenu;
};

#endif