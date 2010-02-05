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
#include "MStrings.h"
#include "MUtils.h"
#include "MPreferences.h"
#include "MePubContentFile.h"
#include "MList.h"

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
	kDCDateViewID			= 'dcDT',
	kDCDescriptionViewID	= 'dcDE',
	kDCCoverageViewID		= 'dcCO',
	kDCSourceViewID			= 'dcSO',
	kDCRightsViewID			= 'dcRI',
	kDCSubjectViewID		= 'dcSU',
	
	kTOCListViewID			= 'tre3',
	
	kDocIDSchemeViewID		= 'docS',
	kDocIDGenerateButtonID	= 'genI',
	
	kNoteBookID				= 'note'
};

enum {
	kInfoPageNr,
	kFilesPageNr,
	kTOCPageNr
};

GtkListStore* CreateListStoreForMediaTypes()
{
	GtkListStore* model = gtk_list_store_new(1, G_TYPE_STRING);
	GtkTreeIter iter;
	
	gtk_list_store_append(model, &iter);
	gtk_list_store_set(model, &iter, 0, "application/xhtml+xml", -1);
	gtk_list_store_append(model, &iter);
	gtk_list_store_set(model, &iter, 0, "application/xml", -1);
	gtk_list_store_append(model, &iter);
	gtk_list_store_set(model, &iter, 0, "application/x-dtbncx+xml", -1);
	gtk_list_store_append(model, &iter);
	gtk_list_store_set(model, &iter, 0, "application/x-dtbook+xml", -1);
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
	kePubFileLinearColumn,
	kePubFileMediaTypeColumn,
	kePubFileDataSizeColumn,
	kePubFileDirtyColumn,

	kePubFileColumnCount
};

// ---------------------------------------------------------------------------

namespace MePubFileFields
{
	struct name {};
	struct id {};
	struct linear {};
	struct mediatype {};
	struct size {};
	struct dirty {};
}

class MePubFileRow : public MListRow<
	MePubFileRow,
	MePubFileFields::name,		string,
	MePubFileFields::id,		string,
	MePubFileFields::linear,	bool,
	MePubFileFields::mediatype,	string,
	MePubFileFields::size,		string,
	MePubFileFields::dirty,		GdkPixbuf*
>
{
  public:
					MePubFileRow(
						MProjectItem*	inItem)
						: mItem(inItem)
						, mEPubItem(dynamic_cast<MePubItem*>(inItem))
					{
					}
	
	void			GetData(
						const MePubFileFields::name&,
						string&		outData)
					{
						outData = mItem->GetName();
					}
	
	void			GetData(
						const MePubFileFields::id&,
						string&		outData)
					{
						if (mEPubItem != nil)
							outData = mEPubItem->GetID();
					}

	void			GetData(
						const MePubFileFields::linear&,
						bool&		outData)
					{
						if (mEPubItem != nil)
							outData = mEPubItem->IsLinear();
					}

	void			GetData(
						const MePubFileFields::mediatype&,
						string&		outData)
					{
						if (mEPubItem != nil)
							outData = mEPubItem->GetMediaType();
					}

	void			GetData(
						const MePubFileFields::size&,
						string&		outData)
					{
						uint32 size = mItem->GetDataSize();
						
						stringstream s;
						if (size >= 1024 * 1024 * 1024)
							s << (size / (1024 * 1024 * 1024)) << 'G';
						else if (size >= 1024 * 1024)
							s << (size / (1024 * 1024)) << 'M';
						else if (size >= 1024)
							s << (size / (1024)) << 'K';
						else if (size > 0)
							s << size;
						
						outData = s.str();
					}

	void			GetData(
						const MePubFileFields::dirty&,
						GdkPixbuf*&		outData)
					{
						const uint32 kDotSize = 6;
						const MColor
							kDirtyColor = MColor("#ff664c");
						
						static GdkPixbuf* kDirtyDot = CreateDot(kDirtyColor, kDotSize);

						if (mItem->IsOutOfDate())
							outData = kDirtyDot;
						else
							outData = nil;
					}

	virtual void	ColumnEdited(
						uint32			inColumnNr,
						const string&	inNewText)
					{
						switch (inColumnNr)
						{
							case kePubFileNameColumn:
								mItem->SetName(inNewText);
								RowChanged();
								break;
							
							case kePubFileIDColumn:
								mEPubItem->SetID(inNewText);
								RowChanged();
								break;
							
							case kePubFileMediaTypeColumn:
								mEPubItem->SetMediaType(inNewText);
								RowChanged();
								break;
						}
					}
	
	virtual void	ColumnToggled(
						uint32			inColumnNr)
					{
						if (mEPubItem != nil and inColumnNr == kePubFileLinearColumn)
						{
							mEPubItem->SetLinear(not mEPubItem->IsLinear());
							RowChanged();
						}
					}


	virtual bool	RowDropPossible() const		{ return dynamic_cast<MProjectGroup*>(mItem) != nil; }

	MProjectItem*	GetProjectItem() const				{ return mItem; }
	MePubItem*		GetEPubItem() const					{ return mEPubItem; }
	
  private:
	MProjectItem*	mItem;
	MePubItem*		mEPubItem;
};

// ---------------------------------------------------------------------------

enum {
	kTOCTitleColumn,
	kTOCSrcColumn,
	kTOCClassColumn,

	kTOCColumnCount
};

namespace MePubTOCFields
{
	struct title {};
	struct src {};
	struct cls {};
}

class MePubTOCRow : public MListRow<
	MePubTOCRow,
	MePubTOCFields::title,	string,
	MePubTOCFields::src,	string,
	MePubTOCFields::cls,	string
>
{
  public:
					MePubTOCRow(
						MProjectItem* inTOCItem)
						: mItem(inTOCItem)
						, mTOCItem(dynamic_cast<MePubTOCItem*>(inTOCItem))
					{
					}
	
	void			GetData(
						const MePubTOCFields::title&,
						string&			outData)
					{
						outData = mItem->GetName();
					}

	void			GetData(
						const MePubTOCFields::src&,
						string&			outData)
					{
						if (mTOCItem != nil)
							outData = mTOCItem->GetSrc();
					}

	void			GetData(
						const MePubTOCFields::cls&,
						string&			outData)
					{
						if (mTOCItem != nil)
							outData = mTOCItem->GetClass();
					}

	virtual void	ColumnEdited(
						uint32			inColumnNr,
						const string&	inNewText)
					{
						switch (inColumnNr)
						{
							case kTOCTitleColumn:
								mItem->SetName(inNewText);
								RowChanged();
								break;
							
							case kTOCSrcColumn:
								mTOCItem->SetSrc(inNewText);
								RowChanged();
								break;
							
							case kTOCClassColumn:
								mTOCItem->SetClass(inNewText);
								RowChanged();
								break;
						}
					}

	virtual bool	RowDropPossible() const		{ return dynamic_cast<MProjectGroup*>(mItem) != nil; }

	MProjectItem*	GetProjectItem()			{ return mItem; }
	MePubTOCItem*	GetTOCItem()				{ return mTOCItem; }

  private:
	MProjectItem*	mItem;
	MePubTOCItem*	mTOCItem;
};

// ---------------------------------------------------------------------------

MePubWindow::MePubWindow()
	: MDocWindow("epub-window")
	, eKeyPressEvent(this, &MePubWindow::OnKeyPressEvent)
	, eSelectFileRow(this, &MePubWindow::SelectFileRow)
	, eInvokeFileRow(this, &MePubWindow::InvokeFileRow)
	, eDraggedFileRow(this, &MePubWindow::DraggedFileRow)
	, eSelectTOCRow(this, &MePubWindow::SelectTOCRow)
	, eInvokeTOCRow(this, &MePubWindow::InvokeTOCRow)
	, eDraggedTOCRow(this, &MePubWindow::DraggedTOCRow)
	, eSubjectChanged(this, &MePubWindow::SubjectChanged)
	, eDateChanged(this, &MePubWindow::DateChanged)
//	, mEditedItemName(this, &MePubWindow::EditedItemName)
//	, mEditedItemID(this, &MePubWindow::EditedItemID)
//	, mEditedItemLinear(this, &MePubWindow::EditedItemLinear)
//	, mEditedItemMediaType(this, &MePubWindow::EditedItemMediaType)
//	, mEditedTOCTitle(this, &MePubWindow::EditedTOCTitle)
//	, mEditedTOCSrc(this, &MePubWindow::EditedTOCSrc)
//	, mEditedTOCClass(this, &MePubWindow::EditedTOCClass)
	, mFileTree(nil)
	, mTOCTree(nil)
{
	mController = new MController(this);
	
	mMenubar.Initialize(GetWidget('mbar'), "epub-window-menu");
	mMenubar.SetTarget(mController);

	ConnectChildSignals();

	GtkWidget* wdgt = GetWidget(kDCSubjectViewID);
	if (wdgt)
		eSubjectChanged.Connect(G_OBJECT(gtk_text_view_get_buffer(GTK_TEXT_VIEW(wdgt))), "changed");

	wdgt = GetWidget(kDCDateViewID);
	if (wdgt)
		eDateChanged.Connect(G_OBJECT(gtk_text_view_get_buffer(GTK_TEXT_VIEW(wdgt))), "changed");
}

MePubWindow::~MePubWindow()
{
	delete mFileTree;
	mFileTree = nil;
	
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
	
	MList<MePubFileRow>* fileTree = new MList<MePubFileRow>(GetWidget(kFilesListViewID));
	mFileTree = fileTree;
	mFileTree->AllowMultipleSelectedItems();
	AddRoute(fileTree->eRowSelected, eSelectFileRow);
	AddRoute(fileTree->eRowInvoked, eInvokeFileRow);
	AddRoute(fileTree->eRowDragged, eDraggedFileRow);
	mFileTree->SetColumnTitle(kePubFileNameColumn, _("File"));
	mFileTree->SetExpandColumn(kePubFileNameColumn);
	mFileTree->SetColumnEditable(kePubFileNameColumn, true);
	mFileTree->SetColumnTitle(kePubFileIDColumn, _("ID"));
	mFileTree->SetColumnTitle(kePubFileLinearColumn, _("Linear"));
	mFileTree->SetColumnTitle(kePubFileMediaTypeColumn, _("Media-Type"));
	mFileTree->SetColumnTitle(kePubFileDataSizeColumn, _("Size"));
	mFileTree->SetColumnAlignment(kePubFileDataSizeColumn, 1.0f);
	AddItemsToList(mEPub->GetFiles(), nil, fileTree);
	mFileTree->ExpandAll();

	MList<MePubTOCRow>* tocTree = new MList<MePubTOCRow>(GetWidget(kTOCListViewID));
	mTOCTree = tocTree;
	AddRoute(tocTree->eRowSelected, eSelectTOCRow);
	AddRoute(tocTree->eRowInvoked, eInvokeTOCRow);
	AddRoute(tocTree->eRowDragged, eDraggedTOCRow);
	tocTree->SetColumnTitle(kTOCTitleColumn, _("Title"));
	tocTree->SetExpandColumn(kTOCTitleColumn);
	tocTree->SetColumnEditable(kTOCTitleColumn, true);
	tocTree->SetColumnTitle(kTOCSrcColumn, _("Source"));
	tocTree->SetColumnTitle(kTOCClassColumn, _("Class"));
	AddItemsToList(mEPub->GetTOC(), nil, tocTree);
	tocTree->ExpandAll();
	
//	MGtkTreeView tocTree(GetWidget(kTOCListViewID));
//	InitializeTOCTreeView(tocTree);
//	mTOCTree = new MTOCTree(mEPub->GetTOC());
//
//	AddRoute(mEPub->eItemMoved, mTOCTree->eProjectItemMoved);
//	AddRoute(mEPub->eItemRemoved, mTOCTree->eProjectItemRemoved);
////	AddRoute(mEPub->eCreateItem, mFileTree->eProjectCreateItem);
//	
//	tocTree.SetModel(mTOCTree->GetModel());
//	tocTree.ExpandAll();

	// fill in the information fields
	
	SetText(kDCIDViewID,			mEPub->GetDocumentID());
	SetText(kDocIDSchemeViewID,		mEPub->GetDocumentIDScheme());

	SetText(kDCTitleViewID,			mEPub->GetDublinCoreValue("title"));
	SetText(kDCLanguageViewID,		mEPub->GetDublinCoreValue("language"));
	SetText(kDCCreatorViewID,		mEPub->GetDublinCoreValue("creator"));
	SetText(kDCPublisherViewID,		mEPub->GetDublinCoreValue("publisher"));
	SetText(kDCDateViewID,			mEPub->GetDublinCoreValue("date"));
	SetText(kDCDescriptionViewID,	mEPub->GetDublinCoreValue("description"));
	SetText(kDCCoverageViewID,		mEPub->GetDublinCoreValue("coverage"));
	SetText(kDCSourceViewID,		mEPub->GetDublinCoreValue("source"));
	SetText(kDCRightsViewID,		mEPub->GetDublinCoreValue("rights"));
	SetText(kDCSubjectViewID,		mEPub->GetDublinCoreValue("subject"));

	bool useState = false;
	MePubState state = {};
	
	if (Preferences::GetInteger("save state", 1))
	{
		ssize_t r = mEPub->GetFile().ReadAttribute(kJapieePubState, &state, kMePubStateSize);
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
}

template<class R>
void MePubWindow::AddItemsToList(
	MProjectGroup*		inGroup,
	MListRowBase*		inParent,
	MList<R>*			inList)
{
	for (auto iter = inGroup->GetItems().begin(); iter != inGroup->GetItems().end(); ++iter)
	{
		R* item = new R(*iter);
		
		inList->AppendRow(item, static_cast<R*>(inParent));
		
		MProjectGroup* group = dynamic_cast<MProjectGroup*>(*iter);
		if (group != nil)
			AddItemsToList(group, item, inList);
	}
}

void MePubWindow::DocumentLoaded(
	MDocument*		inDocument)
{
//	AddItemsToList(mProject->GetFiles(), nil, fileTree);
	mFileTree->ExpandAll();

//	MGtkTreeView filesTree(GetWidget(kFilesListViewID));
//	filesTree.SetModel(nil);
//	filesTree.SetModel(mFileTree->GetModel());
//	filesTree.ExpandAll();
//
//	MGtkTreeView tocTree(GetWidget(kTOCListViewID));
//	tocTree.SetModel(nil);
//	tocTree.SetModel(mTOCTree->GetModel());
//	tocTree.ExpandAll();

	// fill in the information fields
	
	SetText(kDCIDViewID,			mEPub->GetDocumentID());
	SetText(kDocIDSchemeViewID,		mEPub->GetDocumentIDScheme());

	SetText(kDCTitleViewID,			mEPub->GetDublinCoreValue("title"));
	SetText(kDCLanguageViewID,		mEPub->GetDublinCoreValue("language"));
	SetText(kDCCreatorViewID,		mEPub->GetDublinCoreValue("creator"));
	SetText(kDCPublisherViewID,		mEPub->GetDublinCoreValue("publisher"));
	SetText(kDCDateViewID,			mEPub->GetDublinCoreValue("date"));
	SetText(kDCDescriptionViewID,	mEPub->GetDublinCoreValue("description"));
	SetText(kDCCoverageViewID,		mEPub->GetDublinCoreValue("coverage"));
	SetText(kDCSourceViewID,		mEPub->GetDublinCoreValue("source"));
	SetText(kDCRightsViewID,		mEPub->GetDublinCoreValue("rights"));
	SetText(kDCSubjectViewID,		mEPub->GetDublinCoreValue("subject"));

	bool useState = false;
	MePubState state = {};
	
	if (Preferences::GetInteger("save state", 1))
	{
		ssize_t r = mEPub->GetFile().ReadAttribute(kJapieePubState, &state, kMePubStateSize);
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
	
	mEPub->SetModified(true);
}

bool MePubWindow::DoClose()
{
	bool result = true;

	if (GetDocument() != nil)
	{
		MProjectGroup* files = mEPub->GetFiles();
		
		for (MProjectGroup::iterator item = files->begin(); item != files->end(); ++item)
		{
			MProjectItem* pItem = &(*item);
			
			MePubItem* epubItem = dynamic_cast<MePubItem*>(pItem);
			if (epubItem == nil)
				continue;
			
			MFile file(new MePubContentFile(mEPub, epubItem->GetPath()));
	
			MDocument* doc = MDocument::GetDocumentForFile(file);
			if (doc == nil)
				continue;
			
			MController* controller = doc->GetFirstController();
			if (not controller->TryCloseController(kSaveChangesClosingDocument))
			{
				result = false;
				break;
			}
		}
	}
	
	if (result == true)
		result = MDocWindow::DoClose();
	
	return result;
}

// ---------------------------------------------------------------------------
//	SaveState

void MePubWindow::SaveState()
{
	try
	{
		MePubState state = { };

		mEPub->GetFile().ReadAttribute(kJapieePubState, &state, kMePubStateSize);
		
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
		
		mEPub->GetFile().WriteAttribute(kJapieePubState, &state, kMePubStateSize);
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
			outEnabled =
				notebook.GetPage() == kFilesPageNr;
			break;
		
		case cmd_AddFileToProject:
			outEnabled =
				notebook.GetPage() == kFilesPageNr or
				notebook.GetPage() == kTOCPageNr;
			break;
		
		case cmd_RenameItem:
			outEnabled = GetSelectedItem() != nil;
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

	switch (inCommand)
	{
		case cmd_NewGroup:
			CreateNew(true);
			break;

		case cmd_AddFileToProject:
			CreateNew(false);
			break;
		
		case cmd_RenameItem:
			RenameItem();
			break;
		
		default:
			result = MDocWindow::ProcessCommand(inCommand, inMenu, inItemIndex, inModifiers);
			break;
	}
	
	return result;
}

//// ---------------------------------------------------------------------------
////	InitializeTreeView
//
//void MePubWindow::InitializeTreeView(
//	GtkTreeView*		inGtkTreeView)
//{
//	THROW_IF_NIL(inGtkTreeView);
//
//	gtk_tree_view_enable_model_drag_dest(inGtkTreeView,
//		kTreeDragTargets, kTreeDragTargetCount,
//		GdkDragAction(GDK_ACTION_COPY|GDK_ACTION_MOVE));
//	gtk_tree_view_enable_model_drag_source(inGtkTreeView, GDK_BUTTON1_MASK,
//		kTreeDragTargets, kTreeDragTargetCount,
//		GdkDragAction(GDK_ACTION_COPY|GDK_ACTION_MOVE));
//	
//	// Add the columns and renderers
//
//	// the name column
//	GtkCellRenderer* renderer = gtk_cell_renderer_text_new();
//	GtkTreeViewColumn* column = gtk_tree_view_column_new_with_attributes (
//		_("File"), renderer, "text", kePubFileNameColumn, nil);
//	g_object_set(G_OBJECT(column), "expand", true, nil);
////	g_object_set(G_OBJECT(renderer), "editable", true, nil);
//	mFileNameCell = renderer;
//	mFileNameColumn = column;
//	mEditedItemName.Connect(G_OBJECT(renderer), "edited");
//	gtk_tree_view_append_column(inGtkTreeView, column);
//	
//	// the ID column
//	renderer = gtk_cell_renderer_text_new();
//	column = gtk_tree_view_column_new_with_attributes (
//		_("ID"), renderer, "text", kePubFileIDColumn, nil);
//	g_object_set(G_OBJECT(renderer), "editable", true, nil);
//	mEditedItemID.Connect(G_OBJECT(renderer), "edited");
//	gtk_tree_view_append_column(inGtkTreeView, column);
//	
//	// the Linear column
//	renderer = gtk_cell_renderer_toggle_new();
//	column = gtk_tree_view_column_new_with_attributes (
//		_("Linear"), renderer, "active", kePubFileLinearColumn, nil);
////	g_object_set(G_OBJECT(renderer), "editable", true, nil);
//	mEditedItemLinear.Connect(G_OBJECT(renderer), "toggled");
//	gtk_tree_view_append_column(inGtkTreeView, column);
//	
//	// the MediaType column
//	renderer = gtk_cell_renderer_combo_new();
//	column = gtk_tree_view_column_new_with_attributes (
//		_("Media-Type"), renderer, "text", kePubFileMediaTypeColumn, nil);
//	g_object_set(G_OBJECT(renderer), "text-column", 0, "editable", true,
//		"has-entry", true, "model", CreateListStoreForMediaTypes(), nil);
//	mEditedItemMediaType.Connect(G_OBJECT(renderer), "edited");
//	gtk_tree_view_append_column(inGtkTreeView, column);
//	
//	// the data size column
//	
//	renderer = gtk_cell_renderer_text_new();
//	column = gtk_tree_view_column_new_with_attributes (
//		_("Size"), renderer, "text", kePubFileDataSizeColumn, nil);
//	g_object_set(G_OBJECT(renderer), "xalign", 1.0f, nil);
//	gtk_tree_view_column_set_alignment(column, 1.0f);
//	gtk_tree_view_append_column(inGtkTreeView, column);
//	
//	// at last the dirty mark	
//	renderer = gtk_cell_renderer_pixbuf_new();
//	column = gtk_tree_view_column_new_with_attributes(
//		_(" "), renderer, "pixbuf", kePubFileDirtyColumn, nil);
//	gtk_tree_view_append_column(inGtkTreeView, column);
//
//	gtk_widget_show_all(GTK_WIDGET(inGtkTreeView));
//	
//	eKeyPressEvent.Connect(G_OBJECT(inGtkTreeView), "key-press-event");
//}
//
//// ---------------------------------------------------------------------------
////	InitializeTOCTreeView
//
//void MePubWindow::InitializeTOCTreeView(
//	GtkTreeView*		inGtkTreeView)
//{
//	THROW_IF_NIL(inGtkTreeView);
//
//	gtk_tree_view_enable_model_drag_dest(inGtkTreeView,
//		kTreeDragTargets, kTreeDragTargetCount,
//		GdkDragAction(GDK_ACTION_COPY|GDK_ACTION_MOVE));
//	gtk_tree_view_enable_model_drag_source(inGtkTreeView, GDK_BUTTON1_MASK,
//		kTreeDragTargets, kTreeDragTargetCount,
//		GdkDragAction(GDK_ACTION_COPY|GDK_ACTION_MOVE));
//	
//	// Add the columns and renderers
//
//	// the title column
//	GtkCellRenderer* renderer = gtk_cell_renderer_text_new();
//	GtkTreeViewColumn* column = gtk_tree_view_column_new_with_attributes (
//		_("Title"), renderer, "text", kTOCTitleColumn, nil);
//	g_object_set(G_OBJECT(column), "expand", true, nil);
////	g_object_set(G_OBJECT(renderer), "editable", true, nil);
//	mTOCTitleCell = renderer;
//	mTOCTitleColumn = column;
//	mEditedTOCTitle.Connect(G_OBJECT(renderer), "edited");
//	gtk_tree_view_append_column(inGtkTreeView, column);
//	
//	// the src column
//	renderer = gtk_cell_renderer_text_new();
//	column = gtk_tree_view_column_new_with_attributes (
//		_("Source"), renderer, "text", kTOCSrcColumn, nil);
//	g_object_set(G_OBJECT(renderer), "editable", true, nil);
//	mEditedTOCSrc.Connect(G_OBJECT(renderer), "edited");
//	gtk_tree_view_append_column(inGtkTreeView, column);
//	
//	// the class column
//	renderer = gtk_cell_renderer_text_new();
//	column = gtk_tree_view_column_new_with_attributes (
//		_("Class"), renderer, "text", kTOCClassColumn, nil);
//	g_object_set(G_OBJECT(renderer), "editable", true, nil);
//	mEditedTOCClass.Connect(G_OBJECT(renderer), "edited");
//	gtk_tree_view_append_column(inGtkTreeView, column);
//	
//	gtk_widget_show_all(GTK_WIDGET(inGtkTreeView));
//	
//	eKeyPressEvent.Connect(G_OBJECT(inGtkTreeView), "key-press-event");
//}

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
		DeleteSelectedItem();
		result = true;
	}
	
	return result;
}

// ---------------------------------------------------------------------------
//	MProjectWindow::SelectFileRow

void MePubWindow::SelectFileRow(
	MePubFileRow*	inRow)
{
	MePubItem* ePubItem = inRow->GetEPubItem();
	
	mFileTree->SetColumnEditable(kePubFileNameColumn, ePubItem != nil);
	mFileTree->SetColumnEditable(kePubFileIDColumn, ePubItem != nil);
//	mFileTree->SetColumnToggleable(kePubFileLinearColumn, ePubItem != nil);
	mFileTree->SetColumnEditable(kePubFileMediaTypeColumn, ePubItem != nil);
}

// ---------------------------------------------------------------------------
//	MProjectWindow::InvokeFileRow

void MePubWindow::InvokeFileRow(
	MePubFileRow*	inRow)
{
	MePubItem* ePubItem = inRow->GetEPubItem();

	if (ePubItem != nil)
	{
		if (ePubItem->IsEncrypted())
			THROW(("Cannot open an encrypted ePub item"));
		
		fs::path path = ePubItem->GetPath();

		MFile file(new MePubContentFile(mEPub, path));
		
		MDocument* doc = MDocument::GetDocumentForFile(file);

		if (doc == nil)
		{
			doc = MDocument::Create<MTextDocument>(file);
			AddRoute(doc->eFileSpecChanged, eFileSpecChanged);
		}

		gApp->DisplayDocument(doc);
	}
}

// ---------------------------------------------------------------------------
//	MProjectWindow::DraggedFileRow

void MePubWindow::DraggedFileRow(
	MePubFileRow*	inRow)
{
	MProjectItem* item = inRow->GetProjectItem();
	
	MePubFileRow* parent;
	uint32 position;
	if (inRow->GetParentAndPosition(parent, position))
	{
		MProjectGroup* oldGroup = item->GetParent();
		MProjectGroup* newGroup;
		
		if (parent != nil)
			newGroup = dynamic_cast<MProjectGroup*>(parent->GetEPubItem());
		else
			newGroup = mEPub->GetFiles();

		if (oldGroup != nil)
			oldGroup->RemoveProjectItem(item);

		if (newGroup != nil)
			newGroup->AddProjectItem(item, position);
	}
}

// ---------------------------------------------------------------------------
//	MProjectWindow::SelectTOCRow

void MePubWindow::SelectTOCRow(
	MePubTOCRow*	inRow)
{
	MePubTOCItem* ePubItem = inRow->GetTOCItem();
	
	mFileTree->SetColumnEditable(kePubFileIDColumn, ePubItem != nil);
//	mFileTree->SetColumnToggleable(kePubFileLinearColumn, ePubItem != nil);
	mFileTree->SetColumnEditable(kePubFileMediaTypeColumn, ePubItem != nil);
}

// ---------------------------------------------------------------------------
//	MProjectWindow::InvokeTOCRow

void MePubWindow::InvokeTOCRow(
	MePubTOCRow*	inRow)
{
//	MePubTOCItem* tocItem = inRow->GetTOCItem();
//
//	if (tocItem != nil)
//	{
//		if (tocItem->IsEncrypted())
//			THROW(("Cannot open an encrypted ePub item"));
//		
//		fs::path path = tocItem->GetPath();
//
//		MFile file(new MePubContentFile(mEPub, path));
//		
//		MDocument* doc = MDocument::GetDocumentForFile(file);
//
//		if (doc == nil)
//		{
//			doc = MDocument::Create<MTextDocument>(file);
//			AddRoute(doc->eFileSpecChanged, eFileSpecChanged);
//		}
//
//		gApp->DisplayDocument(doc);
//	}
}

// ---------------------------------------------------------------------------
//	MProjectWindow::DraggedTOCRow

void MePubWindow::DraggedTOCRow(
	MePubTOCRow*	inRow)
{
	MProjectItem* item = inRow->GetProjectItem();
	
	MePubTOCRow* parent;
	uint32 position;
	if (inRow->GetParentAndPosition(parent, position))
	{
		MProjectGroup* oldGroup = item->GetParent();
		MProjectGroup* newGroup;
		
		if (parent != nil)
			newGroup = dynamic_cast<MProjectGroup*>(parent->GetTOCItem());
		else
			newGroup = mEPub->GetFiles();

		if (oldGroup != nil)
			oldGroup->RemoveProjectItem(item);

		if (newGroup != nil)
			newGroup->AddProjectItem(item, position);
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

		case kDCDateViewID:
			mEPub->SetDublinCoreValue("date", GetText(inID));
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
		
		case kDocIDGenerateButtonID:
			mEPub->GenerateNewDocumentID();
			SetText(kDCIDViewID, mEPub->GetDocumentID());
			SetText(kDocIDSchemeViewID, mEPub->GetDocumentIDScheme());
			break;

		case kDocIDSchemeViewID:
			mEPub->SetDocumentIDScheme(GetText(inID));
			break;
	}
	
	mEPub->SetModified(true);
}

// ---------------------------------------------------------------------------
//	CreateNew

void MePubWindow::CreateNew(
	bool 		inDirectory)
{//
//	MGtkNotebook notebook(GetWidget(kNoteBookID));
//	unique_ptr<MGtkTreeView> treeView;
//	MProjectTree* model;
//	MProjectGroup* group;
//	GtkTreeViewColumn* column;
//	GtkCellRenderer* cell;
//
//	unique_ptr<MProjectItem> newItem;
//	
//	if (notebook.GetPage() == kFilesPageNr)
//	{
//		treeView.reset(new MGtkTreeView(GetWidget(kFilesListViewID)));
//		model = mFileTree;
//		group = mEPub->GetFiles();
//		column = mFileNameColumn;
//		cell = mFileNameCell;
//		if (inDirectory)
//			newItem.reset(new MProjectGroup(_("New Directory"), group));
//		else
//			newItem.reset(new MePubItem(_("New File"), group));
//	}
//	else if (notebook.GetPage() == kTOCPageNr)
//	{
//		treeView.reset(new MGtkTreeView(GetWidget(kTOCListViewID)));
//		model = mTOCTree;
//		group = mEPub->GetTOC();
//		column = mTOCTitleColumn;
//		cell = mTOCTitleCell;
//		newItem.reset(new MePubTOCItem(_("New TOC Entry"), group));
//	}
//	else
//		return;
//		
//	int32 index = 0;
//	
//	GtkTreePath* path = nil;
//	GtkTreeIter iter;
//	
//	if (not treeView->GetFirstSelectedRow(path))
//		path = gtk_tree_path_new_from_indices(0, -1);
//	
//	if (model->GetIter(&iter, path))
//	{
//		MProjectItem* item = reinterpret_cast<MProjectItem*>(iter.user_data);
//		group = item->GetParent();
//		index = item->GetPosition();
//	}
//	
//	group->AddProjectItem(newItem.get(), index);
//	model->ProjectItemInserted(newItem.release());
//	g_object_set(G_OBJECT(cell), "editable", true, nil);
//	treeView->SetCursor(path, column, true);
//	mEditingName = true;
//	
//	if (path != nil)
//		gtk_tree_path_free(path);
//	
//	mEPub->SetModified(true);
}

MProjectItem* MePubWindow::GetSelectedItem()
{//
//	MGtkNotebook notebook(GetWidget(kNoteBookID));
//
//	GtkTreePath* path = nil;
//	MProjectItem* result = nil;
//
//	if (notebook.GetPage() == kFilesPageNr)
//	{
//		MGtkTreeView treeView(GetWidget(kFilesListViewID));
//		treeView.GetFirstSelectedRow(path);
//
//		GtkTreeIter iter;
//		if (path != nil and mFileTree->GetIter(&iter, path))
//			result = reinterpret_cast<MProjectItem*>(iter.user_data);
//	}
//	else if (notebook.GetPage() == kTOCPageNr)
//	{
//		MGtkTreeView treeView(GetWidget(kTOCListViewID));
//		treeView.GetFirstSelectedRow(path);
//
//		GtkTreeIter iter;
//		if (path != nil and mTOCTree->GetIter(&iter, path))
//			result = reinterpret_cast<MProjectItem*>(iter.user_data);
//	}
//
//	if (path != nil)
//		gtk_tree_path_free(path);
//
//	return result;
}

void MePubWindow::DeleteSelectedItem()
{//
//	MGtkNotebook notebook(GetWidget(kNoteBookID));
//
//	GtkTreePath* path = nil;
//
//	if (notebook.GetPage() == kFilesPageNr)
//	{
//		MGtkTreeView treeView(GetWidget(kFilesListViewID));
//		treeView.GetFirstSelectedRow(path);
//
//		GtkTreeIter iter;
//		if (path != nil and mFileTree->GetIter(&iter, path))
//			mFileTree->RemoveItem(reinterpret_cast<MProjectItem*>(iter.user_data));
//	}
//	else if (notebook.GetPage() == kTOCPageNr)
//	{
//		MGtkTreeView treeView(GetWidget(kTOCListViewID));
//		treeView.GetFirstSelectedRow(path);
//
//		GtkTreeIter iter;
//		if (path != nil and mTOCTree->GetIter(&iter, path))
//			mTOCTree->RemoveItem(reinterpret_cast<MProjectItem*>(iter.user_data));
//	}
//
//	if (path != nil)
//		gtk_tree_path_free(path);
}

void MePubWindow::RenameItem()
{//
//	if (mEditingName)
//		return;
//	
//	MGtkNotebook notebook(GetWidget(kNoteBookID));
//
//	GtkTreePath* path = nil;
//
//	if (notebook.GetPage() == kFilesPageNr)
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
//	else if (notebook.GetPage() == kTOCPageNr)
//	{
//		MGtkTreeView treeView(GetWidget(kTOCListViewID));
//		treeView.GetFirstSelectedRow(path);
//		if (path != nil)
//		{
//			g_object_set(G_OBJECT(mTOCTitleCell), "editable", true, nil);
//			treeView.SetCursor(path, mTOCTitleColumn);
//			mEditingName = true;
//		}
//	}
}

//void MePubWindow::EditedItemName(
//	gchar*				inPath,
//	gchar*				inNewValue)
//{
//	mEditingName = false;
//	g_object_set(G_OBJECT(mFileNameCell), "editable", false, nil);
//	if (mFileTree->ProjectItemNameEdited(inPath, inNewValue))
//	{
//		MePubItem* item = dynamic_cast<MePubItem*>(
//			mFileTree->GetProjectItemForPath(inPath));
//
//		if (item != nil and item->GetMediaType().empty())
//			item->GuessMediaType();
//
//		mEPub->SetModified(true);
//	}
//}
//
//void MePubWindow::EditedItemID(
//	gchar*				inPath,
//	gchar*				inNewValue)
//{
//	if (mFileTree->EditedItemID(inPath, inNewValue))
//		mEPub->SetModified(true);
//}
//
//void MePubWindow::EditedItemLinear(
//	gchar*				inPath)
//{
//	if (mFileTree->EditedItemLinear(inPath))
//		mEPub->SetModified(true);
//}
//
//void MePubWindow::EditedItemMediaType(
//	gchar*				inPath,
//	gchar*				inNewValue)
//{
//	if (mFileTree->EditedItemMediaType(inPath, inNewValue))
//		mEPub->SetModified(true);
//}
//
//void MePubWindow::EditedTOCTitle(
//	gchar*				inPath,
//	gchar*				inNewValue)
//{
//	mEditingName = false;
//	g_object_set(G_OBJECT(mTOCTitleCell), "editable", false, nil);
//	if (mTOCTree->ProjectItemNameEdited(inPath, inNewValue))
//		mEPub->SetModified(true);
//}
//
//void MePubWindow::EditedTOCSrc(
//	gchar*				inPath,
//	gchar*				inNewValue)
//{
//	if (mTOCTree->EditedTOCSrc(inPath, inNewValue))
//		mEPub->SetModified(true);
//}
//
//void MePubWindow::EditedTOCClass(
//	gchar*				inPath,
//	gchar*				inNewValue)
//{
//	if (mTOCTree->EditedTOCClass(inPath, inNewValue))
//		mEPub->SetModified(true);
//}

void MePubWindow::SubjectChanged()
{
	ValueChanged(kDCSubjectViewID);
}

void MePubWindow::DateChanged()
{
	ValueChanged(kDCDateViewID);
}

//void MePubWindow::FileItemInserted(
//	MProjectItem*	inItem)
//{
//	mFileTree->ProjectItemInserted(inItem);
//}
