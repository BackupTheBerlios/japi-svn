//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "MJapi.h"

#include "MJapiApp.h"

#include "MePubDocument.h"
#include "MePubWindow.h"

#include "MGtkWrappers.h"
#include "MProjectTree.h"
#include "MStrings.h"

#include "MError.h"

using namespace std;

namespace {

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
	
	kTOCViewID				= 'tre3',
	
	kNoteBookID				= 'note'
};
	
}

MePubWindow::MePubWindow()
	: MDocWindow("epub-window")
	, eKeyPressEvent(this, &MePubWindow::OnKeyPressEvent)
	, eInvokeFileRow(this, &MePubWindow::InvokeFileRow)
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
	mFilesTree = new MProjectTree(mEPub->GetFiles());

	AddRoute(mEPub->eInsertedFile, mFilesTree->eProjectItemInserted);
	AddRoute(mEPub->eRemovedFile, mFilesTree->eProjectItemRemoved);
	
	filesTree.SetModel(mFilesTree->GetModel());
	filesTree.ExpandAll();

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
}

bool MePubWindow::DoClose()
{
	bool result = false;
	
	if (MDocWindow::DoClose())
	{
		// need to do this here, otherwise we crash in destructor code
		
		delete mFilesTree;
		mFilesTree = nil;
		
		result = true;
	}
	
	return result;
}

bool MePubWindow::UpdateCommandStatus(
	uint32			inCommand,
	MMenu*			inMenu,
	uint32			inItemIndex,
	bool&			outEnabled,
	bool&			outChecked)
{
	return MDocWindow::UpdateCommandStatus(inCommand, inMenu, inItemIndex, outEnabled, outChecked);
}

bool MePubWindow::ProcessCommand(
	uint32			inCommand,
	const MMenu*	inMenu,
	uint32			inItemIndex,
	uint32			inModifiers)
{
	return MDocWindow::ProcessCommand(inCommand, inMenu, inItemIndex, inModifiers);
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
		_("File"), renderer, "text", kFilesNameColumn, nil);
	g_object_set(G_OBJECT(column), "expand", true, nil);
	gtk_tree_view_append_column(inGtkTreeView, column);
	
	// the data size column
	
	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes (
		_("Size"), renderer, "text", kFilesDataSizeColumn, nil);
	g_object_set(G_OBJECT(renderer), "xalign", 1.0f, nil);
	gtk_tree_view_column_set_alignment(column, 1.0f);
	gtk_tree_view_append_column(inGtkTreeView, column);
	
	// at last the dirty mark	
	renderer = gtk_cell_renderer_pixbuf_new();
	column = gtk_tree_view_column_new_with_attributes(
		_(" "), renderer, "pixbuf", kFilesDirtyColumn, nil);
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
		MProjectFile* file = dynamic_cast<MProjectFile*>(item);
		if (file != nil)
		{
			fs::path p = file->GetPath();
			gApp->OpenOneDocument(MUrl(p));
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
