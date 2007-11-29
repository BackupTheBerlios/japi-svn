#ifndef MMENU_H
#define MMENU_H

#include <vector>
#include <gdk/gdkkeysyms.h>

#include "MCommands.h"
#include "MCallbacks.h"

struct MMenuItem;
typedef std::vector<MMenuItem*>	MMenuItemList;

class MMenu
{
  public:
					MMenu(
						const std::string&	inLabel,
						GtkWidget*			inMenuWidget = nil);

	virtual			~MMenu();

	void			AppendItem(
						const std::string&	inLabel,
						uint32				inCommand);
	
	void			AppendSeparator();

	virtual void	AppendMenu(
						MMenu*				inMenu);

	void			AppendRecentMenu(
						const std::string&	inLabel);

	uint32			CountItems();
	
	void			RemoveItems(
						uint32				inFromIndex,
						uint32				inCount);

	void			SetTarget(
						MHandler*			inHandler);

	void			UpdateCommandStatus();

	GtkWidget*		GetGtkMenu()			{ return mGtkMenu; }
	std::string		GetLabel()				{ return mLabel; }
	MHandler*		GetTarget()				{ return mTarget; }
	
	void			SetAcceleratorGroup(
						GtkAccelGroup*		inAcceleratorGroup);

	void			Popup(
						MHandler*			inTarget,
						GdkEventButton*		inEvent,
						int32				inX,
						int32				inY,
						bool				inBottomMenu);
	
  protected:

	static void		MenuPosition(
						GtkMenu*			inMenu,
						gint*				inX,
						gint*				inY,
						gboolean*			inPushIn,
						gpointer			inUserData);

	MMenuItem*		CreateNewItem(
						const std::string&	inLabel,
						uint32				inCommand);

	virtual bool	OnDestroy();
	virtual void	OnSelectionDone();

	MSlot<bool()>	mOnDestroy;
	MSlot<void()>	mOnSelectionDone;

	GtkWidget*		mGtkMenu;
	std::string		mLabel;
	MMenuItemList	mItems;
	MHandler*		mTarget;
	int32			mPopupX, mPopupY;
};

class MMenubar
{
  public:
					MMenubar(
						MHandler*			inTarget,
						GtkWidget*			inContainer,
						GtkWidget*			inWindow);

	void			AddMenu(
						MMenu*				inMenu,
						bool				isWindowMenu = false);

	void			BuildFromResource(
						const char*			inResourceName);

  private:
	
	bool			OnButtonPress(
						GdkEventButton*		inEvent);

	MMenu*			CreateMenu(
						xmlNodePtr			inXMLNode);
	
	MSlot<bool(GdkEventButton*)>
					mOnButtonPressEvent;
	GtkWidget*		mGtkMenubar;
	GtkAccelGroup*	mGtkAccel;
	MHandler*		mTarget;
	std::list<MMenu*>
					mMenus;
	MMenu*			mWindowMenu;
};

#endif
