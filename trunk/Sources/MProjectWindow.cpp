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
#include "MNewGroupDialog.h"

namespace ba = boost::algorithm;

using namespace std;

namespace {

enum MTreeDragKind {
	kTreeDragItemTreeItem,
	kTreeDragItemURI
};

const GtkTargetEntry kTreeDragTargets[] =
{
	{ "application/x-japi-tree-item", GTK_TARGET_SAME_WIDGET, 0 },
    { "text/uri-list", 0, 0 },
};

const GdkAtom kTreeDragTargetAtoms[] =
{
	gdk_atom_intern(kTreeDragTargets[kTreeDragItemTreeItem].target, false),
	gdk_atom_intern(kTreeDragTargets[kTreeDragItemURI].target, false),
};

struct MProjectState
{
	uint16			mWindowPosition[2];
	uint16			mWindowSize[2];
	uint8			mSelectedTarget;
	uint8			mSelectedPanel;
	uint8			mFillers[2];
//	int32			mScrollPosition[ePanelCount];
//	uint32			mSelectedFile;
	
	void			Swap();
};

const char
	kJapieProjectState[] = "com.hekkelman.japi.ProjectState";

const uint32
	kMProjectStateSize = sizeof(MProjectState);

void MProjectState::Swap()
{
	net_swapper swap;
	
	mWindowPosition[0] = swap(mWindowPosition[0]);
	mWindowPosition[1] = swap(mWindowPosition[1]);
	mWindowSize[0] = swap(mWindowSize[0]);
	mWindowSize[1] = swap(mWindowSize[1]);
//	mScrollPosition[ePanelFiles] = swap(mScrollPosition[ePanelFiles]);
//	mScrollPosition[ePanelLinkOrder] = swap(mScrollPosition[ePanelLinkOrder]);
//	mScrollPosition[ePanelPackage] = swap(mScrollPosition[ePanelPackage]);
//	mSelectedFile = swap(mSelectedFile);
}

enum {
	kFilesListViewID	= 'tre1',
	kLinkOrderViewID	= 'tre2',
	kResourceViewID		= 'tre3',
	
	kTargetPopupID		= 'targ',
	
	kNoteBookID			= 'note'
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
		if (group != nil and indices[ix] < group->Count())
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
		gtk_tree_path_prepend_index(path, item->GetPosition());
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
		item = item->GetNext();

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
	
	return group != nil and group->Count() > 0;
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
	
	if (group != nil and inIndex < group->Count())
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
		result = true;

		if (outData->target == kTreeDragTargetAtoms[kTreeDragItemURI])
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
		else if (outData->target == kTreeDragTargetAtoms[kTreeDragItemTreeItem])
		{
			char* p = gtk_tree_path_to_string(inPath);
			gtk_selection_data_set(outData, outData->target,
				8, (guchar*)p, strlen(p) + 1);
			g_free(p);
		}
		else
			result = false;
	}
	
	return result;
}

bool MProjectTreeModel::DragDataDelete(
	GtkTreePath*		inPath)
{
	bool result = false;
	
//	GtkTreeIter iter;
//	if (GetIter(&iter, inPath))
//	{
//		mProject->RemoveItem(reinterpret_cast<MProjectItem*>(iter.user_data));
//		result = true;
//	}
	
	return result;
}

bool MProjectTreeModel::DragDataReceived(
	GtkTreePath*		inPath,
	GtkSelectionData*	inData)
{
	bool result = false;

	// find the location where to insert the items
	MProjectGroup* group = mItems;

	int32 depth = gtk_tree_path_get_depth(inPath);
	int32* indices = gtk_tree_path_get_indices(inPath);
	int32 index = 0;
	
	for (int32 ix = 0; ix < depth - 1 and group != nil; ++ix)
	{
		if (indices[ix] < group->Count() and
			dynamic_cast<MProjectGroup*>(group->GetItem(indices[ix])))
		{
			group = static_cast<MProjectGroup*>(group->GetItem(indices[ix]));
			index = indices[ix + 1];
		}
		else
			break;
	}
	
	if (inData->target == kTreeDragTargetAtoms[kTreeDragItemURI])
	{
		// split the data into an array of files
		vector<string> files;
		boost::iterator_range<const char*> text(
			reinterpret_cast<const char*>(inData->data),
			reinterpret_cast<const char*>(inData->data + inData->length));
		ba::split(files, text, boost::is_any_of("\n\r"), boost::token_compress_on);
		
		// now add the files
	
		if (files.size() and group != nil)
		{
			mProject->AddFiles(files, group, index);
			result = true;
		}
	}
	else if (inData->target == kTreeDragTargetAtoms[kTreeDragItemTreeItem])
	{
		GtkTreePath* path = gtk_tree_path_new_from_string((const gchar*)inData->data);
		
		GtkTreeIter iter;
		if (GetIter(&iter, path))
		{
			mProject->MoveItem(reinterpret_cast<MProjectItem*>(iter.user_data),
				group, index);
			result = true;
		}
		
		if (path != nil)
			gtk_tree_path_free(path);
	}
	
	return result;
}

bool MProjectTreeModel::RowDropPossible(
	GtkTreePath*		inPath,
	GtkSelectionData*	inData)
{
	// find the location where to insert the items
	MProjectGroup* group = mItems;

	int32 depth = gtk_tree_path_get_depth(inPath);
	int32* indices = gtk_tree_path_get_indices(inPath);
	int32 index = 0;
	
	for (int32 ix = 0; ix < depth - 1 and group != nil; ++ix)
	{
		if (indices[ix] < group->Count() and
			dynamic_cast<MProjectGroup*>(group->GetItem(indices[ix])))
		{
			group = static_cast<MProjectGroup*>(group->GetItem(indices[ix]));
			index = indices[ix + 1];
		}
		else
			break;
	}
	
	return group != nil and index <= group->Count();
}

class MGtkWidget
{
  public:
				MGtkWidget(
					GtkWidget*		inWidget)
					: mGtkWidget(inWidget)
				{
//					g_object_ref(G_OBJECT(mGtkWidget));
				}

	virtual		~MGtkWidget()
				{
//					g_object_unref(G_OBJECT(mGtkWidget));
				}
				
				operator GtkWidget*()		{ return mGtkWidget; }

  protected:
	GtkWidget*	mGtkWidget;

  private:
				MGtkWidget(
					const MGtkWidget&	rhs);
				
	MGtkWidget&	operator=(
					const MGtkWidget&	rhs);
};

class MGtkNotebook : public MGtkWidget
{
  public:
				MGtkNotebook(
					GtkWidget*		inWidget)
					: MGtkWidget(inWidget)
				{
					assert(GTK_IS_NOTEBOOK(mGtkWidget));
				}
	
				operator GtkNotebook*()		{ return GTK_NOTEBOOK(mGtkWidget); }
	
	int32		GetPage() const
				{
					return gtk_notebook_get_current_page(GTK_NOTEBOOK(mGtkWidget));
				}

	void		SetPage(
					int32			inPage)
				{
					return gtk_notebook_set_current_page(GTK_NOTEBOOK(mGtkWidget), inPage);
				}
};

class MGtkTreeView : public MGtkWidget
{
  public:
				MGtkTreeView(
					GtkWidget*		inWidget)
					: MGtkWidget(inWidget)
				{
					assert(GTK_IS_TREE_VIEW(mGtkWidget));
				}	

				operator GtkTreeView*()		{ return GTK_TREE_VIEW(mGtkWidget); }

	void		SetModel(
					GtkTreeModel*	inModel)
				{
					gtk_tree_view_set_model(GTK_TREE_VIEW(mGtkWidget), inModel);
				}

	void		ExpandAll()
				{
					gtk_tree_view_expand_all(GTK_TREE_VIEW(mGtkWidget));
				}

	bool		GetFirstSelectedRow(
					GtkTreePath*&	outRow)
				{
					bool result = false;
					GList* rows = gtk_tree_selection_get_selected_rows(
						gtk_tree_view_get_selection(GTK_TREE_VIEW(mGtkWidget)),
						nil);
					
					if (rows != nil)
					{
						outRow = gtk_tree_path_copy((GtkTreePath*)rows->data);
						result = outRow != nil;
						
						g_list_foreach(rows, (GFunc)(gtk_tree_path_free), nil);
						g_list_free(rows);
					}
					
					return result;
				}

	void		GetSelectedRows(
					vector<GtkTreePath*>&	outRows)
				{
					GList* rows = gtk_tree_selection_get_selected_rows(
						gtk_tree_view_get_selection(GTK_TREE_VIEW(mGtkWidget)),
						nil);
					
					if (rows != nil)
					{
						GList* row = rows;
						while (row != nil)
						{
							outRows.push_back(gtk_tree_path_copy((GtkTreePath*)row->data));
							row = row->next;
						}
						
						g_list_foreach(rows, (GFunc)(gtk_tree_path_free), nil);
						g_list_free(rows);
					}
				}
	
//	int32		GetScrollPosition() const
//				{
//					GdkRectangle r;
//					gtk_tree_view_get_visible_rect(GTK_TREE_VIEW(mGtkWidget), &r);
//					return r.y;
//				}
//	
//	void		SetScrollPosition(
//					int32			inPosition)
//				{
//					gtk_tree_view_scroll_to_point(GTK_TREE_VIEW(mGtkWidget), -1, inPosition);
//				}
};

class MGtkComboBox : public MGtkWidget
{
  public:
				MGtkComboBox(
					GtkWidget*		inWidget)
					: MGtkWidget(inWidget)
				{
					assert(GTK_IS_COMBO_BOX(mGtkWidget));
				}
	
	int32		GetActive() const
				{
					return gtk_combo_box_get_active(GTK_COMBO_BOX(mGtkWidget));
				}
	
	void		SetActive(
					int32			inActive)
				{
					gtk_combo_box_set_active(GTK_COMBO_BOX(mGtkWidget), inActive);
				}
	
	void		RemoveAll()
				{
					GtkTreeModel* model = gtk_combo_box_get_model(GTK_COMBO_BOX(mGtkWidget));
					int32 count = gtk_tree_model_iter_n_children(model, nil);
				
					while (count-- > 0)
						gtk_combo_box_remove_text(GTK_COMBO_BOX(mGtkWidget), count);
				}

	void		Append(
					const string&	inLabel)
				{
					gtk_combo_box_append_text(GTK_COMBO_BOX(mGtkWidget), inLabel.c_str());
				}
};

}

MProjectWindow::MProjectWindow()
	: MDocWindow("project-window")
	, eStatus(this, &MProjectWindow::SetStatus)
	, eInvokeFileRow(this, &MProjectWindow::InvokeFileRow)
	, eInvokeResourceRow(this, &MProjectWindow::InvokeResourceRow)
	, eKeyPressEvent(this, &MProjectWindow::OnKeyPressEvent)
	, eTargetChanged(this, &MProjectWindow::TargetChanged)
	, eInfoClicked(this, &MProjectWindow::InfoClicked)
	, eMakeClicked(this, &MProjectWindow::MakeClicked)
	, mProject(nil)
	, mFilesTree(nil)
	, mResourcesTree(nil)
	, mBusy(false)
{
	mController = new MController(this);
	
	mMenubar.Initialize(GetWidget('mbar'), "project-window-menu");
	mMenubar.SetTarget(mController);

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
	eInfoClicked.Connect(GetGladeXML(), "on_info_clicked");
	eMakeClicked.Connect(GetGladeXML(), "on_make_clicked");

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
	MGtkTreeView filesTree(GetWidget(kFilesListViewID));
	InitializeTreeView(filesTree, ePanelFiles);
	eInvokeFileRow.Connect(filesTree, "row-activated");
	mFilesTree = new MProjectTreeModel(mProject, mProject->GetFiles());
	filesTree.SetModel(mFilesTree->GetModel());
	filesTree.ExpandAll();

	// Resources tree

	MGtkTreeView resourcesTree(GetWidget(kResourceViewID));
	InitializeTreeView(resourcesTree, ePanelPackage);
	eInvokeResourceRow.Connect(resourcesTree, "row-activated");
	mResourcesTree = new MProjectTreeModel(mProject, mProject->GetResources());
	resourcesTree.SetModel(mResourcesTree->GetModel());
	resourcesTree.ExpandAll();

	// initialize interface

	SyncInterfaceWithProject();

	// read the project's state, if any
	
	bool useState = false;
	MProjectState state = {};
	
	if (Preferences::GetInteger("save state", 1))
	{
		MPath file = inDocument->GetURL().GetPath();
		
		ssize_t r = read_attribute(file, kJapieProjectState, &state, kMProjectStateSize);
		
		useState = r > 0 and static_cast<uint32>(r) == kMProjectStateSize;
	}
	
	if (useState)
	{
		state.Swap();

		if (state.mWindowSize[0] > 50 and state.mWindowSize[1] > 50 and
			state.mWindowSize[0] < 2000 and state.mWindowSize[1] < 2000)
		{
			MRect r(
				state.mWindowPosition[0], state.mWindowPosition[1],
				state.mWindowSize[0], state.mWindowSize[1]);
		
			SetWindowPosition(r);
		}
		
		MGtkNotebook book(GetWidget(kNoteBookID));
		book.SetPage(state.mSelectedPanel);
		
//		treeView = MGtkTreeView(GetWidget(kFilesListViewID));
//		treeView.SetScrollPosition(state.mScrollPosition[ePanelFiles]);
//
//		treeView = MGtkTreeView(GetWidget(kResourceViewID));
//		treeView.SetScrollPosition(state.mScrollPosition[ePanelPackage]);

		mProject->SelectTarget(state.mSelectedTarget);
	}
	else
	{
		mProject->SelectTarget(0);
	}
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
	
	MGtkNotebook notebook(GetWidget(kNoteBookID));
	
	switch (inCommand)
	{
		case cmd_AddFileToProject:
		case cmd_OpenIncludeFile:
			outEnabled = true;
			break;

		case cmd_NewGroup:
			outEnabled = notebook.GetPage() != ePanelLinkOrder;
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
		case cmd_AddFileToProject:
			AddFilesToProject();
			break;
		
		case cmd_NewGroup:
			new MNewGroupDialog(this);
			break;
		
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
	MGtkComboBox targetPopup(GetWidget(kTargetPopupID));

	eTargetChanged.Block(targetPopup, "on_targ_changed");
	
	targetPopup.RemoveAll();

	vector<MProjectTarget*> targets = mProject->GetTargets();
	for (vector<MProjectTarget*>::iterator t = targets.begin(); t != targets.end(); ++t)
		targetPopup.Append((*t)->GetName());

	targetPopup.SetActive(mProject->GetSelectedTarget());

	eTargetChanged.Unblock(targetPopup, "on_targ_changed");
}

// ---------------------------------------------------------------------------
//	TargetChanged

void MProjectWindow::TargetChanged()
{
	if (mProject != nil)
	{
		MGtkComboBox targetPopup(GetWidget('targ'));
		mProject->SelectTarget(targetPopup.GetActive());
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
		
		delete mResourcesTree;
		mResourcesTree = nil;
		
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
		
		if (mProject != nil and mProject->IsSpecified())
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

		state.mSelectedTarget = mProject->GetSelectedTarget();

		MGtkNotebook book(GetWidget(kNoteBookID));
		state.mSelectedPanel = book.GetPage();

//		MGtkTreeView treeView(GetWidget(kFilesListViewID));
//		state.mScrollPosition[ePanelFiles] = treeView.GetScrollPosition();
//
//		treeView = MGtkTreeView(GetWidget(kResourceViewID));
//		state.mScrollPosition[ePanelPackage] = treeView.GetScrollPosition();

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
	GtkTreeView*		inGtkTreeView,
	int32				inPanel)
{
	THROW_IF_NIL(inGtkTreeView);

	gtk_tree_view_enable_model_drag_dest(inGtkTreeView,
		kTreeDragTargets, sizeof(kTreeDragTargets) / sizeof(GtkTargetEntry),
		GdkDragAction(GDK_ACTION_COPY|GDK_ACTION_MOVE));
	gtk_tree_view_enable_model_drag_source(inGtkTreeView, GDK_BUTTON1_MASK,
		kTreeDragTargets, sizeof(kTreeDragTargets) / sizeof(GtkTargetEntry),
		GdkDragAction(GDK_ACTION_COPY|GDK_ACTION_MOVE));
	
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
	
	if (inPanel == ePanelFiles or inPanel == ePanelLinkOrder)
	{
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
	}
	else if (inPanel == ePanelPackage)
	{
		// the data size column
		
		renderer = gtk_cell_renderer_text_new();
		column = gtk_tree_view_column_new_with_attributes (
			_("Size"), renderer, "text", kFilesDataSizeColumn, nil);
		g_object_set(G_OBJECT(renderer), "xalign", 1.0f, nil);
		gtk_tree_view_column_set_alignment(column, 1.0f);
		gtk_tree_view_append_column(inGtkTreeView, column);
	}
	
	// at last the dirty mark	
	renderer = gtk_cell_renderer_pixbuf_new();
	column = gtk_tree_view_column_new_with_attributes(
		_(" "), renderer, "pixbuf", kFilesDirtyColumn, nil);
	gtk_tree_view_append_column(inGtkTreeView, column);

	gtk_widget_show_all(GTK_WIDGET(inGtkTreeView));
	
	eKeyPressEvent.Connect(G_OBJECT(inGtkTreeView), "key-press-event");
}

// ---------------------------------------------------------------------------
//	CreateNewGroup

void MProjectWindow::CreateNewGroup(
	const string&		inGroupName)
{
	MGtkNotebook notebook(GetWidget(kNoteBookID));
	
	if (notebook.GetPage() == ePanelFiles)
	{
		MGtkTreeView treeView(GetWidget(kFilesListViewID));
		
		MProjectGroup* group = mProject->GetFiles();
		int32 index = 0;
		
		GtkTreePath* path;
		GtkTreeIter iter;
		
		if (treeView.GetFirstSelectedRow(path) and
			mFilesTree->GetIter(&iter, path))
		{
			MProjectItem* item = reinterpret_cast<MProjectItem*>(iter.user_data);
			group = item->GetParent();
			index = item->GetPosition();
		}
			
		mProject->CreateNewGroup(inGroupName, group, index);
		
		if (path != nil)
			gtk_tree_path_free(path);
	}
	else
	{
		MGtkTreeView treeView(GetWidget(kResourceViewID));
		
	}
}

// ---------------------------------------------------------------------------
//	AddFilesToProject

void MProjectWindow::AddFilesToProject()
{
	vector<MUrl> urls;
	if (ChooseFiles(true, urls))
	{
		MProjectGroup* group = mProject->GetFiles();
		int32 index = 0;

		MGtkNotebook notebook(GetWidget(kNoteBookID));
		
		if (notebook.GetPage() == ePanelFiles)
		{
			MGtkTreeView treeView(GetWidget(kFilesListViewID));

			GtkTreePath* path;
			GtkTreeIter iter;
			
			if (treeView.GetFirstSelectedRow(path) and
				mFilesTree->GetIter(&iter, path))
			{
				MProjectItem* item = reinterpret_cast<MProjectItem*>(iter.user_data);
				group = item->GetParent();
				index = item->GetPosition();
			}
		}
		
		vector<string> files;
		transform(urls.begin(), urls.end(), back_inserter(files),
			boost::bind(&MUrl::str, _1, false));
		
		mProject->AddFiles(files, group, index);
	}
}

// ---------------------------------------------------------------------------
//	DeleteSelectedItems

void MProjectWindow::DeleteSelectedItems()
{
	vector<MProjectItem*> items;
	vector<GtkTreePath*> paths;

	MGtkNotebook notebook(GetWidget(kNoteBookID));
	if (notebook.GetPage() == ePanelFiles)
	{
		MGtkTreeView treeView(GetWidget(kFilesListViewID));
		treeView.GetSelectedRows(paths);
		
		for (vector<GtkTreePath*>::iterator path = paths.begin(); path != paths.end(); ++path)
		{
			GtkTreeIter iter;
			if (mFilesTree->GetIter(&iter, *path))
				items.push_back(reinterpret_cast<MProjectItem*>(iter.user_data));
			gtk_tree_path_free(*path);
		}
	}
	else
	{
		MGtkTreeView treeView(GetWidget(kResourceViewID));
		treeView.GetSelectedRows(paths);

		for (vector<GtkTreePath*>::iterator path = paths.begin(); path != paths.end(); ++path)
		{
			GtkTreeIter iter;
			if (mResourcesTree->GetIter(&iter, *path))
				items.push_back(reinterpret_cast<MProjectItem*>(iter.user_data));
			gtk_tree_path_free(*path);
		}
	}
	
	mProject->RemoveItems(items);
}

// ---------------------------------------------------------------------------
//	InfoClicked

void MProjectWindow::InfoClicked()
{
}

// ---------------------------------------------------------------------------
//	MakeClicked

void MProjectWindow::MakeClicked()
{
	mProject->Make();
}

// ---------------------------------------------------------------------------
//	OnKeyPressEvent

bool MProjectWindow::OnKeyPressEvent(
	GdkEventKey*	inEvent)
{
	bool result = false;
	
	uint32 modifiers = inEvent->state & gtk_accelerator_get_default_mod_mask();
	uint32 keyValue = inEvent->keyval;
	
	if (modifiers == 0 and (keyValue == GDK_BackSpace or keyValue == GDK_Delete))
	{
		DeleteSelectedItems();
		result = true;
	}
	
	return result;
}