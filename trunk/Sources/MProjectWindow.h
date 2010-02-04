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
class MProjectGroup;
class MFileRowItem;
class MRsrcRowItem;
class MListBase;
class MListRowBase;
template<class R> class MList;

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

	template<class R>
	void			AddItemsToList(
						MProjectGroup*		inGroup,
						MListRowBase*		inParent,
						MList<R>*			inList);

	bool			DoClose();
	
	void			CreateNewGroup();

	void			AddFilesToProject();
	
	void			GetSelectedItems(
						std::vector<MProjectItem*>&
										outItems);
	
	void			DeleteSelectedItems();

	void			SyncInterfaceWithProject();

	void			SetStatus(
						std::string		inStatus,
						bool			inBusy);

	void			SelectFileRow(
						MFileRowItem*	inRow);

	MEventIn<void(MFileRowItem*)>
					eSelectFileRow;

	void			InvokeFileRow(
						MFileRowItem*	inRow);

	MEventIn<void(MFileRowItem*)>
					eInvokeFileRow;

	void			SelectRsrcRow(
						MRsrcRowItem*	inRow);

	MEventIn<void(MRsrcRowItem*)>
					eSelectRsrcRow;

	void			InvokeRsrcRow(
						MRsrcRowItem*	inRow);

	MEventIn<void(MRsrcRowItem*)>
					eInvokeRsrcRow;

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
						MFileRowItem*		inRow,
						const std::string&	inNewName);

	MEventIn<void(MFileRowItem*,const std::string&)>
					eEditedFileGroupName;

	void			EditedRsrcGroupName(
						MRsrcRowItem*		inRow,
						const std::string&	inNewName);

	MEventIn<void(MRsrcRowItem*,const std::string&)>
					eEditedRsrcGroupName;

	MProject*		mProject;
	GtkWidget*		mStatusPanel;
	MListBase*		mFileTree;
	MListBase*		mRsrcTree;
//	GtkTreeViewColumn*	mFileNameColumn;
//	GtkCellRenderer*	mFileNameCell;
//	GtkTreeViewColumn*	mResourceNameColumn;
//	GtkCellRenderer*	mResourceNameCell;
	bool			mBusy, mEditingName;
};

#endif
