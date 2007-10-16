#include "MJapieG.h"
#include <iostream>

#include "MCallbacks.h"
#include "MMenu.h"
#include "MWindow.h"

using namespace std;

struct MCommandToString
{
	char mCommandString[10];
	
	MCommandToString(uint32 inCommand)
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
						uint32			inCommand,
						uint32			inAcceleratorKey,
						GdkModifierType	inAcceleratorModifiers);

					MMenuItem(
						MMenu*			inMenu,
						MMenu*			inSubMenu);

					MMenuItem();

	virtual			~MMenuItem();

	void			ItemCallback();
	
	void			RecentItemActivated();

	MSlot<void()>	mCallback;
	MSlot<void()>	mRecentItemActivated;

	GtkWidget*		mGtkMenuItem;
	std::string		mLabel;
	uint32			mCommand;
	MMenu*			mMenu;
	MMenu*			mSubMenu;
	uint32			mAcceleratorKey;
	GdkModifierType	mAcceleratorModifiers;
	bool			mEnabled;
	bool			mChecked;
};

MMenuItem::MMenuItem(
	MMenu*			inMenu,
	const string&	inLabel,
	uint32			inCommand,
	uint32			inAcceleratorKey,
	GdkModifierType	inAcceleratorModifiers)
	: mCallback(this, &MMenuItem::ItemCallback)
	, mRecentItemActivated(this, &MMenuItem::RecentItemActivated)
	, mGtkMenuItem(nil)
	, mLabel(inLabel)
	, mCommand(inCommand)
	, mMenu(inMenu)
	, mSubMenu(nil)
	, mAcceleratorKey(inAcceleratorKey)
	, mAcceleratorModifiers(inAcceleratorModifiers)
	, mEnabled(true)
	, mChecked(false)
{
	mGtkMenuItem = gtk_menu_item_new_with_label(mLabel.c_str());
	mCallback.Connect(mGtkMenuItem, "activate");
	gtk_widget_show(mGtkMenuItem);
}

MMenuItem::MMenuItem(
	MMenu*			inMenu,
	MMenu*			inSubMenu)
	: mCallback(this, &MMenuItem::ItemCallback)
	, mRecentItemActivated(this, &MMenuItem::RecentItemActivated)
	, mGtkMenuItem(nil)
	, mLabel(inSubMenu->GetLabel())
	, mCommand(0)
	, mMenu(inMenu)
	, mSubMenu(inSubMenu)
	, mAcceleratorKey(0)
	, mAcceleratorModifiers(GdkModifierType(0))
	, mEnabled(true)
	, mChecked(false)
{
	mGtkMenuItem = gtk_menu_item_new_with_label(mLabel.c_str());
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(mGtkMenuItem), inSubMenu->GetGtkMenu());
	gtk_widget_show(mGtkMenuItem);
}

MMenuItem::MMenuItem()
	: mCallback(this, &MMenuItem::ItemCallback)
	, mRecentItemActivated(this, &MMenuItem::RecentItemActivated)
	, mGtkMenuItem(nil)
	, mLabel("-")
	, mCommand(0)
	, mMenu(nil)
	, mSubMenu(nil)
	, mEnabled(true)
	, mChecked(false)
{
	mGtkMenuItem = gtk_separator_menu_item_new();
	gtk_widget_show(mGtkMenuItem);
}

MMenuItem::~MMenuItem()
{
//	if (mGtkMenuItem != nil)
//		gtk_widget_destroy(mGtkMenuItem);
}

void MMenuItem::ItemCallback()
{
	if (mMenu != nil and mMenu->GetTarget() != nil)
	{
		if (not mMenu->GetTarget()->ProcessCommand(mCommand))
			cout << "Unhandled command: " << MCommandToString(mCommand) << endl;
	}
}

void MMenuItem::RecentItemActivated()
{
	assert(mSubMenu);
	assert(GTK_IS_RECENT_CHOOSER(mSubMenu->GetGtkMenu()));	
	
	char* uri = gtk_recent_chooser_get_current_uri(GTK_RECENT_CHOOSER(mSubMenu->GetGtkMenu()));
	
	if (uri != nil and strncmp(uri, "file://", 7) == 0)
	{
		MPath p(uri + 7);
		gApp->OpenOneDocument(p);
	}
	
	g_free(uri);
}

// --------------------------------------------------------------------

MMenu::MMenu(
	const string&	inLabel)
	: mGtkMenu(nil)
	, mGtkMenuItem(nil)
	, mGtkAccel(nil)
	, mParent(nil)
	, mLabel(inLabel)
	, mTarget(nil)
{
	mGtkMenu = gtk_menu_new();

	mGtkMenuItem = gtk_menu_item_new_with_label(mLabel.c_str());
	gtk_widget_show(mGtkMenuItem);

	gtk_menu_item_set_submenu(GTK_MENU_ITEM(mGtkMenuItem), mGtkMenu);
}

MMenu::MMenu(
	const string&	inLabel,
	GtkWidget*		inMenuWidget)
	: mGtkMenu(inMenuWidget)
	, mGtkMenuItem(nil)
	, mGtkAccel(nil)
	, mParent(nil)
	, mLabel(inLabel)
	, mTarget(nil)
{
	mGtkMenuItem = gtk_menu_item_new_with_label(mLabel.c_str());
	gtk_widget_show(mGtkMenuItem);
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(mGtkMenuItem), mGtkMenu);
}

MMenu::~MMenu()
{
	if (mGtkMenu != nil)
		gtk_widget_destroy(mGtkMenu);
}

void MMenu::AppendItem(
	const string&	inLabel,
	uint32			inCommand,
	uint32			inAcceleratorKey,
	uint32			inAcceleratorModifiers)
{
	if (inAcceleratorKey != 0 and not gtk_accelerator_valid(inAcceleratorKey, GdkModifierType(inAcceleratorModifiers)))
		cerr << "*** WARNING: Not a valid accelerator combination for " << inLabel << endl;
		
	MMenuItem* item = new MMenuItem(
		this, inLabel, inCommand, inAcceleratorKey, GdkModifierType(inAcceleratorModifiers));
	
	mItems.push_back(item);
	gtk_menu_shell_append(GTK_MENU_SHELL(mGtkMenu), item->mGtkMenuItem);
}

void MMenu::AppendSeparator()
{
	mItems.push_back(new MMenuItem());
	gtk_menu_shell_append(GTK_MENU_SHELL(mGtkMenu), mItems.back()->mGtkMenuItem);
}

void MMenu::AppendMenu(
	MMenu*			inMenu)
{
	mItems.push_back(new MMenuItem(this, inMenu));
	gtk_menu_shell_append(GTK_MENU_SHELL(mGtkMenu), mItems.back()->mGtkMenuItem);
}

void MMenu::AppendRecentMenu(
	const string&	inLabel)
{
	MMenu* recentMenu = new MMenu(inLabel,
		gtk_recent_chooser_menu_new_for_manager(gApp->GetRecentMgr()));

	MMenuItem* item = new MMenuItem(this, recentMenu);
	
	item->mRecentItemActivated.Connect(recentMenu->mGtkMenu, "item-activated");
	
	mItems.push_back(item);

	gtk_menu_shell_append(GTK_MENU_SHELL(mGtkMenu), item->mGtkMenuItem);
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
	mGtkAccel = inAcceleratorGroup;
	
	gtk_menu_set_accel_group(GTK_MENU(mGtkMenu), mGtkAccel);
	
	for (MMenuItemList::iterator mi = mItems.begin(); mi != mItems.end(); ++mi)
	{
		MMenuItem* item = *mi;
		
		if (item->mAcceleratorKey != 0)
		{
			gtk_widget_add_accelerator(item->mGtkMenuItem, "activate", mGtkAccel,
				item->mAcceleratorKey, item->mAcceleratorModifiers, GTK_ACCEL_VISIBLE);
		}
		
		if (item->mSubMenu != nil)
			item->mSubMenu->SetAcceleratorGroup(inAcceleratorGroup);
	}
}

// --------------------------------------------------------------------

MMenubar::MMenubar(
	MHandler*		inTarget,
	GtkWidget*		inContainer,
	GtkWidget*		inWindow)
	: mOnButtonPressEvent(this, &MMenubar::OnButtonPress)
	, mTarget(inTarget)
{
	mGtkMenubar = gtk_menu_bar_new();
	mGtkAccel = gtk_accel_group_new();

	gtk_box_pack_start(GTK_BOX(inContainer), mGtkMenubar, false, false, 0);
	
	mOnButtonPressEvent.Connect(mGtkMenubar, "button-press-event");
	
//	gtk_window_add_accel_group(GTK_WINDOW(inWindow), mGtkAccel);
}

void MMenubar::AddMenu(
	MMenu*				inMenu)
{
	gtk_menu_shell_append(GTK_MENU_SHELL(mGtkMenubar), inMenu->GetGtkMenuItem());
	inMenu->SetTarget(mTarget);
	inMenu->SetAcceleratorGroup(mGtkAccel);
	mMenus.push_back(inMenu);
}

bool MMenubar::OnButtonPress(
	GdkEventButton*		inEvent)
{
	for (list<MMenu*>::iterator m = mMenus.begin(); m != mMenus.end(); ++m)
		(*m)->UpdateCommandStatus();
	
	return false;
}
