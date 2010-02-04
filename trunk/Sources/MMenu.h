//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef MMENU_H
#define MMENU_H

#include <list>
#include <gdk/gdkkeysyms.h>

#include "MCommands.h"
#include "MCallbacks.h"
#include "MP2PEvents.h"

class MHandler;
class MFile;

struct MMenuItem;
typedef std::list<MMenuItem*>	MMenuItemList;

namespace zeep { namespace xml { class node; } }

class MMenu
{
  public:
					MMenu(
						const std::string&	inLabel,
						GtkWidget*			inMenuWidget = nil);

	virtual			~MMenu();

	static void		AddToRecentMenu(
						const MFile&		inFileRef);

	static MMenu*	CreateFromResource(
						const char*			inResourceName);

	void			AppendItem(
						const std::string&	inLabel,
						uint32				inCommand);
	
	void			AppendRadioItem(
						const std::string&	inLabel,
						uint32				inCommand);
	
	void			AppendCheckItem(
						const std::string&	inLabel,
						uint32				inCommand);
	
	void			AppendSeparator();

	virtual void	AppendMenu(
						MMenu*				inMenu);

	uint32			CountItems();
	
	void			RemoveItems(
						uint32				inFromIndex,
						uint32				inCount);

	std::string		GetItemLabel(
						uint32				inIndex) const;

	bool			GetRecentItem(
						uint32				inIndex,
						MFile&				outURL) const;

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
	
	bool			IsRecentMenu() const;
	
  protected:

	static MMenu*	Create(
						zeep::xml::node&	inXMLNode);

	static void		MenuPosition(
						GtkMenu*			inMenu,
						gint*				inX,
						gint*				inY,
						gboolean*			inPushIn,
						gpointer			inUserData);

	MMenuItem*		CreateNewItem(
						const std::string&	inLabel,
						uint32				inCommand,
						GSList**			ioRadioGroup);

	virtual bool	OnDestroy();
	virtual void	OnSelectionDone();

	MSlot<bool()>	mOnDestroy;
	MSlot<void()>	mOnSelectionDone;

	GtkWidget*		mGtkMenu;
	std::string		mLabel;
	MMenuItemList	mItems;
	MHandler*		mTarget;
	GSList*			mRadioGroup;
	int32			mPopupX, mPopupY;
};

class MMenubar
{
  public:
					MMenubar(
						MHandler*			inTarget);

	void			AddMenu(
						MMenu*				inMenu);

	void			Initialize(
						GtkWidget*			inMBarWidget,
						const char*			inResourceName);

	void			SetTarget(
						MHandler*			inTarget);

  private:
	
	MMenu*			CreateMenu(
						zeep::xml::node&	inXMLNode);
	
	bool			OnButtonPress(
						GdkEventButton*		inEvent);

	MSlot<bool(GdkEventButton*)>
					mOnButtonPressEvent;
	GtkWidget*		mGtkMenubar;
	GtkAccelGroup*	mGtkAccel;
	MHandler*		mTarget;
	std::list<MMenu*>
					mMenus;

	MEventOut<void(const std::string&,MMenu*)>	eUpdateSpecialMenu;

	typedef std::list<std::pair<std::string,MMenu*> > MSpecialMenus;

	MSpecialMenus	mSpecialMenus;
};

#endif
