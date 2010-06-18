//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef MMENU_H
#define MMENU_H

#include <list>

#include "MCommands.h"
#include "MCallbacks.h"
#include "MP2PEvents.h"

class MHandler;
class MWindow;
class MFile;
class MMenuImpl;

namespace zeep { namespace xml { class node; class element; } }

class MMenu
{
  public:
					MMenu(
						const std::string&	inLabel,
						bool				inPopup);

	virtual			~MMenu();

	static MMenu*	CreateFromResource(
						const char*			inResourceName,
						bool				inPopup);

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

	uint32			GetItemCommand(
						uint32				inIndex) const;

	void			SetTarget(
						MHandler*			inHandler);

	void			UpdateCommandStatus();

	std::string		GetLabel()				{ return mLabel; }
	MHandler*		GetTarget()				{ return mTarget; }
	
	void			Popup(
						MWindow*			inTarget,
						int32				inX,
						int32				inY,
						bool				inBottomMenu);
	
	static MMenu*	Create(
						zeep::xml::element*	inXMLNode,
						bool				inPopup);

	MMenuImpl*		impl() const			{ return mImpl; }

  protected:

	MMenuImpl*		mImpl;
	std::string		mLabel;
	std::string		mSpecial;
	MHandler*		mTarget;
};

class MMenubar
{
  public:
					MMenubar(
						MHandler*			inTarget);

	void			AddMenu(
						MMenu*				inMenu);

	void			SetTarget(
						MHandler*			inTarget);

  private:
	
	MMenu*			CreateMenu(
						zeep::xml::element*	inXMLNode);

	MHandler*		mTarget;
	std::list<MMenu*>
					mMenus;

	MEventOut<void(const std::string&,MMenu*)>	eUpdateSpecialMenu;

	typedef std::list<std::pair<std::string,MMenu*> > MSpecialMenus;

	MSpecialMenus	mSpecialMenus;
};

#endif
