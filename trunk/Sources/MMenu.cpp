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
						uint32			inCommand);

					MMenuItem(
						MMenu*			inMenu,
						MMenu*			inSubMenu);

					MMenuItem();

	virtual			~MMenuItem();

	void			ItemCallback();

	MSlot<void()>	mCallback;

	GtkWidget*		mGtkMenuItem;
	std::string		mLabel;
	uint32			mCommand;
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
	, mGtkMenuItem(nil)
	, mLabel(inLabel)
	, mCommand(inCommand)
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
	, mGtkMenuItem(nil)
	, mLabel(inSubMenu->GetLabel())
	, mCommand(0)
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
	cout << "Callback for item " << MCommandToString(mCommand) << endl;
	if (mMenu != nil and mMenu->GetTarget() != nil)
	{
		if (not mMenu->GetTarget()->ProcessCommand(mCommand))
			cout << "Unhandled command: " << MCommandToString(mCommand) << endl;
	}
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
	
	if (mParent != nil)
		mGtkAccel = mParent->mGtkAccel;
}

MMenu::~MMenu()
{
	if (mGtkMenu != nil)
		gtk_widget_destroy(mGtkMenu);
}

void MMenu::AppendItem(
	const string&	inLabel,
	uint32			inCommand)
{
	mItems.push_back(new MMenuItem(this, inLabel, inCommand));
	gtk_menu_shell_append(GTK_MENU_SHELL(mGtkMenu), mItems.back()->mGtkMenuItem);
}

void MMenu::AddSeparator()
{
	mItems.push_back(new MMenuItem());
	gtk_menu_shell_append(GTK_MENU_SHELL(mGtkMenu), mItems.back()->mGtkMenuItem);
}

void MMenu::AddMenu(
	MMenu*			inMenu)
{
	mItems.push_back(new MMenuItem(this, inMenu));
	gtk_menu_shell_append(GTK_MENU_SHELL(mGtkMenu), mItems.back()->mGtkMenuItem);
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

// --------------------------------------------------------------------

MMenubar::MMenubar(
	MHandler*		inTarget,
	GtkWidget*		inContainer)
	: mOnButtonPressEvent(this, &MMenubar::OnButtonPress)
	, mTarget(inTarget)
{
	mGtkMenubar = gtk_menu_bar_new();
	mGtkAccel = gtk_accel_group_new();

	gtk_box_pack_start(GTK_BOX(inContainer), mGtkMenubar, false, false, 0);
	
	mOnButtonPressEvent.Connect(mGtkMenubar, "button-press-event");
}

void MMenubar::AddMenu(
	MMenu*				inMenu)
{
	gtk_menu_shell_append(GTK_MENU_SHELL(mGtkMenubar), inMenu->GetGtkMenuItem());
	inMenu->SetTarget(mTarget);
	
	mMenus.push_back(inMenu);
}

bool MMenubar::OnButtonPress(
	GdkEventButton*		inEvent)
{
	for (list<MMenu*>::iterator m = mMenus.begin(); m != mMenus.end(); ++m)
		(*m)->UpdateCommandStatus();
	
	return false;
}
