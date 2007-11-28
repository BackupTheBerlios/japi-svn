#include "MJapieG.h"
#include <iostream>

#include <libxml/tree.h>
#include <libxml/parser.h>
//#include <libxml/xpath.h>
//#include <libxml/xpathInternals.h>
//#include <libxml/xmlwriter.h>

#include "MCallbacks.h"
#include "MMenu.h"
#include "MWindow.h"
#include "MAcceleratorTable.h"
#include "MUrl.h"
#include "MStrings.h"
#include "MResources.h"

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

	void			ItemCallback();
	
	void			RecentItemActivated();

	MSlot<void()>	mCallback;
	MSlot<void()>	mRecentItemActivated;

	GtkWidget*		mGtkMenuItem;
	std::string		mLabel;
	uint32			mCommand;
	uint32			mIndex;
	MMenu*			mMenu;
	MMenu*			mSubMenu;
	bool			mEnabled;
	bool			mChecked;
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
	, mChecked(false)
{
	if (inLabel == "-")
		mGtkMenuItem = gtk_separator_menu_item_new();
	else
	{
		mGtkMenuItem = gtk_menu_item_new_with_label(mLabel.c_str());
		if (inCommand != 0)	
			mCallback.Connect(mGtkMenuItem, "activate");
	}

	gtk_widget_show(mGtkMenuItem);
}

void MMenuItem::ItemCallback()
{
	try
	{
		if (mMenu != nil and mMenu->GetTarget() != nil)
		{
			if (not mMenu->GetTarget()->ProcessCommand(mCommand, mMenu, mIndex))
				cout << "Unhandled command: " << MCommandToString(mCommand) << endl;
		}
	}
	catch (exception& e)
	{
		MError::DisplayError(e);
	}
	catch (...) {}
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

MMenuItem* MMenu::CreateNewItem(
	const string&	inLabel,
	uint32			inCommand)
{
	MMenuItem* item = new MMenuItem(this, inLabel, inCommand);

	item->mIndex = mItems.size();
	mItems.push_back(item);
	gtk_menu_shell_append(GTK_MENU_SHELL(mGtkMenu), item->mGtkMenuItem);
	
	return item;
}

void MMenu::AppendItem(
	const string&	inLabel,
	uint32			inCommand)
{
	CreateNewItem(inLabel, inCommand);
}

void MMenu::AppendSeparator()
{
	CreateNewItem("-", 0);
}

void MMenu::AppendMenu(
	MMenu*			inMenu)
{
	MMenuItem* item = CreateNewItem(inMenu->GetLabel(), 0);
	item->mSubMenu = inMenu;
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(item->mGtkMenuItem), inMenu->mGtkMenu);
}

void MMenu::AppendRecentMenu(
	const string&	inLabel)
{
	MMenuItem* item = CreateNewItem(inLabel, 0);
	
	GtkWidget* recMenu = gtk_recent_chooser_menu_new_for_manager(gApp->GetRecentMgr());
	item->mSubMenu = new MMenu(inLabel, recMenu);;

	gtk_menu_item_set_submenu(GTK_MENU_ITEM(item->mGtkMenuItem), recMenu);
	item->mRecentItemActivated.Connect(recMenu, "item-activated");
}

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
			
			if (mTarget->UpdateCommandStatus(item->mCommand, enabled, checked))
			{
				if (enabled != item->mEnabled)
				{
					gtk_widget_set_sensitive(item->mGtkMenuItem, enabled);
					item->mEnabled = enabled;
				}
			
//				if (enabled != item->mEnabled)
//				{
//					gtk_widget_set_sensitive(item->mGtkMenuItem, enabled);
//					item->mEnabled = enabled;
//				}
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
{
	mGtkMenubar = gtk_menu_bar_new();
	mGtkAccel = gtk_accel_group_new();

	gtk_box_pack_start(GTK_BOX(inContainer), mGtkMenubar, false, false, 0);
	
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
		
		for (xmlNodePtr node = xmlDoc->children; node != nil; node = node->next)
		{
			if (xmlNodeIsText(node) or strcmp((const char*)node->name, "menubar"))
				continue;
			
			for (xmlNodePtr menu = node->children; menu != nil; menu = menu->next)
			{
				if (xmlNodeIsText(menu) or strcmp((const char*)menu->name, "menu"))
					continue;
			
				MMenu* obj = CreateMenu(menu);

				const char* wm = (const char*)xmlGetProp(menu, BAD_CAST "window_menu");
				AddMenu(obj, wm != nil and strcmp(wm, "true") == 0);
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
	xmlNodePtr		inXMLNode)
{
	const char* label = (const char*)xmlGetProp(inXMLNode, BAD_CAST "label");
	if (label == nil)
		THROW(("Invalid menu specification, label is missing"));
	
	MMenu* menu = new MMenu(label);
	
	for (xmlNodePtr item = inXMLNode->children; item != nil; item = item->next)
	{
		if (xmlNodeIsText(item))
			continue;

		if (strcmp((const char*)item->name, "item") == 0)
		{
			label = (const char*)xmlGetProp(item, BAD_CAST "label");
			if (label == nil)
				THROW(("Invalid menu item specification, label is missing"));
			
			if (strcmp(label, "-") == 0)
				menu->AppendSeparator();
			else
			{
				uint32 cmd = 0;

				const char* cs = (const char*)xmlGetProp(item, BAD_CAST "cmd");
				if (cs == nil or strlen(cs) != 4)
					THROW(("Invalid menu item specification, cmd is not correct"));
				
				for (int i = 0; i < 4; ++i)
					cmd |= cs[i] << ((3 - i) * 8);
				
//				cout << "command for " << label << " is " << MCommandToString(cmd) << endl; 
				
				menu->AppendItem(label, cmd);
			}
		}
		else if (strcmp((const char*)item->name, "menu") == 0)
		{
			const char* rm = (const char*)xmlGetProp(item, BAD_CAST "recent_menu");
			if (rm != nil and strcmp(rm, "true") == 0)
			{
				label = (const char*)xmlGetProp(item, BAD_CAST "label");
				if (label == nil)
					THROW(("Invalid menu item specification, label is missing"));

				menu->AppendRecentMenu(label);
			}
			else
				menu->AppendMenu(CreateMenu(item));
		}
	}
	
	return menu;
}

void MMenubar::AddMenu(
	MMenu*				inMenu,
	bool				isWindowMenu)
{
	GtkWidget* menuItem = gtk_menu_item_new_with_label(inMenu->GetLabel().c_str());
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menuItem), inMenu->GetGtkMenu());
	gtk_menu_shell_append(GTK_MENU_SHELL(mGtkMenubar), menuItem);
	inMenu->SetTarget(mTarget);
	inMenu->SetAcceleratorGroup(mGtkAccel);
	mMenus.push_back(inMenu);
	
	if (isWindowMenu)
		mWindowMenu = inMenu;
}

bool MMenubar::OnButtonPress(
	GdkEventButton*		inEvent)
{
	for (list<MMenu*>::iterator m = mMenus.begin(); m != mMenus.end(); ++m)
		(*m)->UpdateCommandStatus();
	
	if (mWindowMenu != nil)
		gApp->UpdateWindowMenu(mWindowMenu);	
	
	return false;
}
