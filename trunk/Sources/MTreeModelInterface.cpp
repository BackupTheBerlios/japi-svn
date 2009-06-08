//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "MJapi.h"

#include "MTreeModelInterface.h"
#include "MError.h"
#include "MAlerts.h"

using namespace std;

#define MTREE_TYPE_LIST            (MTreeModelImp::GetType())
#define MTREE_LIST(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), MTREE_TYPE_LIST, MTreeModelImp))
#define MTREE_LIST_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass),  MTREE_TYPE_LIST, MTreeModelImpClass))
#define MTREE_IS_LIST(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), MTREE_TYPE_LIST))
#define MTREE_IS_LIST_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),  MTREE_TYPE_LIST))
#define MTREE_LIST_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj),  MTREE_TYPE_LIST, MTreeModelImpClass))

struct MTreeModelImpClass
{
	GObjectClass parent_class;
};

struct MTreeModelImp
{
	GObject					parent;      /* this MUST be the first member */
	MTreeModelInterface*	interface;
	GtkTreeModelFlags		flags;

	static GObjectClass*	sParentClass;

	static void		ClassInit(
						MTreeModelImpClass*		inClass);

	static void		ModelInit(
						GtkTreeModelIface*		iface);

	static void		DragSourceInit(
						GtkTreeDragSourceIface*	iface);

	static void		DragDestInit(
						GtkTreeDragDestIface*	iface);
	
	static void		Init(
						MTreeModelImp*			self);
	
	static GType	GetType();

  /* Signals */
	static void		row_changed(
						GtkTreeModel *tree_model,
						GtkTreePath  *path,
						GtkTreeIter  *iter);

	static void		row_inserted(
						GtkTreeModel *tree_model,
						GtkTreePath  *path,
						GtkTreeIter  *iter);

	static void		row_has_child_toggled(
						GtkTreeModel *tree_model,
						GtkTreePath  *path,
						GtkTreeIter  *iter);
						
	static void		row_deleted(
						GtkTreeModel *tree_model,
						GtkTreePath  *path);
						
	static void		rows_reordered(
						GtkTreeModel *tree_model,
						GtkTreePath  *path,
						GtkTreeIter  *iter,
						gint         *new_order);

  /* Model Virtual Table */
	static GtkTreeModelFlags
					get_flags(
						GtkTreeModel *tree_model);

	static gint		get_n_columns(
						GtkTreeModel *tree_model);

	static GType	get_column_type(
						GtkTreeModel *tree_model,
						gint          index_);

	static gboolean	get_iter(
						GtkTreeModel *tree_model,
						GtkTreeIter  *iter,
						GtkTreePath  *path);
						
	static GtkTreePath*
					get_path(
						GtkTreeModel *tree_model,
						GtkTreeIter  *iter);
						
	static void		get_value(
						GtkTreeModel *tree_model,
						GtkTreeIter  *iter,
						gint          column,
						GValue       *value);

	static gboolean	iter_next(
						GtkTreeModel *tree_model,
						GtkTreeIter  *iter);

	static gboolean	iter_children(
						GtkTreeModel *tree_model,
						GtkTreeIter  *iter,
						GtkTreeIter  *parent);

	static gboolean	iter_has_child(
						GtkTreeModel *tree_model,
						GtkTreeIter  *iter);

	static gint		iter_n_children(
						GtkTreeModel *tree_model,
						GtkTreeIter  *iter);

	static gboolean	iter_nth_child(
						GtkTreeModel *tree_model,
						GtkTreeIter  *iter,
						GtkTreeIter  *parent,
						gint          n);

	static gboolean	iter_parent(
						GtkTreeModel *tree_model,
						GtkTreeIter  *iter,
						GtkTreeIter  *child);

	static void		ref_node(
						GtkTreeModel *tree_model,
						GtkTreeIter  *iter);

	static void		unref_node(
						GtkTreeModel *tree_model,
						GtkTreeIter  *iter);

	/* DragSource VTable */
	
	static gboolean	row_draggable(
						GtkTreeDragSource*	drag_source,
						GtkTreePath*		path);

	static gboolean	drag_data_get(
						GtkTreeDragSource*	drag_source,
						GtkTreePath*		path,
						GtkSelectionData*	selection_data);

	static gboolean	drag_data_delete(
						GtkTreeDragSource*	drag_source,
						GtkTreePath*		path);

	/* DragDest VTable */
	
	static gboolean	drag_data_received(
						GtkTreeDragDest*	drag_dest,
						GtkTreePath*		dest,
						GtkSelectionData*	selection_data);

	static gboolean	row_drop_possible(
						GtkTreeDragDest*	drag_dest,
						GtkTreePath*		dest_path,
						GtkSelectionData*	selection_data);

	// common
	
	static void		Finalize(
						GObject*	inObject);

};

GObjectClass* MTreeModelImp::sParentClass;

void MTreeModelImp::ClassInit(
	MTreeModelImpClass*	inClass)
{
	GObjectClass *object_class;

	sParentClass = (GObjectClass*)g_type_class_peek_parent(inClass);
	object_class = (GObjectClass*)inClass;

	object_class->finalize = &MTreeModelImp::Finalize;
}

void MTreeModelImp::ModelInit(
	GtkTreeModelIface*	iface)
{
	iface->row_changed =			&MTreeModelImp::row_changed;
	iface->row_inserted =			&MTreeModelImp::row_inserted;
	iface->row_has_child_toggled =	&MTreeModelImp::row_has_child_toggled;
	iface->row_deleted =			&MTreeModelImp::row_deleted;
	iface->rows_reordered =			&MTreeModelImp::rows_reordered;
	iface->get_flags =				&MTreeModelImp::get_flags;
	iface->get_n_columns =			&MTreeModelImp::get_n_columns;
	iface->get_column_type =		&MTreeModelImp::get_column_type;
	iface->get_iter =				&MTreeModelImp::get_iter;
	iface->get_path =				&MTreeModelImp::get_path;
	iface->get_value =				&MTreeModelImp::get_value;
	iface->iter_next =				&MTreeModelImp::iter_next;
	iface->iter_children =			&MTreeModelImp::iter_children;
	iface->iter_has_child =			&MTreeModelImp::iter_has_child;
	iface->iter_n_children =		&MTreeModelImp::iter_n_children;
	iface->iter_nth_child =			&MTreeModelImp::iter_nth_child;
	iface->iter_parent =			&MTreeModelImp::iter_parent;
	iface->ref_node =				&MTreeModelImp::ref_node;
	iface->unref_node =				&MTreeModelImp::unref_node;
}

void MTreeModelImp::DragSourceInit(
	GtkTreeDragSourceIface*	iface)
{
//PRINT((__func__));

	iface->row_draggable =			&MTreeModelImp::row_draggable;
	iface->drag_data_get =			&MTreeModelImp::drag_data_get;
	iface->drag_data_delete =		&MTreeModelImp::drag_data_delete;
}

void MTreeModelImp::DragDestInit(
	GtkTreeDragDestIface*	iface)
{
//PRINT((__func__));

	iface->drag_data_received =		&MTreeModelImp::drag_data_received;
	iface->row_drop_possible =		&MTreeModelImp::row_drop_possible;
}

void MTreeModelImp::Init(
	MTreeModelImp*	self)
{
////PRINT((__func__));

	self->flags = GtkTreeModelFlags(0);
	self->interface = nil;
}

void MTreeModelImp::Finalize(
	GObject*		inObject)
{
//PRINT((__func__));

//	PRINT(("Finalize"));
	(*sParentClass->finalize)(inObject);
}

GType MTreeModelImp::GetType()
{
//PRINT((__func__));

	static GType sMTreeListType = 0;
	
	/* Some boilerplate type registration stuff */
	if (sMTreeListType == 0)
	{
		/* First register our new derived type with the GObject type system */
		static const GTypeInfo sMTreeListInfo =
		{
			sizeof(MTreeModelImpClass),
			nil,                                         /* base_init */
			nil,                                         /* base_finalize */
			(GClassInitFunc)&MTreeModelImp::ClassInit,
			nil,                                         /* class finalize */
			nil,                                         /* class_data */
			sizeof(MTreeModelImp),
			0,                                           /* n_preallocs */
			(GInstanceInitFunc)&MTreeModelImp::Init
		};

		sMTreeListType = g_type_register_static(
			G_TYPE_OBJECT, "MTreeList", &sMTreeListInfo, (GTypeFlags)0);
		
		/* Then register our GtkTreeModel interface with the type system */
		static const GInterfaceInfo sMTreeModelInfo =
		{
			(GInterfaceInitFunc)&MTreeModelImp::ModelInit,
			nil,
			nil
		};
		
		g_type_add_interface_static(sMTreeListType, GTK_TYPE_TREE_MODEL, &sMTreeModelInfo);

		static const GInterfaceInfo sMTreeDragSourceInfo =
		{
			(GInterfaceInitFunc)&MTreeModelImp::DragSourceInit,
			nil,
			nil
		};
		
		g_type_add_interface_static(sMTreeListType, GTK_TYPE_TREE_DRAG_SOURCE, &sMTreeDragSourceInfo);

		static const GInterfaceInfo sMTreeDragDestInfo =
		{
			(GInterfaceInitFunc)&MTreeModelImp::DragDestInit,
			nil,
			nil
		};
		
		g_type_add_interface_static(sMTreeListType, GTK_TYPE_TREE_DRAG_DEST, &sMTreeDragDestInfo);
	}
	
	return sMTreeListType;
}

void MTreeModelImp::row_changed(
						GtkTreeModel *tree_model,
						GtkTreePath  *path,
						GtkTreeIter  *iter)
{
//PRINT((__func__));

	try
	{
	//PRINT(("row_changed"));
		MTreeModelImp* self = reinterpret_cast<MTreeModelImp*>(tree_model);
		if (self != nil and self->interface != nil)
			self->interface->RowChanged(path, iter);
	}
	catch (exception& e)
	{
		DisplayError(e);
	}
}

void MTreeModelImp::row_inserted(
						GtkTreeModel *tree_model,
						GtkTreePath  *path,
						GtkTreeIter  *iter)
{
//PRINT((__func__));

	try
	{
	//PRINT(("row_inserted"));
		MTreeModelImp* self = reinterpret_cast<MTreeModelImp*>(tree_model);
		if (self != nil and self->interface != nil)
			self->interface->RowInserted(path, iter);
	}
	catch (exception& e)
	{
		DisplayError(e);
	}
}

void MTreeModelImp::row_has_child_toggled(
						GtkTreeModel *tree_model,
						GtkTreePath  *path,
						GtkTreeIter  *iter)
{
//PRINT((__func__));

	try
	{
	//PRINT(("row_has_child_toggled"));
		MTreeModelImp* self = reinterpret_cast<MTreeModelImp*>(tree_model);
		if (self != nil and self->interface != nil)
			self->interface->RowHasChildToggled(path, iter);
	}
	catch (exception& e)
	{
		DisplayError(e);
	}
}

void MTreeModelImp::row_deleted(
						GtkTreeModel *tree_model,
						GtkTreePath  *path)
{
//PRINT((__func__));

	try
	{
	//PRINT(("row_deleted"));
		MTreeModelImp* self = reinterpret_cast<MTreeModelImp*>(tree_model);
		if (self != nil and self->interface != nil)
			self->interface->RowDeleted(path);
	}
	catch (exception& e)
	{
		DisplayError(e);
	}
}

void MTreeModelImp::rows_reordered(
						GtkTreeModel *tree_model,
						GtkTreePath  *path,
						GtkTreeIter  *iter,
						gint         *new_order)
{
//PRINT((__func__));

	try
	{
	//PRINT(("rows_reordered"));
		MTreeModelImp* self = reinterpret_cast<MTreeModelImp*>(tree_model);
		if (self != nil and self->interface != nil)
			self->interface->RowsReordered(path, iter, *new_order);
	}
	catch (exception& e)
	{
		DisplayError(e);
	}
}

GtkTreeModelFlags MTreeModelImp::get_flags(
						GtkTreeModel *tree_model)
{
//PRINT((__func__));

	try
	{
		MTreeModelImp* self = reinterpret_cast<MTreeModelImp*>(tree_model);
		if (self != nil)
			return self->flags;
		else
			return GtkTreeModelFlags(0);
	}
	catch (exception& e)
	{
		DisplayError(e);
	}
}

gint MTreeModelImp::get_n_columns(
						GtkTreeModel *tree_model)
{
//PRINT((__func__));

	gint result = 0;

	try
	{
		MTreeModelImp* self = reinterpret_cast<MTreeModelImp*>(tree_model);
		if (self != nil and self->interface != nil)
			result = self->interface->GetColumnCount();
	}
	catch (exception& e)
	{
		DisplayError(e);
	}
	
	return result;
}

GType MTreeModelImp::get_column_type(
						GtkTreeModel *tree_model,
						gint          index_)
{
//PRINT((__func__));

	GType result = G_TYPE_NONE;
	
	try
	{
		MTreeModelImp* self = reinterpret_cast<MTreeModelImp*>(tree_model);
		if (self != nil and self->interface != nil)
			result = self->interface->GetColumnType(index_);
	}
	catch (exception& e)
	{
		DisplayError(e);
	}
	
	return result;
}

gboolean MTreeModelImp::get_iter(
						GtkTreeModel *tree_model,
						GtkTreeIter  *iter,
						GtkTreePath  *path)
{
//PRINT((__func__));

	bool result = false;

	try
	{
		MTreeModelImp* self = reinterpret_cast<MTreeModelImp*>(tree_model);
		if (self != nil and self->interface != nil)
			result = self->interface->GetIter(iter, path);
	}
	catch (exception& e)
	{
		DisplayError(e);
	}
	
	return result;
}

GtkTreePath* MTreeModelImp::get_path(
						GtkTreeModel *tree_model,
						GtkTreeIter  *iter)
{
//PRINT((__func__));

	GtkTreePath* result = nil;

	try
	{
		MTreeModelImp* self = reinterpret_cast<MTreeModelImp*>(tree_model);
		if (self != nil and self->interface != nil)
			result = self->interface->GetPath(iter);
	}
	catch (exception& e)
	{
		DisplayError(e);
	}
	
	return result;
}

void MTreeModelImp::get_value(
						GtkTreeModel *tree_model,
						GtkTreeIter  *iter,
						gint          column,
						GValue       *value)
{
//PRINT((__func__));

	try
	{
		MTreeModelImp* self = reinterpret_cast<MTreeModelImp*>(tree_model);
		if (self != nil and self->interface != nil)
			self->interface->GetValue(iter, column, value);
	}
	catch (exception& e)
	{
		DisplayError(e);
	}
}

gboolean MTreeModelImp::iter_next(
						GtkTreeModel *tree_model,
						GtkTreeIter  *iter)
{
//PRINT((__func__));

	bool result = false;
	
	try
	{
		MTreeModelImp* self = reinterpret_cast<MTreeModelImp*>(tree_model);
		if (self != nil and self->interface != nil)
			result = self->interface->Next(iter);
	}
	catch (exception& e)
	{
		DisplayError(e);
	}
	
	return result;
}

gboolean MTreeModelImp::iter_children(
						GtkTreeModel *tree_model,
						GtkTreeIter  *iter,
						GtkTreeIter  *parent)
{
//PRINT((__func__));

	bool result = false;
	
	try
	{
		MTreeModelImp* self = reinterpret_cast<MTreeModelImp*>(tree_model);
		if (self != nil and self->interface != nil)
			result = self->interface->Children(iter, parent);
	}
	catch (exception& e)
	{
		DisplayError(e);
	}
	
	return result;
}

gboolean MTreeModelImp::iter_has_child(
						GtkTreeModel *tree_model,
						GtkTreeIter  *iter)
{
//PRINT((__func__));

	bool result = false;
	
	try
	{
		MTreeModelImp* self = reinterpret_cast<MTreeModelImp*>(tree_model);
		if (self != nil and self->interface != nil)
			result = self->interface->HasChildren(iter);
	}
	catch (exception& e)
	{
		DisplayError(e);
	}
	
	return result;
}

gint MTreeModelImp::iter_n_children(
						GtkTreeModel *tree_model,
						GtkTreeIter  *iter)
{
//PRINT((__func__));

	bool result = false;
		
	try
	{
		MTreeModelImp* self = reinterpret_cast<MTreeModelImp*>(tree_model);
		if (self != nil and self->interface != nil)
			result = self->interface->CountChildren(iter);
	}
	catch (exception& e)
	{
		DisplayError(e);
	}
	
	return result;
}

gboolean MTreeModelImp::iter_nth_child(
						GtkTreeModel *tree_model,
						GtkTreeIter  *iter,
						GtkTreeIter  *parent,
						gint          n)
{
//PRINT((__func__));

	bool result = false;
	
	try
	{
		MTreeModelImp* self = reinterpret_cast<MTreeModelImp*>(tree_model);
		if (self != nil and self->interface != nil)
			result = self->interface->GetChild(iter, parent, n);
	}
	catch (exception& e)
	{
		DisplayError(e);
	}
	
	return result;
}

gboolean MTreeModelImp::iter_parent(
						GtkTreeModel *tree_model,
						GtkTreeIter  *iter,
						GtkTreeIter  *child)
{
//PRINT((__func__));

	bool result = false;

	try
	{
		MTreeModelImp* self = reinterpret_cast<MTreeModelImp*>(tree_model);
		if (self != nil and self->interface != nil)
			result = self->interface->GetParent(iter, child);
	}
	catch (exception& e)
	{
		DisplayError(e);
	}
	
	return result;
}

void MTreeModelImp::ref_node(
						GtkTreeModel *tree_model,
						GtkTreeIter  *iter)
{
//PRINT((__func__));

	try
	{
		MTreeModelImp* self = reinterpret_cast<MTreeModelImp*>(tree_model);
		if (self != nil and self->interface != nil)
			self->interface->RefNode(iter);
	}
	catch (exception& e)
	{
		DisplayError(e);
	}
}

void MTreeModelImp::unref_node(
						GtkTreeModel *tree_model,
						GtkTreeIter  *iter)
{
//PRINT((__func__));

	try
	{
		MTreeModelImp* self = reinterpret_cast<MTreeModelImp*>(tree_model);
		if (self != nil and self->interface != nil)
			self->interface->UnrefNode(iter);
	}
	catch (exception& e)
	{
		DisplayError(e);
	}
}

gboolean MTreeModelImp::row_draggable(
	GtkTreeDragSource*	drag_source,
	GtkTreePath*		path)
{
//PRINT((__func__));

	bool result = false;
	
	try
	{
		MTreeModelImp* self = reinterpret_cast<MTreeModelImp*>(drag_source);
	
		if (self != nil and self->interface != nil)
			result = self->interface->RowDraggable(path);
	}
	catch (exception& e)
	{
		DisplayError(e);
	}
	
	return result;
}

gboolean MTreeModelImp::drag_data_get(
	GtkTreeDragSource*	drag_source,
	GtkTreePath*		path,
	GtkSelectionData*	selection_data)
{
//PRINT((__func__));

	bool result = false;
	
	try
	{
		MTreeModelImp* self = reinterpret_cast<MTreeModelImp*>(drag_source);
	
		if (self != nil and self->interface != nil)
			result = self->interface->DragDataGet(path, selection_data);
	}
	catch (exception& e)
	{
		DisplayError(e);
	}
	
	return result;
}

gboolean MTreeModelImp::drag_data_delete(
	GtkTreeDragSource*	drag_source,
	GtkTreePath*		path)
{
//PRINT((__func__));

	bool result = false;
	
	try
	{
		MTreeModelImp* self = reinterpret_cast<MTreeModelImp*>(drag_source);
	
		if (self != nil and self->interface != nil)
			result = self->interface->DragDataDelete(path);
	}
	catch (exception& e)
	{
		DisplayError(e);
	}
	
	return result;
}

gboolean MTreeModelImp::drag_data_received(
	GtkTreeDragDest*	drag_dest,
	GtkTreePath*		dest,
	GtkSelectionData*	selection_data)
{
//PRINT((__func__));

	bool result = false;
	
	try
	{
		MTreeModelImp* self = reinterpret_cast<MTreeModelImp*>(drag_dest);
	
		if (self != nil and self->interface != nil)
			result = self->interface->DragDataReceived(dest, selection_data);
	}
	catch (exception& e)
	{
		DisplayError(e);
	}
	
	return result;
}

gboolean MTreeModelImp::row_drop_possible(
	GtkTreeDragDest*	drag_dest,
	GtkTreePath*		dest_path,
	GtkSelectionData*	selection_data)
{
//PRINT((__func__));

	bool result = false;

	try
	{
		MTreeModelImp* self = reinterpret_cast<MTreeModelImp*>(drag_dest);
	
		if (self != nil and self->interface != nil)
			result = self->interface->RowDropPossible(dest_path, selection_data);
	}
	catch (exception& e)
	{
		DisplayError(e);
	}
	
	return result;
}

// --------------------------------------------------------------------
//	MTreeModelInterface

MTreeModelInterface::MTreeModelInterface(
	MTreeModelFlags		inFlags)
	: mImpl(reinterpret_cast<MTreeModelImp*>(g_object_new(MTREE_TYPE_LIST, nil)))
{
//PRINT((__func__));

	THROW_IF_NIL(mImpl);
	
	mImpl->flags = inFlags;
	mImpl->interface = this;
}

MTreeModelInterface::~MTreeModelInterface()
{
//PRINT((__func__));

	mImpl->interface = nil;
	g_object_unref(mImpl);
}

void MTreeModelInterface::RowChanged(
	GtkTreePath*	inPath,
	GtkTreeIter*	inIter)
{
//PRINT((__func__));

}

void MTreeModelInterface::RowInserted(
	GtkTreePath*	inPath,
	GtkTreeIter*	inIter)
{
//PRINT((__func__));

}

void MTreeModelInterface::RowHasChildToggled(
	GtkTreePath*	inPath,
	GtkTreeIter*	inIter)
{
//PRINT((__func__));

}

void MTreeModelInterface::RowDeleted(
	GtkTreePath*	inPath)
{
//PRINT((__func__));

}

void MTreeModelInterface::RowsReordered(
	GtkTreePath*	inPath,
	GtkTreeIter*	inIter,
	int32&			inNewOrder)
{
//PRINT((__func__));

}

bool MTreeModelInterface::GetIter(
	GtkTreeIter*	outIter,
	GtkTreePath*	inPath)
{
//PRINT((__func__));

	return false;
}

GtkTreePath* MTreeModelInterface::GetPath(
	GtkTreeIter*	inIter)
{
//PRINT((__func__));

	return nil;
}

bool MTreeModelInterface::Next(
	GtkTreeIter*	inIter)
{
//PRINT((__func__));

	return false;
}

bool MTreeModelInterface::Children(
	GtkTreeIter*	outIter,
	GtkTreeIter*	inParent)
{
//PRINT((__func__));

	return false;
}

bool MTreeModelInterface::HasChildren(
	GtkTreeIter*	inIter)
{
//PRINT((__func__));

	return false;
}

int32 MTreeModelInterface::CountChildren(
	GtkTreeIter*	inIter)
{
//PRINT((__func__));

	return 0;
}

bool MTreeModelInterface::GetChild(
	GtkTreeIter*	outIter,
	GtkTreeIter*	inParent,
	int32			inIndex)
{
//PRINT((__func__));

	return false;
}

bool MTreeModelInterface::GetParent(
	GtkTreeIter*	outIter,
	GtkTreeIter*	inChild)
{
//PRINT((__func__));

	return false;
}

void MTreeModelInterface::RefNode(
	GtkTreeIter*	inIter)
{
//PRINT((__func__));

}

void MTreeModelInterface::UnrefNode(
	GtkTreeIter*	inIter)
{
//PRINT((__func__));

}

bool MTreeModelInterface::RowDraggable(
	GtkTreePath*		inPath)
{
//PRINT((__func__));

	return false;
}

bool MTreeModelInterface::DragDataGet(
	GtkTreePath*		inPath,
	GtkSelectionData*	outData)
{
//PRINT((__func__));

	return false;
}

bool MTreeModelInterface::DragDataDelete(
	GtkTreePath*		inPath)
{
//PRINT((__func__));

	return false;
}

bool MTreeModelInterface::DragDataReceived(
	GtkTreePath*		inPath,
	GtkSelectionData*	inData)
{
//PRINT((__func__));

	return false;
}

bool MTreeModelInterface::RowDropPossible(
	GtkTreePath*		inPath,
	GtkSelectionData*	inData)
{
//PRINT((__func__));

	return false;
}

void MTreeModelInterface::DoRowChanged(
	GtkTreePath*	inPath,
	GtkTreeIter*	inIter)
{
//PRINT((__func__));

	gtk_tree_model_row_changed(GTK_TREE_MODEL(mImpl), inPath, inIter);
}

void MTreeModelInterface::DoRowInserted(
	GtkTreePath*	inPath,
	GtkTreeIter*	inIter)
{
//PRINT((__func__));

	gtk_tree_model_row_inserted(GTK_TREE_MODEL(mImpl), inPath, inIter);
}	

void MTreeModelInterface::DoRowDeleted(
	GtkTreePath*	inPath)
{
//PRINT((__func__));

	gtk_tree_model_row_deleted(GTK_TREE_MODEL(mImpl), inPath);
}	

