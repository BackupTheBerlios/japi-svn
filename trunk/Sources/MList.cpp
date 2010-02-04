//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "MJapi.h"

#include <iostream>
#include <cassert>

#include "MList.h"

using namespace std;

//---------------------------------------------------------------------
// MListRowBase

MListRowBase::MListRowBase()
	: mRowReference(NULL)
{
}

MListRowBase::~MListRowBase()
{
	cout << "MListRowBase deleted" << endl;
	if (mRowReference != NULL)
		gtk_tree_row_reference_free(mRowReference);
}

GtkTreePath* MListRowBase::GetTreePath() const
{
	return gtk_tree_row_reference_get_path(mRowReference);
}

bool MListRowBase::GetModelAndIter(
	GtkTreeStore*&	outTreeStore,
	GtkTreeIter&	outTreeIter)
{
	bool result = false;
	if (mRowReference != nil and gtk_tree_row_reference_valid(mRowReference))
	{
		outTreeStore = GTK_TREE_STORE(gtk_tree_row_reference_get_model(mRowReference));
		if (outTreeStore != nil)
		{
			GtkTreePath* path = gtk_tree_row_reference_get_path(mRowReference);
			if (path != nil)
			{
				if (gtk_tree_model_get_iter(GTK_TREE_MODEL(outTreeStore), &outTreeIter, path))
					result = true;
				gtk_tree_path_free(path);
			}
		}
	}
	
	return result;
}

void MListRowBase::RowChanged()
{
	UpdateDataInTreeStore();
	
	if (mRowReference != nil and gtk_tree_row_reference_valid(mRowReference))
	{
		GtkTreeModel* model = gtk_tree_row_reference_get_model(mRowReference);
		if (model != nil)
		{
			GtkTreePath* path = gtk_tree_row_reference_get_path(mRowReference);
			if (path != nil)
			{
				GtkTreeIter iter;
				if (gtk_tree_model_get_iter(model, &iter, path))
					gtk_tree_model_row_changed(model, path, &iter);
				gtk_tree_path_free(path);
			}
		}
	}
	
}

//---------------------------------------------------------------------
// MListBase

MListBase::MListBase(
	GtkWidget*	inTreeView)
	: MView(inTreeView, false)
	, mCursorChanged(this, &MListBase::CursorChanged)
	, mRowActivated(this, &MListBase::RowActivated)
	, mRowChanged(this, &MListBase::RowChanged)
	, mRowDeleted(this, &MListBase::RowDeleted)
	, mRowInserted(this, &MListBase::RowInserted)
	, mRowsReordered(this, &MListBase::RowsReordered)
	, mEdited(this, &MListBase::Edited)
{
	mCursorChanged.Connect(inTreeView, "cursor-changed");
	mRowActivated.Connect(inTreeView, "row-activated");

	gtk_tree_view_set_reorderable(GTK_TREE_VIEW(GetGtkWidget()), true);

//	gtk_tree_view_enable_model_drag_dest(GTK_TREE_VIEW(inTreeView),
//		kTreeDragTargets, kTreeDragTargetCount,
//		GdkDragAction(GDK_ACTION_COPY|GDK_ACTION_MOVE));
//	gtk_tree_view_enable_model_drag_source(GTK_TREE_VIEW(inTreeView), GDK_BUTTON1_MASK,
//		kTreeDragTargets, kTreeDragTargetCount,
//		GdkDragAction(GDK_ACTION_COPY|GDK_ACTION_MOVE));
}

MListBase::~MListBase()
{
	cout << "list deleted" << endl;
}

void MListBase::CreateTreeStore(
	std::vector<GType>&		inTypes,
	std::vector<std::pair<GtkCellRenderer*,const char*>>&
							inRenderers)
{
    mTreeStore = gtk_tree_store_newv(inTypes.size(), &inTypes[0]);

	gtk_tree_view_set_model(GTK_TREE_VIEW(GetGtkWidget()), GTK_TREE_MODEL(mTreeStore));

	GtkTreeDragDestIface* dragIface = GTK_TREE_DRAG_DEST_GET_IFACE(mTreeStore);
	g_object_set_data(G_OBJECT(mTreeStore), "mlistbase", this);
	mSavedRowDropPossible = dragIface->row_drop_possible;
	dragIface->row_drop_possible = &MListBase::RowDropPossibleCallback;

	mRowChanged.Connect(G_OBJECT(mTreeStore), "row-changed");
	mRowDeleted.Connect(G_OBJECT(mTreeStore), "row-deleted");
	mRowInserted.Connect(G_OBJECT(mTreeStore), "row-inserted");
	mRowsReordered.Connect(G_OBJECT(mTreeStore), "rows-reordered");
	
	for (size_t i = 0; i < inRenderers.size(); ++i)
	{
	    GtkCellRenderer* renderer = inRenderers[i].first;
	    
	    mRenderers.push_back(renderer);
	    
	    const char* attribute = inRenderers[i].second;
	    GtkTreeViewColumn* column = gtk_tree_view_column_new();
	    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(column), renderer, TRUE);
	    gtk_tree_view_column_set_attributes(column, renderer, attribute, i, NULL);
	    gtk_tree_view_append_column(GTK_TREE_VIEW(GetGtkWidget()), column);
	}
}

MListRowBase* MListBase::GetRowForPath(
	GtkTreePath*		inPath)
{
	MListRowBase* row = nil;
	
	GtkTreeIter iter;
	if (inPath != nil and gtk_tree_model_get_iter(GTK_TREE_MODEL(mTreeStore), &iter, inPath))
		gtk_tree_model_get(GTK_TREE_MODEL(mTreeStore), &iter, GetColumnCount(), &row, -1);
	
	return row;
}

GtkTreePath* MListBase::GetTreePathForRow(
	MListRowBase*		inRow)
{
	return inRow->GetTreePath();
}

bool MListBase::GetTreeIterForRow(
	MListRowBase*		inRow,
	GtkTreeIter*		outIter)
{
	bool result = false;
	GtkTreePath* path = GetTreePathForRow(inRow);
	if (path != nil)
	{
		if (gtk_tree_model_get_iter(GTK_TREE_MODEL(mTreeStore), outIter, path))
			result = true;
		gtk_tree_path_free(path);
	}
	return result;
}

MListRowBase* MListBase::GetCursorRow()
{
	MListRowBase* row = nil;
	
	GtkTreePath* path;
	gtk_tree_view_get_cursor(GTK_TREE_VIEW(GetGtkWidget()), &path, nil);
	if (path != nil)
	{
		row = GetRowForPath(path);
		gtk_tree_path_free(path);
	}
	
	return row;
}

void MListBase::SetColumnTitle(
	uint32			inColumnNr,
	const string&	inTitle)
{
	GtkTreeViewColumn* column = gtk_tree_view_get_column(GTK_TREE_VIEW(GetGtkWidget()), inColumnNr);
	if (column == NULL)
		throw "column not found";
	gtk_tree_view_column_set_title(column, inTitle.c_str());
}

void MListBase::SetExpandColumn(
	uint32			inColumnNr)
{
	GtkTreeViewColumn* column = gtk_tree_view_get_column(GTK_TREE_VIEW(GetGtkWidget()), inColumnNr);
	if (column == NULL)
		throw "column not found";
	g_object_set(G_OBJECT(column), "expand", true, nil);
}

void MListBase::SetColumnAlignment(
	uint32				inColumnNr,
	float				inAlignment)
{
	GtkTreeViewColumn* column = gtk_tree_view_get_column(GTK_TREE_VIEW(GetGtkWidget()), inColumnNr);
	if (column == NULL)
		throw "column not found";
	gtk_tree_view_column_set_alignment(column, inAlignment);
	if (inColumnNr < mRenderers.size())
		g_object_set(G_OBJECT(mRenderers[inColumnNr]), "xalign", inAlignment, nil);
}

void MListBase::SetColumnEditable(
	uint32			inColumnNr,
	bool			inEditable)
{
	if (inColumnNr < mRenderers.size())
	{
		 g_object_set(G_OBJECT(mRenderers[inColumnNr]), "editable", inEditable, nil);
		 mEdited.Connect(G_OBJECT(mRenderers[inColumnNr]), "edited");
	}
}

void MListBase::CollapseRow(
	MListRowBase*		inRow)
{
	GtkTreePath* path = GetTreePathForRow(inRow);
	if (path != nil)
	{
		gtk_tree_view_collapse_row(GTK_TREE_VIEW(GetGtkWidget()), path);
		gtk_tree_path_free(path);
	}
}

void MListBase::ExpandRow(
	MListRowBase*		inRow,
	bool				inExpandAll)
{
	GtkTreePath* path = GetTreePathForRow(inRow);
	if (path != nil)
	{
		gtk_tree_view_expand_row(GTK_TREE_VIEW(GetGtkWidget()), path, inExpandAll);
		gtk_tree_path_free(path);
	}
}

void MListBase::CollapseAll()
{
	gtk_tree_view_collapse_all(GTK_TREE_VIEW(GetGtkWidget()));
}

void MListBase::ExpandAll()
{
	gtk_tree_view_expand_all(GTK_TREE_VIEW(GetGtkWidget()));
}

void MListBase::AppendRowInt(
	MListRowBase*		inRow,
	MListRowBase*		inParentRow)
{
	// store in tree
	GtkTreeIter iter, parent;
	
	if (inParentRow != nil and GetTreeIterForRow(inParentRow, &parent))
		gtk_tree_store_append(mTreeStore, &iter, &parent);
	else
		gtk_tree_store_append(mTreeStore, &iter, NULL);

	GtkTreePath* path = gtk_tree_model_get_path(GTK_TREE_MODEL(mTreeStore), &iter);
	if (path != nil)
	{
		inRow->mRowReference = gtk_tree_row_reference_new(GTK_TREE_MODEL(mTreeStore), path);
		gtk_tree_path_free(path);
	}

	inRow->UpdateDataInTreeStore();
}

void MListBase::SelectRow(
	MListRowBase*		inRow)
{
	GtkTreePath* path = inRow->GetTreePath();
	if (path != nil)
	{
		gtk_tree_view_set_cursor(GTK_TREE_VIEW(GetGtkWidget()),
			path, nil, false);
		gtk_tree_path_free(path);
	}
}

gboolean MListBase::RowDropPossibleCallback(
	GtkTreeDragDest*	inTreeDragDest,
	GtkTreePath*		inPath,
	GtkSelectionData*	inSelectionData)
{
	bool result = false;

	MListBase* self = reinterpret_cast<MListBase*>(g_object_get_data(G_OBJECT(inTreeDragDest), "mlistbase"));
	if (self != nil)
	{
		RowDropPossibleFunc func = self->mSavedRowDropPossible;
		
		if ((*func)(inTreeDragDest, inPath, inSelectionData))
			result = self->RowDropPossible(inPath, inSelectionData);
	}

	return result;
}

bool MListBase::RowDropPossible(
	GtkTreePath*		inTreePath,
	GtkSelectionData*	inSelectionData)
{
	bool result = false;

	GtkTreePath* path = gtk_tree_path_copy(inTreePath);

	if (gtk_tree_path_get_depth(path) <= 1)
		result = true;
	else if (gtk_tree_path_up(path))
	{
		GtkTreeIter iter;
		if (gtk_tree_model_get_iter(GTK_TREE_MODEL(mTreeStore), &iter, path))
		{
			MListRowBase* row;
			gtk_tree_model_get(GTK_TREE_MODEL(mTreeStore), &iter, GetColumnCount(), &row, -1);
			
			if (row != NULL)
				result = row->RowDropPossible();
		}
	}
	
	gtk_tree_path_free(path);
	
	return result;
}
	
void MListBase::CursorChanged()
{
	MListRowBase* row = GetCursorRow();
	if (row != nil)
		RowSelected(row);
}

void MListBase::RowActivated(
	GtkTreePath*		inTreePath,
	GtkTreeViewColumn*	inColumn)
{
	MListRowBase* row = GetRowForPath(inTreePath);
	if (row != nil)
		RowActivated(row);
}

void MListBase::RowChanged(
	GtkTreePath*		inTreePath,
	GtkTreeIter*		inTreeIter)
{
}

void MListBase::RowDeleted(
	GtkTreePath*		inTreePath)
{
}

void MListBase::RowInserted(
	GtkTreePath*		inTreePath,
	GtkTreeIter*		inTreeIter)
{
}

void MListBase::RowsReordered(
	GtkTreePath*		inTreePath,
	GtkTreeIter*		inTreeIter,
	gint*				inNewOrder)
{
}

void MListBase::Edited(
	gchar*				inPath,
	gchar*				inNewText)
{
	GtkTreePath* path = gtk_tree_path_new_from_string(inPath);
	if (path != nil and inNewText != nil)
	{
		GtkTreeIter iter;
		if (gtk_tree_model_get_iter(GTK_TREE_MODEL(mTreeStore), &iter, path))
		{
			MListRowBase* row = GetRowForPath(path);
			
			if (row != nil)
				EmitRowEdited(row, inNewText);
		}
		
		gtk_tree_path_free(path);
	}
}
