/*
	Copyright (c) 2006, Maarten L. Hekkelman
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

#ifndef MDIFFWINDOW_H
#define MDIFFWINDOW_H

#include "MDialog.h"
#include "MDiff.h"
#include "MFile.h"

class MDiffWindow : public MDialog
{
  public:
						MDiffWindow(
							MDocument*		inDocument = nil);
	virtual				~MDiffWindow();

	MEventIn<void()>	eDocument1Closed;
	MEventIn<void()>	eDocument2Closed;
	
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
							uint32			inItemIndex);

	virtual void		ValueChanged(
							uint32				inID);

	void				Document1Closed();
	void				Document2Closed();

	void				ChooseFile(int inFileNr);
	void				MergeToFile(int inFileNr);

	void				SetDocument(int inDocNr, MDocument* inDocument);
	void				SetDirectory(int inDirNr, const MPath& inPath);

	void				RecalculateDiffs();
	void				RecalculateDiffsForDirs();
	
	void				DiffSelected();

	void				DiffInvoked(
							int32				inDiffNr);
	
	MSlot<void()>		mSelected;

	void				SetButtonTitle(int inButtonNr, const std::string& inTitle);

	virtual void		FocusChanged(
							uint32				inFocussedID);

	void				AddDirDiff(const std::string& inName, uint32 inStatus);
	bool				FilesDiffer(const MUrl& inA, const MUrl& inB) const;
	
	void				ArrangeWindows();

	void				ClearList();
	
	int32				GetSelectedRow() const;
	
	void				SelectRow(
							int32			inRow);

	void				InvokeRow(
							GtkTreePath*		inPath,
							GtkTreeViewColumn*	inColumn);

	void				RemoveRow(
							int32			inRow);

  private:
	MDocument*			mDoc1;
	MDocument*			mDoc2;
	
	MPath				mDir1, mDir2;
	
	MDiffScript			mScript;
	
	struct MDirDiffItem
	{
		std::string		name;
		uint32			status;
	};
	
	typedef std::vector<MDirDiffItem>	MDirDiffScript;
	
	MDirDiffScript		mDScript;

	MSlot<void(GtkTreePath*path, GtkTreeViewColumn*)>
						mInvokeRow;
};

#endif
