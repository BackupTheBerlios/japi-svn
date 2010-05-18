//          Copyright Maarten L. Hekkelman 2006-2010
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef MWINMENU_H
#define MWINMENU_H

#include "MWinProcMixin.h"
class MWinWindowImpl;
class MMenu;

class MWinMenubar : public MWinProcMixin
{
  public:
					MWinMenubar(
						MWinWindowImpl*	inWindowImpl,
						const char*		inMenuResource);

					~MWinMenubar();

  protected:
	virtual void	CreateParams(DWORD& outStyle, DWORD& outExStyle,
						std::wstring& outClassName, HMENU& outMenu);
	virtual void	RegisterParams(UINT& outStyle, HCURSOR& outCursor,
						HICON& outIcon, HICON& outSmallIcon, HBRUSH& outBackground);

	bool			NDropDown(WPARAM inWParam, LPARAM inLParam, int& outResult);

	MWinWindowImpl*	mWindowImpl;
	std::vector<MMenu*>
					mMenus;
};

#endif