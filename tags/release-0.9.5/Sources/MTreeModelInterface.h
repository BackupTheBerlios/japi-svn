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
