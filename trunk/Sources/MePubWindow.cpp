//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "MJapi.h"

#include "MJapiApp.h"

#include "MePubDocument.h"
#include "MePubWindow.h"
#include "MePubItem.h"
#include "MTextDocument.h"
#include "MGtkWrappers.h"
#include "MProjectTree.h"
#include "MStrings.h"
#include "MUtils.h"
#include "MPreferences.h"
#include "MNewGroupDialog.h"

#include "MError.h"

using namespace std;

// ---------------------------------------------------------------------------

namespace {

struct MePubState
{
	uint16			mWindowPosition[2];
	uint16			mWindowSize[2];
	uint8			mSelectedPanel;
	uint8			mFillers[3];
	
	void			Swap();
};

const char
	kJapieePubState[] = "com.hekkelman.japi.ePubState";

const uint32
	kMePubStateSize = sizeof(MePubState);

void MePubState::Swap()
{
	net_swapper swap;
	
	mWindowPosition[0] = swap(mWindowPosition[0]);
	mWindowPosition[1] = swap(mWindowPosition[1]);
	mWindowSize[0] = swap(mWindowSize[0]);
	mWindowSize[1] = swap(mWindowSize[1]);
}

enum {
	kFilesListViewID		= 'tre1',
	
	kDCIDViewID				= 'dcID',
	kDCTitleViewID			= 'dcTI',
	kDCLanguageViewID		= 'dcLA',
	kDCCreatorViewID		= 'dcCR',
	kDCPublisherViewID		= 'dcPU',
	kDCDescriptionViewID	= 'dcDE',
	kDCCoverageViewID		= 'dcCO',
	kDCSourceViewID			= 'dcSO',
	kDCRightsViewID			= 'dcRI',
	kDCSubjectViewID		= 'dcSU',
	
	kTOCListViewID			= 'tre3',
	
	kNoteBookID				= 'note'
};

enum {
	kFilesPageNr = 1
};

GtkListStore* CreateListStoreForMediaTypes()
{
	GtkListStore* model = gtk_list_store_new(1, G_TYPE_STRING);
	GtkTreeIter iter;
	
	gtk_list_store_append(model, &iter);
	gtk_list_store_set(model, &iter, 0, "application/x-dtbncx+xml", -1);
	gtk_list_store_append(model, &iter);
	gtk_list_store_set(model, &iter, 0, "application/x-dtbook+xml", -1);
	gtk_list_store_append(model, &iter);
	gtk_list_store_set(model, &iter, 0, "application/xhtml+xml", -1);
	gtk_list_store_append(model, &iter);
	gtk_list_store_set(model, &iter, 0, "application/xml", -1);
	gtk_list_store_append(model, &iter);
	gtk_list_store_set(model, &iter, 0, "image/gif", -1);
	gtk_list_store_append(model, &iter);
	gtk_list_store_set(model, &iter, 0, "image/jpeg", -1);
	gtk_list_store_append(model, &iter);
	gtk_list_store_set(model, &iter, 0, "image/png", -1);
	gtk_list_store_append(model, &iter);
	gtk_list_store_set(model, &iter, 0, "image/svg+xml", -1);
	gtk_list_store_append(model, &iter);
	gtk_list_store_set(model, &iter, 0, "text/css", -1);
	gtk_list_store_append(model, &iter);
	gtk_list_store_set(model, &iter, 0, "text/x-oeb1-css", -1);
	gtk_list_store_append(model, &iter);
	gtk_list_store_set(model, &iter, 0, "text/x-oeb1-document", -1);
	
	return model;
}

}

// ---------------------------------------------------------------------------

enum {
	kePubFileNameColumn,
	kePubFileIDColumn,
	kePubFileMediaTypeColumn,
	kePubFileDataSizeColumn,
	kePubFileDirtyColumn,

	kePubFileColumnCount
};


class MePubFileTree : public MProjectTree
{
  public:
					MePubFileTree(
						MProjectGroup*	inItems)
						: MProjectTree(inItems)	{}
	
	virtual uint32	GetColumnCount() const;

	virtual GType	GetColumnType(
						uint32			inColumn) const;

	virtual void	GetValue(
						GtkTreeIter*	inIter,
						uint32			inColumn,
						GValue*			outValue) const;

	bool			EditedItemID(
						gchar*			path,
						gchar*			new_text);

	bool			EditedItemName(
						gchar*			path,
						gchar*			new_text);

	bool			EditedItemMediaType(
						gchar*			path,
						gchar*			new_text);
};

uint32 MePubFileTree::GetColumnCount() const
{
	return kePubFileColumnCount;
}

GType MePubFileTree::GetColumnType(
	uint32			inColumn) const
{
	GType result = G_TYPE_STRING;
	
	if (inColumn == kePubFileDirtyColumn)
		result = GDK_TYPE_PIXMAP;
	
	return result;
}

void MePubFileTree::GetValue(
	GtkTreeIter*	inIter,
	uint32			inColumn,
	GValue*			outValue) const
{
	// dots
	
	const uint32 kDotSize = 6;
	const MColor
		kDirtyColor = MColor("#ff664c");
	
	static GdkPixbuf* kDirtyDot = CreateDot(kDirtyColor, kDotSize);
	
	MProjectItem* item = reinterpret_cast<MProjectItem*>(inIter->user_data);
	MePubItem* ePubItem = dynamic_cast<MePubItem*>(item);

	if (item != nil)
	{
		switch (inColumn)
		{
			case kePubFileNameColumn:
				g_value_init(outValue, G_TYPE_STRING);
				g_value_set_string(outValue, item->GetName().c_str());
				break;
			
			case kePubFileIDColumn:
				g_value_init(outValue, G_TYPE_STRING);
				if (ePubItem != nil)
					g_value_set_string(outValue, ePubItem->GetID().c_str());
				break;
			
			case kePubFileMediaTypeColumn:
				g_value_init(outValue, G_TYPE_STRING);
				if (ePubItem != nil)
					g_value_set_string(outValue, ePubItem->GetMediaType().c_str());
				break;
			
			case kePubFileDataSizeColumn:
			{
				g_value_init(outValue, G_TYPE_STRING);
				uint32 size = item->GetDataSize();
				
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

			case kePubFileDirtyColumn:
				g_value_init(outValue, G_TYPE_OBJECT);
				if (item->IsOutOfDate())
					g_value_set_object(outValue, kDirtyDot);
				break;
			
		}
	}
	
}

// ---------------------------------------------------------------------------
//	EditedItemName

bool MePubFileTree::EditedItemName(
	gchar*				inPath,
	gchar*				inNewValue)
{
	bool result = false;
	GtkTreePath* path = gtk_tree_path_new_from_string(inPath);
	
	GtkTreeIter iter;
	if (GetIter(&iter, path))
	{
		MProjectItem* item = reinterpret_cast<MProjectItem*>(iter.user_data);

		if (item->GetName() != inNewValue)
		{
			item->SetName(inNewValue);
			RowChanged(path, &iter);
			result = true;
		}
	}
	
	if (path != nil)
		gtk_tree_path_free(path);
	
	return result;
}

// ---------------------------------------------------------------------------
//	EditedItemID

bool MePubFileTree::EditedItemID(
	gchar*				inPath,
	gchar*				inNewValue)
{
	bool result = false;
	
	GtkTreePath* path = gtk_tree_path_new_from_string(inPath);
	
	GtkTreeIter iter;
	if (GetIter(&iter, path))
	{
		MProjectItem* item = reinterpret_cast<MProjectItem*>(iter.user_data);
		MePubItem* ePubItem = dynamic_cast<MePubItem*>(item);
		
		if (ePubItem != nil and ePubItem->GetID() != inNewValue)
		{
			ePubItem->SetID(inNewValue);
			RowChanged(path, &iter);
			result = true;
		}
	}
	
	if (path != nil)
		gtk_tree_path_free(path);
	
	return result;
}

// ---------------------------------------------------------------------------
//	EditedItemMediaType

bool MePubFileTree::EditedItemMediaType(
	gchar*				inPath,
	gchar*				inNewValue)
{
	bool result = false;
	GtkTreePath* path = gtk_tree_path_new_from_string(inPath);
	
	GtkTreeIter iter;
	if (GetIter(&iter, path))
	{
		MProjectItem* item = reinterpret_cast<MProjectItem*>(iter.user_data);
		MePubItem* ePubItem = dynamic_cast<MePubItem*>(item);
		
		if (ePubItem != nil and ePubItem->GetMediaType() != inNewValue)
		{
			ePubItem->SetMediaType(inNewValue);
			RowChanged(path, &iter);
			result = true;
		}
	}
	
	if (path != nil)
		gtk_tree_path_free(path);
	
	return result;
}

// ---------------------------------------------------------------------------

enum {
	kTOCTitleColumn,
	kTOCSrcColumn,
	kTOCClassColumn,

	kTOCColumnCount
};

class MTOCTree : public MProjectTree
{
  public:
					MTOCTree(
						MProjectGroup*	inItems)
						: MProjectTree(inItems)	{}
	
	virtual uint32	GetColumnCount() const;

	virtual GType	GetColumnType(
						uint32			inColumn) const;

	virtual void	GetValue(
						GtkTreeIter*	inIter,
						uint32			inColumn,
						GValue*			outValue) const;

	bool			EditedTOCTitle(
						gchar*			path,
						gchar*			new_text);

	bool			EditedTOCSrc(
						gchar*			path,
						gchar*			new_text);

	bool			EditedTOCClass(
						gchar*			path,
						gchar*			new_text);
};

uint32 MTOCTree::GetColumnCount() const
{
	return kTOCColumnCount;
}

GType MTOCTree::GetColumnType(
	uint32			inColumn) const
{
	return G_TYPE_STRING;
}

void MTOCTree::GetValue(
	GtkTreeIter*	inIter,
	uint32			inColumn,
	GValue*			outValue) const
{
	MProjectItem* item = reinterpret_cast<MProjectItem*>(inIter->user_data);
	MePubTOCItem* tocItem = dynamic_cast<MePubTOCItem*>(item);

	if (tocItem != nil)
	{
		switch (inColumn)
		{
			case kTOCTitleColumn:
				g_value_init(outValue, G_TYPE_STRING);
				g_value_set_string(outValue, tocItem->GetName().c_str());
				break;
			
			case kTOCSrcColumn:
				g_value_init(outValue, G_TYPE_STRING);
				g_value_set_string(outValue, tocItem->GetSrc().c_str());
				break;
			
			case kTOCClassColumn:
				g_value_init(outValue, G_TYPE_STRING);
				g_value_set_string(outValue, tocItem->GetClass().c_str());
				break;
		}
	}
}

// ---------------------------------------------------------------------------
//	EditedTOCTitle

bool MTOCTree::EditedTOCTitle(
	gchar*				inPath,
	gchar*				inNewValue)
{
	bool result = false;
	GtkTreePath* path = gtk_tree_path_new_from_string(inPath);
	
	GtkTreeIter iter;
	if (GetIter(&iter, path))
	{
		MProjectItem* item = reinterpret_cast<MProjectItem*>(iter.user_data);

		if (item->GetName() != inNewValue)
		{
			item->SetName(inNewValue);
			RowChanged(path, &iter);
			result = true;
		}
	}
	
	if (path != nil)
		gtk_tree_path_free(path);
	
	return result;
}

// ---------------------------------------------------------------------------
//	EditedTOCSrc

bool MTOCTree::EditedTOCSrc(
	gchar*				inPath,
	gchar*				inNewValue)
{
	bool result = false;
	
	GtkTreePath* path = gtk_tree_path_new_from_string(inPath);
	
	GtkTreeIter iter;
	if (GetIter(&iter, path))
	{
		MProjectItem* item = reinterpret_cast<MProjectItem*>(iter.user_data);
		MePubTOCItem* tocItem = dynamic_cast<MePubTOCItem*>(item);
		
		if (tocItem != nil and tocItem->GetSrc() != inNewValue)
		{
			tocItem->SetSrc(inNewValue);
			RowChanged(path, &iter);
			result = true;
		}
	}
	
	if (path != nil)
		gtk_tree_path_free(path);
	
	return result;
}

// ---------------------------------------------------------------------------
//	EditedTOCClass

bool MTOCTree::EditedTOCClass(
	gchar*				inPath,
	gchar*				inNewValue)
{
	bool result = false;
	
	GtkTreePath* path = gtk_tree_path_new_from_string(inPath);
	
	GtkTreeIter iter;
	if (GetIter(&iter, path))
	{
		MProjectItem* item = reinterpret_cast<MProjectItem*>(iter.user_data);
		MePubTOCItem* tocItem = dynamic_cast<MePubTOCItem*>(item);
		
		if (tocItem != nil and tocItem->GetClass() != inNewValue)
		{
			tocItem->SetClass(inNewValue);
			RowChanged(path, &iter);
			result = true;
		}
	}
	
	if (path != nil)
		gtk_tree_path_free(path);
	
	return result;
}


// ---------------------------------------------------------------------------

MePubWindow::MePubWindow()
	: MDocWindow("epub-window")
	, eKeyPressEvent(this, &MePubWindow::OnKeyPressEvent)
	, eInvokeFileRow(this, &MePubWindow::InvokeFileRow)
	, eDocumentClosed(this, &MePubWindow::TextDocClosed)
	, eFileSpecChanged(this, &MePubWindow::TextDocFileSpecChanged)
	, eCreateNewGroup(this, &MePubWindow::CreateNewGroup)
	, mEditedItemName(this, &MePubWindow::EditedItemName)
	, mEditedItemID(this, &MePubWindow::EditedItemID)
	, mEditedItemMediaType(this, &MePubWindow::EditedItemMediaType)
	, mEditedTOCTitle(this, &MePubWindow::EditedTOCTitle)
	, mEditedTOCSrc(this, &MePubWindow::EditedTOCSrc)
	, mEditedTOCClass(this, &MePubWindow::EditedTOCClass)
{
	mController = new MController(this);
	
	mMenubar.Initialize(GetWidget('mbar'), "epub-window-menu");
	mMenubar.SetTarget(mController);

	ConnectChildSignals();
}

MePubWindow::~MePubWindow()
{
	delete mFilesTree;
	mFilesTree = nil;
	
	delete mTOCTree;
	mTOCTree = nil;
}

void MePubWindow::Initialize(
	MDocument*		inDocument)
{
	mEPub = dynamic_cast<MePubDocument*>(inDocument);
	
	if (mEPub == nil)
		THROW(("Invalid document type passed"));
	
	MDocWindow::Initialize(inDocument);

	MGtkTreeView filesTree(GetWidget(kFilesListViewID));
	InitializeTreeView(filesTree);
	eInvokeFileRow.Connect(filesTree, "row-activated");
	mFilesTree = new MePubFileTree(mEPub->GetFiles());

	AddRoute(mEPub->eInsertedFile, mFilesTree->eProjectItemInserted);
	AddRoute(mEPub->eRemovedFile, mFilesTree->eProjectItemRemoved);
	AddRoute(mEPub->eItemMoved, mFilesTree->eProjectItemMoved);
	AddRoute(mEPub->eCreateItem, mFilesTree->eProjectCreateItem);
	
	filesTree.SetModel(mFilesTree->GetModel());
	filesTree.ExpandAll();

	MGtkTreeView tocTree(GetWidget(kTOCListViewID));
	InitializeTOCTreeView(tocTree);
	mTOCTree = new MTOCTree(mEPub->GetTOC());

//	AddRoute(mEPub->eInsertedFile, mFilesTree->eProjectItemInserted);
//	AddRoute(mEPub->eRemovedFile, mFilesTree->eProjectItemRemoved);
//	AddRoute(mEPub->eItemMoved, mFilesTree->eProjectItemMoved);
//	AddRoute(mEPub->eCreateItem, mFilesTree->eProjectCreateItem);
	
	tocTree.SetModel(mTOCTree->GetModel());
	tocTree.ExpandAll();

	// fill in the information fields
	
	SetText(kDCIDViewID,			mEPub->GetDocumentID());
	SetText(kDCTitleViewID,			mEPub->GetDublinCoreValue("title"));
	SetText(kDCLanguageViewID,		mEPub->GetDublinCoreValue("language"));
	SetText(kDCCreatorViewID,		mEPub->GetDublinCoreValue("creator"));
	SetText(kDCPublisherViewID,		mEPub->GetDublinCoreValue("publisher"));
	SetText(kDCDescriptionViewID,	mEPub->GetDublinCoreValue("description"));
	SetText(kDCCoverageViewID,		mEPub->GetDublinCoreValue("coverage"));
	SetText(kDCSourceViewID,		mEPub->GetDublinCoreValue("source"));
	SetText(kDCRightsViewID,		mEPub->GetDublinCoreValue("rights"));
	SetText(kDCSubjectViewID,		mEPub->GetDublinCoreValue("subject"));

	bool useState = false;
	MePubState state = {};
	
	if (Preferences::GetInteger("save state", 1))
	{
		fs::path file = inDocument->GetURL().GetPath();
		
		ssize_t r = read_attribute(file, kJapieePubState, &state, kMePubStateSize);
		
		useState = static_cast<uint32>(r) == kMePubStateSize;
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
	}
	
	mEPub->SetModified(false);
}

bool MePubWindow::DoClose()
{
	bool result = false;

	while (not mOpenFiles.empty())
	{
		MDocument* doc = mOpenFiles.begin()->second;
		if (doc == nil)
		{
			mOpenFiles.erase(mOpenFiles.begin());
			continue;
		}
		
		MController* controller = doc->GetFirstController();
		if (not controller->TryCloseController(kSaveChangesClosingDocument))
			break;
	}
	
	if (mOpenFiles.empty() and MDocWindow::DoClose())
	{
		// need to do this here, otherwise we crash in destructor code
		
		delete mFilesTree;
		mFilesTree = nil;
		
		delete mTOCTree;
		mTOCTree = nil;
		
		result = true;
	}
	
	return result;
}

// ---------------------------------------------------------------------------
//	SaveState

void MePubWindow::SaveState()
{
	try
	{
		fs::path file = mEPub->GetURL().GetPath();
		MePubState state = { };

		(void)read_attribute(file, kJapieePubState, &state, kMePubStateSize);
		
		state.Swap();

		MGtkNotebook book(GetWidget(kNoteBookID));
		state.mSelectedPanel = book.GetPage();

		MRect r;
		GetWindowPosition(r);
		state.mWindowPosition[0] = r.x;
		state.mWindowPosition[1] = r.y;
		state.mWindowSize[0] = r.width;
		state.mWindowSize[1] = r.height;

		state.Swap();
		
		write_attribute(file, kJapieePubState, &state, kMePubStateSize);
	}
	catch (...) {}
}

bool MePubWindow::UpdateCommandStatus(
	uint32			inCommand,
	MMenu*			inMenu,
	uint32			inItemIndex,
	bool&			outEnabled,
	bool&			outChecked)
{
	bool result = true;
	
	MGtkNotebook notebook(GetWidget(kNoteBookID));

	switch (inCommand)
	{
		case cmd_NewGroup:
			outEnabled = notebook.GetPage() == kFilesPageNr;
			break;
		
		default:
			result = MDocWindow::UpdateCommandStatus(inCommand, inMenu, inItemIndex, outEnabled, outChecked);
	}
	
	return result;
}

bool MePubWindow::ProcessCommand(
	uint32			inCommand,
	const MMenu*	inMenu,
	uint32			inItemIndex,
	uint32			inModifiers)
{
	bool result = true;

//	vector<MProjectItem*> selectedItems;
//	GetSelectedItems(selectedItems);

	switch (inCommand)
	{
		case cmd_NewGroup:
		{
			MNewGroupDialog* dlog = new MNewGroupDialog(this);
			AddRoute(dlog->eCreateNewGroup, eCreateNewGroup);
			break;
		}
		
		default:
			result = MDocWindow::ProcessCommand(inCommand, inMenu, inItemIndex, inModifiers);
			break;
	}
	
	return result;
}

// ---------------------------------------------------------------------------
//	InitializeTreeView

void MePubWindow::InitializeTreeView(
	GtkTreeView*		inGtkTreeView)
{
	THROW_IF_NIL(inGtkTreeView);

	gtk_tree_view_enable_model_drag_dest(inGtkTreeView,
		kTreeDragTargets, kTreeDragTargetCount,
		GdkDragAction(GDK_ACTION_COPY|GDK_ACTION_MOVE));
	gtk_tree_view_enable_model_drag_source(inGtkTreeView, GDK_BUTTON1_MASK,
		kTreeDragTargets, kTreeDragTargetCount,
		GdkDragAction(GDK_ACTION_COPY|GDK_ACTION_MOVE));
	
	GtkTreeSelection* selection = gtk_tree_view_get_selection(inGtkTreeView);
	if (selection != nil)
		gtk_tree_selection_set_mode(selection, GTK_SELECTION_MULTIPLE);

	// Add the columns and renderers

	// the name column
	GtkCellRenderer* renderer = gtk_cell_renderer_text_new();
	GtkTreeViewColumn* column = gtk_tree_view_column_new_with_attributes (
		_("File"), renderer, "text", kePubFileNameColumn, nil);
	g_object_set(G_OBJECT(column), "expand", true, nil);
	g_object_set(G_OBJECT(renderer), "editable", true, "editable-set", false, nil);
	mEditedItemName.Connect(G_OBJECT(renderer), "edited");
	gtk_tree_view_append_column(inGtkTreeView, column);
	
	// the ID column
	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes (
		_("ID"), renderer, "text", kePubFileIDColumn, nil);
	g_object_set(G_OBJECT(renderer), "editable", true, nil);
	mEditedItemID.Connect(G_OBJECT(renderer), "edited");
	gtk_tree_view_append_column(inGtkTreeView, column);
	
	// the MediaType column
	renderer = gtk_cell_renderer_combo_new();
	column = gtk_tree_view_column_new_with_attributes (
		_("Media-Type"), renderer, "text", kePubFileMediaTypeColumn, nil);
	g_object_set(G_OBJECT(renderer), "text-column", 0, "editable", true,
		"has-entry", true, "model", CreateListStoreForMediaTypes(), nil);
	mEditedItemMediaType.Connect(G_OBJECT(renderer), "edited");
	gtk_tree_view_append_column(inGtkTreeView, column);
	
	// the data size column
	
	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes (
		_("Size"), renderer, "text", kePubFileDataSizeColumn, nil);
	g_object_set(G_OBJECT(renderer), "xalign", 1.0f, nil);
	gtk_tree_view_column_set_alignment(column, 1.0f);
	gtk_tree_view_append_column(inGtkTreeView, column);
	
	// at last the dirty mark	
	renderer = gtk_cell_renderer_pixbuf_new();
	column = gtk_tree_view_column_new_with_attributes(
		_(" "), renderer, "pixbuf", kePubFileDirtyColumn, nil);
	gtk_tree_view_append_column(inGtkTreeView, column);

	gtk_widget_show_all(GTK_WIDGET(inGtkTreeView));
	
	eKeyPressEvent.Connect(G_OBJECT(inGtkTreeView), "key-press-event");
}

// ---------------------------------------------------------------------------
//	InitializeTOCTreeView

void MePubWindow::InitializeTOCTreeView(
	GtkTreeView*		inGtkTreeView)
{
	THROW_IF_NIL(inGtkTreeView);

	gtk_tree_view_enable_model_drag_dest(inGtkTreeView,
		kTreeDragTargets, kTreeDragTargetCount,
		GdkDragAction(GDK_ACTION_COPY|GDK_ACTION_MOVE));
	gtk_tree_view_enable_model_drag_source(inGtkTreeView, GDK_BUTTON1_MASK,
		kTreeDragTargets, kTreeDragTargetCount,
		GdkDragAction(GDK_ACTION_COPY|GDK_ACTION_MOVE));
	
	// Add the columns and renderers

	// the title column
	GtkCellRenderer* renderer = gtk_cell_renderer_text_new();
	GtkTreeViewColumn* column = gtk_tree_view_column_new_with_attributes (
		_("Title"), renderer, "text", kTOCTitleColumn, nil);
	g_object_set(G_OBJECT(column), "expand", true, nil);
	g_object_set(G_OBJECT(renderer), "editable", true, nil);
	mEditedTOCTitle.Connect(G_OBJECT(renderer), "edited");
	gtk_tree_view_append_column(inGtkTreeView, column);
	
	// the src column
	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes (
		_("Source"), renderer, "text", kTOCSrcColumn, nil);
	g_object_set(G_OBJECT(renderer), "editable", true, nil);
	mEditedTOCSrc.Connect(G_OBJECT(renderer), "edited");
	gtk_tree_view_append_column(inGtkTreeView, column);
	
	// the class column
	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes (
		_("Class"), renderer, "text", kTOCClassColumn, nil);
	g_object_set(G_OBJECT(renderer), "editable", true, nil);
	mEditedTOCClass.Connect(G_OBJECT(renderer), "edited");
	gtk_tree_view_append_column(inGtkTreeView, column);
	
	gtk_widget_show_all(GTK_WIDGET(inGtkTreeView));
	
	eKeyPressEvent.Connect(G_OBJECT(inGtkTreeView), "key-press-event");
}

// ---------------------------------------------------------------------------
//	GetSelectedItems

void MePubWindow::GetSelectedItems(
	vector<MProjectItem*>&	outItems)
{
	vector<GtkTreePath*> paths;

	MGtkNotebook notebook(GetWidget(kNoteBookID));

	MGtkTreeView treeView(GetWidget(kFilesListViewID));
	treeView.GetSelectedRows(paths);
	
	for (vector<GtkTreePath*>::iterator path = paths.begin(); path != paths.end(); ++path)
	{
		GtkTreeIter iter;
		if (mFilesTree->GetIter(&iter, *path))
			outItems.push_back(reinterpret_cast<MProjectItem*>(iter.user_data));
		gtk_tree_path_free(*path);
	}
}

// ---------------------------------------------------------------------------
//	DeleteSelectedItems

void MePubWindow::DeleteSelectedItems()
{
	vector<MProjectItem*> items;
	
	GetSelectedItems(items);
	
//	mProject->RemoveItems(items);
}

// ---------------------------------------------------------------------------
//	OnKeyPressEvent

bool MePubWindow::OnKeyPressEvent(
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

// ---------------------------------------------------------------------------
//	MProjectWindow::InvokeFileRow

void MePubWindow::InvokeFileRow(
	GtkTreePath*		inPath,
	GtkTreeViewColumn*	inColumn)
{
	GtkTreeIter iter;
	if (gtk_tree_model_get_iter(mFilesTree->GetModel(), &iter, inPath))
	{
		MProjectItem* item = reinterpret_cast<MProjectItem*>(iter.user_data);
		MePubItem* ePubItem = dynamic_cast<MePubItem*>(item);
		if (ePubItem != nil)
		{
			fs::path path = ePubItem->GetPath();
			
			MEPubTextDocMap::iterator di = mOpenFiles.find(path);
			MTextDocument* doc;

			if (di != mOpenFiles.end())
				doc = di->second;
			else
			{
				doc = new MTextDocument(mEPub, path);
				AddRoute(doc->eDocumentClosed, eDocumentClosed);
				AddRoute(doc->eFileSpecChanged, eFileSpecChanged);
				mOpenFiles[path] = doc;
			}

			gApp->DisplayDocument(doc);
		}
	}
}

void MePubWindow::TextDocClosed(
	MDocument*			inDocument)
{
	MEPubTextDocMap::iterator i;
	for (i = mOpenFiles.begin(); i != mOpenFiles.end(); ++i)
	{
		if (i->second == inDocument)
		{
			mOpenFiles.erase(i);
			break;
		}
	}
}

void MePubWindow::TextDocFileSpecChanged(
	MDocument*			inDocument,
	const MUrl&			inURL)
{
	MEPubTextDocMap::iterator i;
	for (i = mOpenFiles.begin(); i != mOpenFiles.end(); ++i)
	{
		if (i->second == inDocument)
		{
			mOpenFiles.erase(i);
			break;
		}
	}
}

// ---------------------------------------------------------------------------
//	MProjectWindow::ValueChanged

void MePubWindow::ValueChanged(
	uint32			inID)
{
	switch (inID)
	{
		case kDCIDViewID:
			mEPub->SetDocumentID(GetText(inID));
			break;

		case kDCTitleViewID:
			mEPub->SetDublinCoreValue("title", GetText(inID));
			break;

		case kDCLanguageViewID:
			mEPub->SetDublinCoreValue("language", GetText(inID));
			break;

		case kDCCreatorViewID:
			mEPub->SetDublinCoreValue("creator", GetText(inID));
			break;

		case kDCPublisherViewID:
			mEPub->SetDublinCoreValue("publisher", GetText(inID));
			break;

		case kDCDescriptionViewID:
			mEPub->SetDublinCoreValue("description", GetText(inID));
			break;

		case kDCCoverageViewID:
			mEPub->SetDublinCoreValue("coverage", GetText(inID));
			break;

		case kDCSourceViewID:
			mEPub->SetDublinCoreValue("source", GetText(inID));
			break;

		case kDCRightsViewID:
			mEPub->SetDublinCoreValue("rights", GetText(inID));
			break;

		case kDCSubjectViewID:
			mEPub->SetDublinCoreValue("subject", GetText(inID));
			break;

	}
}

// ---------------------------------------------------------------------------
//	CreateNewGroup

void MePubWindow::CreateNewGroup(
	const string&		inGroupName)
{
	MGtkNotebook notebook(GetWidget(kNoteBookID));
	auto_ptr<MGtkTreeView> treeView(new MGtkTreeView(GetWidget(kFilesListViewID)));
	MProjectGroup* group = mEPub->GetFiles();
	
	int32 index = 0;
	
	GtkTreePath* path = nil;
	GtkTreeIter iter;
	
	if (treeView->GetFirstSelectedRow(path) and
		mFilesTree->GetIter(&iter, path))
	{
		MProjectItem* item = reinterpret_cast<MProjectItem*>(iter.user_data);
		group = item->GetParent();
		index = item->GetPosition();
	}
		
	mEPub->CreateNewGroup(inGroupName, group, index);
	
	if (path != nil)
		gtk_tree_path_free(path);
}

void MePubWindow::EditedItemName(
	gchar*				inPath,
	gchar*				inNewValue)
{
	if (mFilesTree->EditedItemName(inPath, inNewValue))
		mEPub->SetModified(true);
}

void MePubWindow::EditedItemID(
	gchar*				inPath,
	gchar*				inNewValue)
{
	if (mFilesTree->EditedItemID(inPath, inNewValue))
		mEPub->SetModified(true);
}

void MePubWindow::EditedItemMediaType(
	gchar*				inPath,
	gchar*				inNewValue)
{
	if (mFilesTree->EditedItemMediaType(inPath, inNewValue))
		mEPub->SetModified(true);
}

void MePubWindow::EditedTOCTitle(
	gchar*				inPath,
	gchar*				inNewValue)
{
	if (mTOCTree->EditedTOCTitle(inPath, inNewValue))
		mEPub->SetModified(true);
}

void MePubWindow::EditedTOCSrc(
	gchar*				inPath,
	gchar*				inNewValue)
{
	if (mTOCTree->EditedTOCSrc(inPath, inNewValue))
		mEPub->SetModified(true);
}

void MePubWindow::EditedTOCClass(
	gchar*				inPath,
	gchar*				inNewValue)
{
	if (mTOCTree->EditedTOCClass(inPath, inNewValue))
		mEPub->SetModified(true);
}
