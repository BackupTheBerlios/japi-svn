#include "MJapieG.h"
#include <iostream>

#include "MCallbacks.h"
#include "MMenu.h"
#include "MWindow.h"
#include "MAcceleratorTable.h"

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
						uint32			inCommand);

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
	, mIndex(0)
	, mMenu(inMenu)
	, mSubMenu(inSubMenu)
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
	, mIndex(0)
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
cout << "Deleting menu item " << mLabel << endl;
//	if (mGtkMenuItem != nil)
//		gtk_widget_destroy(mGtkMenuItem);
}

void MMenuItem::ItemCallback()
{
	if (mMenu != nil and mMenu->GetTarget() != nil)
	{
		if (not mMenu->GetTarget()->ProcessCommand(mCommand, mMenu, mIndex))
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

MMenu::MMenu()
	: mOnDestroy(this, &MMenu::OnDestroy)
	, mGtkMenu(nil)
	, mGtkMenuItem(nil)
	, mGtkAccel(nil)
	, mTarget(nil)
{
	mGtkMenu = gtk_menu_new();

	mOnDestroy.Connect(mGtkMenu, "destroy");
}

MMenu::MMenu(
	const string&	inLabel)
	: mOnDestroy(this, &MMenu::OnDestroy)
	, mGtkMenu(nil)
	, mGtkMenuItem(nil)
	, mGtkAccel(nil)
	, mLabel(inLabel)
	, mTarget(nil)
{
	mGtkMenu = gtk_menu_new();

	mGtkMenuItem = gtk_menu_item_new_with_label(mLabel.c_str());
	gtk_widget_show(mGtkMenuItem);

	gtk_menu_item_set_submenu(GTK_MENU_ITEM(mGtkMenuItem), mGtkMenu);

	mOnDestroy.Connect(mGtkMenu, "destroy");
}

MMenu::MMenu(
	const string&	inLabel,
	GtkWidget*		inMenuWidget)
	: mOnDestroy(this, &MMenu::OnDestroy)
	, mGtkMenu(inMenuWidget)
	, mGtkMenuItem(nil)
	, mGtkAccel(nil)
	, mLabel(inLabel)
	, mTarget(nil)
{
	mGtkMenuItem = gtk_menu_item_new_with_label(mLabel.c_str());
	gtk_widget_show(mGtkMenuItem);
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(mGtkMenuItem), mGtkMenu);
	
	mOnDestroy.Connect(mGtkMenu, "destroy");
}

MMenu::~MMenu()
{
cout << "Deleting menu" << endl;

	for (MMenuItemList::iterator mi = mItems.begin(); mi != mItems.end(); ++mi)
		delete *mi;
}

void MMenu::AppendItem(
	const string&	inLabel,
	uint32			inCommand)
{
	MMenuItem* item = new MMenuItem(
		this, inLabel, inCommand);

	item->mIndex = mItems.size();
	
	mItems.push_back(item);
	gtk_menu_shell_append(GTK_MENU_SHELL(mGtkMenu), item->mGtkMenuItem);
}

void MMenu::AppendSeparator()
{
	MMenuItem* item = new MMenuItem();	
	
	mItems.push_back(item);

	item->mIndex = mItems.size();

	gtk_menu_shell_append(GTK_MENU_SHELL(mGtkMenu), item->mGtkMenuItem);
}

void MMenu::AppendMenu(
	MMenu*			inMenu)
{
	MMenuItem* item = new MMenuItem(this, inMenu);	

	item->mIndex = mItems.size();
	
	mItems.push_back(item);
	gtk_menu_shell_append(GTK_MENU_SHELL(mGtkMenu), item->mGtkMenuItem);
}

void MMenu::AppendRecentMenu(
	const string&	inLabel)
{
	MMenu* recentMenu = new MMenu(inLabel,
		gtk_recent_chooser_menu_new_for_manager(gApp->GetRecentMgr()));

	MMenuItem* item = new MMenuItem();
	item->mIndex = mItems.size();
	item->mSubMenu = recentMenu;
	item->mGtkMenuItem = recentMenu->GetGtkMenuItem();
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
	
	MAcceleratorTable& at = MAcceleratorTable::Instance();
	
	gtk_menu_set_accel_group(GTK_MENU(mGtkMenu), mGtkAccel);
	
	for (MMenuItemList::iterator mi = mItems.begin(); mi != mItems.end(); ++mi)
	{
		MMenuItem* item = *mi;
		
		uint32 key, mod;
		
		if (at.GetAcceleratorKeyForCommand(item->mCommand, key, mod))
		{
			gtk_widget_add_accelerator(item->mGtkMenuItem, "activate", mGtkAccel,
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
