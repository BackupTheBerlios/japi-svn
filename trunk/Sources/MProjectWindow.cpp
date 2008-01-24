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

#include "MJapi.h"

#include <boost/bind.hpp>
#include <boost/algorithm/string.hpp>

#include "MProjectWindow.h"
#include "MProjectItem.h"
#include "MStrings.h"
#include "MError.h"
#include "MFindAndOpenDialog.h"
#include "MProject.h"
#include "MProjectTarget.h"
#include "MTreeModelInterface.h"
#include "MUtils.h"
#include "MPreferences.h"
#include "MAlerts.h"
#include "MJapiApp.h"

namespace ba = boost::algorithm;

using namespace std;

namespace {

struct MProjectState
{
	uint16			mWindowPosition[2];
	uint16			mWindowSize[2];
	uint8			mSelectedTarget;
	uint8			mSelectedPanel;
	uint8			mFillers[2];
	int32			mScrollPosition[ePanelCount];
	uint32			mSelectedFile;
	
	void			Swap();
};

const char
	kJapieProjectState[] = "com.hekkelman.japi.ProjectState";

const uint32
	kMProjectStateSize = 7 * sizeof(uint32); // sizeof(MProjectState);

void MProjectState::Swap()
{
	net_swapper swap;
	
	mWindowPosition[0] = swap(mWindowPosition[0]);
	mWindowPosition[1] = swap(mWindowPosition[1]);
	mWindowSize[0] = swap(mWindowSize[0]);
	mWindowSize[1] = swap(mWindowSize[1]);
	mScrollPosition[ePanelFiles] = swap(mScrollPosition[ePanelFiles]);
	mScrollPosition[ePanelLinkOrder] = swap(mScrollPosition[ePanelLinkOrder]);
	mScrollPosition[ePanelPackage] = swap(mScrollPosition[ePanelPackage]);
	mSelectedFile = swap(mSelectedFile);
}



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


class MProjectTreeModel : public MTreeModelInterface
{
  public:
					MProjectTreeModel(
						MProject*		inProject,
						MProjectGroup*	inItems);

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


	virtual bool	RowDraggable(
						GtkTreePath*		inPath);

	virtual bool	DragDataGet(
						GtkTreePath*		inPath,
						GtkSelectionData*	outData);

	virtual bool	DragDataDelete(
						GtkTreePath*		inPath);

	virtual bool	DragDataReceived(
						GtkTreePath*		inPath,
						GtkSelectionData*	inData);

	virtual bool	RowDropPossible(
						GtkTreePath*		inPath,
						GtkSelectionData*	inData);

	MEventIn<void(MProjectItem*)>			eProjectItemStatusChanged;
	MEventIn<void(MProjectItem*)>			eProjectItemInserted;
	MEventIn<void(MProjectGroup*,int32)>	eProjectItemRemoved;

  private:

	void			ProjectItemStatusChanged(
						MProjectItem*	inItem);

	void			ProjectItemInserted(
						MProjectItem*	inItem);

	void			ProjectItemRemoved(
						MProjectGroup*	inGroup,
						int32			inIndex);

	MProject*		mProject;
	MProjectGroup*	mItems;
};

MProjectTreeModel::MProjectTreeModel(
	MProject*			inProject,
	MProjectGroup*		inItems)
	: MTreeModelInterface(GTK_TREE_MODEL_ITERS_PERSIST)
	, eProjectItemStatusChanged(this, &MProjectTreeModel::ProjectItemStatusChanged)
	, eProjectItemInserted(this, &MProjectTreeModel::ProjectItemInserted)
	, eProjectItemRemoved(this, &MProjectTreeModel::ProjectItemRemoved)
	, mProject(inProject)
	, mItems(inItems)
{
	vector<MProjectItem*> items;
	mItems->Flatten(items);
	
	for (vector<MProjectItem*>::iterator i = items.begin(); i != items.end(); ++i)
		AddRoute((*i)->eStatusChanged, eProjectItemStatusChanged);

	if (inItems == inProject->GetFiles())
	{
		AddRoute(mProject->eInsertedFile, eProjectItemInserted);
		AddRoute(mProject->eRemovedFile, eProjectItemRemoved);
	}
	else
	{
		AddRoute(mProject->eInsertedResource, eProjectItemInserted);
		AddRoute(mProject->eRemovedResource, eProjectItemRemoved);
	}
}

uint32 MProjectTreeModel::GetColumnCount() const
{
	return 4;
}

GType MProjectTreeModel::GetColumnType(
	uint32			inColumn) const
{
	GType result;

	if (inColumn == kFilesDirtyColumn)
		result = GDK_TYPE_PIXMAP;
	else
		result = G_TYPE_STRING;
	
	return result;
}

void MProjectTreeModel::GetValue(
	GtkTreeIter*	inIter,
	uint32			inColumn,
	GValue*			outValue) const
{
	// dots
	
	const uint32 kDotSize = 6;
	const MColor
		kOutOfDateColor = MColor("#ff664c"),
		kCompilingColor = MColor("#ffbb6b");
	
	static GdkPixbuf* kOutOfDateDot = CreateDot(kOutOfDateColor, kDotSize);
	static GdkPixbuf* kCompilingDot = CreateDot(kCompilingColor, kDotSize);
	
	MProjectItem* item = reinterpret_cast<MProjectItem*>(inIter->user_data);

	if (item != nil)
	{
		switch (inColumn)
		{
			case kFilesDirtyColumn:
				g_value_init(outValue, G_TYPE_OBJECT);
				if (item->IsCompiling())
					g_value_set_object(outValue, kCompilingDot);
				else if (item->IsOutOfDate())
					g_value_set_object(outValue, kOutOfDateDot);
				break;
			
			case kFilesNameColumn:
				g_value_init(outValue, G_TYPE_STRING);
				g_value_set_string(outValue, item->GetName().c_str());
				break;
			
			case kFilesDataSizeColumn:
			case kFilesTextSizeColumn:
			{
				g_value_init(outValue, G_TYPE_STRING);
				uint32 size;
				if (inColumn == kFilesDataSizeColumn)
					size = item->GetDataSize();
				else
					size = item->GetTextSize();
				
				stringstream s;
				if (size >= 1024 * 1024 * 1024)
					s << (size / (1024 * 1024 * 1024)) << 'G';
				else if (size >= 1024 * 1024)
					s << (size / (1024 * 1024)) << 'M';
				else if (size >= 1024)
					s << (size / (1024)) << 'K';
				else if (size > 0)
					s << size;
				
				g_value_set_string(outValue, s.str().c_str());
				break;
			}
		}
	}
}

bool MProjectTreeModel::GetIter(
	GtkTreeIter*	outIter,
	GtkTreePath*	inPath)
{
	bool result = false;
	
	int32 depth = gtk_tree_path_get_depth(inPath);
	int32* indices = gtk_tree_path_get_indices(inPath);
	
	MProjectItem* item = mItems;
	
	for (int32 ix = 0; ix < depth and item != nil; ++ix)
	{
		MProjectGroup* group = dynamic_cast<MProjectGroup*>(item);
		if (group != nil)
			item = group->GetItem(indices[ix]);
		else
		{
			item = nil;
			break;
		}
	}
	
	outIter->user_data = item;

	if (item != nil)
		result = true;
	
	return result;
}

GtkTreePath* MProjectTreeModel::GetPath(
	GtkTreeIter*	inIter)
{
	GtkTreePath* path = gtk_tree_path_new();
	MProjectItem* item = reinterpret_cast<MProjectItem*>(inIter->user_data);
	
	while (item->GetParent() != nil)
	{
		gtk_tree_path_prepend_index(path, item->GetSiblingPosition());
		item = item->GetParent();
	}

	return path;
}

bool MProjectTreeModel::Next(
	GtkTreeIter*	ioIter)
{
	bool result = false;

	MProjectItem* item = reinterpret_cast<MProjectItem*>(ioIter->user_data);
	
	if (item != nil)
		item = item->GetNextSibling();

	if (item != nil)
	{
		ioIter->user_data = item;
		result = true;
	}
	
	return result;
}

bool MProjectTreeModel::Children(
	GtkTreeIter*	outIter,
	GtkTreeIter*	inParent)
{
	bool result = false;
	
	if (inParent == nil)
	{
		outIter->user_data = mItems;
		result = true;
	}
	else
	{
		MProjectItem* item = reinterpret_cast<MProjectItem*>(inParent->user_data);
		MProjectGroup* group = dynamic_cast<MProjectGroup*>(item);
		
		if (group != nil and group->Count() > 0)
		{
			outIter->user_data = group->GetItem(0);
			result = true;
		}
	}

	if (not result)
		outIter->user_data = 0;
	
	return result;
}

bool MProjectTreeModel::HasChildren(
	GtkTreeIter*	inIter)
{
	MProjectItem* item = reinterpret_cast<MProjectItem*>(inIter->user_data);
	MProjectGroup* group = dynamic_cast<MProjectGroup*>(item);
	
	return group != nil;
}

int32 MProjectTreeModel::CountChildren(
	GtkTreeIter*	inIter)
{
	int32 result = 0;
	
	if (inIter == nil)
		result = mItems->Count();
	else
	{
		MProjectItem* item = reinterpret_cast<MProjectItem*>(inIter->user_data);
		MProjectGroup* group = dynamic_cast<MProjectGroup*>(item);
		
		if (group != nil)
			result = group->Count();
	}

	return result;
}

bool MProjectTreeModel::GetChild(
	GtkTreeIter*	outIter,
	GtkTreeIter*	inParent,
	int32			inIndex)
{
	bool result = false;
	
	MProjectGroup* group;
	
	if (inParent == nil)
		group = mItems;
	else
	{
		MProjectItem* item = reinterpret_cast<MProjectItem*>(inParent->user_data);
		group = dynamic_cast<MProjectGroup*>(item);
	}
	
	if (group != nil and static_cast<uint32>(inIndex) < group->Count())
	{
		outIter->user_data = group->GetItem(inIndex);
		result = true;
	}
	else
		outIter->user_data = 0;

	return result;
}

bool MProjectTreeModel::GetParent(
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
	else
		outIter->user_data = 0;
	
	return result;
}

void MProjectTreeModel::ProjectItemStatusChanged(
	MProjectItem*	inItem)
{
	try
	{
		GtkTreeIter iter = {};
		
		iter.user_data = inItem;
		
		GtkTreePath* path = GetPath(&iter);
		if (path != nil)
		{
			DoRowChanged(path, &iter);
			gtk_tree_path_free(path);
		}
	}
	catch (...) {}
}

void MProjectTreeModel::ProjectItemInserted(
	MProjectItem*	inItem)
{
	try
	{
		GtkTreeIter iter = {};
		
		iter.user_data = inItem;
		
		GtkTreePath* path = GetPath(&iter);
		if (path != nil)
		{
			DoRowInserted(path, &iter);
			gtk_tree_path_free(path);
		}
	}
	catch (...) {}
}

void MProjectTreeModel::ProjectItemRemoved(
	MProjectGroup*	inGroup,
	int32			inIndex)
{
	try
	{
		GtkTreeIter iter = {};
		
		iter.user_data = inGroup;
		
		GtkTreePath* path = GetPath(&iter);
		if (path != nil)
		{
			gtk_tree_path_append_index(path, inIndex);
			DoRowDeleted(path);
			gtk_tree_path_free(path);
		}
	}
	catch (...) {}
}

bool MProjectTreeModel::RowDraggable(
	GtkTreePath*		inPath)
{
	return true;
}

bool MProjectTreeModel::DragDataGet(
	GtkTreePath*		inPath,
	GtkSelectionData*	outData)
{
	bool result = false;
	
	GtkTreeIter iter;
	if (GetIter(&iter, inPath))
	{
		stringstream s;
		vector<MProjectItem*> items;
		
		reinterpret_cast<MProjectItem*>(iter.user_data)->Flatten(items);
		
		for (vector<MProjectItem*>::iterator i = items.begin(); i != items.end(); ++i)
		{
			MProjectFile* file = dynamic_cast<MProjectFile*>(*i);
			if (file != nil)
				s << MUrl(file->GetPath()).str(true) << endl;
		}
		
		string data = s.str();
		
		gtk_selection_data_set(outData, outData->target,
			8, (guchar*)data.c_str(), data.length());
	}
	
	return result;
}

bool MProjectTreeModel::DragDataDelete(
	GtkTreePath*		inPath)
{
	bool result = false;
	
	GtkTreeIter iter;
	if (GetIter(&iter, inPath))
	{
		mProject->RemoveItem(reinterpret_cast<MProjectItem*>(iter.user_data));
		result = true;
	}
	
	return result;
}

bool MProjectTreeModel::DragDataReceived(
	GtkTreePath*		inPath,
	GtkSelectionData*	inData)
{
	bool result = false;

	// split the data into an array of files
	vector<string> files;
	boost::iterator_range<const char*> text(
		reinterpret_cast<const char*>(inData->data),
		reinterpret_cast<const char*>(inData->data + inData->length));
	ba::split(files, text, boost::is_any_of("\n\r"), boost::token_compress_on);
	
	// find the location where to insert the items
	MProjectGroup* group = mItems;

	int32 depth = gtk_tree_path_get_depth(inPath);
	int32* indices = gtk_tree_path_get_indices(inPath);
	int32 index = 0;
	
	for (int32 ix = 0; ix < depth - 1 and group != nil; ++ix)
	{
		if (dynamic_cast<MProjectGroup*>(group->GetItem(indices[ix])))
		{
			group = static_cast<MProjectGroup*>(group->GetItem(indices[ix]));
			index = indices[ix + 1];
		}
		else
			break;
	}
	
	// now add the files

	if (files.size() and group != nil)
	{
		mProject->AddFiles(files, group, index);
		result = true;
	}
	
	return result;
}

bool MProjectTreeModel::RowDropPossible(
	GtkTreePath*		inPath,
	GtkSelectionData*	inData)
{
	bool result = false;
	
	GtkTreeIter iter;
	if (GetIter(&iter, inPath))
	{
		MProjectItem* item = reinterpret_cast<MProjectItem*>(iter.user_data);
		result = dynamic_cast<MProjectGroup*>(item) != nil;
PRINT(("returning %s for RowDropPossible on row %s",
	result ? "true" : "false",
	item->GetName().c_str()));
	}

	return false;
}



}

MProjectWindow::MProjectWindow()
	: MDocWindow("project-window")
	, eStatus(this, &MProjectWindow::SetStatus)
	, eInvokeFileRow(this, &MProjectWindow::InvokeFileRow)
	, eInvokeResourceRow(this, &MProjectWindow::InvokeResourceRow)
	, eTargetChanged(this, &MProjectWindow::TargetChanged)
	, mProject(nil)
	, mFilesTree(nil)
	, mResourcesTree(nil)
	, mBusy(false)
{
	mController = new MController(this);
	
	mController->SetWindow(this);
	
	mMenubar.Initialize(GetWidget('mbar'), "project-window-menu");
	mMenubar.SetTarget(mController);

	GtkWidget* treeView = GetWidget(kFilesListViewID);
	InitializeTreeView(GTK_TREE_VIEW(treeView));
	eInvokeFileRow.Connect(treeView, "row-activated");

	treeView = GetWidget(kResourceViewID);
	InitializeTreeView(GTK_TREE_VIEW(treeView));
	eInvokeResourceRow.Connect(treeView, "row-activated");

	// status panel
	
	GtkWidget* statusBar = GetWidget('stat');

	GtkShadowType shadow_type;
	gtk_widget_style_get(statusBar, "shadow_type", &shadow_type, nil);

	GtkWidget* frame = gtk_frame_new(nil);
	gtk_frame_set_shadow_type(GTK_FRAME(frame), shadow_type);
	
	mStatusPanel = gtk_label_new(nil);
	gtk_label_set_single_line_mode(GTK_LABEL(mStatusPanel), true);
	gtk_misc_set_alignment(GTK_MISC(mStatusPanel), 0, 0.5);
	gtk_container_add(GTK_CONTAINER(frame), mStatusPanel);

	gtk_box_pack_start(GTK_BOX(statusBar), frame, false, false, 0);
	gtk_box_reorder_child(GTK_BOX(statusBar), frame, 0);
	
	gtk_widget_show_all(statusBar);

	eTargetChanged.Connect(GetGladeXML(), "on_targ_changed");

	ConnectChildSignals();
}

// ---------------------------------------------------------------------------
//	MProjectWindow::~MProjectWindow

MProjectWindow::~MProjectWindow()
{
	delete mFilesTree;
	mFilesTree = nil;
	
	delete mResourcesTree;
	mResourcesTree = nil;
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

	// Files tree
	GtkWidget* treeView = GetWidget(kFilesListViewID);
	THROW_IF_NIL((treeView));
	
	mFilesTree = new MProjectTreeModel(mProject, mProject->GetFiles());
	gtk_tree_view_set_model(GTK_TREE_VIEW(treeView), mFilesTree->GetModel());

	gtk_tree_view_expand_all(GTK_TREE_VIEW(treeView));

	// Resources tree

	treeView = GetWidget(kResourceViewID);
	THROW_IF_NIL((treeView));
	
	mResourcesTree = new MProjectTreeModel(mProject, mProject->GetResources());
	gtk_tree_view_set_model(GTK_TREE_VIEW(treeView), mResourcesTree->GetModel());

	gtk_tree_view_expand_all(GTK_TREE_VIEW(treeView));

	// read the project's state, if any
	
	if (Preferences::GetInteger("save state", 1))
	{
		MPath file = inDocument->GetURL().GetPath();
		
		MProjectState state = {};
		ssize_t r = read_attribute(file, kJapieProjectState, &state, kMProjectStateSize);
		if (r > 0 and static_cast<uint32>(r) == kMProjectStateSize)
		{
			state.Swap();

//			mFileList->ScrollToPosition(0, state.mScrollPosition[ePanelFiles]);
			
			if (state.mWindowSize[0] > 50 and state.mWindowSize[1] > 50 and
				state.mWindowSize[0] < 2000 and state.mWindowSize[1] < 2000)
			{
				MRect r(
					state.mWindowPosition[0], state.mWindowPosition[1],
					state.mWindowSize[0], state.mWindowSize[1]);
			
				SetWindowPosition(r);
			}
			
	//		mLinkOrderList->ScrollToPosition(::CGPointMake(0, state.mScrollPosition[ePanelLinkOrder]));
	//		mPackageList->ScrollToPosition(::CGPointMake(0, state.mScrollPosition[ePanelPackage]));
			
	//		if (state.mSelectedPanel < ePanelCount)
	//		{
	//			mPanel = static_cast<MProjectListPanel>(state.mSelectedPanel);
	//			::SetControl32BitValue(mPanelSegmentRef, uint32(mPanel) + 1);
	//			SelectPanel(mPanel);
	//		}
	//		else
	//			mPanel = ePanelFiles;
	//		
	//		::MoveWindow(GetSysWindow(),
	//			state.mWindowPosition[0], state.mWindowPosition[1], true);
	//
	//		::SizeWindow(GetSysWindow(),
	//			state.mWindowSize[0], state.mWindowSize[1], true);
	//
	//		::ConstrainWindowToScreen(GetSysWindow(),
	//			kWindowStructureRgn, kWindowConstrainStandardOptions,
	//			NULL, NULL);
	
			mProject->SelectTarget(state.mSelectedTarget);
//			mFileList->SelectItem(state.mSelectedFile);
		}
	}
	
	SyncInterfaceWithProject();
}

// ---------------------------------------------------------------------------
//	MProjectWindow::InvokeFileRow

void MProjectWindow::InvokeFileRow(
	GtkTreePath*		inPath,
	GtkTreeViewColumn*	inColumn)
{
	GtkTreeIter iter;
	if (gtk_tree_model_get_iter(mFilesTree->GetModel(), &iter, inPath))
	{
		MProjectItem* item = reinterpret_cast<MProjectItem*>(iter.user_data);
		MProjectFile* file = dynamic_cast<MProjectFile*>(item);
		if (file != nil)
		{
			MPath p = file->GetPath();
			gApp->OpenOneDocument(MUrl(p));
		}
	}
}

// ---------------------------------------------------------------------------
//	MProjectWindow::InvokeResourceRow

void MProjectWindow::InvokeResourceRow(
	GtkTreePath*		inPath,
	GtkTreeViewColumn*	inColumn)
{
	GtkTreeIter iter;
	if (gtk_tree_model_get_iter(mResourcesTree->GetModel(), &iter, inPath))
	{
		MProjectItem* item = reinterpret_cast<MProjectItem*>(iter.user_data);
		MProjectFile* file = dynamic_cast<MProjectFile*>(item);
		if (file != nil)
		{
			MPath p = file->GetPath();
			
			if (FileNameMatches("*.glade", p))
			{
				// start up glade
				string cmd = Preferences::GetString("glade", "glade-3");
				cmd += " ";
				cmd += p.string();
				cmd += "&";
				
				system(cmd.c_str());
			}
			else
				gApp->OpenOneDocument(MUrl(p));
		}
	}
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
	bool result = true;
	
	switch (inCommand)
	{
		case cmd_OpenIncludeFile:
			outEnabled = true;
			break;

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

	switch (inCommand)
	{
		case cmd_OpenIncludeFile:
			new MFindAndOpenDialog(mProject, this);
			break;
		
		default:
			result = MDocWindow::ProcessCommand(inCommand, inMenu, inItemIndex);
			break;
	}
	
	return result;
}

// ---------------------------------------------------------------------------
//	SetStatus

void MProjectWindow::SetStatus(
	string			inStatus,
	bool			inBusy)
{
	if (GTK_IS_LABEL(mStatusPanel))
	{
		gtk_label_set_text(GTK_LABEL(mStatusPanel), inStatus.c_str());
		
		if (mBusy != inBusy)
		{
			if (inBusy)
				gtk_widget_show(mStatusPanel);
			else
				gtk_widget_hide(mStatusPanel);

			mBusy = inBusy;
		}
	}
}

// ---------------------------------------------------------------------------
//	SyncInterfaceWithProject

void MProjectWindow::SyncInterfaceWithProject()
{
	GtkWidget* wdgt = GetWidget(kTargetPopupID);
	THROW_IF_NIL(wdgt);

	eTargetChanged.Block(wdgt, "on_targ_changed");

	GtkTreeModel* model = gtk_combo_box_get_model(GTK_COMBO_BOX(wdgt));
	int32 count = gtk_tree_model_iter_n_children(model, nil);

	while (count-- > 0)
		gtk_combo_box_remove_text(GTK_COMBO_BOX(wdgt), count);

	vector<MProjectTarget*> targets = mProject->GetTargets();
	for (vector<MProjectTarget*>::iterator t = targets.begin(); t != targets.end(); ++t)
		gtk_combo_box_append_text(GTK_COMBO_BOX(wdgt), (*t)->GetName().c_str());

	gtk_combo_box_set_active(GTK_COMBO_BOX(wdgt), mProject->GetSelectedTarget());

	eTargetChanged.Unblock(wdgt, "on_targ_changed");
}

// ---------------------------------------------------------------------------
//	TargetChanged

void MProjectWindow::TargetChanged()
{
	if (mProject != nil)
	{
		GtkWidget* wdgt = GetWidget('targ');
		if (GTK_IS_COMBO_BOX(wdgt))
			mProject->SelectTarget(gtk_combo_box_get_active(GTK_COMBO_BOX(wdgt)));
	}
}

// ---------------------------------------------------------------------------
//	DocumentChanged

void MProjectWindow::DocumentChanged(
	MDocument*		inDocument)
{
	if (inDocument != mProject)
	{
		delete mFilesTree;
		mFilesTree = nil;
		
		mProject = dynamic_cast<MProject*>(inDocument);
	}
	
	MDocWindow::DocumentChanged(inDocument);
}

// ---------------------------------------------------------------------------
//	DoClose

bool MProjectWindow::DoClose()
{
	bool result = false;

	if (mBusy == false or
		DisplayAlert("stop-building-alert", mProject->GetName()))
	{
		if (mBusy)
			mProject->StopBuilding();
		
		if (mProject != nil)
			SaveState();
		
		result = MDocWindow::DoClose();
	}
	
	return result;
}

// ---------------------------------------------------------------------------
//	SaveState

void MProjectWindow::SaveState()
{
	try
	{
		MPath file = mProject->GetURL().GetPath();
		MProjectState state = { };

		(void)read_attribute(file, kJapieProjectState, &state, kMProjectStateSize);
		
		state.Swap();

//		state.mSelectedFile = mFileList->GetSelected();
		state.mSelectedTarget = mProject->GetSelectedTarget();

//		int32 x;
//		mFileList->GetScrollPosition(x, state.mScrollPosition[ePanelFiles]);
//				mLinkOrderList->GetScrollPosition(pt);
//				state.mScrollPosition[ePanelLinkOrder] = static_cast<uint32>(pt.y);
//				mPackageList->GetScrollPosition(pt);
//				state.mScrollPosition[ePanelPackage] = static_cast<uint32>(pt.y);
//		
//		state.mSelectedPanel = mPanel;

		MRect r;
		GetWindowPosition(r);
		state.mWindowPosition[0] = r.x;
		state.mWindowPosition[1] = r.y;
		state.mWindowSize[0] = r.width;
		state.mWindowSize[1] = r.height;

		state.Swap();
		
		write_attribute(file, kJapieProjectState, &state, kMProjectStateSize);
	}
	catch (...) {}
}

// ---------------------------------------------------------------------------
//	InitializeTreeView

void MProjectWindow::InitializeTreeView(
	GtkTreeView*	inGtkTreeView)
{
	THROW_IF_NIL(inGtkTreeView);

	const GtkTargetEntry targets[] =
	{
	    { "text/uri-list", 0, 1 },
	};

	gtk_tree_view_enable_model_drag_dest(inGtkTreeView,
		targets, 1, GdkDragAction(GDK_ACTION_COPY|GDK_ACTION_MOVE));
	gtk_tree_view_enable_model_drag_source(inGtkTreeView,
		GDK_BUTTON1_MASK, targets, 1, GdkDragAction(GDK_ACTION_COPY|GDK_ACTION_MOVE));
	
	GtkTreeSelection* selection = gtk_tree_view_get_selection(inGtkTreeView);
	if (selection != nil)
		gtk_tree_selection_set_mode(selection, GTK_SELECTION_MULTIPLE);

	// Add the columns and renderers

	// the name column
	GtkCellRenderer* renderer = gtk_cell_renderer_text_new();
	GtkTreeViewColumn* column = gtk_tree_view_column_new_with_attributes (
		_("File"), renderer, "text", kFilesNameColumn, nil);
	g_object_set(G_OBJECT(column), "expand", true, nil);
	gtk_tree_view_append_column(inGtkTreeView, column);
	
	// the text size column
	
	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes (
		_("Text"), renderer, "text", kFilesTextSizeColumn, nil);
	g_object_set(G_OBJECT(renderer), "xalign", 1.0f, nil);
	gtk_tree_view_column_set_alignment(column, 1.0f);
	gtk_tree_view_append_column(inGtkTreeView, column);

	// the data size column
	
	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes (
		_("Data"), renderer, "text", kFilesDataSizeColumn, nil);
	g_object_set(G_OBJECT(renderer), "xalign", 1.0f, nil);
	gtk_tree_view_column_set_alignment(column, 1.0f);
	gtk_tree_view_append_column(inGtkTreeView, column);

	// at last the dirty mark	
	renderer = gtk_cell_renderer_pixbuf_new();
	column = gtk_tree_view_column_new_with_attributes(
		_(" "), renderer, "pixbuf", kFilesDirtyColumn, nil);
	gtk_tree_view_append_column(inGtkTreeView, column);

	gtk_widget_show_all(GTK_WIDGET(inGtkTreeView));
}
