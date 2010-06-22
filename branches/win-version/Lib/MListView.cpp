//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "MLib.h"

//#include <iostream>
//#include <cassert>

#include "MListViewImpl.h"

using namespace std;

MListView::MListView(const string& inID, MRect inBounds)
	: MView(inID, inBounds)
	, MHandler(nil)
	, mImpl(MListViewImpl::Create(this))
{
}

MListView::~MListView()
{
	delete mImpl;
}

void MListView::ResizeFrame(int32 inXDelta, int32 inYDelta,
	int32 inWidthDelta, int32 inHeightDelta)
{
	MView::ResizeFrame(inXDelta, inYDelta, inWidthDelta, inHeightDelta);
	mImpl->FrameResized();
}

void MListView::AddedToWindow()
{
	MView::AddedToWindow();
	mImpl->AddedToWindow();

	mImpl->AppendColumn("Een", 100);
	mImpl->AppendColumn("Twee", 100);
}

////---------------------------------------------------------------------
//// MListRowBase
//
//MListRowBase::MListRowBase()
//	: mRowReference(NULL)
//{
//}
//
//MListRowBase::~MListRowBase()
//{
//	if (mRowReference != NULL)
//		gtk_tree_row_reference_free(mRowReference);
//}
//
//GtkTreePath* MListRowBase::GetTreePath() const
//{
//	return gtk_tree_row_reference_get_path(mRowReference);
//}
//
//void MListRowBase::UpdateRowReference(
//	GtkTreeModel*	inNewModel,
//	GtkTreePath*	inNewPath)
//{
//	if (mRowReference != nil)
//		gtk_tree_row_reference_free(mRowReference);
//	mRowReference = gtk_tree_row_reference_new(inNewModel, inNewPath);
//}
//
//bool MListRowBase::GetModelAndIter(
//	GtkTreeStore*&	outTreeStore,
//	GtkTreeIter&	outTreeIter)
//{
//	bool result = false;
//	if (mRowReference != nil and gtk_tree_row_reference_valid(mRowReference))
//	{
//		outTreeStore = GTK_TREE_STORE(gtk_tree_row_reference_get_model(mRowReference));
//		if (outTreeStore != nil)
//		{
//			GtkTreePath* path = gtk_tree_row_reference_get_path(mRowReference);
//			if (path != nil)
//			{
//				if (gtk_tree_model_get_iter(GTK_TREE_MODEL(outTreeStore), &outTreeIter, path))
//					result = true;
//				gtk_tree_path_free(path);
//			}
//		}
//	}
//	
//	return result;
//}
//
//void MListRowBase::RowChanged()
//{
//	UpdateDataInTreeStore();
//	
//	if (mRowReference != nil and gtk_tree_row_reference_valid(mRowReference))
//	{
//		GtkTreeModel* model = gtk_tree_row_reference_get_model(mRowReference);
//		if (model != nil)
//		{
//			GtkTreePath* path = gtk_tree_row_reference_get_path(mRowReference);
//			if (path != nil)
//			{
//				GtkTreeIter iter;
//				if (gtk_tree_model_get_iter(model, &iter, path))
//					gtk_tree_model_row_changed(model, path, &iter);
//				gtk_tree_path_free(path);
//			}
//		}
//	}
//}
//
//bool MListRowBase::GetParentAndPosition(
//	MListRowBase*&		outParent,
//	uint32&				outPosition,
//	uint32				inObjectColumn)
//{
//	bool result = false;
//
//	outPosition = 0;
//	outParent = nil;
//	
//	if (mRowReference != nil and gtk_tree_row_reference_valid(mRowReference))
//	{
//		GtkTreeModel* model = gtk_tree_row_reference_get_model(mRowReference);
//		if (model != nil)
//		{
//			GtkTreePath* path = gtk_tree_row_reference_get_path(mRowReference);
//			if (path != nil)
//			{
//				result = true;
//				
//				int depth = gtk_tree_path_get_depth(path);
//				gint* indices = gtk_tree_path_get_indices(path);
//
//				if (depth > 0)
//				{
//					outPosition = indices[depth - 1];
//					
//					if (depth > 1)
//					{
//						GtkTreeIter iter;
//						if (gtk_tree_path_up(path) and gtk_tree_model_get_iter(model, &iter, path))
//							gtk_tree_model_get(model, &iter, inObjectColumn, &outParent, -1);
//					}
//				}
//
//				gtk_tree_path_free(path);
//			}
//		}
//	}
//	
//	return result;
//}
//
////---------------------------------------------------------------------
//// MListColumnEditedListener
//
//class MListColumnEditedListener
//{
//  public:
//					MListColumnEditedListener(
//						uint32				inColumnNr,
//						MListBase*			inList,
//						GtkCellRenderer*	inRenderer,
//						bool				inListenToToggle,
//						bool				inListenToEdited,
//						bool				inListenToChanged = false)
//						: eToggled(this, &MListColumnEditedListener::Toggled)
//						, eEdited(this, &MListColumnEditedListener::Edited)
//						, mColumnNr(inColumnNr)
//						, mList(inList)
//					{
//						if (inListenToToggle)
//							eToggled.Connect(G_OBJECT(inRenderer), "toggled");
//						if (inListenToEdited)
//							eEdited.Connect(G_OBJECT(inRenderer), "edited");
//						if (inListenToChanged)
//							eEdited.Connect(G_OBJECT(inRenderer), "changed");
//					}
//
//  private:
//
//	void			Toggled(
//						gchar*				inPath)
//					{
//						GtkTreePath* path = gtk_tree_path_new_from_string(inPath);
//						if (path != nil)
//						{
//							MListRowBase* row = mList->GetRowForPath(path);
//							row->ColumnToggled(mColumnNr);
//							gtk_tree_path_free(path);
//						}
//					}
//					
//	MSlot<void(gchar*)>						eToggled;
//
//	void			Edited(
//						gchar*				inPath,
//						gchar*				inNewText)
//					{
//						string newText;
//						if (inNewText != nil)
//							newText = inNewText;
//						
//						GtkTreePath* path = gtk_tree_path_new_from_string(inPath);
//						if (path != nil)
//						{
//							MListRowBase* row = mList->GetRowForPath(path);
//							row->ColumnEdited(mColumnNr, newText);
//							gtk_tree_path_free(path);
//						}
//					}
//					
//	MSlot<void(gchar*,gchar*)>				eEdited;
//
//	uint32			mColumnNr;
//	MListBase*		mList;
//};
//
////---------------------------------------------------------------------
//// MListBase
//
//MListBase::RowDropPossibleFunc MListBase::sSavedRowDropPossible;
//MListBase::DragDataReceivedFunc MListBase::sSavedDragDataReceived;
//
//MListBase::MListBase(
//	GtkWidget*	inTreeView)
//	: MView(inTreeView, false)
//	, mCursorChanged(this, &MListBase::CursorChanged)
//	, mRowActivated(this, &MListBase::RowActivated)
//	, mRowChanged(this, &MListBase::RowChanged)
//	, mRowDeleted(this, &MListBase::RowDeleted)
//	, mRowInserted(this, &MListBase::RowInserted)
//	, mRowsReordered(this, &MListBase::RowsReordered)
//{
//	mCursorChanged.Connect(inTreeView, "cursor-changed");
//	mRowActivated.Connect(inTreeView, "row-activated");
//	
//	mRowSelectedTimer.eTimedOut.SetProc(this, &MListBase::RowSelectedTimeOutWaited);
//	mRowEditingTimedOut.eTimedOut.SetProc(this, &MListBase::DisableEditingAndTimer);
//
//	gtk_tree_view_set_reorderable(GTK_TREE_VIEW(GetGtkWidget()), true);
//
////	gtk_tree_view_enable_model_drag_dest(GTK_TREE_VIEW(inTreeView),
////		kTreeDragTargets, kTreeDragTargetCount,
////		GdkDragAction(GDK_ACTION_COPY|GDK_ACTION_MOVE));
////	gtk_tree_view_enable_model_drag_source(GTK_TREE_VIEW(inTreeView), GDK_BUTTON1_MASK,
////		kTreeDragTargets, kTreeDragTargetCount,
////		GdkDragAction(GDK_ACTION_COPY|GDK_ACTION_MOVE));
//}
//
//MListBase::~MListBase()
//{
//	cout << "list deleted" << endl;
//}
//
//void MListBase::CreateTreeStore(
//	std::vector<GType>&		inTypes,
//	std::vector<MGtkRendererInfo>&
//							inRenderers)
//{
//    mTreeStore = gtk_tree_store_newv(inTypes.size(), &inTypes[0]);
//
//	gtk_tree_view_set_model(GTK_TREE_VIEW(GetGtkWidget()), GTK_TREE_MODEL(mTreeStore));
//
//	g_object_set_data(G_OBJECT(mTreeStore), "mlistbase", this);
//	GtkTreeDragDestIface* dragIface = GTK_TREE_DRAG_DEST_GET_IFACE(mTreeStore);
//	if (sSavedRowDropPossible == nil)
//	{
//		sSavedRowDropPossible = dragIface->row_drop_possible;
//		dragIface->row_drop_possible = &MListBase::RowDropPossibleCallback;
//		
//		sSavedDragDataReceived = dragIface->drag_data_received;
//		dragIface->drag_data_received = &MListBase::DragDataReceivedCallback;
//	}
//
//	mRowChanged.Connect(G_OBJECT(mTreeStore), "row-changed");
//	mRowDeleted.Connect(G_OBJECT(mTreeStore), "row-deleted");
//	mRowInserted.Connect(G_OBJECT(mTreeStore), "row-inserted");
//	mRowsReordered.Connect(G_OBJECT(mTreeStore), "rows-reordered");
//	
//	for (size_t i = 0; i < inRenderers.size(); ++i)
//	{
//	    GtkCellRenderer* renderer = get<0>(inRenderers[i]);
//	    
//	    mRenderers.push_back(renderer);
//	    
//	    const char* attribute = get<1>(inRenderers[i]);
//	    GtkTreeViewColumn* column = gtk_tree_view_column_new();
//	    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(column), renderer, TRUE);
//	    gtk_tree_view_column_set_attributes(column, renderer, attribute, i, NULL);
//	    gtk_tree_view_append_column(GTK_TREE_VIEW(GetGtkWidget()), column);
//
//		mListeners.push_back(new MListColumnEditedListener(i, this, renderer,
//					get<2>(inRenderers[i]), get<3>(inRenderers[i])));
//	}
//}
//
//void MListBase::AllowMultipleSelectedItems()
//{
//	GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(GetGtkWidget()));
//	if (selection != nil)
//		gtk_tree_selection_set_mode(selection, GTK_SELECTION_MULTIPLE);
//}
//
//MListRowBase* MListBase::GetRowForPath(
//	GtkTreePath*		inPath) const
//{
//	MListRowBase* row = nil;
//	
//	GtkTreeIter iter;
//	if (inPath != nil and gtk_tree_model_get_iter(GTK_TREE_MODEL(mTreeStore), &iter, inPath))
//		gtk_tree_model_get(GTK_TREE_MODEL(mTreeStore), &iter, GetColumnCount(), &row, -1);
//	
//	return row;
//}
//
//GtkTreePath* MListBase::GetTreePathForRow(
//	MListRowBase*		inRow)
//{
//	return inRow->GetTreePath();
//}
//
//bool MListBase::GetTreeIterForRow(
//	MListRowBase*		inRow,
//	GtkTreeIter*		outIter)
//{
//	bool result = false;
//	GtkTreePath* path = GetTreePathForRow(inRow);
//	if (path != nil)
//	{
//		if (gtk_tree_model_get_iter(GTK_TREE_MODEL(mTreeStore), outIter, path))
//			result = true;
//		gtk_tree_path_free(path);
//	}
//	return result;
//}
//
//MListRowBase* MListBase::GetCursorRow() const
//{
//	MListRowBase* row = nil;
//	
//	GtkTreePath* path;
//	gtk_tree_view_get_cursor(GTK_TREE_VIEW(GetGtkWidget()), &path, nil);
//	if (path != nil)
//	{
//		row = GetRowForPath(path);
//		gtk_tree_path_free(path);
//	}
//	
//	return row;
//}
//
//void MListBase::GetSelectedRows(
//	list<MListRowBase*>&	outRows) const
//{
//	GList* rows = gtk_tree_selection_get_selected_rows(
//		gtk_tree_view_get_selection(GTK_TREE_VIEW(GetGtkWidget())), nil);
//					
//	if (rows != nil)
//	{
//		GList* row = rows;
//		while (row != nil)
//		{
//			outRows.push_back(GetRowForPath((GtkTreePath*)row->data));
//			row = row->next;
//		}
//		
//		g_list_foreach(rows, (GFunc)(gtk_tree_path_free), nil);
//		g_list_free(rows);
//	}
//}
//
//void MListBase::SetColumnTitle(
//	uint32			inColumnNr,
//	const string&	inTitle)
//{
//	GtkTreeViewColumn* column = gtk_tree_view_get_column(GTK_TREE_VIEW(GetGtkWidget()), inColumnNr);
//	if (column == NULL)
//		throw "column not found";
//	gtk_tree_view_column_set_title(column, inTitle.c_str());
//}
//
//void MListBase::SetExpandColumn(
//	uint32			inColumnNr)
//{
//	GtkTreeViewColumn* column = gtk_tree_view_get_column(GTK_TREE_VIEW(GetGtkWidget()), inColumnNr);
//	if (column == NULL)
//		throw "column not found";
//	g_object_set(G_OBJECT(column), "expand", true, nil);
//}
//
//void MListBase::SetColumnAlignment(
//	uint32				inColumnNr,
//	float				inAlignment)
//{
//	GtkTreeViewColumn* column = gtk_tree_view_get_column(GTK_TREE_VIEW(GetGtkWidget()), inColumnNr);
//	if (column == NULL)
//		throw "column not found";
//	gtk_tree_view_column_set_alignment(column, inAlignment);
//	if (inColumnNr < mRenderers.size())
//		g_object_set(G_OBJECT(mRenderers[inColumnNr]), "xalign", inAlignment, nil);
//}
//
//void MListBase::SetColumnEditable(
//	uint32			inColumnNr,
//	bool			inEditable)
//{
//	if (inColumnNr < mRenderers.size())
//	{
//		if (inEditable)
//			mEnabledEditColumns.insert(inColumnNr);
//		else
//		{
//			mEnabledEditColumns.erase(inColumnNr);
//			g_object_set(G_OBJECT(mRenderers[inColumnNr]), "editable", false, nil);
//		}
//	}
//}
//
//void MListBase::SetColumnToggleable(
//	uint32			inColumnNr,
//	bool			inToggleable)
//{
//	if (inColumnNr < mRenderers.size())
//		 g_object_set(G_OBJECT(mRenderers[inColumnNr]), "activatable", inToggleable, nil);
//}
//
//void MListBase::SetListOfOptionsForColumn(
//	uint32					inColumnNr,
//	const vector<string>&	inOptions)
//{
//	if (inColumnNr >= mRenderers.size())
//		THROW(("Invalid column specified for SetListOfOptionsForColumn"));
//	
//	GtkListStore* model = gtk_list_store_new(1, G_TYPE_STRING);
//	
//	for (auto option = inOptions.begin(); option != inOptions.end(); ++option)
//	{
//		GtkTreeIter iter;
//		gtk_list_store_append(model, &iter);
//		gtk_list_store_set(model, &iter, 0, option->c_str(), -1);
//	}
//
//	g_object_set(G_OBJECT(mRenderers[inColumnNr]),
//		"text-column", 0,
//		"editable", true,
//		"has-entry", true,
//		"model", model,
//		nil);
//	
////	GtkTreeViewColumn* column = gtk_tree_view_get_column(GTK_TREE_VIEW(GetGtkWidget()), inColumnNr);
////	gtk_cell_layout_clear(GTK_CELL_LAYOUT(column));
////
////	GtkCellRenderer* renderer = gtk_cell_renderer_combo_new();
////    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(column), renderer, TRUE);
////    gtk_tree_view_column_set_attributes(column, renderer, "text", inColumnNr, nil);
////	g_object_set(G_OBJECT(renderer), "text-column", 0, "editable", true,
////		"has-entry", true, "model", model, nil);
////
////	delete mListeners[inColumnNr];
////	mListeners[inColumnNr] = new MListColumnEditedListener(inColumnNr, this, renderer,
////		false, false, true);
//}
//
//void MListBase::CollapseRow(
//	MListRowBase*		inRow)
//{
//	GtkTreePath* path = GetTreePathForRow(inRow);
//	if (path != nil)
//	{
//		gtk_tree_view_collapse_row(GTK_TREE_VIEW(GetGtkWidget()), path);
//		gtk_tree_path_free(path);
//	}
//}
//
//void MListBase::ExpandRow(
//	MListRowBase*		inRow,
//	bool				inExpandAll)
//{
//	GtkTreePath* path = GetTreePathForRow(inRow);
//	if (path != nil)
//	{
//		gtk_tree_view_expand_row(GTK_TREE_VIEW(GetGtkWidget()), path, inExpandAll);
//		gtk_tree_path_free(path);
//	}
//}
//
//void MListBase::CollapseAll()
//{
//	gtk_tree_view_collapse_all(GTK_TREE_VIEW(GetGtkWidget()));
//}
//
//void MListBase::ExpandAll()
//{
//	gtk_tree_view_expand_all(GTK_TREE_VIEW(GetGtkWidget()));
//}
//
//void MListBase::AppendRow(
//	MListRowBase*		inRow,
//	MListRowBase*		inParentRow)
//{
//	// store in tree
//	GtkTreeIter iter, parent;
//	
//	if (inParentRow != nil and GetTreeIterForRow(inParentRow, &parent))
//		gtk_tree_store_append(mTreeStore, &iter, &parent);
//	else
//		gtk_tree_store_append(mTreeStore, &iter, NULL);
//
//	GtkTreePath* path = gtk_tree_model_get_path(GTK_TREE_MODEL(mTreeStore), &iter);
//	if (path != nil)
//	{
//		inRow->mRowReference = gtk_tree_row_reference_new(GTK_TREE_MODEL(mTreeStore), path);
//		gtk_tree_path_free(path);
//	}
//
//	inRow->UpdateDataInTreeStore();
//}
//
//void MListBase::InsertRow(
//	MListRowBase*		inRow,
//	MListRowBase*		inBefore)
//{
//	// store in tree
//	GtkTreeIter iter, siblingIter;
//	
//	if (inBefore != nil and GetTreeIterForRow(inBefore, &siblingIter))
//		gtk_tree_store_insert_before(mTreeStore, &iter, nil, &siblingIter);
//	else
//		gtk_tree_store_append(mTreeStore, &iter, NULL);
//
//	GtkTreePath* path = gtk_tree_model_get_path(GTK_TREE_MODEL(mTreeStore), &iter);
//	if (path != nil)
//	{
//		inRow->mRowReference = gtk_tree_row_reference_new(GTK_TREE_MODEL(mTreeStore), path);
//		gtk_tree_path_free(path);
//	}
//
//	inRow->UpdateDataInTreeStore();
//}
//
//void MListBase::RemoveRow(
//	MListRowBase*		inRow)
//{
//	GtkTreeIter iter;
//	if (GetTreeIterForRow(inRow, &iter))
//		gtk_tree_store_remove(mTreeStore, &iter);
//}
//
//gboolean MListBase::RemoveAllCB(
//	GtkTreeModel*		inModel,
//	GtkTreePath*		inPath,
//	GtkTreeIter*		inIter,
//	gpointer			inData)
//{
//	MListRowBase* row = reinterpret_cast<MListBase*>(inData)->GetRowForPath(inPath);
//	delete row;
//	return false;
//}
//
//void MListBase::RemoveAll()
//{
//	gtk_tree_model_foreach(GTK_TREE_MODEL(mTreeStore), &MListBase::RemoveAllCB, this);
//	
//	gtk_tree_store_clear(mTreeStore);
//}
//
//void MListBase::SelectRow(
//	MListRowBase*		inRow)
//{
//	GtkTreePath* path = inRow->GetTreePath();
//	if (path != nil)
//	{
//		GtkTreeViewColumn* column = gtk_tree_view_get_column(GTK_TREE_VIEW(GetGtkWidget()), 0);
//		
//		gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(GetGtkWidget()),
//			path, column, false, 0, 0);
//
//		gtk_tree_view_set_cursor(GTK_TREE_VIEW(GetGtkWidget()),
//			path, column, false);
//
//		gtk_tree_path_free(path);
//	}
//}
//
//void MListBase::SelectRowAndStartEditingColumn(
//	MListRowBase*		inRow,
//	uint32				inColumnNr)
//{
//	GtkTreePath* path = inRow->GetTreePath();
//	if (path != nil)
//	{
//		GtkTreeViewColumn* column = gtk_tree_view_get_column(GTK_TREE_VIEW(GetGtkWidget()), 0);
//		
//		gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(GetGtkWidget()),
//			path, column, false, 0, 0);
//
//		gtk_tree_view_set_cursor(GTK_TREE_VIEW(GetGtkWidget()),
//			path, column, true);
//
//		gtk_tree_path_free(path);
//	}
//}
//
//gboolean MListBase::RowDropPossibleCallback(
//	GtkTreeDragDest*	inTreeDragDest,
//	GtkTreePath*		inPath,
//	GtkSelectionData*	inSelectionData)
//{
//	bool result = (*MListBase::sSavedRowDropPossible)(inTreeDragDest, inPath, inSelectionData);
//	if (result)
//	{
//		MListBase* self = reinterpret_cast<MListBase*>(g_object_get_data(G_OBJECT(inTreeDragDest), "mlistbase"));
//		if (self != nil)
//			result = self->RowDropPossible(inPath, inSelectionData);
//	}
//
//	return result;
//}
//
//bool MListBase::RowDropPossible(
//	GtkTreePath*		inTreePath,
//	GtkSelectionData*	inSelectionData)
//{
//	bool result = false;
//
//	GtkTreePath* path = gtk_tree_path_copy(inTreePath);
//
//	if (gtk_tree_path_get_depth(path) <= 1)
//		result = true;
//	else if (gtk_tree_path_up(path))
//	{
//		GtkTreeIter iter;
//		if (gtk_tree_model_get_iter(GTK_TREE_MODEL(mTreeStore), &iter, path))
//		{
//			MListRowBase* row;
//			gtk_tree_model_get(GTK_TREE_MODEL(mTreeStore), &iter, GetColumnCount(), &row, -1);
//			
//			if (row != NULL)
//				result = row->RowDropPossible();
//		}
//	}
//	
//	gtk_tree_path_free(path);
//	
//	return result;
//}
//
//gboolean MListBase::DragDataReceivedCallback(
//	GtkTreeDragDest*	inTreeDragDest,
//	GtkTreePath*		inPath,
//	GtkSelectionData*	inSelectionData)
//{
//	bool result = (*MListBase::sSavedDragDataReceived)(inTreeDragDest, inPath, inSelectionData);
//	if (result)
//	{
//		MListBase* self = reinterpret_cast<MListBase*>(g_object_get_data(G_OBJECT(inTreeDragDest), "mlistbase"));
//		if (self != nil)
//			result = self->DragDataReceived(inPath, inSelectionData);
//	}
//
//	return result;
//}
//
//bool MListBase::DragDataReceived(
//	GtkTreePath*		inTreePath,
//	GtkSelectionData*	inSelectionData)
//{
//	RowDragged(GetRowForPath(inTreePath));
//	return true;
//}	
//
//void MListBase::CursorChanged()
//{
//	MListRowBase* row = GetCursorRow();
//	
//	if (row != nil)
//	{
//		mRowSelectedTimer.Stop();
//		mRowEditingTimedOut.Stop();
//		
//		RowSelected(row);
//		mRowSelectedTimer.Start(0.3);
//	}
//}
//
//void MListBase::RowSelectedTimeOutWaited()
//{
//	for (auto col = mEnabledEditColumns.begin(); col != mEnabledEditColumns.end(); ++col)
//		 g_object_set(G_OBJECT(mRenderers[*col]), "editable", true, nil);
//	
//	mRowEditingTimedOut.Start(2.0);
//}
//
//void MListBase::DisableEditingAndTimer()
//{
//	for (auto col = mEnabledEditColumns.begin(); col != mEnabledEditColumns.end(); ++col)
//		 g_object_set(G_OBJECT(mRenderers[*col]), "editable", false, nil);
//	mEnabledEditColumns.clear();
//}
//
//void MListBase::RowActivated(
//	GtkTreePath*		inTreePath,
//	GtkTreeViewColumn*	inColumn)
//{
//	mRowSelectedTimer.Stop();
//	mRowEditingTimedOut.Stop();
//
//	mEnabledEditColumns.clear();
//	
//	MListRowBase* row = GetRowForPath(inTreePath);
//	if (row != nil)
//		RowActivated(row);
//}
//
//void MListBase::RowChanged(
//	GtkTreePath*		inTreePath,
//	GtkTreeIter*		inTreeIter)
//{
//	MListRowBase* row = GetRowForPath(inTreePath);
//	if (row != nil)
//		row->UpdateRowReference(GTK_TREE_MODEL(mTreeStore), inTreePath);
//}
//
//void MListBase::RowDeleted(
//	GtkTreePath*		inTreePath)
//{
//	eRowsReordered();
//}
//
//void MListBase::RowInserted(
//	GtkTreePath*		inTreePath,
//	GtkTreeIter*		inTreeIter)
//{
//	eRowsReordered();
//	
//	MListRowBase* row = GetRowForPath(inTreePath);
//	if (row != nil)
//		row->UpdateRowReference(GTK_TREE_MODEL(mTreeStore), inTreePath);
//}
//
//void MListBase::RowsReordered(
//	GtkTreePath*		inTreePath,
//	GtkTreeIter*		inTreeIter,
//	gint*				inNewOrder)
//{
//	eRowsReordered();
//}
//
