//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef MTREEMODELINTERFACE
#define MTREEMODELINTERFACE

typedef GtkTreeModelFlags	MTreeModelFlags;

class MTreeModelInterface
{
  public:
					MTreeModelInterface(
						MTreeModelFlags	inFlags);

	virtual			~MTreeModelInterface();

	GtkTreeModel*	GetModel() const			{ return GTK_TREE_MODEL(mImpl); }

	// signals
	
	virtual void	RowChanged(
						GtkTreePath*	inPath,
						GtkTreeIter*	inIter);

	virtual void	RowInserted(
						GtkTreePath*	inPath,
						GtkTreeIter*	inIter);

	virtual void	RowHasChildToggled(
						GtkTreePath*	inPath,
						GtkTreeIter*	inIter);

	virtual void	RowDeleted(
						GtkTreePath*	inPath);

	virtual void	RowsReordered(
						GtkTreePath*	inPath,
						GtkTreeIter*	inIter,
						int32&			inNewOrder);

	// these should at least be implemented:

	virtual uint32	GetColumnCount() const = 0;

	virtual GType	GetColumnType(
						uint32			inColumn) const = 0;

	virtual void	GetValue(
						GtkTreeIter*	inIter,
						uint32			inColumn,
						GValue*			outValue) const = 0;

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

	virtual void	RefNode(
						GtkTreeIter*	inIter);

	virtual void	UnrefNode(
						GtkTreeIter*	inIter);

	// DND
	
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

  protected:

	void			DoRowChanged(
						GtkTreePath*	inPath,
						GtkTreeIter*	inIter);
	
	void			DoRowInserted(
						GtkTreePath*	inPath,
						GtkTreeIter*	inIter);
	
	void			DoRowDeleted(
						GtkTreePath*	inPath);
	
  private:
	struct MTreeModelImp*	mImpl;
};

#endif
