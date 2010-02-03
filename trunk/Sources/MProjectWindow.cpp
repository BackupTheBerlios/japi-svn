//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "MJapi.h"

#include <boost/bind.hpp>
#include <boost/algorithm/string.hpp>

#include "MProjectWindow.h"
#include "MProjectItem.h"
#include "MStrings.h"
#include "MError.h"
#include "MFindAndOpenDialog.h"
#include "MProject.h"
#include "MTreeModelInterface.h"
#include "MUtils.h"
#include "MPreferences.h"
#include "MAlerts.h"
#include "MJapiApp.h"
#include "MProjectInfoDialog.h"
#include "MLibraryInfoDialog.h"
#include "MGtkWrappers.h"
//#include "MProjectTree.h"

#include "MList.h"

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

}

namespace {

const uint32 kDotSize = 6;
const MColor
	kOutOfDateColor = MColor("#ff664c"),
	kCompilingColor = MColor("#ffbb6b");

GdkPixbuf* GetOutOfDateDot()
{
	static GdkPixbuf* kOutOfDateDot = CreateDot(kOutOfDateColor, kDotSize);
	return kOutOfDateDot;
}

GdkPixbuf* GetCompilingDot()
{
	static GdkPixbuf* kCompilingDot = CreateDot(kCompilingColor, kDotSize);
	return kCompilingDot;
}

void GetSize(
	uint32		inSize,
	string&		outSizeAsString)
{
	stringstream s;
	if (inSize >= 1024 * 1024 * 1024)
		s << (inSize / (1024 * 1024 * 1024)) << 'G';
	else if (inSize >= 1024 * 1024)
		s << (inSize / (1024 * 1024)) << 'M';
	else if (inSize >= 1024)
		s << (inSize / (1024)) << 'K';
	else if (inSize > 0)
		s << inSize;
	outSizeAsString = s.str();
}

}

//---------------------------------------------------------------------
// MFileRowItem

namespace MItemColumns
{
	struct name {};
	struct text {};
	struct data {};
	struct dirty {};
}

class MFileRowItem : public MListRow<
	MFileRowItem,
	MItemColumns::name,		string,
	MItemColumns::text,		string,
	MItemColumns::data,		string,
	MItemColumns::dirty,	GdkPixbuf*
>
{
  public:
					MFileRowItem(
						MProjectItem*	inItem)
						: mItem(inItem)
					{
					}
				
	void			GetData(
						const MItemColumns::name&,
						string&			outName)
					{
						outName = mItem->GetDisplayName();
					}

	void			GetData(
						const MItemColumns::text&,
						string&			outSize)
					{
						GetSize(mItem->GetTextSize(), outSize);
					}

	void			GetData(
						const MItemColumns::data&,
						string&			outSize)
					{
						GetSize(mItem->GetDataSize(), outSize);
					}

	void			GetData(
						const MItemColumns::dirty&,
						GdkPixbuf*&		outPixbuf)
					{
						if (mItem->IsCompiling())
							outPixbuf = GetCompilingDot();
						else if (mItem->IsOutOfDate())
							outPixbuf = GetOutOfDateDot();
						else
							outPixbuf = nil;
					}

	virtual bool	RowDropPossible() const		{ return dynamic_cast<MProjectGroup*>(mItem) != nil; }

	MProjectItem*	mItem;
};

class MRsrcRowItem : public MListRow<
	MRsrcRowItem,
	MItemColumns::name,		string,
	MItemColumns::data,		string,
	MItemColumns::dirty,	GdkPixbuf*
>
{
  public:
					MRsrcRowItem(
						MProjectItem*	inItem)
						: mItem(inItem)
					{
					}
				
	void			GetData(
						const MItemColumns::name&,
						string&			outName)
					{
						outName = mItem->GetDisplayName();
					}

	void			GetData(
						const MItemColumns::data&,
						string&			outSize)
					{
						GetSize(mItem->GetDataSize(), outSize);
					}

	void			GetData(
						const MItemColumns::dirty&,
						GdkPixbuf*&		outPixbuf)
					{
						if (mItem->IsCompiling())
							outPixbuf = GetCompilingDot();
						else if (mItem->IsOutOfDate())
							outPixbuf = GetOutOfDateDot();
						else
							outPixbuf = nil;
					}

	virtual bool	RowDropPossible() const		{ return dynamic_cast<MProjectGroup*>(mItem) != nil; }

	MProjectItem*	mItem;
};

//---------------------------------------------------------------------
// MProjectWindow

MProjectWindow::MProjectWindow()
	: MDocWindow("project-window")
	, eStatus(this, &MProjectWindow::SetStatus)
	, eSelectFileRow(this, &MProjectWindow::SelectFileRow)
	, eInvokeFileRow(this, &MProjectWindow::InvokeFileRow)
	, eInvokeResourceRow(this, &MProjectWindow::InvokeResourceRow)
	, eKeyPressEvent(this, &MProjectWindow::OnKeyPressEvent)
	, eTargetChanged(this, &MProjectWindow::TargetChanged)
	, eTargetsChanged(this, &MProjectWindow::TargetsChanged)
	, eInfoClicked(this, &MProjectWindow::InfoClicked)
	, eMakeClicked(this, &MProjectWindow::MakeClicked)
	, mEditedFileGroupName(this, &MProjectWindow::EditedFileGroupName)
	, mEditedResourceGroupName(this, &MProjectWindow::EditedResourceGroupName)
	, mProject(nil)
	, mBusy(false)
	, mEditingName(false)
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

	eTargetChanged.Connect(this, "on_targ_changed");
	eInfoClicked.Connect(this, "on_info_clicked");
	eMakeClicked.Connect(this, "on_make_clicked");

	ConnectChildSignals();
}

// ---------------------------------------------------------------------------
//	MProjectWindow::~MProjectWindow

MProjectWindow::~MProjectWindow()
{
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
	AddRoute(mProject->eTargetsChanged, eTargetsChanged);
	
	MDocWindow::Initialize(inDocument);

	// Files tree
	
	MList<MFileRowItem>* fileTree = new MList<MFileRowItem>(GetWidget(kFilesListViewID));
	mFileTree = fileTree;
	AddRoute(fileTree->eRowSelected, eSelectFileRow);
	AddRoute(fileTree->eRowInvoked, eInvokeFileRow);
	AddRoute(fileTree->eRowEdited, mEditedFileGroupName);
	mFileTree->SetColumnTitle(0, _("File"));
	mFileTree->SetExpandColumn(0);
	mFileTree->SetColumnTitle(1, _("Tekst"));
	mFileTree->SetColumnTitle(2, _("Data"));
	
	AddFileItemsToList(mProject->GetFiles(), nil, mFileTree);
	mFileTree->ExpandAll();
	
//	MGtkTreeView filesTree(GetWidget(kFilesListViewID));
//	InitializeTreeView(filesTree, ePanelFiles);
//	eInvokeFileRow.Connect(filesTree, "row-activated");
//	mFilesTree = new MProjectTree(mProject->GetFiles());
//
//	AddRoute(mProject->eProjectItemRemoved, mFilesTree->eProjectItemRemoved);
//	AddRoute(mProject->eProjectItemMoved, mFilesTree->eProjectItemMoved);
//	AddRoute(mProject->eProjectCreateFileItem, mFilesTree->eProjectCreateItem);
//	
//	filesTree.SetModel(mFilesTree->GetModel());
//	filesTree.ExpandAll();

	// Resources tree

//	MGtkTreeView resourcesTree(GetWidget(kResourceViewID));
//	InitializeTreeView(resourcesTree, ePanelPackage);
//	eInvokeResourceRow.Connect(resourcesTree, "row-activated");
//	mResourcesTree = new MProjectTree(mProject->GetResources());
//	resourcesTree.SetModel(mResourcesTree->GetModel());
//	resourcesTree.ExpandAll();
//
//	AddRoute(mProject->eProjectItemRemoved, mResourcesTree->eProjectItemRemoved);
//	AddRoute(mProject->eProjectItemMoved, mResourcesTree->eProjectItemMoved);
//	AddRoute(mProject->eProjectCreateResourceItem, mResourcesTree->eProjectCreateItem);

	// read the project's state, if any
	
	bool useState = false;
	MProjectState state = {};
	
	if (Preferences::GetInteger("save state", 1))
	{
		ssize_t r = inDocument->GetFile().ReadAttribute(kJapieProjectState, &state, kMProjectStateSize);
		useState = static_cast<uint32>(r) == kMProjectStateSize;
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

	// initialize interface
	SyncInterfaceWithProject();
}

void MProjectWindow::AddFileItemsToList(
	MProjectGroup*		inGroup,
	MListRowBase*		inParent,
	MListBase*			inList)
{
	for (auto iter = inGroup->GetItems().begin(); iter != inGroup->GetItems().end(); ++iter)
	{
		MFileRowItem* item = new MFileRowItem(*iter);
		
		inList->AppendRowInt(item, inParent);
		
		MProjectGroup* group = dynamic_cast<MProjectGroup*>(*iter);
		if (group != nil)
			AddFileItemsToList(group, item, inList);
	}
}

// ---------------------------------------------------------------------------
//	MProjectWindow::SelectFileRow

void MProjectWindow::SelectFileRow(
	MFileRowItem*		inFileRow)
{
	MProjectItem* item = inFileRow->mItem;
	MProjectGroup* group = dynamic_cast<MProjectGroup*>(item);
	mFileTree->SetColumnEditable(0, group != nil);
}

// ---------------------------------------------------------------------------
//	MProjectWindow::InvokeFileRow

void MProjectWindow::InvokeFileRow(
	MFileRowItem*		inFileRow)
{
	MProjectItem* item = inFileRow->mItem;
	MProjectFile* file = dynamic_cast<MProjectFile*>(item);
	if (file != nil)
	{
		fs::path p = file->GetPath();
		gApp->OpenOneDocument(MFile(p));
	}
	else
	{
		MProjectLib* lib = dynamic_cast<MProjectLib*>(item);
		if (lib != nil)
		{
			auto_ptr<MLibraryInfoDialog> dlog(new MLibraryInfoDialog);
			dlog->Initialize(mProject, lib);
			dlog->Show(this);
			dlog.release();
		}
	}
}

// ---------------------------------------------------------------------------
//	MProjectWindow::InvokeResourceRow

void MProjectWindow::InvokeResourceRow(
	MRsrcRowItem*		inResourceRow)
{
	MProjectItem* item = inResourceRow->mItem;
	MProjectFile* file = dynamic_cast<MProjectFile*>(item);
	if (file != nil)
	{
		fs::path p = file->GetPath();
		
		bool openSelf = true;
		
		if (FileNameMatches("*.glade", p) or FileNameMatches("*.ui", p))
		{
			// start up glade
			string cmd = Preferences::GetString("glade", "glade-3");
			cmd += " ";
			cmd += p.string();
			cmd += "&";
			
			int r = system(cmd.c_str());
			if (r != 0)
			{
				DisplayAlert("error-alert",
					string("Could not execute command:\n") + cmd);
			}
			else
				openSelf = false;
		}

		if (openSelf)
			gApp->OpenOneDocument(MFile(p));
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
		case cmd_Preprocess:
		case cmd_CheckSyntax:
		case cmd_Compile:
		case cmd_Disassemble:
		{
			vector<MProjectItem*> selectedItems;
			GetSelectedItems(selectedItems);
			
			if (selectedItems.size() == 1)
			{
				for (vector<MProjectItem*>::iterator item = selectedItems.begin();
					outEnabled == false and item != selectedItems.end();
					++item)
				{
					MProjectFile* file = dynamic_cast<MProjectFile*>(*item);
					outEnabled = file != nil and file->IsCompilable();
				}
			}
			break;
		}

		case cmd_AddFileToProject:
		case cmd_OpenIncludeFile:
			outEnabled = true;
			break;

		case cmd_NewGroup:
			outEnabled = notebook.GetPage() != ePanelLinkOrder;
			break;
		
		case cmd_RenameItem:
			if (notebook.GetPage() != ePanelLinkOrder)
			{
				vector<MProjectItem*> selectedItems;
				GetSelectedItems(selectedItems);
				
				outEnabled = 
					selectedItems.size() == 1 and
					dynamic_cast<MProjectGroup*>(selectedItems.front());
			}
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
	uint32			inItemIndex,
	uint32			inModifiers)
{
	bool result = true;

	vector<MProjectItem*> selectedItems;
	GetSelectedItems(selectedItems);

	MProjectFile* file = nil;
	if (selectedItems.size() == 1)
		file = dynamic_cast<MProjectFile*>(selectedItems.front());

	switch (inCommand)
	{
		case cmd_Preprocess:
			if (file != nil)
				mProject->Preprocess(file->GetPath());
			break;
			
		case cmd_CheckSyntax:
			if (file != nil)
				mProject->CheckSyntax(file->GetPath());
			break;

		case cmd_Compile:
			if (file != nil)
				mProject->Compile(file->GetPath());
			break;

		case cmd_Disassemble:
			if (file != nil)
				mProject->Disassemble(file->GetPath());
			break;

		case cmd_AddFileToProject:
			AddFilesToProject();
			break;
		
		case cmd_NewGroup:
			CreateNewGroup();
			break;
		
		case cmd_RenameItem:
			RenameGroup();
			break;
		
		case cmd_OpenIncludeFile:
			new MFindAndOpenDialog(mProject, this);
			break;
		
		default:
			result = MDocWindow::ProcessCommand(inCommand, inMenu, inItemIndex, inModifiers);
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

	const vector<MProjectTarget>& targets = mProject->GetTargets();
	for (vector<MProjectTarget>::const_iterator t = targets.begin(); t != targets.end(); ++t)
		targetPopup.Append(t->mName);

	targetPopup.SetActive(mProject->GetSelectedTarget());

	eTargetChanged.Unblock(targetPopup, "on_targ_changed");
}

// ---------------------------------------------------------------------------
//	TargetChanged

void MProjectWindow::TargetChanged()
{
	if (mProject != nil)
	{
		MGtkComboBox targetPopup(GetWidget(kTargetPopupID));
		mProject->SelectTarget(targetPopup.GetActive());
	}
}

// ---------------------------------------------------------------------------
//	TargetsChanged

void MProjectWindow::TargetsChanged()
{
	SyncInterfaceWithProject();
}

// ---------------------------------------------------------------------------
//	DocumentChanged

void MProjectWindow::DocumentChanged(
	MDocument*		inDocument)
{
	if (inDocument != mProject)
	{
//		delete mFilesTree;
//		mFilesTree = nil;
//		
//		delete mResourcesTree;
//		mResourcesTree = nil;
		
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
	MProjectState state = { };

	mProject->GetFile().ReadAttribute(kJapieProjectState, &state, kMProjectStateSize);
	
	state.Swap();

	state.mSelectedTarget = mProject->GetSelectedTarget();

	MGtkNotebook book(GetWidget(kNoteBookID));
	state.mSelectedPanel = book.GetPage();

	MRect r;
	GetWindowPosition(r);
	state.mWindowPosition[0] = r.x;
	state.mWindowPosition[1] = r.y;
	state.mWindowSize[0] = r.width;
	state.mWindowSize[1] = r.height;

	state.Swap();
	
	mProject->GetFile().WriteAttribute(kJapieProjectState, &state, kMProjectStateSize);
}

// ---------------------------------------------------------------------------
//	InitializeTreeView

void MProjectWindow::InitializeTreeView(
	GtkTreeView*		inGtkTreeView,
	int32				inPanel)
{
//	THROW_IF_NIL(inGtkTreeView);
//
//	gtk_tree_view_enable_model_drag_dest(inGtkTreeView,
//		kTreeDragTargets, kTreeDragTargetCount,
//		GdkDragAction(GDK_ACTION_COPY|GDK_ACTION_MOVE));
//	gtk_tree_view_enable_model_drag_source(inGtkTreeView, GDK_BUTTON1_MASK,
//		kTreeDragTargets, kTreeDragTargetCount,
//		GdkDragAction(GDK_ACTION_COPY|GDK_ACTION_MOVE));
//	
//	GtkTreeSelection* selection = gtk_tree_view_get_selection(inGtkTreeView);
//	if (selection != nil)
//		gtk_tree_selection_set_mode(selection, GTK_SELECTION_MULTIPLE);
//
//	// Add the columns and renderers
//
//	// the name column
//	GtkCellRenderer* renderer = gtk_cell_renderer_text_new();
//	GtkTreeViewColumn* column = gtk_tree_view_column_new_with_attributes (
//		_("File"), renderer, "text", kFilesNameColumn, nil);
//	g_object_set(G_OBJECT(column), "expand", true, nil);
//	gtk_tree_view_append_column(inGtkTreeView, column);
//	
//	if (inPanel == ePanelFiles or inPanel == ePanelLinkOrder)
//	{
//		mFileNameColumn = column;
//		mFileNameCell = renderer;
//		mEditedFileGroupName.Connect(G_OBJECT(renderer), "edited");
//
//		// the text size column
//		
//		renderer = gtk_cell_renderer_text_new();
//		column = gtk_tree_view_column_new_with_attributes (
//			_("Text"), renderer, "text", kFilesTextSizeColumn, nil);
//		g_object_set(G_OBJECT(renderer), "xalign", 1.0f, nil);
//		gtk_tree_view_column_set_alignment(column, 1.0f);
//		gtk_tree_view_append_column(inGtkTreeView, column);
//	
//		// the data size column
//		
//		renderer = gtk_cell_renderer_text_new();
//		column = gtk_tree_view_column_new_with_attributes (
//			_("Data"), renderer, "text", kFilesDataSizeColumn, nil);
//		g_object_set(G_OBJECT(renderer), "xalign", 1.0f, nil);
//		gtk_tree_view_column_set_alignment(column, 1.0f);
//		gtk_tree_view_append_column(inGtkTreeView, column);
//	}
//	else if (inPanel == ePanelPackage)
//	{
//		mResourceNameColumn = column;
//		mResourceNameCell = renderer;
//		mEditedResourceGroupName.Connect(G_OBJECT(renderer), "edited");
//		
//		// the data size column
//		
//		renderer = gtk_cell_renderer_text_new();
//		column = gtk_tree_view_column_new_with_attributes (
//			_("Size"), renderer, "text", kFilesDataSizeColumn, nil);
//		g_object_set(G_OBJECT(renderer), "xalign", 1.0f, nil);
//		gtk_tree_view_column_set_alignment(column, 1.0f);
//		gtk_tree_view_append_column(inGtkTreeView, column);
//	}
//	
//	// at last the dirty mark	
//	renderer = gtk_cell_renderer_pixbuf_new();
//	column = gtk_tree_view_column_new_with_attributes(
//		_(" "), renderer, "pixbuf", kFilesDirtyColumn, nil);
//	gtk_tree_view_append_column(inGtkTreeView, column);
//
//	gtk_widget_show_all(GTK_WIDGET(inGtkTreeView));
//	
//	eKeyPressEvent.Connect(G_OBJECT(inGtkTreeView), "key-press-event");
}

// ---------------------------------------------------------------------------
//	CreateNewGroup

void MProjectWindow::CreateNewGroup()
{
//	MGtkNotebook notebook(GetWidget(kNoteBookID));
//	auto_ptr<MGtkTreeView> treeView;
//	MProjectTree* model;
//	MProjectGroup* group;
//	GtkTreeViewColumn* column;
//	GtkCellRenderer* cell;
//	
//	if (notebook.GetPage() == ePanelFiles)
//	{
//		treeView.reset(new MGtkTreeView(GetWidget(kFilesListViewID)));
//		model = mFilesTree;
//		group = mProject->GetFiles();
//		column = mFileNameColumn;
//		cell = mFileNameCell;
//	}
//	else
//	{
//		treeView.reset(new MGtkTreeView(GetWidget(kResourceViewID)));
//		model = mResourcesTree;
//		group = mProject->GetResources();
//		column = mResourceNameColumn;
//		cell = mResourceNameCell;
//	}
//		
//	int32 index = 0;
//	
//	GtkTreePath* path = nil;
//	GtkTreeIter iter;
//	
//	if (treeView->GetFirstSelectedRow(path) and
//		model->GetIter(&iter, path))
//	{
//		MProjectItem* item = reinterpret_cast<MProjectItem*>(iter.user_data);
//		group = item->GetParent();
//		index = item->GetPosition();
//	}
//	
//	MProjectGroup* newGroup = new MProjectGroup(_("New Folder"), group);
//	group->AddProjectItem(newGroup, index);
//	model->ProjectItemInserted(newGroup);
//	g_object_set(G_OBJECT(cell), "editable", true, nil);
//	treeView->SetCursor(path, column, true);
//	mEditingName = true;
//	
//	if (path != nil)
//		gtk_tree_path_free(path);
//	
//	mProject->SetModified(true);
}

void MProjectWindow::RenameGroup()
{
//	if (mEditingName)
//		return;
//	
//	MGtkNotebook notebook(GetWidget(kNoteBookID));
//
//	GtkTreePath* path = nil;
//
//	if (notebook.GetPage() == ePanelFiles)
//	{
//		MGtkTreeView treeView(GetWidget(kFilesListViewID));
//		treeView.GetFirstSelectedRow(path);
//		if (path != nil)
//		{
//			g_object_set(G_OBJECT(mFileNameCell), "editable", true, nil);
//			treeView.SetCursor(path, mFileNameColumn);
//			mEditingName = true;
//		}
//	}
//	else if (notebook.GetPage() == ePanelPackage)
//	{
//		MGtkTreeView treeView(GetWidget(kResourceViewID));
//		treeView.GetFirstSelectedRow(path);
//		if (path != nil)
//		{
//			g_object_set(G_OBJECT(mResourceNameCell), "editable", true, nil);
//			treeView.SetCursor(path, mResourceNameColumn);
//			mEditingName = true;
//		}
//	}
}

void MProjectWindow::EditedFileGroupName(
	MFileRowItem*		inRow,
	const string&		inNewName)
{
//	mEditingName = false;
//	g_object_set(G_OBJECT(mFileNameCell), "editable", false, nil);
//	if (mFilesTree->ProjectItemNameEdited(inPath, inNewValue))
//		mProject->SetModified(true);
}

void MProjectWindow::EditedResourceGroupName(
	gchar*				inPath,
	gchar*				inNewValue)
{
//	mEditingName = false;
//	g_object_set(G_OBJECT(mResourceNameCell), "editable", false, nil);
//	if (mResourcesTree->ProjectItemNameEdited(inPath, inNewValue))
//		mProject->SetModified(true);
}

// ---------------------------------------------------------------------------
//	AddFilesToProject

void MProjectWindow::AddFilesToProject()
{
//	vector<MFile> urls;
//	if (ChooseFiles(true, urls))
//	{
//		MProjectTree* tree = nil;
//		MProjectGroup* group = nil;
//		int32 index = 0;
//
//		MGtkNotebook notebook(GetWidget(kNoteBookID));
//		
//		if (notebook.GetPage() == ePanelFiles)
//		{
//			tree = mFilesTree;
//			group = mProject->GetFiles();
//			
//			MGtkTreeView treeView(GetWidget(kFilesListViewID));
//
//			GtkTreePath* path;
//			GtkTreeIter iter;
//			
//			if (treeView.GetFirstSelectedRow(path) and
//				mFilesTree->GetIter(&iter, path))
//			{
//				MProjectItem* item = reinterpret_cast<MProjectItem*>(iter.user_data);
//				if (dynamic_cast<MProjectGroup*>(item) != nil)
//					group = static_cast<MProjectGroup*>(item);
//				else
//				{
//					group = item->GetParent();
//					index = item->GetPosition();
//				}
//			}
//		}
//		else if (notebook.GetPage() == ePanelPackage)
//		{
//			tree = mResourcesTree;
//			group = mProject->GetResources();
//
//			MGtkTreeView treeView(GetWidget(kResourceViewID));
//
//			GtkTreePath* path;
//			GtkTreeIter iter;
//			
//			if (treeView.GetFirstSelectedRow(path) and
//				mResourcesTree->GetIter(&iter, path))
//			{
//				MProjectItem* item = reinterpret_cast<MProjectItem*>(iter.user_data);
//				if (dynamic_cast<MProjectGroup*>(item) != nil)
//					group = static_cast<MProjectGroup*>(item);
//				else
//				{
//					group = item->GetParent();
//					index = item->GetPosition();
//				}
//			}
//		}
//		
//		if (tree != nil)
//		{
//			vector<string> files;
//			transform(urls.begin(), urls.end(), back_inserter(files),
//				boost::bind(&MFile::GetURI, _1));
//			
//			tree->AddFiles(files, group, index);
//			
//			mProject->SetModified(true);
//		}
//	}
}

// ---------------------------------------------------------------------------
//	GetSelectedItems

void MProjectWindow::GetSelectedItems(
	vector<MProjectItem*>&	outItems)
{
//	vector<GtkTreePath*> paths;
//
//	MGtkNotebook notebook(GetWidget(kNoteBookID));
//	if (notebook.GetPage() == ePanelFiles)
//	{
//		MGtkTreeView treeView(GetWidget(kFilesListViewID));
//		treeView.GetSelectedRows(paths);
//		
//		for (vector<GtkTreePath*>::iterator path = paths.begin(); path != paths.end(); ++path)
//		{
//			GtkTreeIter iter;
//			if (mFilesTree->GetIter(&iter, *path))
//				outItems.push_back(reinterpret_cast<MProjectItem*>(iter.user_data));
//			gtk_tree_path_free(*path);
//		}
//	}
//	else
//	{
//		MGtkTreeView treeView(GetWidget(kResourceViewID));
//		treeView.GetSelectedRows(paths);
//
//		for (vector<GtkTreePath*>::iterator path = paths.begin(); path != paths.end(); ++path)
//		{
//			GtkTreeIter iter;
//			if (mResourcesTree->GetIter(&iter, *path))
//				outItems.push_back(reinterpret_cast<MProjectItem*>(iter.user_data));
//			gtk_tree_path_free(*path);
//		}
//	}
}

// ---------------------------------------------------------------------------
//	DeleteSelectedItems

void MProjectWindow::DeleteSelectedItems()
{
//	vector<MProjectItem*> items;
//	
//	GetSelectedItems(items);
//	
//	MGtkNotebook notebook(GetWidget(kNoteBookID));
//	if (notebook.GetPage() == ePanelFiles)
//	{
//		for (vector<MProjectItem*>::iterator item = items.begin(); item != items.end(); ++item)
//			mFilesTree->RemoveItem(*item);
//	}
//	else
//	{
//		for (vector<MProjectItem*>::iterator item = items.begin(); item != items.end(); ++item)
//			mResourcesTree->RemoveItem(*item);
//	}
}

// ---------------------------------------------------------------------------
//	InfoClicked

void MProjectWindow::InfoClicked()
{
	auto_ptr<MProjectInfoDialog> dlog(new MProjectInfoDialog);
	dlog->Initialize(mProject);
	dlog->Show(this);
	dlog.release();
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
