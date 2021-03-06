/*
	Copyright (c) 2008, Maarten L. Hekkelman
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

#ifndef MPROJECTWINDOW_H
#define MPROJECTWINDOW_H

#include "MDocWindow.h"
#include "MDocument.h"

class MProject;
class MProjectItem;
class MTreeModelInterface;

class MProjectWindow : public MDocWindow
{
  public:
					MProjectWindow();

					~MProjectWindow();

	virtual void	Initialize(
						MDocument*		inDocument);

	virtual bool	UpdateCommandStatus(
						uint32			inCommand,
						MMenu*			inMenu,
						uint32			inItemIndex,
						bool&			outEnabled,
						bool&			outChecked);

	virtual bool	ProcessCommand(
						uint32			inCommand,
						const MMenu*	inMenu,
						uint32			inItemIndex,
						uint32			inModifiers);

	MEventIn<void(std::string,bool)>	eStatus;

	void			CreateNewGroup(
						const std::string&
										inGroupName);
	
  protected:

	bool			DoClose();
	
	void			AddFilesToProject();
	
	void			GetSelectedItems(
						std::vector<MProjectItem*>&
										outItems);
	
	void			DeleteSelectedItems();

	void			SyncInterfaceWithProject();

	void			SetStatus(
						std::string		inStatus,
						bool			inBusy);

	void			InvokeFileRow(
						GtkTreePath*	inPath,
						GtkTreeViewColumn*
										inColumn);

	MSlot<void(GtkTreePath*path, GtkTreeViewColumn*)>
					eInvokeFileRow;

	void			InvokeResourceRow(
						GtkTreePath*	inPath,
						GtkTreeViewColumn*
										inColumn);

	MSlot<void(GtkTreePath*path, GtkTreeViewColumn*)>
					eInvokeResourceRow;

	virtual bool	OnKeyPressEvent(
						GdkEventKey*	inEvent);

	MSlot<bool(GdkEventKey*)>
					eKeyPressEvent;

	virtual void	DocumentChanged(
						MDocument*		inDocument);

	void			SaveState();

	void			InitializeTreeView(
						GtkTreeView*	inGtkTreeView,
						int32			inPanel);

	void			TargetChanged();
	MSlot<void()>	eTargetChanged;

	void			TargetsChanged();
	MEventIn<void()>eTargetsChanged;
	
	void			InfoClicked();
	MSlot<void()>	eInfoClicked;

	void			MakeClicked();
	MSlot<void()>	eMakeClicked;

	MProject*		mProject;
	MTreeModelInterface*
					mFilesTree;
	MTreeModelInterface*
					mResourcesTree;
	GtkWidget*		mStatusPanel;
	bool			mBusy;
};

#endif
