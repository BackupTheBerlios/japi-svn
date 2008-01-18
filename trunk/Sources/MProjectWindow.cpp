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

#include "MJapieG.h"

#include "MProjectWindow.h"
#include "MProjectItem.h"
#include "MStrings.h"
#include "MError.h"
#include "MFindAndOpenDialog.h"
#include "MProject.h"
#include "MProjectTarget.h"
#include "MTreeModelInterface.h"

using namespace std;

namespace {

enum {
	kFilesListViewID	= 'tre1',
	kLinkOrderViewID	= 'tre2',
	kResourceViewID		= 'tre3',
	
	kTargetPopupID		= 'targ'
};

enum {
	kFilesDirtyColumn,
	kFilesNameColumn,
	kFilesTextSizeColumn,
	kFilesDataSizeColumn
};

class MProjectFilesTreeModel : public MTreeModelInterface
{
  public:
					MProjectFilesTreeModel(
						MProject*		inProject);

	virtual uint32	GetColumnCount() const;

	virtual GType	GetColumnType(
						uint32			inColumn) const;

	virtual void	GetValue(
						GtkTreeIter*	inIter,
						uint32			inColumn,
						GValue*			outValue) const;

	virtual bool	GetIter(
						GtkTreeIter*	outIter,
						GtkTreePath*	inPath);

	virtual GtkTreePath*
					GetPath(
						GtkTreeIter*	inIter);

	virtual bool	Next(
						GtkTreeIter*	inIter);

	virtual bool	Children(
						GtkTreeIter*	outIter,
						GtkTreeIter*	inParent);

	virtual bool	HasChildren(
						GtkTreeIter*	inIter);
	
	virtual int32	CountChildren(
						GtkTreeIter*	inIter);

	virtual bool	GetChild(
						GtkTreeIter*	outIter,
						GtkTreeIter*	inParent,
						int32			inIndex);

	virtual bool	GetParent(
						GtkTreeIter*	outIter,
						GtkTreeIter*	inChild);

	virtual void	RefNode(
						GtkTreeIter*	inIter);

	virtual void	UnrefNode(
						GtkTreeIter*	inIter);

  private:
	MProject*		mProject;
};

MProjectFilesTreeModel::MProjectFilesTreeModel(
	MProject*		inProject)
	: MTreeModelInterface(GTK_TREE_MODEL_ITERS_PERSIST)
	, mProject(inProject)
{
}

uint32 MProjectFilesTreeModel::GetColumnCount() const
{
	return 4;
}

GType MProjectFilesTreeModel::GetColumnType(
	uint32			inColumn) const
{
	GType result;

	if (inColumn == kFilesDirtyColumn)
		result = GDK_TYPE_PIXMAP;
	else
		result = G_TYPE_STRING;
	
	return result;
}

void MProjectFilesTreeModel::GetValue(
	GtkTreeIter*	inIter,
	uint32			inColumn,
	GValue*			outValue) const
{
	MProjectItem* item = reinterpret_cast<MProjectItem*>(inIter->user_data);
	
	if (item != nil)
	{
		switch (inColumn)
		{
	//		case kFilesDirtyColumn:
	//			g_value
	//			break;
			
			case kFilesNameColumn:
				g_value_set_string(outValue, item->GetName().c_str());
				break;
			
			case kFilesDataSizeColumn:
				g_value_set_string(outValue, "");
				break;
	
			case kFilesTextSizeColumn:
				g_value_set_string(outValue, "");
				break;
		}
	}
}

bool MProjectFilesTreeModel::GetIter(
	GtkTreeIter*	outIter,
	GtkTreePath*	inPath)
{
	bool result = false;
	
	int32 depth = gtk_tree_path_get_depth(inPath);
	int32* indices = gtk_tree_path_get_indices(inPath);
	
	MProjectItem* item = mProject->GetItems();
	
	for (int32 ix = 0; ix < depth and item != nil; ++ix)
	{
		MProjectGroup* group = dynamic_cast<MProjectGroup*>(item);
		THROW_IF_NIL(group);
		
		if (indices[ix] < 0 or indices[ix] >= group->Count())
			THROW(("Index in path out of range"));
		
		item = group->GetItems()[ix];
	}
	
	if (item != nil)
	{
		outIter->user_data = item;
		result = true;
	}
	
	return result;
}

GtkTreePath* MProjectFilesTreeModel::GetPath(
	GtkTreeIter*	inIter)
{
	GtkTreePath* path = gtk_tree_path_new();
	MProjectItem* item = reinterpret_cast<MProjectItem*>(inIter->user_data);
	
	while (item->GetParent() != nil)
	{
		gtk_tree_path_prepend_index(path, item->GetPosition());
		item = item->GetParent();
	}
	
	return path;
}

bool MProjectFilesTreeModel::Next(
	GtkTreeIter*	ioIter)
{
	bool result = false;

	MProjectItem* item = reinterpret_cast<MProjectItem*>(ioIter->user_data);
	
	if (item != nil)
	{
		MProjectGroup* parent = item->GetParent();
		int32 ix = item->GetPosition();
		
		if (parent != nil and ix + 1 < parent->Count())
		{
			item = parent->GetItems()[ix + 1];
			ioIter->user_data = item;
			result = true;
		}
	}
	
	return result;
}

bool MProjectFilesTreeModel::Children(
	GtkTreeIter*	outIter,
	GtkTreeIter*	inParent)
{
	bool result = false;
	
	if (inParent == nil)
	{
		outIter->user_data = mProject->GetItems();
		result = true;
	}
	else
	{
		MProjectItem* item = reinterpret_cast<MProjectItem*>(inParent->user_data);
		MProjectGroup* group = dynamic_cast<MProjectGroup*>(item);
		
		if (group != nil and group->Count() > 0)
		{
			outIter->user_data = group->GetItems().front();
			result = true;
		}
	}
	
	return result;
}

bool MProjectFilesTreeModel::HasChildren(
	GtkTreeIter*	inIter)
{
	MProjectItem* item = reinterpret_cast<MProjectItem*>(inIter->user_data);
	MProjectGroup* group = dynamic_cast<MProjectGroup*>(item);
	
	return group != nil;
}

int32 MProjectFilesTreeModel::CountChildren(
	GtkTreeIter*	inIter)
{
	int32 result = 0;
	
	if (inIter == nil)
		result = mProject->GetItems()->Count();
	else
	{
		MProjectItem* item = reinterpret_cast<MProjectItem*>(inIter->user_data);
		MProjectGroup* group = dynamic_cast<MProjectGroup*>(item);
		
		if (group != nil)
			result = group->Count();
	}
	
	return result;
}

bool MProjectFilesTreeModel::GetChild(
	GtkTreeIter*	outIter,
	GtkTreeIter*	inParent,
	int32			inIndex)
{
	bool result = false;
	
	MProjectGroup* group;
	
	if (inParent == nil)
		group = mProject->GetItems();
	else
	{
		MProjectItem* item = reinterpret_cast<MProjectItem*>(inParent->user_data);
		group = dynamic_cast<MProjectGroup*>(item);
	}
	
	if (group != nil and inIndex < group->Count())
	{
		outIter->user_data = group->GetItems()[inIndex];
		result = true;
	}
	
	return result;
}

bool MProjectFilesTreeModel::GetParent(
	GtkTreeIter*	outIter,
	GtkTreeIter*	inChild)
{
	bool result = false;
	
	MProjectItem* item = reinterpret_cast<MProjectItem*>(inChild->user_data);
	
	if (item->GetParent() != nil)
	{
		outIter->user_data = item->GetParent();
		result = true;
	}
	
	return result;
}

void MProjectFilesTreeModel::RefNode(
	GtkTreeIter*	inIter)
{
}

void MProjectFilesTreeModel::UnrefNode(
	GtkTreeIter*	inIter)
{
}

}

MProjectWindow::MProjectWindow()
	: MDocWindow("project-window")
	, eStatus(this, &MProjectWindow::SetStatus)
	, mInvokeFileRow(this, &MProjectWindow::InvokeFileRow)
	, mProject(nil)
	, mFilesTree(nil)
{
	mController = new MController(this);
	
	mController->SetWindow(this);
	
	mMenubar.Initialize(GetWidget('mbar'), "project-window-menus");
	mMenubar.SetTarget(mController);

	GtkWidget* treeView = GetWidget(kFilesListViewID);
	THROW_IF_NIL((treeView));

	// create the tree store for the Files tree
//	mFilesTree = new MProjectFilesTreeModel(mProject);
//	gtk_tree_view_set_model(GTK_TREE_VIEW(treeView), mFilesTree->GetModel());
	
	// Add the columns and renderers

	// first the dirty mark	
	GtkCellRenderer* renderer = gtk_cell_renderer_pixbuf_new();
	GtkTreeViewColumn* column = gtk_tree_view_column_new_with_attributes(
		_(" "), renderer, "pixbuf", kFilesDirtyColumn, nil);
	gtk_tree_view_append_column(GTK_TREE_VIEW(treeView), column);

	// the name column
	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes (
		_("File"), renderer, "text", kFilesNameColumn, nil);
	g_object_set(G_OBJECT(column), "expand", true, nil);
	gtk_tree_view_append_column(GTK_TREE_VIEW(treeView), column);
	
	// the text size column
	
	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes (
		_("Text"), renderer, "text", kFilesTextSizeColumn, nil);
	g_object_set(G_OBJECT(renderer), "xalign", 1.0f, nil);
	gtk_tree_view_column_set_alignment(column, 1.0f);
	gtk_tree_view_append_column(GTK_TREE_VIEW(treeView), column);

	// the data size column
	
	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes (
		_("Data"), renderer, "text", kFilesDataSizeColumn, nil);
	g_object_set(G_OBJECT(renderer), "xalign", 1.0f, nil);
	gtk_tree_view_column_set_alignment(column, 1.0f);
	gtk_tree_view_append_column(GTK_TREE_VIEW(treeView), column);

	gtk_widget_show_all(treeView);

	mInvokeFileRow.Connect(treeView, "row-activated");

	ConnectChildSignals();
}

// ---------------------------------------------------------------------------
//	MProjectWindow::Initialize

void MProjectWindow::Initialize(
	MDocument*		inDocument)
{
	mProject = dynamic_cast<MProject*>(inDocument);
	if (mProject == nil)
		THROW(("Invalid document type passed"));
	
	AddRoute(mProject->eStatus, eStatus);
	
	MDocWindow::Initialize(inDocument);

	GtkWidget* treeView = GetWidget(kFilesListViewID);
	THROW_IF_NIL((treeView));

	mFilesTree = new MProjectFilesTreeModel(mProject);
	gtk_tree_view_set_model(GTK_TREE_VIEW(treeView), mFilesTree->GetModel());
	
	SyncInterfaceWithProject();
}

// ---------------------------------------------------------------------------
//	MProjectWindow::InvokeFileRow

void MProjectWindow::InvokeFileRow(
	GtkTreePath*		inPath,
	GtkTreeViewColumn*	inColumn)
{
	cout << "Row invoked" << endl;
}

// ---------------------------------------------------------------------------
//	MProjectWindow::UpdateCommandStatus

bool MProjectWindow::UpdateCommandStatus(
	uint32				inCommand,
	MMenu*				inMenu,
	uint32				inItemIndex,
	bool&				outEnabled,
	bool&				outChecked)
{
	bool result = true, isCompilable = false;
	
	outEnabled = false;
	
//	switch (mPanel)
//	{
//		case ePanelFiles:
//		{
//			int32 selected = mFileList->GetSelected();
//			
//			if (selected >= 0)
//				isCompilable = GetItem(selected)->IsCompilable();
//			break;
//		}
//		
////		case ePanelLinkOrder:
////		{
////			int32 selected = mLinkOrderList->GetSelected();
////			
////			if (selected >= 0)
////				isCompilable = GetItem(selected)->IsCompilable();
////			break;
////		}
//		
//		default:
//			break;
//	}
	
	switch (inCommand)
	{
//		case cmd_Save:
//			outEnabled = mModified;
//			break;
//		
//		case cmd_AddFileToProject:
//			break;
//
//		case cmd_Preprocess:
//		case cmd_CheckSyntax:
//		case cmd_Compile:
//		case cmd_Disassemble:
//			outEnabled = isCompilable;
//			break;

//		case cmd_RecheckFiles:
//		case cmd_BringUpToDate:
//		case cmd_MakeClean:
//		case cmd_Make:
//		case cmd_NewGroup:
		case cmd_OpenIncludeFile:
			outEnabled = true;
			break;

//		case cmd_Run:
//			break;
//		
//		case cmd_Stop:
//			outEnabled = mCurrentJob.get() != nil;
//			break;
		
		default:
			result = MDocWindow::UpdateCommandStatus(
				inCommand, inMenu, inItemIndex, outEnabled, outChecked);
			break;
	}
	
	return result;
}

// ---------------------------------------------------------------------------
//	MProject::ProcessCommand

bool MProjectWindow::ProcessCommand(
	uint32			inCommand,
	const MMenu*	inMenu,
	uint32			inItemIndex)
{
	bool result = true;

	MProjectItem* item = nil;

//	switch (mPanel)
//	{
//		case ePanelFiles:
//		{
//			int32 selected = mFileList->GetSelected();
//			
//			if (selected >= 0)
//				item = GetItem(selected);
//			break;
//		}
//		
////		case ePanelLinkOrder:
////		{
////			int32 selected = mLinkOrderList->GetSelected();
////			
////			if (selected >= 0)
////				item = GetItem(selected);
////			break;
////		}
//		
//		default:
//			break;
//	}
	
	MProjectFile* file = dynamic_cast<MProjectFile*>(item);
	
	switch (inCommand)
	{
//		case cmd_Save:
//			SaveDocument();
//			break;
//
//		case cmd_SaveAs:
//			SaveDocumentAs(this, mProjectFile.leaf());
//			break;
//
//		case cmd_Revert:
//			TryDiscardChanges(mName, this);
//			break;
		
		case cmd_OpenIncludeFile:
			new MFindAndOpenDialog(mProject, this);
			break;
		
//		case cmd_RecheckFiles:
//			CheckIsOutOfDate();
//			break;
//		
//		case cmd_BringUpToDate:
//			BringUpToDate();
//			break;
//		
//		case cmd_Preprocess:
//			if (file != nil)
//				Preprocess(file->GetPath());
//			break;
//		
//		case cmd_CheckSyntax:
//			if (file != nil)
//				CheckSyntax(file->GetPath());
//			break;
//		
//		case cmd_Compile:
//			if (file != nil)
//				Compile(file->GetPath());
//			break;
//
//		case cmd_Disassemble:
//			if (file != nil)
//				Disassemble(file->GetPath());
//			break;
//		
//		case cmd_MakeClean:
//			MakeClean();
//			break;
//		
//		case cmd_Make:
//			Make();
//			break;
//		
//		case cmd_Stop:
//			StopBuilding();
//			break;
		
//		case cmd_NewGroup:
//		{
//			auto_ptr<MNewGroupDialog> dlog(new MNewGroupDialog);
//			dlog->Initialize(this);
//			dlog.release();
//			break;
//		}
//		
//		case cmd_EditProjectInfo:
//		{
//			auto_ptr<MProjectInfoDialog> dlog(new MProjectInfoDialog);
//			dlog->Initialize(this, mCurrentTarget);
//			dlog.release();
//			break;
//		}
		
//		case cmd_EditProjectPaths:
//		{
//			auto_ptr<MProjectPathsDialog> dlog(new MProjectPathsDialog);
//			dlog->Initialize(this, mUserSearchPaths, mSysSearchPaths,
//				mLibSearchPaths, mFrameworkPaths);
//			dlog.release();
//			break;
//		}
		
//		case cmd_ChangePanel:
//			switch (::GetControl32BitValue(mPanelSegmentRef))
//			{
//				case 1:	SelectPanel(ePanelFiles); break;
//				case 2: SelectPanel(ePanelLinkOrder); break;
//				case 3: SelectPanel(ePanelPackage); break;
//			}
//			break;
		
		default:
//			if ((inCommand & 0xFFFFFF00) == cmd_SwitchTarget)
//			{
//				uint32 target = inCommand & 0x000000FF;
//				SelectTarget(target);
////				Invalidate();
//				mFileList->Invalidate();
////				mLinkOrderList->Invalidate();
////				mPackageList->Invalidate();
//			}

			result = MDocWindow::ProcessCommand(inCommand, inMenu, inItemIndex);
			break;
	}
	
	return result;
}

// ---------------------------------------------------------------------------
//	SetStatus

void MProjectWindow::SetStatus(
	string			inStatus,
	bool			inHide)
{
	
}

// ---------------------------------------------------------------------------
//	SyncInterfaceWithProject

void MProjectWindow::SyncInterfaceWithProject()
{
	GtkWidget* wdgt = GetWidget(kTargetPopupID);
	THROW_IF_NIL(wdgt);

	GtkTreeModel* model = gtk_combo_box_get_model(GTK_COMBO_BOX(wdgt));
	int32 count = gtk_tree_model_iter_n_children(model, nil);

	while (count-- > 0)
		gtk_combo_box_remove_text(GTK_COMBO_BOX(wdgt), count);

	vector<MProjectTarget*> targets = mProject->GetTargets();
	for (vector<MProjectTarget*>::iterator t = targets.begin(); t != targets.end(); ++t)
		gtk_combo_box_append_text(GTK_COMBO_BOX(wdgt), (*t)->GetName().c_str());

	gtk_combo_box_set_active(GTK_COMBO_BOX(wdgt), 0);
}
