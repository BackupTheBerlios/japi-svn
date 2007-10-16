#ifndef MMENU_H
#define MMENU_H

#include <list>
#include <gdk/gdkkeysyms.h>

#include "MCommands.h"
#include "MCallbacks.h"

struct MMenuItem;
typedef std::list<MMenuItem*>	MMenuItemList;

class MMenu
{
  public:
					MMenu(
						const std::string&	inLabel);

	virtual			~MMenu();

	void			AppendItem(
						const std::string&	inLabel,
						uint32				inCommand,
						uint32				inAcceleratorKey = 0,
						uint32				inAcceleratorModifiers = 0);
	
	void			AppendSeparator();

	virtual void	AppendMenu(
						MMenu*				inMenu);

	void			AppendRecentMenu(
						const std::string&	inLabel);

	void			SetTarget(
						MHandler*			inHandler);

	void			UpdateCommandStatus();

	GtkWidget*		GetGtkMenu()			{ return mGtkMenu; }
	GtkWidget*		GetGtkMenuItem()		{ return mGtkMenuItem; }
	GtkAccelGroup*	GetGtkAccelerator()		{ return mGtkAccel; }
	std::string		GetLabel()				{ return mLabel; }
	MHandler*		GetTarget()				{ return mTarget; }
	
	void			SetAcceleratorGroup(
						GtkAccelGroup*		inAcceleratorGroup);
	
  protected:

					MMenu(
						const std::string&	inLabel,
						GtkWidget*			inMenuWidget);

	GtkWidget*		mGtkMenu;
	GtkWidget*		mGtkMenuItem;
	GtkAccelGroup*	mGtkAccel;
	MMenu*			mParent;
	std::string		mLabel;
	MMenuItemList	mItems;
	MHandler*		mTarget;
};

class MMenubar
{
  public:
					MMenubar(
						MHandler*			inTarget,
						GtkWidget*			inContainer,
						GtkWidget*			inWindow);

	void			AddMenu(
						MMenu*				inMenu);

  private:
	
	bool			OnButtonPress(
						GdkEventButton*		inEvent);
	
	MSlot<bool(GdkEventButton*)>
					mOnButtonPressEvent;
	GtkWidget*		mGtkMenubar;
	GtkAccelGroup*	mGtkAccel;
	MHandler*		mTarget;
	std::list<MMenu*>
					mMenus;
};

#endif
