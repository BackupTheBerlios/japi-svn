/*
	Copyright (c) 2007, Maarten L. Hekkelman
	All rights reserved.

	Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions
	are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in
      the documentation and/or other materials provided with the
      distribution.
    * Neither the name of the Maarten L. Hekkelman nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
	"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
	LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
	FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
	COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
	INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
	(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
	SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
	HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
	STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
	ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
	OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef MMENU_H
#define MMENU_H

#include <vector>
#include <gdk/gdkkeysyms.h>

#include "MCommands.h"
#include "MCallbacks.h"

struct MMenuItem;
typedef std::vector<MMenuItem*>	MMenuItemList;

class XMLNode;

class MMenu
{
  public:
					MMenu(
						const std::string&	inLabel,
						GtkWidget*			inMenuWidget = nil);

	virtual			~MMenu();

	static MMenu*	CreateFromResource(
						const char*			inResourceName);

	void			AppendItem(
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
						XMLNode&			inXMLNode);

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
						XMLNode&			inXMLNode);
	
	bool			OnButtonPress(
						GdkEventButton*		inEvent);

	MSlot<bool(GdkEventButton*)>
					mOnButtonPressEvent;
	GtkWidget*		mGtkMenubar;
	GtkAccelGroup*	mGtkAccel;
	MHandler*		mTarget;
	std::list<MMenu*>
					mMenus;
	MMenu*			mWindowMenu;
	MMenu*			mTemplateMenu;
};

#endif
