//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "MJapi.h"

#include <sstream>
#include <cstring>
#include <boost/range/iterator_range.hpp>
#include <boost/algorithm/string.hpp>

#include "MFile.h"
#include "MFile.h"
#include "MColor.h"
#include "MUtils.h"
#include "MError.h"
#include "MProjectTree.h"

using namespace std;
namespace ba = boost::algorithm;

const GtkTargetEntry kTreeDragTargets[] =
{
	{ const_cast<gchar*>("application/x-japi-tree-item"), GTK_TARGET_SAME_WIDGET, 0 },
    { const_cast<gchar*>("text/uri-list"), 0, 0 },
};

const GdkAtom kTreeDragTargetAtoms[] =
{
	gdk_atom_intern(kTreeDragTargets[kTreeDragItemTreeItem].target, false),
	gdk_atom_intern(kTreeDragTargets[kTreeDragItemURI].target, false),
};

const uint32 kTreeDragTargetCount = sizeof(kTreeDragTargets) / sizeof(GtkTargetEntry);

MProjectTree::MProjectTree(
	MProjectGroup*		inItems)
	: MTreeModelInterface(GTK_TREE_MODEL_ITERS_PERSIST)
	, eProjectItemStatusChanged(this, &MProjectTree::ProjectItemStatusChanged)
//	, eProjectItemRemoved(this, &MProjectTree::ProjectItemRemoved)
	, mItems(inItems)
{
	vector<MProjectItem*> items;
	mItems->Flatten(items);
	
	for (vector<MProjectItem*>::iterator i = items.begin(); i != items.end(); ++i)
		AddRoute((*i)->eStatusChanged, eProjectItemStatusChanged);
}

uint32 MProjectTree::GetColumnCount() const
{
//PRINT((__func__));

	return 4;
}

GType MProjectTree::GetColumnType(
	uint32			inColumn) const
{
//PRINT((__func__));

	GType result;

	if (inColumn == kFilesDirtyColumn)
		result = GDK_TYPE_PIXMAP;
	else
		result = G_TYPE_STRING;
	
	return result;
}

void MProjectTree::GetValue(
	GtkTreeIter*	inIter,
	uint32			inColumn,
	GValue*			outValue) const
{
//PRINT((__func__));

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

bool MProjectTree::GetIter(
	GtkTreeIter*	outIter,
	GtkTreePath*	inPath)
{
//PRINT((__func__));

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

GtkTreePath* MProjectTree::GetPath(
	GtkTreeIter*	inIter)
{
//PRINT((__func__));

	GtkTreePath* path;
	MProjectItem* item = reinterpret_cast<MProjectItem*>(inIter->user_data);
	
	if (item->GetParent() == nil)
		path = gtk_tree_path_new_first();
	else
	{
		path = gtk_tree_path_new();
		while (item->GetParent() != nil)
		{
			gtk_tree_path_prepend_index(path, item->GetPosition());
			item = item->GetParent();
		}
	}

	return path;
}

bool MProjectTree::Next(
	GtkTreeIter*	ioIter)
{
//PRINT((__func__));

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

bool MProjectTree::Children(
	GtkTreeIter*	outIter,
	GtkTreeIter*	inParent)
{
//PRINT((__func__));

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

bool MProjectTree::HasChildren(
	GtkTreeIter*	inIter)
{
//PRINT((__func__));

	MProjectItem* item = reinterpret_cast<MProjectItem*>(inIter->user_data);
	MProjectGroup* group = dynamic_cast<MProjectGroup*>(item);
	
	return group != nil and group->Count() > 0;
}

int32 MProjectTree::CountChildren(
	GtkTreeIter*	inIter)
{
//PRINT((__func__));

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

bool MProjectTree::GetChild(
	GtkTreeIter*	outIter,
	GtkTreeIter*	inParent,
	int32			inIndex)
{
//PRINT((__func__));

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

bool MProjectTree::GetParent(
	GtkTreeIter*	outIter,
	GtkTreeIter*	inChild)
{
//PRINT((__func__));

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

void MProjectTree::ProjectItemStatusChanged(
	MProjectItem*	inItem)
{
//PRINT((__func__));

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

void MProjectTree::ProjectItemInserted(
	MProjectItem*	inItem)
{
//PRINT((__func__));

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

void MProjectTree::RemoveAll()
{
//PRINT((__func__));

	while (mItems->Count() > 0)
		RemoveItem(mItems->GetItem(0));
}

void MProjectTree::RemoveItem(
	MProjectItem*		inItem)
{
//PRINT((__func__));

	if (inItem != mItems and mItems->Contains(inItem))
	{
		RemoveRecursive(inItem);
		inItem->GetParent()->RemoveProjectItem(inItem);
		eProjectItemRemoved();
	}
}

bool MProjectTree::RowDraggable(
	GtkTreePath*		inPath)
{
//PRINT((__func__));

	return true;
}

bool MProjectTree::DragDataGet(
	GtkTreePath*		inPath,
	GtkSelectionData*	outData)
{
//PRINT((__func__));

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
					s << file->GetPath() << endl;
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

bool MProjectTree::DragDataDelete(
	GtkTreePath*		inPath)
{
//PRINT((__func__));

	bool result = false;
	
	PRINT(("DragDataDelete"));
	
//	GtkTreeIter iter;
//	if (GetIter(&iter, inPath))
//	{
//		mProject->RemoveItem(reinterpret_cast<MProjectItem*>(iter.user_data));
//		result = true;
//	}
	
	return result;
}

bool MProjectTree::DragDataReceived(
	GtkTreePath*		inPath,
	GtkSelectionData*	inData)
{
//PRINT((__func__));

	bool result = false;

	// find the location where to insert the items
	MProjectGroup* group = mItems;

	int32 depth = gtk_tree_path_get_depth(inPath);
	int32* indices = gtk_tree_path_get_indices(inPath);
	int32 index = indices[0];
	
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
			AddFiles(files, group, index);
			result = true;
		}
	}
	else if (inData->target == kTreeDragTargetAtoms[kTreeDragItemTreeItem])
	{
		GtkTreePath* path = gtk_tree_path_new_from_string((const gchar*)inData->data);
		
		GtkTreeIter iter;
		if (GetIter(&iter, path))
		{
			MProjectItem* item = reinterpret_cast<MProjectItem*>(iter.user_data);

			if (item->GetParent() == group and index > item->GetPosition() and index > 0)
				--index;
			
			if (group == item or 
				(dynamic_cast<MProjectGroup*>(item) != nil and
				 static_cast<MProjectGroup*>(item)->Contains(group)))
			{
				THROW(("Cannot move item into itself"));
			}	
			
			RemoveRecursive(item);
			item->GetParent()->RemoveProjectItem(item);
			
			group->AddProjectItem(item, index);
			InsertRecursive(item);
		
			eProjectItemMoved();

			result = true;
		}
		
		if (path != nil)
			gtk_tree_path_free(path);
	}
	
	return result;
}

void MProjectTree::AddFiles(
	vector<string>&		inFiles,
	MProjectGroup*		inGroup,
	int32				inIndex)
{
//PRINT((__func__));

	for (vector<string>::iterator file = inFiles.begin(); file != inFiles.end(); ++file)
	{
		ba::trim(*file);
		if (file->length() == 0)
			continue;

		MProjectItem* item = nil;
		eProjectCreateItem(*file, inGroup, item);
	
		if (item != nil)
		{
			inGroup->AddProjectItem(item, inIndex);
			InsertRecursive(item);
			++inIndex;
		}
	}
}

void MProjectTree::RemoveRecursive(
	MProjectItem*		inItem)
{
//PRINT((__func__));

	MProjectGroup* group = dynamic_cast<MProjectGroup*>(inItem);
	if (group != nil)
	{
		vector<MProjectItem*>& items = group->GetItems();
		for (int32 ix = items.size() - 1; ix >= 0; --ix)
			RemoveRecursive(items[ix]);
	}

	MProjectGroup* parent = inItem->GetParent();
	uint32 index = inItem->GetPosition();

	GtkTreeIter iter = {};
	iter.user_data = parent;
	GtkTreePath* path = GetPath(&iter);

	if (path != nil)
	{
		gtk_tree_path_append_index(path, index);
		DoRowDeleted(path);
		gtk_tree_path_free(path);
	}
}

void MProjectTree::InsertRecursive(
	MProjectItem*		inItem)
{
//PRINT((__func__));

	ProjectItemInserted(inItem);

	MProjectGroup* group = dynamic_cast<MProjectGroup*>(inItem);
	if (group != nil)
	{
		MProjectGroup* group = static_cast<MProjectGroup*>(inItem);
		vector<MProjectItem*>& items = group->GetItems();
		
		for (int32 ix = items.size() - 1; ix >= 0; --ix)
			InsertRecursive(items[ix]);
	}
}

bool MProjectTree::RowDropPossible(
	GtkTreePath*		inPath,
	GtkSelectionData*	inData)
{
//PRINT((__func__));

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

MProjectItem* MProjectTree::GetProjectItemForPath(
	const char*			inPath)
{
//PRINT((__func__));

	MProjectItem* result = nil;
	GtkTreePath* path = gtk_tree_path_new_from_string(inPath);
	
	GtkTreeIter iter;
	if (GetIter(&iter, path))
		result = reinterpret_cast<MProjectItem*>(iter.user_data);
	
	if (path != nil)
		gtk_tree_path_free(path);
	
	return result;
}

bool MProjectTree::ProjectItemNameEdited(
	const char*			inPath,
	const char*			inNewName)
{
//PRINT((__func__));

	bool result = false;
	GtkTreePath* path = gtk_tree_path_new_from_string(inPath);
	
	GtkTreeIter iter;
	if (GetIter(&iter, path))
	{
		MProjectItem* item = reinterpret_cast<MProjectItem*>(iter.user_data);
		string oldName = item->GetName();

		if (oldName != inNewName)
		{
			item->SetName(inNewName);
			RowChanged(path, &iter);
			
			eProjectItemRenamed(item, oldName, inNewName);
			
			result = true;
		}
	}
	
	if (path != nil)
		gtk_tree_path_free(path);
	
	return result;
}
