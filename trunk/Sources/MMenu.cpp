/*
	Copyright (c) 2007, Maarten L. Hekkelman
	All rights reserved.

	Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions
	are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in
      the documentation and/or other materials provided with the
      distribution.
    * Neither the name of the Maarten L. Hekkelman nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
	"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
	LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
	FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
	COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
	INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
	(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
	SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
	HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
	STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
	ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
	OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "MJapieG.h"
#include <iostream>

#include <libxml/tree.h>
#include <libxml/parser.h>

#include "MCallbacks.h"
#include "MMenu.h"
#include "MWindow.h"
#include "MAcceleratorTable.h"
#include "MUrl.h"
#include "MStrings.h"
#include "MResources.h"
#include "MUtils.h"

using namespace std;

struct MCommandToString
{
	char mCommandString[10];
	
	MCommandToString(
		uint32			inCommand)
	{
		strcpy(mCommandString, "MCmd_xxxx");
		
		mCommandString[5] = ((inCommand & 0xff000000) >> 24) & 0x000000ff;
		mCommandString[6] = ((inCommand & 0x00ff0000) >> 16) & 0x000000ff;
		mCommandString[7] = ((inCommand & 0x0000ff00) >>  8) & 0x000000ff;
		mCommandString[8] = ((inCommand & 0x000000ff) >>  0) & 0x000000ff;
	}
	
	operator const char*() const	{ return mCommandString; }
};

struct MMenuItem
{
  public:
					MMenuItem(
						MMenu*			inMenu,
						const string&	inLabel,
						uint32			inCommand);

					// plain, simple item
	void			CreateWidget();
	
					// radio menu item
	void			CreateWidget(
						GSList*&		ioRadioGroup);

	void			SetChecked(
						bool			inChecked);

	void			ItemCallback();
	
	void			ItemToggled();
	
	void			RecentItemActivated();

	MSlot<void()>	mCallback;
	MSlot<void()>	mRecentItemActivated;

	GtkWidget*		mGtkMenuItem;
	string			mLabel;
	uint32			mCommand;
	uint32			mIndex;
	MMenu*			mMenu;
	MMenu*			mSubMenu;
	bool			mEnabled;
	bool			mCanCheck;
	bool			mChecked;
	bool			mInhibitCallBack;
};

MMenuItem::MMenuItem(
	MMenu*			inMenu,
	const string&	inLabel,
	uint32			inCommand)
	: mCallback(this, &MMenuItem::ItemCallback)
	, mRecentItemActivated(this, &MMenuItem::RecentItemActivated)
	, mGtkMenuItem(nil)
	, mLabel(inLabel)
	, mCommand(inCommand)
	, mIndex(0)
	, mMenu(inMenu)
	, mSubMenu(nil)
	, mEnabled(true)
	, mCanCheck(false)
	, mChecked(false)
	, mInhibitCallBack(false)
{
}

void MMenuItem::CreateWidget()
{
	if (mLabel == "-")
		mGtkMenuItem = gtk_separator_menu_item_new();
	else
	{
		mGtkMenuItem = gtk_menu_item_new_with_label(_(mLabel.c_str()));
		if (mCommand != 0)	
			mCallback.Connect(mGtkMenuItem, "activate");
	}
}

void MMenuItem::CreateWidget(
	GSList*&		ioRadioGroup)
{
	mGtkMenuItem = gtk_radio_menu_item_new_with_label(ioRadioGroup, _(mLabel.c_str()));

	ioRadioGroup = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(mGtkMenuItem));

	if (mCommand != 0)
		mCallback.Connect(mGtkMenuItem, "toggled");

	mCanCheck = true;
}

void MMenuItem::ItemCallback()
{
	try
	{
		if (mMenu != nil and
			mMenu->GetTarget() != nil and
			not mInhibitCallBack)
		{
			bool process = true;
			
			if (mCanCheck)
			{
				mChecked = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(mGtkMenuItem));
				process = mChecked;
			}

			if (process and not mMenu->GetTarget()->ProcessCommand(mCommand, mMenu, mIndex))
				PRINT(("Unhandled command: %s", (const char*)MCommandToString(mCommand)));
		}
	}
	catch (exception& e)
	{
		MError::DisplayError(e);
	}
	catch (...) {}
}

void MMenuItem::SetChecked(
	bool			inChecked)
{
	if (inChecked != mChecked)
	{
		mInhibitCallBack = true;
		mChecked = inChecked;
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(mGtkMenuItem), mChecked);
		mInhibitCallBack = false;
	}
}

void MMenuItem::RecentItemActivated()
{
	assert(mSubMenu);
	assert(GTK_IS_RECENT_CHOOSER(mSubMenu->GetGtkMenu()));	
	
	char* uri = gtk_recent_chooser_get_current_uri(GTK_RECENT_CHOOSER(mSubMenu->GetGtkMenu()));
	
	if (uri != nil)
	{
		MUrl url(uri);

		g_free(uri);

		try
		{
			gApp->OpenOneDocument(url);
		}
		catch (exception& e)
		{
			MError::DisplayError(e);
		}
		catch (...) {}
	}
}

// --------------------------------------------------------------------

MMenu::MMenu(
	const string&	inLabel,
	GtkWidget*		inMenuWidget)
	: mOnDestroy(this, &MMenu::OnDestroy)
	, mOnSelectionDone(this, &MMenu::OnSelectionDone)
	, mGtkMenu(inMenuWidget)
	, mLabel(inLabel)
	, mTarget(nil)
	, mRadioGroup(nil)
	, mPopupX(-1)
	, mPopupY(-1)
{
	if (mGtkMenu == nil)
		mGtkMenu = gtk_menu_new();

	mOnDestroy.Connect(mGtkMenu, "destroy");
}

MMenu::~MMenu()
{
	for (MMenuItemList::iterator mi = mItems.begin(); mi != mItems.end(); ++mi)
		delete *mi;
}

bool MMenu::IsRecentMenu() const
{
	return GTK_IS_RECENT_CHOOSER_MENU(mGtkMenu);
}

MMenuItem* MMenu::CreateNewItem(
	const string&	inLabel,
	uint32			inCommand,
	GSList**		ioRadioGroup)
{
	MMenuItem* item = new MMenuItem(this, inLabel, inCommand);

	if (ioRadioGroup != nil)
		item->CreateWidget(*ioRadioGroup);
	else
	{
		item->CreateWidget();
		
		if (inLabel != "-")
			mRadioGroup = nil;
	}

	item->mIndex = mItems.size();
	mItems.push_back(item);
	gtk_menu_shell_append(GTK_MENU_SHELL(mGtkMenu), item->mGtkMenuItem);
	
	return item;
}

void MMenu::AppendItem(
	const string&	inLabel,
	uint32			inCommand)
{
	CreateNewItem(inLabel, inCommand, nil);
}

void MMenu::AppendCheckItem(
	const string&	inLabel,
	uint32			inCommand)
{
	CreateNewItem(inLabel, inCommand, &mRadioGroup);
}

void MMenu::AppendSeparator()
{
	CreateNewItem("-", 0, nil);
}

void MMenu::AppendMenu(
	MMenu*			inMenu)
{
	MMenuItem* item = CreateNewItem(inMenu->GetLabel(), 0, nil);
	item->mSubMenu = inMenu;

	if (inMenu->IsRecentMenu())
		item->mRecentItemActivated.Connect(inMenu->GetGtkMenu(), "item-activated");

	gtk_menu_item_set_submenu(GTK_MENU_ITEM(item->mGtkMenuItem), inMenu->mGtkMenu);
}

//void MMenu::AppendRecentMenu(
//	const string&	inLabel)
//{
//	MMenuItem* item = CreateNewItem(inLabel, 0, nil);
//	
//	GtkWidget* recMenu = gtk_recent_chooser_menu_new_for_manager(gApp->GetRecentMgr());
//	item->mSubMenu = new MMenu(inLabel, recMenu);;
//
//	gtk_menu_item_set_submenu(GTK_MENU_ITEM(item->mGtkMenuItem), recMenu);
//	item->mRecentItemActivated.Connect(recMenu, "item-activated");
//}
//
uint32 MMenu::CountItems()
{
	return mItems.size();
}

void MMenu::RemoveItems(
	uint32			inFromIndex,
	uint32			inCount)
{
	if (inFromIndex < mItems.size())
	{
		MMenuItemList::iterator b = mItems.begin() + inFromIndex;

		if (inFromIndex + inCount > mItems.size())
			inCount = mItems.size() - inFromIndex;

		MMenuItemList::iterator e = b + inCount;	
		
		for (MMenuItemList::iterator mi = b; mi != e; ++mi)
			gtk_widget_destroy((*mi)->mGtkMenuItem);
		
		mItems.erase(b, e);
	}
}

string MMenu::GetItemLabel(
	uint32				inIndex) const
{
	if (inIndex >= mItems.size())
		THROW(("Item index out of range"));
	
	return mItems[inIndex]->mLabel;
}

void MMenu::SetTarget(
	MHandler*		inTarget)
{
	mTarget = inTarget;
	
	for (MMenuItemList::iterator mi = mItems.begin(); mi != mItems.end(); ++mi)
	{
		if ((*mi)->mSubMenu != nil)
			(*mi)->mSubMenu->SetTarget(inTarget);
	}
}

void MMenu::UpdateCommandStatus()
{
	if (mTarget == nil)
		return;
	
	for (MMenuItemList::iterator mi = mItems.begin(); mi != mItems.end(); ++mi)
	{
		MMenuItem* item = *mi;
		
		if (item->mCommand != 0)
		{
			bool enabled = item->mEnabled;
			bool checked = item->mChecked;
			
			if (mTarget->UpdateCommandStatus(item->mCommand, this,
				mi - mItems.begin(), enabled, checked))
			{
				if (enabled != item->mEnabled)
				{
					gtk_widget_set_sensitive(item->mGtkMenuItem, enabled);
					item->mEnabled = enabled;
				}
				
				
				if (item->mCanCheck)
					item->SetChecked(checked);
			}
		}
		
		if ((*mi)->mSubMenu != nil)
			(*mi)->mSubMenu->UpdateCommandStatus();
	}
}

void MMenu::SetAcceleratorGroup(
	GtkAccelGroup*		inAcceleratorGroup)
{
	MAcceleratorTable& at = MAcceleratorTable::Instance();
	
	gtk_menu_set_accel_group(GTK_MENU(mGtkMenu), inAcceleratorGroup);
	
	for (MMenuItemList::iterator mi = mItems.begin(); mi != mItems.end(); ++mi)
	{
		MMenuItem* item = *mi;
		
		uint32 key, mod;
		
		if (at.GetAcceleratorKeyForCommand(item->mCommand, key, mod))
		{
			gtk_widget_add_accelerator(item->mGtkMenuItem, "activate", inAcceleratorGroup,
				key, GdkModifierType(mod), GTK_ACCEL_VISIBLE);
		}
		
		if (item->mSubMenu != nil)
			item->mSubMenu->SetAcceleratorGroup(inAcceleratorGroup);
	}
}

void MMenu::MenuPosition(
	GtkMenu*			inMenu,
	gint*				inX,
	gint*				inY,
	gboolean*			inPushIn,
	gpointer			inUserData)
{
	MMenu* self = reinterpret_cast<MMenu*>(g_object_get_data(G_OBJECT(inMenu), "MMenu"));	
	
	*inX = self->mPopupX;
	*inY = self->mPopupY;
	*inPushIn = true;
}

void MMenu::Popup(
	MHandler*			inHandler,
	GdkEventButton*		inEvent,
	int32				inX,
	int32				inY,
	bool				inBottomMenu)
{
	SetTarget(inHandler);
	
	mOnSelectionDone.Connect(mGtkMenu, "selection-done");
	
	mPopupX = inX;
	mPopupY = inY;
	
	g_object_set_data(G_OBJECT(mGtkMenu), "MMenu", this);

	gtk_widget_show_all(mGtkMenu);

	gtk_menu_popup(GTK_MENU(mGtkMenu), nil, nil,
		&MMenu::MenuPosition, nil, inEvent->button, inEvent->time);
}

bool MMenu::OnDestroy()
{
	mGtkMenu = nil;
	
	delete this;
	
	return false;
}

void MMenu::OnSelectionDone()
{
	gtk_widget_destroy(mGtkMenu);
}

// --------------------------------------------------------------------

MMenubar::MMenubar(
	MHandler*		inTarget,
	GtkWidget*		inContainer,
	GtkWidget*		inWindow)
	: mOnButtonPressEvent(this, &MMenubar::OnButtonPress)
	, mTarget(inTarget)
	, mWindowMenu(nil)
	, mTemplateMenu(nil)
{
	mGtkMenubar = gtk_menu_bar_new();
	mGtkAccel = gtk_accel_group_new();

	gtk_box_pack_start(GTK_BOX(inContainer), mGtkMenubar, false, false, 0);
	gtk_widget_show_all(mGtkMenubar);
	
	mOnButtonPressEvent.Connect(mGtkMenubar, "button-press-event");
//	gtk_window_add_accel_group(GTK_WINDOW(inWindow), mGtkAccel);
}

void MMenubar::BuildFromResource(
	const char*		inResourceName)
{
	const char* xml;
	uint32 size;
	
	if (not LoadResource(inResourceName, xml, size))
		THROW(("Menu resource not found: %s", inResourceName));
	
	xmlDocPtr			xmlDoc = nil;
	
	xmlInitParser();

	try
	{
		xmlDoc = xmlParseMemory(xml, size);
		if (xmlDoc == nil or xmlDoc->children == nil)
			THROW(("Failed to parse project file"));
		
		// build a menu
		
		XMLNode node(xmlDoc->children);
		if (node.name() == "menubar")
		{
			for (XMLNode::iterator menu = node.begin(); menu != node.end(); ++menu)
			{
				if (menu->name() == "menu")
				{
					MMenu* obj = CreateMenu(*menu);
					AddMenu(obj);
				}
			}
		}

		xmlFreeDoc(xmlDoc);
	}
	catch (...)
	{
		if (xmlDoc != nil)
			xmlFreeDoc(xmlDoc);
		
		xmlCleanupParser();
		throw;
	}
	
	xmlCleanupParser();
}

MMenu* MMenubar::CreateMenu(
	XMLNode&		inXMLNode)
{
	string label = inXMLNode.property("label");
	if (label.length() == 0)
		THROW(("Invalid menu specification, label is missing"));
	
	string special = inXMLNode.property("special");

	MMenu* menu;

	if (special == "recent")
		menu = new MMenu(label, gtk_recent_chooser_menu_new_for_manager(gApp->GetRecentMgr()));
	else
	{
		menu = new MMenu(label);
		
		for (XMLNode::iterator item = inXMLNode.begin(); item != inXMLNode.end(); ++item)
		{
			if (item->name() == "item")
			{
				label = item->property("label");
				
				if (label == "-")
					menu->AppendSeparator();
				else
				{
					string cs = item->property("cmd").c_str();
	
					if (cs.length() != 4)
						THROW(("Invalid menu item specification, cmd is not correct"));
					
					uint32 cmd = 0;
					for (int i = 0; i < 4; ++i)
						cmd |= cs[i] << ((3 - i) * 8);
					
					if (item->property("check") == "radio")
						menu->AppendCheckItem(label, cmd);
					else
						menu->AppendItem(label, cmd);
				}
			}
			else if (item->name() == "menu")
				menu->AppendMenu(CreateMenu(*item));
		}
	}
	
	if (special == "window")
		mWindowMenu = menu;
	else if (special == "template")
		mTemplateMenu = menu;
	
	return menu;
}

void MMenubar::AddMenu(
	MMenu*				inMenu)
{
	GtkWidget* menuItem = gtk_menu_item_new_with_label(_(inMenu->GetLabel().c_str()));
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menuItem), inMenu->GetGtkMenu());
	gtk_menu_shell_append(GTK_MENU_SHELL(mGtkMenubar), menuItem);
	inMenu->SetTarget(mTarget);
	inMenu->SetAcceleratorGroup(mGtkAccel);
	mMenus.push_back(inMenu);
}

bool MMenubar::OnButtonPress(
	GdkEventButton*		inEvent)
{
	for (list<MMenu*>::iterator m = mMenus.begin(); m != mMenus.end(); ++m)
		(*m)->UpdateCommandStatus();

	if (mWindowMenu != nil)
		gApp->UpdateWindowMenu(mWindowMenu);
	
	if (mTemplateMenu != nil)
		gApp->UpdateTemplateMenu(mTemplateMenu);

	gtk_widget_show_all(mGtkMenubar);

	return false;
}
