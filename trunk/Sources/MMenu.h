#ifndef MMENU_H
#define MMENU_H

#include <list>
#include "MCommands.h"

struct MMenuItem;
typedef std::list<MMenuItem*>	MMenuItemList;

class MMenu
{
  public:
					MMenu(
						const std::string&	inLabel);

	virtual			~MMenu();

	void			AddItem(
						const std::string&	inLabel,
						uint32				inCommand);

	virtual void	AddMenu(
						MMenu*				inMenu);

	// called by MMenuItem
	void			InvokeItem(
						uint32				inCommand);
	
	GtkWidget*		GetGtkMenu()			{ return mGtkMenu; }
	GtkWidget*		GetGtkMenuItem()		{ return mGtkMenuItem; }
	GtkAccelGroup*	GetGtkAccelerator()		{ return mGtkAccel; }
	std::string		GetLabel()				{ return mLabel; }
	
  protected:
	GtkWidget*		mGtkMenu;
	GtkWidget*		mGtkMenuItem;
	GtkAccelGroup*	mGtkAccel;
	MMenu*			mParent;
	std::string		mLabel;
	MMenuItemList	mItems;
};

class MMenubar
{
  public:
					MMenubar(
						GtkWidget*			inContainer);

	void			AddMenu(
						MMenu*				inMenu);

  private:
	GtkWidget*		mGtkMenubar;
	GtkAccelGroup*	mGtkAccel;
};

#endif
