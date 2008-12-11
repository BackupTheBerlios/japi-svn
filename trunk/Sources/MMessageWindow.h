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

#ifndef MMESSAGEWINDOW_H
#define MMESSAGEWINDOW_H

#include "MDocWindow.h"
#include "MFile.h"
#include "MCallbacks.h"

enum MMessageKind
{
	kMsgKindNone,
	kMsgKindNote,
	kMsgKindWarning,
	kMsgKindError
};

struct MMessageItem;
typedef std::vector<fs::path> MFileTable;

class MMessageList
{
  public:
					MMessageList();

					MMessageList(const MMessageList&);

	MMessageList&	operator=(const MMessageList&);

					~MMessageList();

	void			AddMessage(
						MMessageKind		inKind,
						const fs::path&			inFile,
						uint32				inLine,
						uint32				inMinOffset,
						uint32				inMaxOffset,
						const std::string&	inMessage);

	MMessageItem&	GetItem(
						uint32				inIndex) const;
	
	fs::path			GetFile(
						uint32				inFileNr) const;

	uint32			GetCount() const;

  private:
	friend class MMessageWindow;

	struct MMessageListImp*
					mImpl;
};

class MMessageWindow : public MWindow
{
  public:
					MMessageWindow(
						const std::string&	inTitle);
	
	void			ClearList();
	
	void			AddStdErr(
						const char*			inText,
						uint32				inSize);
	
	MEventIn<void(const fs::path&)>
					eBaseDirChanged;
	
	void			SetBaseDirectory(
						const fs::path&			inDir);

	void			SetMessages(
						const std::string&	inDescription,
						MMessageList&		inMessages);
						
  protected:

	virtual void	DocumentChanged(
						MDocument*			inDocument);
	
	virtual bool	DoClose();

  private:

	void			AddMessage(
						MMessageKind		inKind,
						const fs::path&		inFile,
						uint32				inLine,
						uint32				inMinOffset,
						uint32				inMaxOffset,
						const std::string&	inMessage);


	void			SelectItem(
						uint32				inItemNr);

	void			InvokeItem(
						uint32				inItemNr);

	void			InvokeRow(
						GtkTreePath*		inPath,
						GtkTreeViewColumn*	inColumn);

	MSlot<void(GtkTreePath*path, GtkTreeViewColumn*)>
					mInvokeRow;

	fs::path			mBaseDirectory;
	MMessageList	mList;
	std::string		mText;
	double			mLastAddition;
};

#endif
