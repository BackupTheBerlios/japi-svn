//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef MPROJECTTREE_H
#define MPROJECTTREE_H

#include "MTreeModelInterface.h"
#include "MProjectItem.h"

enum MTreeDragKind {
	kTreeDragItemTreeItem,
	kTreeDragItemURI
};

extern const GtkTargetEntry kTreeDragTargets[];
extern const GdkAtom kTreeDragTargetAtoms[];
extern const uint32 kTreeDragTargetCount;

enum {
	kFilesDirtyColumn,
	kFilesNameColumn,
	kFilesTextSizeColumn,
	kFilesDataSizeColumn
};

class MProjectTree : public MTreeModelInterface
{
  public:
					MProjectTree(
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
	
	MEventOut<void(std::vector<std::string>&, MProjectGroup*, int32)>
											eProjectAddFiles;
	
	MEventOut<void(MProjectItem*, MProjectGroup*, int32)>
											eProjectMoveItem;

  private:

	void			ProjectItemStatusChanged(
						MProjectItem*	inItem);

	void			ProjectItemInserted(
						MProjectItem*	inItem);

	void			ProjectItemRemoved(
						MProjectGroup*	inGroup,
						int32			inIndex);

//	MProject*		mProject;
	MProjectGroup*	mItems;
};

#endif