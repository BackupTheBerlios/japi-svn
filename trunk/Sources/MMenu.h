#ifndef MMENU_H
#define MMENU_H

#include <list>
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
						uint32				inCommand);
	
	void			AddSeparator();

	virtual void	AddMenu(
						MMenu*				inMenu);

	void			SetTarget(
						MHandler*			inHandler);

	void			UpdateCommandStatus();

	GtkWidget*		GetGtkMenu()			{ return mGtkMenu; }
	GtkWidget*		GetGtkMenuItem()		{ return mGtkMenuItem; }
	GtkAccelGroup*	GetGtkAccelerator()		{ return mGtkAccel; }
	std::string		GetLabel()				{ return mLabel; }
	MHandler*		GetTarget()				{ return mTarget; }
	
  protected:
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
						GtkWidget*			inContainer);

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
