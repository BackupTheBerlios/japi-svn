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

class MListView;

enum MMessageKind
{
	kMsgKindNone,
	kMsgKindNote,
	kMsgKindWarning,
	kMsgKindError
};

struct MMessageItem;
typedef std::vector<MURL> MFileTable;

class MMessageList
{
  public:
					MMessageList();
					~MMessageList();

	void			AddMessage(
						MMessageKind		inKind,
						const MURL&			inFile,
						uint32				inLine,
						uint32				inMinOffset,
						uint32				inMaxOffset,
						const std::string&	inMessage);

	uint32			GetCount() const;

  private:
	friend class MMessageWindow;

					MMessageList(const MMessageList&);
	MMessageList&	operator=(const MMessageList&);

	struct MMessageListImp*
					mImpl;
};

class MMessageWindow : public MDocWindow
{
  public:
					MMessageWindow();
	
	void			AddStdErr(
						const char*			inText,
						uint32				inSize);
	
//	MEventIn<void(MMessageKind, const MURL&, uint32, const std::string&)>
//					eAddMessage;

	MEventIn<void(const MURL&)>
					eBaseDirChanged;
	
	void			SetBaseDirectory(
						const MURL&			inDir);

	void			AddMessage(
						MMessageKind		inKind,
						const MURL&			inFile,
						uint32				inLine,
						uint32				inMinOffset,
						uint32				inMaxOffset,
						const std::string&	inMessage);

	void			AddMessages(
						MMessageList&		inMessages);
						
	void			ClearList();
	
	void			SelectItem(
						uint32				inItemNr);

	void			InvokeItem(
						uint32				inItemNr);

  protected:

	virtual void	DocumentChanged(
						MDocument*			inDocument);
	
	virtual bool	DoClose();

  private:

	uint32			AddFileToTable(
						const MURL&			inFile);

	void			DrawItem(
						MDevice&			inDevice,
						MRect				inFrame,
						uint32				inRow,
						bool				inSelected,
						const void*			inData,
						uint32				inDataLength);

	MURL			mBaseDirectory;
	MFileTable		mFileTable;
	MListView*		mList;
	std::string		mText;
	double			mLastAddition;
};

#endif
