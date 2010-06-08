//          Copyright Maarten L. Hekkelman 2006-2010
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef MWINMENU_H
#define MWINMENU_H

#include "MMenuImpl.h"

class MWinMenuImpl : public MMenuImpl
{
public:
						MWinMenuImpl(
							MMenu*				inMenu,
							bool				inPopup);

						~MWinMenuImpl();

	virtual void		SetTarget(
							MHandler*			inHandler);

	virtual void		SetItemState(
							uint32				inItem,
							bool				inEnabled,
							bool				inChecked);

	virtual void		AppendItem(
							const std::string&	inLabel,
							uint32				inCommand);

	virtual void		AppendSubmenu(
							MMenu*				inSubmenu);

	virtual void		AppendSeparator();

	virtual void		AppendCheckbox(
							const std::string&	inLabel,
							uint32				inCommand);

	virtual void		AppendRadiobutton(
							const std::string&	inLabel,
							uint32				inCommand);

	virtual uint32		CountItems() const;

	virtual void		RemoveItems(
							uint32				inFirstIndex,
							uint32				inCount);

	virtual std::string	GetItemLabel(
							uint32				inIndex) const;

	virtual uint32		GetItemCommand(
							uint32				inIndex) const;

	virtual MMenu*		GetSubmenu(
							uint32				inIndex) const;

	virtual void		Popup(
							MHandler*			inHandler,
							int32				inX,
							int32				inY,
							bool				inBottomMenu);

	HMENU				GetHandle() const		{ return mMenuHandle; }

	static MMenu*		Lookup(
							HMENU				inHandle);

private:
	HMENU				mMenuHandle;
};


//class MWinMenubar : public MWinProcMixin
//{
//  public:
//					MWinMenubar(
//						MWinWindowImpl*	inWindowImpl,
//						const char*		inMenuResource);
//
//					~MWinMenubar();
//
//  protected:
//	virtual void	CreateParams(DWORD& outStyle, DWORD& outExStyle,
//						std::wstring& outClassName, HMENU& outMenu);
//	virtual void	RegisterParams(UINT& outStyle, HCURSOR& outCursor,
//						HICON& outIcon, HICON& outSmallIcon, HBRUSH& outBackground);
//
//	bool			NDropDown(WPARAM inWParam, LPARAM inLParam, int& outResult);
//
//	MWinWindowImpl*	mWindowImpl;
//	std::vector<MMenu*>
//					mMenus;
//};

#endif