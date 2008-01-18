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

#include "MTreeModelInterface.h"
#include "MError.h"

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
						MTreeModelImpClass*	inClass);

	static void		ModelInit(
						GtkTreeModelIface*	iface);
	
	static void		Init(
						MTreeModelImp*		self);
	
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

  /* Virtual Table */
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

void MTreeModelImp::Init(
	MTreeModelImp*	self)
{
	self->flags = GtkTreeModelFlags(0);
	self->interface = nil;
}

void MTreeModelImp::Finalize(
	GObject*		inObject)
{
	PRINT(("Finalize"));
	(*sParentClass->finalize)(inObject);
}

GType MTreeModelImp::GetType()
{
	static GType sMTreeListType = 0;
	
	/* Some boilerplate type registration stuff */
	if (sMTreeListType == 0)
	{
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

		static const GInterfaceInfo sMTreeModelInfo =
		{
			(GInterfaceInitFunc)&MTreeModelImp::ModelInit,
			nil,
			nil
		};
		
		/* First register our new derived type with the GObject type system */
		sMTreeListType = g_type_register_static (G_TYPE_OBJECT, "MTreeList",
		                                       &sMTreeListInfo, (GTypeFlags)0);
		
		/* Then register our GtkTreeModel interface with the type system */
		g_type_add_interface_static (sMTreeListType, GTK_TYPE_TREE_MODEL, &sMTreeModelInfo);
	}
	
	return sMTreeListType;
}

void MTreeModelImp::row_changed(
						GtkTreeModel *tree_model,
						GtkTreePath  *path,
						GtkTreeIter  *iter)
{
	MTreeModelImp* self = reinterpret_cast<MTreeModelImp*>(tree_model);
	if (self != nil and self->interface != nil)
		self->interface->RowChanged(path, iter);
}

void MTreeModelImp::row_inserted(
						GtkTreeModel *tree_model,
						GtkTreePath  *path,
						GtkTreeIter  *iter)
{
	MTreeModelImp* self = reinterpret_cast<MTreeModelImp*>(tree_model);
	if (self != nil and self->interface != nil)
		self->interface->RowInserted(path, iter);
}

void MTreeModelImp::row_has_child_toggled(
						GtkTreeModel *tree_model,
						GtkTreePath  *path,
						GtkTreeIter  *iter)
{
	MTreeModelImp* self = reinterpret_cast<MTreeModelImp*>(tree_model);
	if (self != nil and self->interface != nil)
		self->interface->RowHasChildToggled(path, iter);
}

void MTreeModelImp::row_deleted(
						GtkTreeModel *tree_model,
						GtkTreePath  *path)
{
	MTreeModelImp* self = reinterpret_cast<MTreeModelImp*>(tree_model);
	if (self != nil and self->interface != nil)
		self->interface->RowDeleted(path);
}

void MTreeModelImp::rows_reordered(
						GtkTreeModel *tree_model,
						GtkTreePath  *path,
						GtkTreeIter  *iter,
						gint         *new_order)
{
	MTreeModelImp* self = reinterpret_cast<MTreeModelImp*>(tree_model);
	if (self != nil and self->interface != nil)
		self->interface->RowsReordered(path, iter, *new_order);
}

GtkTreeModelFlags MTreeModelImp::get_flags(
						GtkTreeModel *tree_model)
{
	MTreeModelImp* self = reinterpret_cast<MTreeModelImp*>(tree_model);
	if (self != nil)
		return self->flags;
	else
		return GtkTreeModelFlags(0);
}

gint MTreeModelImp::get_n_columns(
						GtkTreeModel *tree_model)
{
	MTreeModelImp* self = reinterpret_cast<MTreeModelImp*>(tree_model);
	
	gint result = 0;
	
	if (self != nil and self->interface != nil)
		result = self->interface->GetColumnCount();
	
	return result;
}

GType MTreeModelImp::get_column_type(
						GtkTreeModel *tree_model,
						gint          index_)
{
	MTreeModelImp* self = reinterpret_cast<MTreeModelImp*>(tree_model);
	
	GType result = G_TYPE_NONE;
	
	if (self != nil and self->interface != nil)
		result = self->interface->GetColumnType(index_);
	
	return result;
}

gboolean MTreeModelImp::get_iter(
						GtkTreeModel *tree_model,
						GtkTreeIter  *iter,
						GtkTreePath  *path)
{
	MTreeModelImp* self = reinterpret_cast<MTreeModelImp*>(tree_model);
	
	bool result = false;
	
	if (self != nil and self->interface != nil)
		result = self->interface->GetIter(iter, path);
	
	return result;
}

GtkTreePath* MTreeModelImp::get_path(
						GtkTreeModel *tree_model,
						GtkTreeIter  *iter)
{
	MTreeModelImp* self = reinterpret_cast<MTreeModelImp*>(tree_model);

	GtkTreePath* result = nil;

	if (self != nil and self->interface != nil)
		result = self->interface->GetPath(iter);
	
	return result;
}

void MTreeModelImp::get_value(
						GtkTreeModel *tree_model,
						GtkTreeIter  *iter,
						gint          column,
						GValue       *value)
{
	MTreeModelImp* self = reinterpret_cast<MTreeModelImp*>(tree_model);

	if (self != nil and self->interface != nil)
		self->interface->GetValue(iter, column, value);
}

gboolean MTreeModelImp::iter_next(
						GtkTreeModel *tree_model,
						GtkTreeIter  *iter)
{
	MTreeModelImp* self = reinterpret_cast<MTreeModelImp*>(tree_model);
	
	bool result = false;
	
	if (self != nil and self->interface != nil)
		result = self->interface->Next(iter);
	
	return result;
}

gboolean MTreeModelImp::iter_children(
						GtkTreeModel *tree_model,
						GtkTreeIter  *iter,
						GtkTreeIter  *parent)
{
	MTreeModelImp* self = reinterpret_cast<MTreeModelImp*>(tree_model);
	
	bool result = false;
	
	if (self != nil and self->interface != nil)
		result = self->interface->Children(iter, parent);
	
	return result;
}

gboolean MTreeModelImp::iter_has_child(
						GtkTreeModel *tree_model,
						GtkTreeIter  *iter)
{
	MTreeModelImp* self = reinterpret_cast<MTreeModelImp*>(tree_model);
	
	bool result = false;
	
	if (self != nil and self->interface != nil)
		result = self->interface->HasChildren(iter);
	
	return result;
}

gint MTreeModelImp::iter_n_children(
						GtkTreeModel *tree_model,
						GtkTreeIter  *iter)
{
	MTreeModelImp* self = reinterpret_cast<MTreeModelImp*>(tree_model);
	
	bool result = false;
	
	if (self != nil and self->interface != nil)
		result = self->interface->CountChildren(iter);
	
	return result;
}

gboolean MTreeModelImp::iter_nth_child(
						GtkTreeModel *tree_model,
						GtkTreeIter  *iter,
						GtkTreeIter  *parent,
						gint          n)
{
	MTreeModelImp* self = reinterpret_cast<MTreeModelImp*>(tree_model);
	
	bool result = false;
	
	if (self != nil and self->interface != nil)
		result = self->interface->GetChild(iter, parent, n);
	
	return result;
}

gboolean MTreeModelImp::iter_parent(
						GtkTreeModel *tree_model,
						GtkTreeIter  *iter,
						GtkTreeIter  *child)
{
	MTreeModelImp* self = reinterpret_cast<MTreeModelImp*>(tree_model);

	bool result = false;

	if (self != nil and self->interface != nil)
		result = self->interface->GetParent(iter, child);
	
	return result;
}

void MTreeModelImp::ref_node(
						GtkTreeModel *tree_model,
						GtkTreeIter  *iter)
{
	MTreeModelImp* self = reinterpret_cast<MTreeModelImp*>(tree_model);
	if (self != nil and self->interface != nil)
		self->interface->RefNode(iter);
}

void MTreeModelImp::unref_node(
						GtkTreeModel *tree_model,
						GtkTreeIter  *iter)
{
	MTreeModelImp* self = reinterpret_cast<MTreeModelImp*>(tree_model);
	if (self != nil and self->interface != nil)
		self->interface->UnrefNode(iter);
}

// --------------------------------------------------------------------
//	MTreeModelInterface

MTreeModelInterface::MTreeModelInterface(
	MTreeModelFlags		inFlags)
	: mImpl(reinterpret_cast<MTreeModelImp*>(g_object_new(MTREE_TYPE_LIST, nil)))
{
	THROW_IF_NIL(mImpl);
	
	mImpl->flags = inFlags;
	mImpl->interface = this;
}

MTreeModelInterface::~MTreeModelInterface()
{
	mImpl->interface = nil;
	g_object_unref(mImpl);
}

void MTreeModelInterface::RowChanged(
	GtkTreePath*	inPath,
	GtkTreeIter*	inIter)
{
}

void MTreeModelInterface::RowInserted(
	GtkTreePath*	inPath,
	GtkTreeIter*	inIter)
{
}

void MTreeModelInterface::RowHasChildToggled(
	GtkTreePath*	inPath,
	GtkTreeIter*	inIter)
{
}

void MTreeModelInterface::RowDeleted(
	GtkTreePath*	inPath)
{
}

void MTreeModelInterface::RowsReordered(
	GtkTreePath*	inPath,
	GtkTreeIter*	inIter,
	int32&			inNewOrder)
{
}

bool MTreeModelInterface::GetIter(
	GtkTreeIter*	outIter,
	GtkTreePath*	inPath)
{
	return false;
}

GtkTreePath* MTreeModelInterface::GetPath(
	GtkTreeIter*	inIter)
{
	return nil;
}

bool MTreeModelInterface::Next(
	GtkTreeIter*	inIter)
{
	return false;
}

bool MTreeModelInterface::Children(
	GtkTreeIter*	outIter,
	GtkTreeIter*	inParent)
{
	return false;
}

bool MTreeModelInterface::HasChildren(
	GtkTreeIter*	inIter)
{
	return false;
}

int32 MTreeModelInterface::CountChildren(
	GtkTreeIter*	inIter)
{
	return 0;
}

bool MTreeModelInterface::GetChild(
	GtkTreeIter*	outIter,
	GtkTreeIter*	inParent,
	int32			inIndex)
{
	return false;
}

bool MTreeModelInterface::GetParent(
	GtkTreeIter*	outIter,
	GtkTreeIter*	inChild)
{
	return false;
}

void MTreeModelInterface::RefNode(
	GtkTreeIter*	inIter)
{
}

void MTreeModelInterface::UnrefNode(
	GtkTreeIter*	inIter)
{
}
