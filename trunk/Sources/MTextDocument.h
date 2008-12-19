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

/*
	
	MTextDocument is an implementation of MDocument
	
*/

#ifndef MTEXTDOCUMENT_H
#define MTEXTDOCUMENT_H

#include "MDocument.h"
#include "MTextBuffer.h"
#include "MLineInfo.h"
#include "MSelection.h"
#include "MCommands.h"

class MLanguage;
struct MNamedRange;
class MIncludeFileList;
class MWindow;
class MTextView;
class MShell;
class MMessageWindow;
class MMessageList;
class MMenu;
class MDevice;
class MSftpChannel;

struct MTextInputAreaInfo
{
	int32				fOffset[9];
	int32				fLength[9];
};

struct MDocState
{
	uint32			mSelection[4];
	uint32			mScrollPosition[2];
	uint16			mWindowPosition[2];
	uint16			mWindowSize[2];
	union
	{
		struct
		{
			bool	mSoftwrap			: 1;
			bool	mSelectionIsBlock	: 1;
			uint32	mReserved1			: 6;
			uint8	mTabWidth			: 8;
			uint32	mReserved2			: 16;
		}			mFlags;
		uint32		mSwapHelper;
	};
	
	void			Swap();
};

class MTextDocument : public MDocument
{
  public:
  
  	// first the implementation of / overrides for MDocument
  	
	explicit			MTextDocument(
							const MUrl*			inURL);

	virtual				~MTextDocument();

	static MTextDocument*
						GetFirstTextDocument();

	virtual void		SetFileNameHint(
							const std::string&	inName);

	virtual bool		DoSave();

	virtual bool		DoSaveAs(
							const MUrl&			inFile);

	virtual void		RevertDocument();

	virtual void		AddNotifier(
							MDocClosedNotifier&	inNotifier,
							bool				inRead);
	
	virtual MController*
						GetFirstController() const;

	virtual void		SetModified(
							bool				inModified);
	
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

	uint32				GetWrapWidth() const					{ return mWrapWidth; }

	void				SetWrapWidth(
							uint32			inWrapWidth);

  protected:

	virtual void		ReadFile(
							std::istream&		inFile);

	virtual void		WriteFile(
							std::ostream&		inFile);

	virtual void		SaveState();

	// and now the MTextDocument specific methods

  public:

	void				SetText(
							const char*				inText,
							uint32					inTextLength);
	
	void				SetTargetTextView(MTextView* inTextView);
	
	// SFTP support
	
	MEventOut<void(float,std::string)>		eSSHProgress;
	
	void				SFTPGetChannelEvent(
							int				inMessage);

	void				SFTPPutChannelEvent(
							int				inMessage);

	void				SFTPChannelMessage(
							std::string 	inMessage);

	
	MTextBuffer&		GetTextBuffer()						{ return mText; }

	static void			SetWorksheet(
							MTextDocument*	inDocument);

	bool				IsWorksheet() const;
	
	static MDocument*	GetWorksheet();

	const char*			GetCWD() const;

	bool				StopRunningShellCommand();
	
	void				Reset();

	void				StartAction(
							const char*		inAction);

	void				FinishAction();

	bool				ReadDocState(
							MDocState&		ioDocState);

	void				GoToLine(
							uint32			inLineNr);
	
	void				GetLine(
							uint32			inLineNr,
							std::string&	outText) const;
	
	bool				GetFastFindMode() const				{ return mFastFindMode; }

	void				GetStyledText(
							uint32			inOffset,
							uint32			inLength,
							uint16			inState,
							MDevice&		inDevice,
							std::string& 	outText) const;

	void				GetStyledText(
							uint32			inLine,
							MDevice&		inDevice,
							std::string&	outText) const;

	void				HashLines(
							std::vector<uint32>&
											outHashes) const;
	
	MSelection			GetSelection() const				{ return mSelection; }

	void				GetSelectionRegion(
							MRegion&		outRegion) const;

	void				GetSelectedText(
							std::string&	outText) const;

	void				Select(
							MSelection		inSelection);
	
	void				Select(
							uint32			inAnchor,
							uint32			inCaret,
							bool			inBlock);

	void				Select(
							uint32			inAnchor,
							uint32			inCaret,
							MScrollMessage	inScrollMessage = kScrollNone);

	void				SelectAll();
	
	void				OffsetToPosition(
							uint32			inOffset,
							uint32&			outLine,
							int32&			outX) const;

	void				PositionToOffset(
							int32			inLocationX,
							int32			inLocationY,
							uint32&			outOffset) const;
	
	void				PositionToLineColumn(
							int32			inLocationX,
							int32			inLocationY,
							uint32&			outLine,
							uint32&			outColumn) const;
	
	uint32				OffsetToColumn(
							uint32			inOffset) const;

	uint32				LineAndColumnToOffset(
							uint32			inLine,
							uint32			inColumn) const;

	uint32				OffsetToLine(
							uint32			inOffset) const;

	uint32				GuessMaxWidth() const;

	uint32				LineStart(
							uint32			inLine) const;
							// line end is the offset before the EOLN char, if any

	uint32				LineEnd(
							uint32			inLine) const;

	bool				IsLineDirty(
							uint32			inLine) const;

	bool				IsLineMarked(
							uint32			inLine) const;

	uint32				GetLineIndent(
							uint32			inLine) const;

	uint32				GetLineIndentWidth(
							uint32			inLine) const;

	uint32				GetIndent(
							uint32			inOffset) const;
	
	MLanguage*			GetLanguage() const					{ return mLanguage; }

	void				SetLanguage(
							const std::string&
											inLanguage);

	EOLNKind			GetEOLNKind() const					{ return mText.GetEOLNKind(); }

	MEncoding			GetEncoding() const					{ return mText.GetEncoding(); }

	uint32				GetCharsPerTab() const				{ return mCharsPerTab; }

	void				SetCharsPerTab(
							uint32			inCharsPerTab);

	void				SetDocInfo(
							const std::string&
											inLanguage,
							EOLNKind		inEOLNKind,
							MEncoding		inEncoding,
							uint32			inCharsPerTab);
	
	void				SetSoftwrap(
							bool			inSoftwrap);

	bool				GetSoftwrap() const;

	bool				GetShowWhiteSpace() const			{ return mShowWhiteSpace; }

	void				HandleFindDialogCommand(
							uint32			inCommand);

	void				FindAll(
							std::string		inWhat,
							bool			inIgnoreCase, 
							bool			inRegex,
							bool			inSelection,
							MMessageList&	outHits);

						// this one is for the MFindDialog
	static void			FindAll(
							const fs::path&	inPath,
							const std::string&
											inWhat,
							bool			inIgnoreCase, 
							bool			inRegex,
							bool			inSelection,
							MMessageList&	outHits);

	void				FindWord(
							uint32			inOffset,
							uint32&			outMinAnchor,
							uint32&			outMaxAnchor);

	void				MarkLine(
							uint32			inLineNr,
							bool			inMark = true);

	void				MarkMatching(
							const std::string&
											inText,
							bool			inIgnoreCase,
							bool			inRegEx);

	void				ClearAllMarkers();

	void				CCCMarkedLines(
							bool			inCopy,
							bool			inClear);
	
	void				ClearBreakpointsAndStatements();

	void				SetIsStatement(
							uint32			inLine,
							bool			inIsStatement = true);

	bool				IsStatement(
							uint32			inLine) const;

	void				SetIsBreakpoint(
							uint32			inLine,
							bool			inIsBreakpoint = true);

	bool				IsBreakpoint(
							uint32			inLine) const;

	void				SetPCLine(
							uint32			inLine);

	uint32				GetPCLine() const					{ return mPCLine; }
	
	void				ClearDiffs();
	
	void				TouchLine(
							uint32			inLineNr,
							bool			inDirty = true);

	void				TouchAllLines();

	void				DeleteSelectedText();

	void				ReplaceSelectedText(
							const std::string&
											inText,
							bool			isBlock,
							bool			inSelectPastedText);
	
	void				Type(
							const char*		inText,
							uint32			inLength);
							
	void				Drop(
							uint32			inOffset,
							const char*		inText,
							uint32			inSize,
							bool			inDragMove);

	uint32				CountLines() const					{ return mLineInfo.size(); }

	uint32				GetTextSize() const					{ return mText.GetSize(); }
	
	const MTextInputAreaInfo&
						GetTextInputAreaInfo() const		{ return mTextInputAreaInfo; }

	void				OnKeyPressEvent(
							GdkEventKey*	inEvent);
	
	void				OnCommit(
							const char*		inText,
							uint32			inLength);

	MEventOut<void()>					eLineCountChanged;
	MEventOut<void(MSelection,std::string)>	eSelectionChanged;
	MEventOut<void()>					eInvalidateDirtyLines;
	MEventOut<void(uint32,int32)>		eShiftLines;
	MEventOut<void(MScrollMessage)>		eScroll;
	MEventOut<void(bool)>				eShellStatus;

	MEventIn<void()>					eBoundsChanged;
	MEventIn<void()>					ePrefsChanged;
	
	MEventIn<void(MWindow*)>			eMsgWindowClosed;
	MEventIn<void(double)>				eIdle;

	// the actions MDocument can perform

	void				DoUndo();
	void				DoRedo();
	void				DoBalance();
	void				DoShiftLeft();
	void				DoShiftRight();
	void				DoComment();
	void				DoUncomment();
	void				DoEntab();
	void				DoDetab();
	void				DoCut(bool inAppend);
	void				DoCopy(bool inAppend);
	void				DoPaste();
	void				DoPasteNext();
	void				DoClear();
	void				DoMarkLine();
	void				DoJumpToNextMark(MDirection inDirection);
	void				DoReplace(bool inFindNext, MDirection inDirection);
	void				DoReplaceAll();
	void				DoComplete(MDirection inDirection);
	bool				DoFindFirst();
	bool				DoFindNext(MDirection inDirection);
	void				DoFastFind(MDirection inDirection);
	void				DoSoftwrap();
	void				DoApplyScript(const std::string& inScript);

	bool				CanUndo(std::string& outAction);
	bool				CanRedo(std::string& outAction);

	void				ShellStatusIn(bool inActive);
	void				StdOut(const char* inText, uint32 inSize);
	void				StdErr(const char* inText, uint32 inSize);
	void				MsgWindowClosed(MWindow* inWindow);
	
	bool				CanReplace();

	bool				GetParsePopupItems(MMenu& inMenu);
	void				SelectParsePopupItem(uint32 inItem);

	bool				GetIncludePopupItems(MMenu& inMenu);
	void				SelectIncludePopupItem(uint32 inItem);

  private:

						MTextDocument();

	void				Init();
	void				ReInit();

	void				Insert(
							uint32			inOffset,
							const char*		inText,
							uint32			inLength);

	void				Insert(
							uint32			inOffset,
							const std::string&
											inText);

	void				Delete(
							uint32			inOffset,
							uint32			inLength);

	uint32				LineColumnToOffsetBreakingTabs(
							uint32			inLine,
							uint32			inColumn,
							bool			inAddSpacesExtendingLine);

	uint32				FindLineBreak(
							uint32			inFromOffset,
							uint16			inState,
							uint32			inIndent) const;

	int32				RewrapLines(
							uint32			inFrom,
							uint32			inTo);

	void				Rewrap();

	void				RestyleDirtyLines(
							uint32			inFromLine);

	void				UpdateDirtyLines();

	void				SendSelectionChangedEvent();
	
	bool				HandleRawKeydown(
							uint32			inKeyValue,
							uint32			inModifiers);

	bool				HandleKeyCommand(
							MKeyCommand		inKeyCommand);

	void				HandleDeleteKey(
							MDirection		inDirection);

	uint32				FindWord(
							uint32			inFromOffset,
							MDirection		inDirection);

	void				RepairAfterUndo(
							uint32			inOffset,
							uint32			inLength,
							int32			inDelta);

	void				ChangeSelection(
							MSelection		inSelection);
							
	void				ChangeSelection(
							uint32			inAnchor,
							uint32			inCaret);

	void				JumpToNextMark(
							int				inDirection);

	bool				FastFind(
							MDirection		inDirection);

	void				FastFindType(
							const char*		inText,
							uint32			inTextLength);
	
	void				Complete(
							int				inDirection);
	
	void				BoundsChanged();

	void				PrefsChanged();
	
	void				Execute();

	void				Idle(
							double			inSystemTime);
	
	int							mDataFD;
	MTextBuffer					mText;
	MTextView*					mTargetTextView;
	uint32						mWrapWidth;
	MLineInfoArray				mLineInfo;
	std::string					mFont;
	uint32						mLineHeight;
	uint32						mCharWidth;	// to be able to calculate tab widths
	uint32						mTabWidth;
	uint32						mCharsPerTab;
	MSelection					mSelection;
	int32						mWalkOffset;
	std::string					mLastAction;
	std::string					mCurrentAction;
	MLanguage*					mLanguage;
	MNamedRange*				mNamedRange;
	MIncludeFileList*			mIncludeFiles;
	bool						mNeedReparse;
	bool						mSoftwrap;
	bool						mShowWhiteSpace;
	bool						mFastFindMode;
	MDirection					mFastFindDirection;
	bool						mFastFindInited;
	uint32						mFastFindStartOffset;
	std::string					mFastFindWhat;
	std::vector<std::string>	mCompletionStrings;
	int32						mCompletionIndex;
	uint32						mCompletionStartOffset;
	MTextInputAreaInfo			mTextInputAreaInfo;
	std::auto_ptr<MShell>		mShell;
	MMessageWindow*				mStdErrWindow;
	bool						mStdErrWindowSelected;
	bool						mPreparedForStdOut;
	uint32						mPCLine;
	
	std::auto_ptr<MSftpChannel>	mSFTPChannel;
	std::string					mSFTPData;
	uint32						mSFTPSize, mSFTPOffset;
	
	static MTextDocument*		sWorksheet;
};

inline
uint32 MTextDocument::LineStart(
	uint32				inLine) const
{
	uint32 result = mText.GetSize();
	if (inLine < mLineInfo.size())
		result = mLineInfo[inLine].start;
	return result;
}

inline
uint32 MTextDocument::LineEnd(
	uint32				inLine) const
{
	uint32 result = mText.GetSize();
	if (inLine + 1 < mLineInfo.size())
		result = mLineInfo[inLine + 1].start - 1;
	return result;
}

inline
bool MTextDocument::IsLineDirty(
	uint32				inLine) const
{
	bool result = false;
	if (inLine < mLineInfo.size())
		result = mLineInfo[inLine].dirty;
	return result;
}

inline
bool MTextDocument::IsLineMarked(
	uint32				inLine) const
{
	bool result = false;
	if (inLine < mLineInfo.size())
		result = mLineInfo[inLine].marked;
	return result;
}

inline
bool MTextDocument::IsStatement(
	uint32				inLine) const
{
	bool result = false;
	if (inLine < mLineInfo.size())
		result = mLineInfo[inLine].stmt;
	return result;
}

inline
bool MTextDocument::IsBreakpoint(
	uint32				inLine) const
{
	bool result = false;
	if (inLine < mLineInfo.size())
		result = mLineInfo[inLine].brkp;
	return result;
}

inline
void MTextDocument::Insert(
	uint32				inOffset,
	const std::string&	inText)
{
	Insert(inOffset, inText.c_str(), inText.length());
}

inline
void MTextDocument::ChangeSelection(
	uint32				inAnchor,
	uint32				inCaret)
{
	ChangeSelection(MSelection(this, inAnchor, inCaret));
}

inline
void MTextDocument::SelectAll()
{
	Select(0, mText.GetSize());
}

#endif
