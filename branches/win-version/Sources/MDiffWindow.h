//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef MDIFFWINDOW_H
#define MDIFFWINDOW_H

#include "MDialog.h"
#include "MDiff.h"
#include "MFile.h"

class MDiffWindow : public MDialog
{
  public:
						MDiffWindow(
							MTextDocument*	inDocument = nil);

	virtual				~MDiffWindow();

	MEventIn<void(MDocument*)>	eDocumentClosed;
	
  protected:

	virtual bool		UpdateCommandStatus(
							uint32			inCommand,
							MMenu*			inMenu,
							uint32			inItemIndex,
							bool&			outEnabled,
							bool&			outChecked);

	virtual bool		ProcessCommand(
							uint32			inCommand,
							const MMenu*	inMenu,
							uint32			inItemIndex,
							uint32			inModifiers);

	virtual void		ValueChanged(
							uint32			inID);

	void				DocumentClosed(
							MDocument*		inDocument);

	void				ChooseFile(
							int				inFileNr);

	void				MergeToFile(
							int				inFileNr);

	void				SetDocument(
							int				inDocNr,
							MTextDocument*	inDocument);

	void				SetDirectory(
							int				inDirNr,
							const fs::path&	inPath);

	void				RecalculateDiffs();

	void				RecalculateDiffsForDirs();
	void				RecalculateDiffsForDirs(
							const MFile&	inDirA,
							const MFile&	inDirB);
	
	void				DiffSelected();

	void				DiffInvoked(
							int32			inDiffNr);
	
	//MSlot<void()>		mSelected;

	void				SetButtonTitle(
							int				inButtonNr,
							const std::string&
											inTitle);

	virtual void		FocusChanged(
							uint32			inFocussedID);

	void				AddDirDiff(
							const std::string&
											inName,
							uint32			inStatus);

	bool				FilesDiffer(
							const MFile&		inA,
							const MFile&		inB) const;
	
	void				ArrangeWindows();

	void				ClearList();
	
	int32				GetSelectedRow() const;
	
	void				SelectRow(
							int32			inRow);

	//void				InvokeRow(
	//						GtkTreePath*	inPath,
	//						GtkTreeViewColumn*
	//										inColumn);

	void				RemoveRow(
							int32			inRow);

  private:
	MTextDocument*		mDoc1;
	MTextDocument*		mDoc2;
	
	fs::path			mDir1, mDir2;
	bool				mDir1Inited, mDir2Inited;
	bool				mRecursive;
	bool				mIgnoreWhitespace;
	std::string			mIgnoreFileNameFilter;
	
	MDiffScript			mScript;
	
	struct MDirDiffItem
	{
		std::string		name;
		uint32			status;
	};
	
	typedef std::vector<MDirDiffItem>	MDirDiffScript;
	
	MDirDiffScript		mDScript;

	//MSlot<void(GtkTreePath*path, GtkTreeViewColumn*)>
	//					mInvokeRow;
};

#endif
