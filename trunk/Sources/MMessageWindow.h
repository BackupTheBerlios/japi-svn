//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

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
struct MMessageRow;
class MListBase;
class MTextView;

class MMessageList
{
  public:
					MMessageList();

					MMessageList(const MMessageList&);

	MMessageList&	operator=(const MMessageList&);

					~MMessageList();

	void			AddMessage(
						MMessageKind		inKind,
						const MFile&		inFile,
						uint32				inLine,
						uint32				inMinOffset,
						uint32				inMaxOffset,
						const std::string&	inMessage);

	MMessageItem&	GetItem(
						uint32				inIndex) const;
	
	MFile			GetFile(
						uint32				inFileNr) const;

	uint32			GetCount() const;
	
	bool			empty() const			{ return GetCount() == 0; }

  private:
	friend class MMessageWindow;

	struct MMessageListImp*					mImpl;
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
						const MFile&		inFile,
						uint32				inLine,
						uint32				inMinOffset,
						uint32				inMaxOffset,
						const std::string&	inMessage);

	void			AddMessageToList(
						MMessageItem&		inItem);

	void			SelectMsg(
						MMessageRow*		inRow);

	void			InvokeMsg(
						MMessageRow*		inRow);

	MEventIn<void(MMessageRow*)>			eSelectMsg;
	MEventIn<void(MMessageRow*)>			eInvokeMsg;

	MListBase*		mListView;
	fs::path		mBaseDirectory;
	MMessageList	mList;
	std::string		mText;
	double			mLastAddition;
//	MTextView*		mTextView;
//	GtkWidget*		mSelectionPanel;
};

#endif
