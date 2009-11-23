//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef MPROJECTWINDOW_H
#define MPROJECTWINDOW_H

#include "MDocWindow.h"
#include "MDocument.h"

class MProject;
class MProjectItem;
class MProjectTree;

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

  protected:

	bool			DoClose();
	
	void			CreateNewGroup();

	void			RenameGroup();
	
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

	void			EditedFileGroupName(
						gchar*			path,
						gchar*			new_text);

	MSlot<void(gchar*,gchar*)>			mEditedFileGroupName;

	void			EditedResourceGroupName(
						gchar*			path,
						gchar*			new_text);

	MSlot<void(gchar*,gchar*)>			mEditedResourceGroupName;

	MProject*		mProject;
	MProjectTree*	mFilesTree;
	MProjectTree*	mResourcesTree;
	GtkWidget*		mStatusPanel;
	GtkTreeViewColumn*	mFileNameColumn;
	GtkCellRenderer*	mFileNameCell;
	GtkTreeViewColumn*	mResourceNameColumn;
	GtkCellRenderer*	mResourceNameCell;
	bool			mBusy, mEditingName;
};

#endif
