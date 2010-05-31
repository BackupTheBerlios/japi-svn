//          Copyright Maarten L. Hekkelman 2006-2010
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <Windows.h>
#include <CommCtrl.h>

#undef GetNextWindow
#undef CreateWindow

#include "MLib.h"

#include <fstream>
#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#define foreach BOOST_FOREACH

#include "zeep/xml/document.hpp"

#include "MWinMenu.h"
#include "MWinWindowImpl.h"
#include "MError.h"
#include "MWinUtils.h"

using namespace std;
using namespace zeep;

MWinMenuImpl::MWinMenuImpl(MMenu* inMenu, bool inPopup)
	: MMenuImpl(inMenu)
{
	if (inPopup)
		mMenuHandle = ::CreatePopupMenu();
	else
		mMenuHandle = ::CreateMenu();

	MENUINFO info = { sizeof(MENUINFO) };
	::GetMenuInfo(mMenuHandle, &info);
	info.fMask |= MIM_STYLE | MIM_MENUDATA;
	info.dwStyle |= MNS_NOTIFYBYPOS;
	info.dwMenuData = (ULONG_PTR)this;
	THROW_IF_WIN_ERROR(::SetMenuInfo(mMenuHandle, &info));
}

MWinMenuImpl::~MWinMenuImpl()
{
	::DestroyMenu(mMenuHandle);
}

void MWinMenuImpl::SetTarget(
	MHandler*		inHandler)
{
}

void MWinMenuImpl::SetItemState(
	uint32				inIndex,
	bool				inEnabled,
	bool				inChecked)
{
	MENUITEMINFOW info = { sizeof(MENUITEMINFOW), MIIM_STATE };
	THROW_IF_WIN_ERROR(::GetMenuItemInfoW(mMenuHandle, inIndex, true, &info));
	
	if (inEnabled and inChecked)
		info.fState = MFS_ENABLED | MFS_CHECKED;
	else if (inEnabled)
		info.fState = MFS_ENABLED | MFS_UNCHECKED;
	else if (inChecked)
		info.fState = MFS_DISABLED | MFS_CHECKED;
	else
		info.fState = MFS_DISABLED | MFS_UNCHECKED;

	THROW_IF_WIN_ERROR(::SetMenuItemInfoW(mMenuHandle, inIndex, true, &info));
}

void MWinMenuImpl::AppendItem(
	const string&	inLabel,
	uint32			inCommand)
{
	/* Add it to the fMenuHandle */
	MENUITEMINFOW lWinItem = { sizeof(MENUITEMINFOW) };
	wstring label = c2w(inLabel);
	
	lWinItem.fMask = MIIM_ID | MIIM_TYPE;
	lWinItem.fType = MFT_STRING;
	lWinItem.wID = inCommand;
	lWinItem.dwTypeData = (LPWSTR)label.c_str();

	THROW_IF_WIN_ERROR(::InsertMenuItemW(mMenuHandle, ::GetMenuItemCount(mMenuHandle) + 1, TRUE, &lWinItem));
}

void MWinMenuImpl::AppendSubmenu(
	MMenu*			inSubmenu)
{
	/* Add it to the fMenuHandle */
	MENUITEMINFOW lWinItem = { sizeof(MENUITEMINFOW) };
	wstring label = c2w(inSubmenu->GetLabel());
	
	lWinItem.fMask = MIIM_ID | MIIM_TYPE | MIIM_SUBMENU;
	lWinItem.fType = MFT_STRING;
	//lWinItem.wID = inCommand;
	lWinItem.hSubMenu = dynamic_cast<MWinMenuImpl*>(inSubmenu->impl())->GetHandle();
	lWinItem.dwTypeData = (LPWSTR)label.c_str();

	THROW_IF_WIN_ERROR(::InsertMenuItemW(mMenuHandle, ::GetMenuItemCount(mMenuHandle) + 1, TRUE, &lWinItem));
}

void MWinMenuImpl::AppendSeparator()
{
	/* Add it to the fMenuHandle */
	MENUITEMINFOW lWinItem = { sizeof(MENUITEMINFOW) };
	
	lWinItem.fMask = MIIM_TYPE;
	lWinItem.fType = MFT_SEPARATOR;

	THROW_IF_WIN_ERROR(::InsertMenuItemW(mMenuHandle, ::GetMenuItemCount(mMenuHandle) + 1, TRUE, &lWinItem));
}

void MWinMenuImpl::AppendCheckbox(
	const string&	inLabel,
	uint32			inCommand)
{
	/* Add it to the fMenuHandle */
	MENUITEMINFOW lWinItem = { sizeof(MENUITEMINFOW) };
	wstring label = c2w(inLabel);
	
	lWinItem.fMask = MIIM_ID | MIIM_TYPE | MIIM_CHECKMARKS;
	lWinItem.fType = MFT_STRING;
	lWinItem.wID = inCommand;
	lWinItem.dwTypeData = (LPWSTR)label.c_str();

	THROW_IF_WIN_ERROR(::InsertMenuItemW(mMenuHandle, ::GetMenuItemCount(mMenuHandle) + 1, TRUE, &lWinItem));
}

void MWinMenuImpl::AppendRadiobutton(
	const string&	inLabel,
	uint32			inCommand)
{
	/* Add it to the fMenuHandle */
	MENUITEMINFOW lWinItem = { sizeof(MENUITEMINFOW) };
	wstring label = c2w(inLabel);
	
	lWinItem.fMask = MIIM_ID | MIIM_TYPE | MIIM_CHECKMARKS;
	lWinItem.fType = MFT_STRING | MFT_RADIOCHECK;
	lWinItem.wID = inCommand;
	lWinItem.dwTypeData = (LPWSTR)label.c_str();

	THROW_IF_WIN_ERROR(::InsertMenuItemW(mMenuHandle, ::GetMenuItemCount(mMenuHandle) + 1, TRUE, &lWinItem));
}

uint32 MWinMenuImpl::CountItems() const
{
	return ::GetMenuItemCount(mMenuHandle);
}

string MWinMenuImpl::GetItemLabel(
	uint32			inIndex) const
{
	MENUITEMINFOW info = { sizeof(MENUITEMINFOW), MIIM_STRING };
	THROW_IF_WIN_ERROR(::GetMenuItemInfoW(mMenuHandle, inIndex, true, &info));

	vector<wchar_t> s(info.cch + 1);
	info.dwTypeData = &s[0];
	info.cch += 1;

	THROW_IF_WIN_ERROR(::GetMenuItemInfoW(mMenuHandle, inIndex, true, &info));
	
	return w2c(info.dwTypeData);
}

uint32 MWinMenuImpl::GetItemCommand(
	uint32			inIndex) const
{
	MENUITEMINFOW info = { sizeof(MENUITEMINFOW), MIIM_ID };
	::GetMenuItemInfoW(mMenuHandle, inIndex, true, &info);
	
	return info.wID;
}

MMenu* MWinMenuImpl::GetSubmenu(
	uint32			inIndex) const
{
	MENUITEMINFOW info = { sizeof(MENUITEMINFOW), MIIM_SUBMENU };
	THROW_IF_WIN_ERROR(::GetMenuItemInfoW(mMenuHandle, inIndex, true, &info));
	
	MMenu* result = nil;
	if (info.hSubMenu != nil)
		result = Lookup(info.hSubMenu);
	return result;
}

void MWinMenuImpl::Popup(
	MHandler*		inHandler,
	int32			inX,
	int32			inY,
	bool			inBottomMenu)
{
	MWindow* window = dynamic_cast<MWindow*>(inHandler);
	if (window != nil)
	{
		MWinWindowImpl* impl = dynamic_cast<MWinWindowImpl*>(window->GetImpl());

		::TrackPopupMenuEx(mMenuHandle, TPM_NONOTIFY, inX, inY, impl->GetHandle(), nil);
	}
}

MMenu* MWinMenuImpl::Lookup(
	HMENU			inMenuHandle)
{
	MENUINFO info = { sizeof(MENUINFO), MIM_MENUDATA };
	MMenu* result = nil;

	if (::GetMenuInfo(inMenuHandle, &info) and info.dwMenuData != 0)
		result = reinterpret_cast<MWinMenuImpl*>(info.dwMenuData)->mMenu;

	return result;
}

MMenuImpl* MMenuImpl::Create(MMenu* inMenu, bool inPopup)
{
	return new MWinMenuImpl(inMenu, inPopup);
}

//// --------------------------------------------------------------------
//
//MWinMenubar::MWinMenubar(MWinWindowImpl* inWindowImpl, const char* inMenuResource)
//	: mWindowImpl(inWindowImpl)
//{
//	MRect r(0, 0, 10, 10);
//	//mWindow->GetBounds(r);
//	Create(inWindowImpl, r, L"menubar");
//
//	inWindowImpl->AddNotify(TBN_DROPDOWN, GetHandle(), boost::bind(&MWinMenubar::NDropDown, this, _1, _2, _3));
//
//	vector<TBBUTTON> buttons;
//	list<wstring> labels;
//
//	//mrsrc::rsrc rsrc(string("Menus/") + inResourceName + ".xml");
//	//
//	//if (not rsrc)
//	//	THROW(("Menu resource not found: %s", inResourceName));
//
//	//io::stream<io::array_source> data(rsrc.data(), rsrc.size());
//	ifstream data("C:\\Users\\maarten\\projects\\japi\\Resources\\Menus\\" + string(inMenuResource) + ".xml");
//	xml::document doc(data);
//	
//	// build a menubar from the resource XML
//	foreach (xml::element* menu, doc.find("/menubar/menu"))
//	{
//		//MMenu* obj = CreateMenu(menu);
//
//		labels.push_back(c2w(menu->get_attribute("label")));
//
//		TBBUTTON btn = {
//			I_IMAGENONE,
//			buttons.size(),
//			TBSTATE_ENABLED,
//			BTNS_DROPDOWN | BTNS_AUTOSIZE,
//			{0},
//			0,
//			(INT_PTR)labels.back().c_str()
//		};
//
//		buttons.push_back(btn);
//
//		mMenus.push_back(MMenu::Create(menu));
//	}
//	
//    // Add buttons.
//    ::SendMessage(GetHandle(), TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
//    ::SendMessage(GetHandle(), TB_ADDBUTTONS, buttons.size(), (LPARAM)&buttons[0]);
//
//    // Tell the toolbar to resize itself, and show it.
//    ::SendMessage(GetHandle(), TB_AUTOSIZE, 0, 0);
//    ::ShowWindow(GetHandle(), TRUE);
//}
//
//MWinMenubar::~MWinMenubar()
//{
//	foreach (MMenu* menu, mMenus)
//		delete menu;
//}
//
//void MWinMenubar::CreateParams(DWORD& outStyle, DWORD& outExStyle,
//	wstring& outClassName, HMENU& outMenu)
//{
//	MWinProcMixin::CreateParams(outStyle, outExStyle, outClassName, outMenu);
//
//	outClassName = TOOLBARCLASSNAME;
//	outStyle = WS_CHILD | TBSTYLE_LIST | TBSTYLE_FLAT | CCS_NODIVIDER | TBSTYLE_WRAPABLE;
//}
//
//void MWinMenubar::RegisterParams(UINT& outStyle, HCURSOR& outCursor,
//	HICON& outIcon, HICON& outSmallIcon, HBRUSH& outBackground)
//{
//	MWinProcMixin::RegisterParams(outStyle, outCursor, outIcon, outSmallIcon, outBackground);
//}
//
//bool MWinMenubar::NDropDown(WPARAM inWParam, LPARAM inLParam, int& outResult)
//{
//	LPNMTOOLBAR toolbar = reinterpret_cast<LPNMTOOLBAR>(inLParam);
//
//	// Get the coordinates of the button.
//	RECT rc;
//	::SendMessage(GetHandle(), TB_GETRECT, (WPARAM)toolbar->iItem, (LPARAM)&rc);
//
//	// Convert to screen coordinates.            
//	::MapWindowPoints(GetHandle(), HWND_DESKTOP, (LPPOINT)&rc, 2);                         
//    
//	MMenu* menu = mMenus[toolbar->iItem];
//
//	if (menu != nil)
//		menu->Popup(mWindowImpl->GetWindow(), rc.left, rc.bottom, false);
//
//	outResult = 0;
//	return true;
//}
