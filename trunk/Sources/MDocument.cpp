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

#include "MJapieG.h"

#include <limits>
#include <cmath>
#include <iostream>

#include <gdk/gdkkeysyms.h>

#include <boost/functional/hash.hpp>

#include "MDocument.h"
#include "MTextView.h"
#include "MClipboard.h"
#include "MUnicode.h"
#include "MGlobals.h"
#include "MLanguage.h"
//#include "MApplication.h"
#include "MFindDialog.h"
#include "MError.h"
#include "MController.h"
#include "MStyles.h"
#include "MPreferences.h"
//#include "MPrefsDialog.h"
#include "MShell.h"
#include "MMessageWindow.h"
#include "MDevice.h"
#include "MCommands.h"
#include "MDocWindow.h"
#include "MUtils.h"
#include "MMenu.h"
#include "MProject.h"
#include "MDevice.h"
#include "MDocClosedNotifier.h"
#include "MSftpPutDialog.h"
#include "MStrings.h"
#include "MAcceleratorTable.h"

using namespace std;

// ---------------------------------------------------------------------------

namespace
{

const char
	kNoAction[]			= "None",
	kTypeAction[]		= "Type",
	kPasteAction[]		= "Paste",
	kReplaceAction[]	= "Replace",
	kDropAction[]		= "Drag",
	kCutAction[]		= "Cut";

const char
	kJapieDocState[] = "com.hekkelman.japie.State",
	kJapieCWD[] = "com.hekkelman.japie.CWD";

const uint32
	kMDocStateSize = 44;	// sizeof(MDocState)

}

MDocument* MDocument::sFirst;

// ---------------------------------------------------------------------------
//	MDocState

void MDocState::Swap()
{
//#if __LITTLE_ENDIAN__
//	mSelection[0] = Endian32_Swap(mSelection[0]);
//	mSelection[1] = Endian32_Swap(mSelection[1]);
//	mSelection[2] = Endian32_Swap(mSelection[2]);
//	mSelection[3] = Endian32_Swap(mSelection[3]);
//	mScrollPosition[0] = Endian32_Swap(mScrollPosition[0]);
//	mScrollPosition[1] = Endian32_Swap(mScrollPosition[1]);
//	mWindowPosition[0] = Endian16_Swap(mWindowPosition[0]);
//	mWindowPosition[1] = Endian16_Swap(mWindowPosition[1]);
//	mWindowSize[0] = Endian16_Swap(mWindowSize[0]);
//	mWindowSize[1] = Endian16_Swap(mWindowSize[1]);
//	mSwapHelper = Endian32_Swap(mSwapHelper);
//#endif
}

// ---------------------------------------------------------------------------
//	MDocument

MDocument::MDocument(
	const MUrl*			inURL)
	: eBoundsChanged(this, &MDocument::BoundsChanged)
	, ePrefsChanged(this, &MDocument::PrefsChanged)
	, eShellStatusIn(this, &MDocument::ShellStatusIn)
	, eStdOut(this, &MDocument::StdOut)
	, eStdErr(this, &MDocument::StdErr)
	, eMsgWindowClosed(this, &MDocument::MsgWindowClosed)
	, eIdle(this, &MDocument::Idle)
	, eNotifyPut(this, &MDocument::NotifyPut)
{
	Init();
	
	if (inURL != nil)
	{
		mURL = *inURL;
		
		if (mURL.IsLocal() and fs::exists(mURL.GetPath()))
			mFileModDate = fs::last_write_time(mURL.GetPath());
		else
			mFileModDate = GetLocalTime();

		mSpecified = true;
	}

//	AddRoute(ePrefsChanged, MStyleArray::Instance().eStylesChanged);
//	AddRoute(ePrefsChanged, MPrefsDialog::ePrefsChanged);
	AddRoute(eIdle, gApp->eIdle);
	
	ReInit();
	
	if (mSpecified and mURL.IsLocal() and fs::exists(mURL.GetPath()))
	{
		ReadFile();
		Rewrap();
	}

//	boost::mutex::scoped_lock lock(sDocListMutex);
	mNext = sFirst;
	sFirst = this;
}

MDocument::MDocument(
	const string&	inText,
	const string&	inFileNameHint)
	: eBoundsChanged(this, &MDocument::BoundsChanged)
	, ePrefsChanged(this, &MDocument::PrefsChanged)
	, eShellStatusIn(this, &MDocument::ShellStatusIn)
	, eStdOut(this, &MDocument::StdOut)
	, eStdErr(this, &MDocument::StdErr)
	, eMsgWindowClosed(this, &MDocument::MsgWindowClosed)
	, eIdle(this, &MDocument::Idle)
	, eNotifyPut(this, &MDocument::NotifyPut)
	, mURL(MUrl("file:///tmp") / inFileNameHint)
	, mText(inText)
{
	Init();
	
//	AddRoute(ePrefsChanged, MStyleArray::Instance().eStylesChanged);
//	AddRoute(ePrefsChanged, MPrefsDialog::ePrefsChanged);
	AddRoute(eIdle, gApp->eIdle);
	
	ReInit();

	mLanguage = MLanguage::GetLanguageForDocument(mURL.GetFileName(), mText);
	if (mLanguage != nil)
	{
		mNamedRange = new MNamedRange;
		mIncludeFiles = new MIncludeFileList;
	}
	
	Rewrap();

//	boost::mutex::scoped_lock lock(sDocListMutex);
	mNext = sFirst;
	sFirst = this;
}

MDocument::MDocument(
	const MUrl&		inFile)
	: eBoundsChanged(this, &MDocument::BoundsChanged)
	, ePrefsChanged(this, &MDocument::PrefsChanged)
	, eShellStatusIn(this, &MDocument::ShellStatusIn)
	, eStdOut(this, &MDocument::StdOut)
	, eStdErr(this, &MDocument::StdErr)
	, eMsgWindowClosed(this, &MDocument::MsgWindowClosed)
	, eIdle(this, &MDocument::Idle)
	, eNotifyPut(this, &MDocument::NotifyPut)

	, mURL(inFile)
{
	Init();

	mSpecified = true;

//	ReInit();

	if (mURL.IsLocal() and fs::exists(mURL.GetPath()))
	{
		ReadFile();
		Rewrap();
	}

//	boost::mutex::scoped_lock lock(sDocListMutex);
	mNext = sFirst;
	sFirst = this;
}

MDocument::MDocument(
	const MUrl&		inFile,
	const string&	inText)
	: eBoundsChanged(this, &MDocument::BoundsChanged)
	, ePrefsChanged(this, &MDocument::PrefsChanged)
	, eShellStatusIn(this, &MDocument::ShellStatusIn)
	, eStdOut(this, &MDocument::StdOut)
	, eStdErr(this, &MDocument::StdErr)
	, eMsgWindowClosed(this, &MDocument::MsgWindowClosed)
	, eIdle(this, &MDocument::Idle)
	, eNotifyPut(this, &MDocument::NotifyPut)
	, mURL(inFile)
	, mText(inText)
{
	Init();

	mSpecified = true;

	mLanguage = MLanguage::GetLanguageForDocument(mURL.GetFileName(), mText);
	if (mLanguage != nil)
	{
		mNamedRange = new MNamedRange;
		mIncludeFiles = new MIncludeFileList;
	}

	ReInit();
	Rewrap();

//	boost::mutex::scoped_lock lock(sDocListMutex);
	mNext = sFirst;
	sFirst = this;
}

void MDocument::Init()
{
	mNext = nil;
	mSpecified = false;
	mTargetTextView = nil;
	mWalkOffset = 0;
	mLastAction = kNoAction;
	mCurrentAction = kNoAction;
	mLanguage = nil;
	mNamedRange = nil;
	mIncludeFiles = nil;
	mDirty = false;
	mNeedReparse = false;
	mSoftwrap = false;
	mFastFindMode = false;
	mCompletionIndex = -1;
	mStdErrWindow = nil;
	mIsWorksheet = false;
	mPCLine = numeric_limits<uint32>::max();
	mPutCount = 0;

	mCharsPerTab = gCharsPerTab;

	mLineInfo.push_back(MLineInfo());

	for (int i = kActiveInputArea; i <= kSelectedText; ++i)
		mTextInputAreaInfo.fOffset[i] = -1;
	
//	PRINT(("Document created %x", this));
}

MDocument::~MDocument()
{
//	PRINT(("Deleting document %x (%s)", this,
//		IsSpecified() ? mURL.str().c_str() : "unspecified"));
//	boost::mutex::scoped_lock lock(sDocListMutex);

	if (sFirst == this)
		sFirst = mNext;
	else
	{
		MDocument* doc = sFirst;
		while (doc != nil)
		{
			MDocument* next = doc->mNext;
			if (next == this)
			{
				doc->mNext = mNext;
				break;
			}
			doc = next;
		}
	}
	
	delete mNamedRange;
	delete mIncludeFiles;
	
	eDocumentClosed();
}

void MDocument::AddController(MController* inController)
{
	if (find(mControllers.begin(), mControllers.end(), inController) == mControllers.end())
		mControllers.push_back(inController);
}

void MDocument::RemoveController(MController* inController)
{
	assert(find(mControllers.begin(), mControllers.end(), inController) != mControllers.end());
	
	mControllers.erase(
		remove(mControllers.begin(), mControllers.end(), inController),
		mControllers.end());

	if (mControllers.size() == 0)
		CloseDocument();
}

void MDocument::SetTargetTextView(MTextView* inTextView)
{
	if (inTextView != mTargetTextView)
	{
		if (mTargetTextView != nil)
			RemoveRoute(eBoundsChanged, mTargetTextView->eBoundsChanged);
		
		mTargetTextView = inTextView;
		
		AddRoute(eBoundsChanged, inTextView->eBoundsChanged);

		BoundsChanged();
	}
}

void MDocument::SetWorksheet(bool inIsWorksheet)
{
	mIsWorksheet = inIsWorksheet;
	
	if (inIsWorksheet)
	{
		string cwd = Preferences::GetString("worksheet wd", "");
		if (cwd.length() > 0)
		{
			mShell.reset(new MShell(true));

			mShell->SetCWD(cwd);

			AddRoute(mShell->eStdOut, eStdOut);
			AddRoute(mShell->eStdErr, eStdErr);
			AddRoute(mShell->eShellStatus, eShellStatusIn);
			
		}
	}		
}

const char* MDocument::GetCWD() const
{
	const char* result = nil;
	if (mShell.get() != nil)
		result = mShell->GetCWD().c_str();
	else if (mSpecified)
	{
		static auto_array<char> cwd(new char[PATH_MAX]);

		int32 r = read_attribute(mURL.GetPath(), kJapieCWD, cwd.get(), PATH_MAX);
		if (r > 0 and r < PATH_MAX)
		{
			cwd.get()[r] = 0;
			result = cwd.get();
		}
	}
	return result;
}

void MDocument::Reset()
{
	if (mCurrentAction != kNoAction)
		FinishAction();

	mCompletionIndex = -1;
	mFastFindMode = false;
	
	for (int i = kActiveInputArea; i <= kSelectedText; ++i)
		mTextInputAreaInfo.fOffset[i] = -1;
}

MDocument* MDocument::GetDocumentForURL(
	const MUrl&		inFile,
	bool			inCreateIfNeeded)
{
//	boost::mutex::scoped_lock lock(sDocListMutex);
	
	MDocument* doc = sFirst;

	while (doc != nil and not doc->UsesFile(inFile))
		doc = doc->mNext;

	if (doc == nil and inCreateIfNeeded)
		doc = new MDocument(&inFile);

	return doc;
}

void MDocument::CheckFile()
{
//	Boolean changed;
//	FSRef newRef;
//	
//	if (mAlias != nil and
//		::FSResolveAliasWithMountFlags(nil, mAlias, &newRef,
//			&changed, kARMSearch | kARMNoUI) == noErr and
//		changed)
//	{
//		(void)::FSRefMakePath(newRef, mURL);
//		eFileSpecChanged(mURL);
//	}
}

void MDocument::MakeFirstDocument()
{
//	boost::mutex::scoped_lock lock(sDocListMutex);

	MDocument* d = sFirst;
	
	if (d != this)
	{
		while (d != nil and d->mNext != this)
			d = d->mNext;
		
		assert(d->mNext == this);
		d->mNext = mNext;
		mNext = sFirst;
		sFirst = this;
	}
}

// ---------------------------------------------------------------------------
//	ReInit, reset all font related member values

void MDocument::ReInit()
{
#pragma message("zet mLayout opties hier")
	
	mFont = Preferences::GetString("font", "monospace 9");
	
	MDevice device;
	
	device.SetFont(mFont);

	mLineHeight = device.GetLineHeight();
	mCharWidth = device.GetStringWidth("          ") / 10;
	mTabWidth = mCharWidth * mCharsPerTab;
}

// ---------------------------------------------------------------------------
//	SetLanguage

void MDocument::SetLanguage(
	const string&		inLanguage)
{
	mLanguage = MLanguage::GetLanguage(inLanguage);

	ReInit();
	Rewrap();
	UpdateDirtyLines();
}

// ---------------------------------------------------------------------------
//	SetCharsPerTab

void MDocument::SetCharsPerTab(
	uint32				inCharsPerTab)
{
	mCharsPerTab = inCharsPerTab;
	
	ReInit();
	Rewrap();
	UpdateDirtyLines();
}

// ---------------------------------------------------------------------------
//	SetDocInfo

void MDocument::SetDocInfo(
	const std::string&	inLanguage,
	EOLNKind			inEOLNKind,
	MEncoding			inEncoding,
	uint32				inCharsPerTab)
{
	mCharsPerTab = inCharsPerTab;
	mText.SetEncoding(inEncoding);
	mText.SetEOLNKind(inEOLNKind);
	
	ReInit();
	Rewrap();
	UpdateDirtyLines();
}

// ---------------------------------------------------------------------------
//	IsModified

bool MDocument::IsModified() const
{
	return mDirty;
}

// ---------------------------------------------------------------------------
//	SetModified

void MDocument::SetModified(bool inModified)
{
	if (inModified)
		mNeedReparse = true;

	if (inModified != mDirty)
	{
		mDirty = inModified;
		eModifiedChanged(mDirty);
	}
}

// ---------------------------------------------------------------------------
//	IsSpecified

bool MDocument::IsSpecified() const
{
	return mSpecified;
}


// ---------------------------------------------------------------------------
//	UsesFile

bool MDocument::UsesFile(
	const MUrl&	inFileRef) const
{
//	CheckFile();
	return mSpecified and mURL == inFileRef;
}

bool MDocument::ReadDocState(MDocState& ioDocState)
{
	bool result = false;
	
	if (IsSpecified())
	{
		ssize_t r = read_attribute(mURL.GetPath(), kJapieDocState, &ioDocState, kMDocStateSize);
		if (r > 0 and static_cast<uint32>(r) == kMDocStateSize)
		{
			ioDocState.Swap();

			if (ioDocState.mFlags.mSelectionIsBlock)
				mSelection.Set(ioDocState.mSelection[0], ioDocState.mSelection[1],
					ioDocState.mSelection[2], ioDocState.mSelection[3]);
			else
				mSelection.Set(ioDocState.mSelection[0], ioDocState.mSelection[1]);
		
			mSoftwrap = ioDocState.mFlags.mSoftwrap;
			
			if (mCharsPerTab != ioDocState.mFlags.mTabWidth and
				ioDocState.mFlags.mTabWidth != 0)
			{
				mCharsPerTab = ioDocState.mFlags.mTabWidth;
				ReInit();
				Rewrap();
			}

			result = true;
		}
	}
	
	return result;
}

uint32 MDocument::FindWord(
	uint32		inFromOffset,
	MDirection	inDirection)
{
	if (inFromOffset <= 1 and inDirection == kDirectionBackward)
		return 0;

	if (inFromOffset + 1 >= mText.GetSize() and inDirection == kDirectionForward)
		return mText.GetSize();

	uint32 line = OffsetToLine(inFromOffset);

	if (line > 0 and
		inFromOffset == LineStart(line) and
		inDirection == kDirectionBackward)
	{
		--line;
		--inFromOffset;
	}
	else if (line + 1 < mLineInfo.size() and
		inFromOffset == LineStart(line + 1) - 1 and
		inDirection == kDirectionForward)
	{
		++line;
		++inFromOffset;
	}
	
	uint32 result;
	
	if (inDirection == kDirectionForward)
		result = mText.NextCursorPosition(inFromOffset, eMoveOneWordForKeyboard);
	else
		result = mText.PreviousCursorPosition(inFromOffset, eMoveOneWordForKeyboard);
	
	return result;
}

void MDocument::FindWord(uint32 inOffset, uint32& outMinAnchor, uint32& outMaxAnchor)
{
	outMaxAnchor = mText.NextCursorPosition(inOffset, eMoveOneWordForKeyboard);
	outMinAnchor = mText.PreviousCursorPosition(outMaxAnchor, eMoveOneWordForKeyboard);
	
	if (outMinAnchor > inOffset or outMaxAnchor < inOffset)
		outMinAnchor = outMaxAnchor = inOffset;
}

// ---------------------------------------------------------------------------
//	HandleDeleteKey

void MDocument::HandleDeleteKey(MDirection inDirection)
{
//	FinishAction();
	StartAction(kTypeAction);
	
	if (mSelection.IsEmpty())
	{
		uint32 caret = mSelection.GetCaret();
		uint32 anchor = caret;
		
		if ((caret > 0 and inDirection == kDirectionBackward) or
			(caret + 1 < mText.GetSize() and inDirection == kDirectionForward))
		{
			if (inDirection == kDirectionForward)
				caret = mText.NextCursorPosition(caret, eMoveOneCharacter);
			else
			{
				bool space = false;
				
				if (gTabEntersSpaces)
				{
					uint32 line = OffsetToLine(caret);
					uint32 start = LineStart(line);
					uint32 mark = start;
					uint32 column = 0;
					
					MTextBuffer::iterator i(&mText, start);
					
					while (i != mText.end() and i.GetOffset() < caret)
					{
						switch (*i)
						{
							case ' ':
								if ((column++ % gSpacesPerTab) == 0)
								{
									mark = i.GetOffset();
									space = true;
								}
								break;
							
							case '\t':
								column = ((column / gSpacesPerTab) + 1) * gSpacesPerTab;
								mark = i.GetOffset();
								space = true;
								break;
							
							default:
							    ++column;
								space = false;
								break;
						}
	
						++i;
					}
					
					if (space)
						caret = mark;
					else
						caret = mText.PreviousCursorPosition(caret, eMoveOneCharacter);
				}
				else
					caret = mText.PreviousCursorPosition(caret, eMoveOneCharacter);
			}
		}
		
		if (anchor > caret)
			swap(anchor, caret);
	
		if (anchor != caret)
			Delete(anchor, caret - anchor);
	
		ChangeSelection(MSelection(anchor, anchor));
	}
	else
		DeleteSelectedText();

	uint32 line;
	OffsetToPosition(mSelection.GetCaret(), line, mWalkOffset);
	SendSelectionChangedEvent();
}

// ---------------------------------------------------------------------------
// DeleteSelectedText

void MDocument::DeleteSelectedText()
{
	if (mSelection.IsBlock())
	{
		
	}
	else
	{
		uint32 anchor = mSelection.GetAnchor();
		uint32 caret = mSelection.GetCaret();
		
		if (anchor > caret)
			swap(anchor, caret);
	
		if (anchor != caret)
			Delete(anchor, caret - anchor);
	
		ChangeSelection(MSelection(anchor, anchor));
	}
}

// ---------------------------------------------------------------------------
// ReplaceSelectedText

void MDocument::ReplaceSelectedText(
	const string&		inText)
{
	if (mSelection.IsBlock())
	{
		
	}
	else
	{
		uint32 anchor = mSelection.GetAnchor();
		uint32 caret = mSelection.GetCaret();
		
		if (anchor > caret)
			swap(anchor, caret);
	
		if (anchor != caret)
			Delete(anchor, caret - anchor);
	
		Insert(anchor, inText);
		ChangeSelection(MSelection(anchor, anchor + inText.length()));
	}
}

// ---------------------------------------------------------------------------
// Type

void MDocument::Type(
	const char*		inText,
	uint32			inLength)
{
	if (mTargetTextView != nil)
		mTargetTextView->ObscureCursor();
	
	StartAction(kTypeAction);
	
	if (not mSelection.IsEmpty())
		DeleteSelectedText();

	uint32 offset = mSelection.GetCaret();

	if (gTabEntersSpaces and *inText == '\t' and inLength == 1)
	{
		uint32 column = OffsetToColumn(offset);
		inLength = gSpacesPerTab - (column % gSpacesPerTab);
		string t(inLength, ' ');
		Insert(offset, t.c_str(), inLength);
	}
	else
		Insert(offset, inText, inLength);

	for (int i = kActiveInputArea; i <= kSelectedText; ++i)
		mTextInputAreaInfo.fOffset[i] = -1;

	MSelection s = mText.GetSelectionAfter();
	if (s.GetCaret() == offset)
		s.SetCaret(offset + inLength);
	mText.SetSelectionAfter(s);
	ChangeSelection(MSelection(offset + inLength, offset + inLength));

	if (gKiss and
		inLength == 1 and
		mLanguage and
		mLanguage->IsBalanceChar(*inText))
	{
		uint32 length = 0;
		
		if (mLanguage->Balance(mText, offset, length) and
			offset > 0 and
			offset + length == mSelection.GetCaret() - 1)
		{
			MSelection save(mSelection);
			ChangeSelection(MSelection(offset - 1, offset));
			eScroll(kScrollForKiss);
			usleep(250000UL);
			ChangeSelection(save);
			eScroll(kScrollReturnAfterKiss);
		}
	}

	if (gSmartIndent and
		inLength == 1 and
		mLanguage and
		mLanguage->IsSmartIndentCloseChar(*inText))
	{
		offset = mSelection.GetCaret() - 1;
		uint32 length = 0;

		uint32 closeLine = OffsetToLine(offset);
		uint32 openLine;
		
		if (mLanguage->Balance(mText, offset, length) and
			(openLine = OffsetToLine(offset)) < closeLine)
		{
			string s;
			MTextBuffer::iterator txt = mText.begin() + LineStart(openLine);
			
			while (txt.GetOffset() < offset and
				(*txt == ' ' or *txt == '\t'))
			{
				s += *txt;
				++txt;
			}

			offset = mSelection.GetCaret() - 1;
			txt = mText.begin() + LineStart(closeLine);
			while (txt.GetOffset() < offset and
				(*txt == ' ' or *txt == '\t'))
			{
				++txt;
			}
			
			if (txt.GetOffset() == offset)
			{
				Delete(LineStart(closeLine), offset - LineStart(closeLine));
				Insert(LineStart(closeLine), s.c_str(), s.length());
			}
			else
			{
				const char kLF = '\n';
				Insert(offset, &kLF, 1);
				Insert(offset + 1, s.c_str(), s.length());
			}
		}
	}

	uint32 line;
	OffsetToPosition(offset + inLength, line, mWalkOffset);
	
	SendSelectionChangedEvent();
	eScroll(kScrollToCaret);
}

// ---------------------------------------------------------------------------
// Drop

void MDocument::Drop(
	uint32			inOffset,
	const char*		inText,
	uint32			inSize,
	bool			inDragMove)
{
	StartAction(kDropAction);
	
	if (inDragMove)
	{
		uint32 start = mSelection.GetMinOffset(*this);
		uint32 length = mSelection.GetMaxOffset(*this) - start;
		
		Delete(start, length);
		if (inOffset > mSelection.GetMaxOffset(*this))
			inOffset -= length;
	}
	
	Insert(inOffset, inText, inSize);
	ChangeSelection(inOffset, inOffset + inSize);

	FinishAction();

	SendSelectionChangedEvent();
}

// ---------------------------------------------------------------------------
//	CloseDocument

void MDocument::CloseDocument()
{
	try
	{
		if (mSpecified)
		{
			MDocState state = { };

			(void)read_attribute(mURL.GetPath(), kJapieDocState, &state, kMDocStateSize);
			
			state.Swap();
			
			if (mSelection.IsBlock())
			{
				mSelection.GetAnchorLineAndColumn(*this,
					state.mSelection[0], state.mSelection[1]);
				mSelection.GetCaretLineAndColumn(*this,
					state.mSelection[2], state.mSelection[3]);
				state.mFlags.mSelectionIsBlock = true;
			}
			else
			{
				state.mSelection[0] = mSelection.GetAnchor();
				state.mSelection[1] = mSelection.GetCaret();
			}
			
			if (mTargetTextView != nil)
			{
				int32 x, y;
				mTargetTextView->GetScrollPosition(x, y);
				state.mScrollPosition[0] = static_cast<uint32>(x);
				state.mScrollPosition[1] = static_cast<uint32>(y);
			}

			if (MWindow* w = MDocWindow::FindWindowForDocument(this))
			{
				MRect r;
				w->GetWindowPosition(r);
				state.mWindowPosition[0] = r.x;
				state.mWindowPosition[1] = r.y;
				state.mWindowSize[0] = r.width;
				state.mWindowSize[1] = r.height;
			}

			state.mFlags.mSoftwrap = mSoftwrap;
			state.mFlags.mTabWidth = mCharsPerTab;
			
			state.Swap();
			
			write_attribute(mURL.GetPath(), kJapieDocState, &state, kMDocStateSize);
			
			if (mShell.get() != nil)
			{
				string cwd = mShell->GetCWD();
				
				write_attribute(mURL.GetPath(), kJapieCWD, cwd.c_str(), cwd.length());
					
				if (mIsWorksheet)
					Preferences::SetString("worksheet wd", cwd);
			}
		}
		
		eDocumentClosed();
	}
	catch (...) {}
	
	delete this;
}

// ---------------------------------------------------------------------------
//	RevertDocument

void MDocument::RevertDocument()
{
	ReadFile();
}

bool MDocument::DoSave()
{
	bool result = false;
	
	CheckFile();	// make sure our filespec is valid
	
	MUrl file = mURL;
	bool specified = mSpecified;
	
	try
	{
		try
		{
			if (Preferences::GetInteger("force newline at eof", 1) == 1 and
				mText.GetChar(mText.GetSize() - 1) != '\n')
			{
				StartAction(kTypeAction);
				Insert(mText.GetSize(), "\n", 1);
				FinishAction();
			}
		}
		catch (...) {}
		
		if (mURL.IsLocal())
		{
			MFile file(mURL.GetPath());
			file.Open(O_RDWR | O_TRUNC | O_CREAT);
			mText.WriteToFile(file);

//		MSafeSaver safe(mURL.GetPath());
//		mText.WriteToFile(*safe.GetTempFile());
//		safe.Commit(mURL.GetPath());

			MProject::RecheckFiles();
		}
		else
		{
			MSftpPutDialog* dlog = MDialog::Create<MSftpPutDialog>();
			dlog->Initialize(this);
			AddRoute(eNotifyPut, dlog->eNotifyPut);
		}
		
		SetModified(false);
		gApp->AddToRecentMenu(mURL);
		
		eFileSpecChanged(mURL);
		
		result = true;
	}
	catch (exception& inErr)
	{
		MError::DisplayError(inErr);
		
		mURL = file;
		mSpecified = specified;
		result = false;
	}
	
	return result;
}

bool MDocument::DoSaveAs(
	const MUrl&		inFile)
{
	bool result = false;
	
	mURL = inFile;

	if (DoSave())
	{
		mSpecified = true;
		mIsWorksheet = false;
	
		eFileSpecChanged(mURL);
		
		if (mLanguage == nil)
		{
			mLanguage = MLanguage::GetLanguageForDocument(mURL.GetFileName(), mText);
	
			if (mLanguage != nil)
			{
				if (mNamedRange == nil)
					mNamedRange = new MNamedRange;
				
				if (mIncludeFiles == nil)
					mIncludeFiles = new MIncludeFileList;

				TouchAllLines();
				UpdateDirtyLines();
			}
		}
		
		result = true;
	}
	
	return result;
}

// ---------------------------------------------------------------------------
//	ReadFile, the hard work

void MDocument::ReadFile()
{
	auto_ptr<MFile> file(new MFile(mURL.GetPath()));
	
	mText.ReadFromFile(*file);
	
	mLanguage = MLanguage::GetLanguageForDocument(mURL.GetFileName(), mText);
	
	if (mLanguage != nil)
	{
		mSoftwrap = mLanguage->Softwrap();

		if (mNamedRange == nil)
			mNamedRange = new MNamedRange;
		
		if (mIncludeFiles == nil)
			mIncludeFiles = new MIncludeFileList;
	}

	if (mTargetTextView)
		Rewrap();
}

// ---------------------------------------------------------------------------
//	MarkLine

void MDocument::MarkLine(
	uint32		inLineNr,
	bool		inMark)
{
	assert(inLineNr < mLineInfo.size());
	if (inLineNr < mLineInfo.size())
	{
		if (mLineInfo[inLineNr].marked != inMark)
			TouchLine(inLineNr);
		mLineInfo[inLineNr].marked = inMark;
	}
}

// ---------------------------------------------------------------------------
//	MarkMatching

void MDocument::MarkMatching(
	const string&	inMatch,
	bool			inIgnoreCase,
	bool			inRegEx)
{
	MSelection savedSelection(mSelection);
	
	MSelection found;
	uint32 offset = 0;
	
	while (mText.Find(offset, inMatch, kDirectionForward, inIgnoreCase, inRegEx, found))
	{
		uint32 line = found.GetMinLine(*this);

		mLineInfo[line].marked = true;
		mLineInfo[line].dirty = true;

		offset = LineStart(line + 1);
	}
	
	mSelection = savedSelection;
	
	eInvalidateDirtyLines();
}

// ---------------------------------------------------------------------------
//	ClearAllMarkers

void MDocument::ClearAllMarkers()
{
	for (uint32 lineNr = 0; lineNr < mLineInfo.size(); ++lineNr)
	{
		if (mLineInfo[lineNr].marked)
		{
			mLineInfo[lineNr].dirty = true;
			mLineInfo[lineNr].marked = false;
		}
	}
	
	eInvalidateDirtyLines();
}

// ---------------------------------------------------------------------------
//	ClearBreakpointsAndStatements

void MDocument::ClearBreakpointsAndStatements()
{
	for (uint32 lineNr = 0; lineNr < mLineInfo.size(); ++lineNr)
	{
		if (mLineInfo[lineNr].stmt or mLineInfo[lineNr].brkp)
		{
			mLineInfo[lineNr].dirty = true;
			mLineInfo[lineNr].stmt = false;
			mLineInfo[lineNr].brkp = false;
		}
	}
	
	eInvalidateDirtyLines();
}

// ---------------------------------------------------------------------------
//	SetIsStatement

void MDocument::SetIsStatement(
	uint32		inLineNr,
	bool		inIsStatement)
{
	--inLineNr;
	
	assert(inLineNr < mLineInfo.size());
	if (inLineNr < mLineInfo.size())
	{
		if (mLineInfo[inLineNr].stmt != inIsStatement)
			TouchLine(inLineNr);
		mLineInfo[inLineNr].stmt = inIsStatement;
	}
}

// ---------------------------------------------------------------------------
//	SetIsBreakpoint

void MDocument::SetIsBreakpoint(
	uint32		inLineNr,
	bool		inIsBreakpoint)
{
	--inLineNr;

	assert(inLineNr < mLineInfo.size());
	if (inLineNr < mLineInfo.size())
	{
		if (mLineInfo[inLineNr].stmt != inIsBreakpoint)
			TouchLine(inLineNr);
		mLineInfo[inLineNr].brkp = inIsBreakpoint;
	}
}

// ---------------------------------------------------------------------------
//	SetPCLine

void MDocument::SetPCLine(
	uint32		inLineNr)
{
	if (mPCLine < mLineInfo.size())
		TouchLine(mPCLine);
	
	if (inLineNr == 0)
		mPCLine = numeric_limits<uint32>::max();
	else
		mPCLine = inLineNr - 1;

	if (mPCLine < mLineInfo.size())
	{
		TouchLine(mPCLine);
		eScroll(kScrollToPC);
	}
}

// ---------------------------------------------------------------------------
//	CCCMarkedLines

void MDocument::CCCMarkedLines(bool inCopy, bool inClear)
{
	if (inCopy)
	{
		string text;
		bool first = true;

		for (uint32 lineNr = 0; lineNr < mLineInfo.size(); ++lineNr)
		{
			if (mLineInfo[lineNr].marked)
			{
				uint32 start = mLineInfo[lineNr].start;
				uint32 end = mText.GetSize();
				if (lineNr + 1 < mLineInfo.size())
					end = mLineInfo[lineNr + 1].start;
				uint32 length = end - start;
				
				mText.GetText(start, length, text);
				
				if (first)
					MClipboard::Instance().SetData(text, mSelection.IsBlock());
				else
					MClipboard::Instance().AddData(text);
				
				first = false;
			}
		}
	}
	
	if (inClear)
	{
		StartAction(kCutAction);
		
		uint32 offset = mSelection.GetMinOffset(*this);
		
		// watch out, unsigned integers in use
		for (uint32 lineNr = mLineInfo.size(); lineNr > 0; --lineNr)
		{
			if (mLineInfo[lineNr - 1].marked)
			{
				Delete(mLineInfo[lineNr - 1].start, mLineInfo[lineNr].start - mLineInfo[lineNr - 1].start);
				mLineInfo[lineNr - 1].marked = false;
				offset = mLineInfo[lineNr - 1].start;
			}
		}
		
		ChangeSelection(MSelection(offset, offset));
		
		FinishAction();
	}
}

// ---------------------------------------------------------------------------
//	ClearDiffs

void MDocument::ClearDiffs()
{
	for (uint32 lineNr = 0; lineNr < mLineInfo.size(); ++lineNr)
	{
		if (mLineInfo[lineNr].diff)
		{
			mLineInfo[lineNr].dirty = true;
			mLineInfo[lineNr].diff = false;
		}
	}
	
	eInvalidateDirtyLines();
}

// ---------------------------------------------------------------------------
//	JumpToNextMark

void MDocument::DoJumpToNextMark(MDirection inDirection)
{
	uint32 currentLine;
	if (inDirection == kDirectionForward)
		currentLine = OffsetToLine(mSelection.GetMaxOffset(*this));
	else
		currentLine = OffsetToLine(mSelection.GetMinOffset(*this));
	uint32 markLine = currentLine;
	uint32 line;

	if (inDirection == kDirectionForward)
	{
		for (line = currentLine; markLine == currentLine and line < mLineInfo.size(); ++line)
		{
			if (mLineInfo[line].marked)
				markLine = line;
		}
		
		for (line = 0; markLine == currentLine and line < currentLine; ++line)
		{
			if (mLineInfo[line].marked)
				markLine = line;
		}
	}
	else
	{
		for (line = currentLine; markLine == currentLine and line > 0; --line)
		{
			if (mLineInfo[line - 1].marked)
				markLine = line - 1;
		}
		
		for (line = mLineInfo.size(); markLine == currentLine and line > currentLine; --line)
		{
			if (mLineInfo[line - 1].marked)
				markLine = line - 1;
		}
	}

	if (markLine != currentLine)
		Select(LineStart(markLine), LineEnd(markLine), kScrollToSelection);
}

// ---------------------------------------------------------------------------
//	GoToLine

void MDocument::GoToLine(
	uint32	inLineNr)
{
	if (inLineNr < CountLines())
		Select(LineStart(inLineNr), LineStart(inLineNr + 1), kScrollToSelection);
}

// ---------------------------------------------------------------------------
//	TouchLine

void MDocument::TouchLine(
	uint32		inLineNr,
	bool		inDirty)
{
	assert(inLineNr < mLineInfo.size());
	if (inLineNr < mLineInfo.size())
		mLineInfo[inLineNr].dirty = inDirty;
}

// ---------------------------------------------------------------------------
//	TouchLine

void MDocument::TouchAllLines()
{
	for (uint32 line = 0; line < mLineInfo.size(); ++line)
		mLineInfo[line].dirty = true;
}

void MDocument::SetSoftwrap(bool inSoftwrap)
{
	if (mSoftwrap != inSoftwrap)
	{
		mSoftwrap = inSoftwrap;
		Rewrap();
	}
}

bool MDocument::GetSoftwrap() const
{
	return mSoftwrap and mTargetTextView != nil;
}

uint32 MDocument::GetIndent(uint32 inOffset) const
{
	uint32 indent = 0;

	MTextBuffer::const_iterator i(&mText, inOffset);
	
	uint32 maxWidth = numeric_limits<uint32>::max();
	if (mTargetTextView)
		maxWidth = mTargetTextView->GetWrapWidth();

	uint32 s = 0;
	while (i != mText.end())
	{
		if (*i == '\t')
		{
			++i;
			++indent;
			s = 0;
		}
		else if (*i == ' ')
		{
			++i;
			if (++s == mCharsPerTab)
			{
				s = 0;
				++indent;
			}
			else
				continue;
		}
		else
			break;

		if ((indent + 1) * mTabWidth >= maxWidth)
		{
			indent = 0;
			break;
		}
	}
	
	return indent;
}

uint32 MDocument::GetLineIndent(uint32 inLine) const
{
	// short cut
	if (not GetSoftwrap() or mLineInfo[inLine].nl or inLine == 0)
		return 0;
	
	while (inLine > 0 and not mLineInfo[inLine].nl)
		--inLine;
	
	return GetIndent(mLineInfo[inLine].start);
}

uint32 MDocument::GetLineIndentWidth(uint32 inLine) const
{
	return GetLineIndent(inLine) * mCharsPerTab * mCharWidth;
}

// ---------------------------------------------------------------------------
//	FindLineBreak

uint32 MDocument::FindLineBreak(
	uint32		inFromOffset,
	uint16		inState,
	uint32		inIndent) const
{
	assert(inFromOffset < mText.GetSize());

	MTextBuffer::const_iterator t = mText.begin() + inFromOffset;
	
	uint32 tabCount = 0;
	
	while (t != mText.end() and *t != '\n')
	{
		if (*t == '\t')
			++tabCount;
		++t;
	}
	
	uint32 result = (t - mText.begin()) + 1;
	
	if (GetSoftwrap() and mTargetTextView != nil and result > inFromOffset + 1)
	{
		uint32 width = mTargetTextView->GetWrapWidth();
		
		width -= inIndent * mCharsPerTab * mCharWidth;

		if (tabCount * mTabWidth > width)
		{
			tabCount = width / mTabWidth;
			result = inFromOffset + tabCount;
		}
		else
		{
			inFromOffset += tabCount;
			
			width -= tabCount * mTabWidth;
	
			uint32 l = result - inFromOffset;
			if (inFromOffset + l > mText.GetSize())
				l = mText.GetSize() - inFromOffset;
	
			if (width <= mTabWidth)
				result = inFromOffset + tabCount;
			else
			{
				string txt;
				MDevice dev;
	
				(void)GetStyledText(inFromOffset, l, inState, dev, txt);
	
				uint32 br;
				if (dev.BreakLine(width, br))
					result = inFromOffset + br;
			}
		}
	}
	
	return result;
}

// ---------------------------------------------------------------------------
//	RewrapLines

int32 MDocument::RewrapLines(
	uint32		inFrom,
	uint32		inTo)
{
	// save the mark offsets
	vector<uint32> markOffsets;
	
	// first get the from line
	uint32 firstLine = OffsetToLine(inFrom);
	if (firstLine >= mLineInfo.size())
		firstLine = mLineInfo.size();
	
	// now take two iterators into the mLineInfo array
	// If this is in a softwrapped paragraph, we
	// extend the rewrap text a bit
	
	MLineInfoArray::iterator lineInfoStart = mLineInfo.begin() + firstLine;

	if ((*lineInfoStart).nl == false and lineInfoStart != mLineInfo.begin())
		--lineInfoStart;
	
	MLineInfoArray::iterator lineInfoEnd = lineInfoStart + 1;

	while (lineInfoEnd != mLineInfo.end() and (*lineInfoEnd).start < inTo)
		++lineInfoEnd;

	while (lineInfoEnd != mLineInfo.end() and (*lineInfoEnd).nl == false)
		++lineInfoEnd;
	
	if (lineInfoEnd == mLineInfo.end())
		inTo = mText.GetSize();
	else
		inTo = (*lineInfoEnd).start - 1;
	
	// start by marking the first line dirty
	
	(*lineInfoStart).dirty = true;
	
	// now if we have more than one line we will erase the old info
	int32 cnt = lineInfoEnd - lineInfoStart;
	if (cnt > 1)
	{
		for (MLineInfoArray::iterator i = lineInfoStart; i != lineInfoEnd; ++i)
		{
			if ((*i).marked)
				markOffsets.push_back((*i).start);
		}
		
		mLineInfo.erase(lineInfoStart + 1, lineInfoEnd);
	}
	cnt = 1 - cnt;
	
	lineInfoEnd = lineInfoStart + 1;
	
	if (mLanguage and lineInfoStart == mLineInfo.begin())
		(*lineInfoStart).state = mLanguage->GetInitialState(mURL.GetFileName(), mText);
	
	uint16 state = (*lineInfoStart).state;
	uint32 start = (*lineInfoStart).start;
	
	uint32 indent = 0;

	if (GetSoftwrap())
	{
		MLineInfoArray::iterator li = lineInfoStart;
		while (li != mLineInfo.begin() and (*li).nl == false)
			--li;
		indent = GetIndent((*li).start);
	}
	
	bool isHardBreak = true;
	
	// loop over the text until we're done
	while (start < mText.GetSize())
	{
		uint32 nextBreak;
		
		if (isHardBreak)
			nextBreak = FindLineBreak(start, state, 0);
		else
			nextBreak = FindLineBreak(start, state, indent);
		
		if (nextBreak > inTo)
			break;
		
		uint32 length = nextBreak - start;
		
		if (mLanguage)
			mLanguage->StyleLine(mText, start, length, state);
		
		isHardBreak = (mText.GetChar(nextBreak - 1) == '\n');

		if (isHardBreak)
			indent = 0;

		lineInfoEnd = mLineInfo.insert(lineInfoEnd,
			MLineInfo(nextBreak, state, isHardBreak));
		++lineInfoEnd;
		
		if (not isHardBreak and indent > 0)
			(*lineInfoEnd).indent = true;

		if (isHardBreak)
			indent = GetIndent(nextBreak);
		
		++cnt;
		start = nextBreak;
	}
	
	for (vector<uint32>::iterator i = markOffsets.begin(); i != markOffsets.end(); ++i)
		MarkLine(OffsetToLine(*i));
	
	mNeedReparse = true;
	
	// return the number of processed lines
	return cnt;
}

void MDocument::BoundsChanged()
{
	if (GetSoftwrap())
		Rewrap();
}

// ---------------------------------------------------------------------------
//	Rewrap

void MDocument::Rewrap()
{
	RewrapLines(0, mText.GetSize());
	eLineCountChanged();
}

// ---------------------------------------------------------------------------
//  RestyleDirtyLines

void MDocument::RestyleDirtyLines(
	uint32	inFrom)
{
	assert(mLanguage != nil);

	for (uint32 line = inFrom; line < mLineInfo.size(); ++line)
	{
		if (mLineInfo[line].dirty)
		{
			uint16 state;
			if (line == 0)
				state = mLanguage->GetInitialState(mURL.GetFileName(), mText);
			else
				state = mLineInfo[line].state;
			
			while (mLineInfo[line].dirty and
				++line < mLineInfo.size())
			{
				mLanguage->StyleLine(mText, LineStart(line - 1),
					LineStart(line) - LineStart(line - 1), state);

				if (state != mLineInfo[line].state)
				{
					mLineInfo[line].dirty = true;
					mLineInfo[line].state = state;
				}
			}
		}
	}
}

// ---------------------------------------------------------------------------
//	GetLine

void MDocument::GetLine(
	uint32		inLineNr,
	string&		outText) const
{
	if (inLineNr < mLineInfo.size())
	{
		uint32 start = mLineInfo[inLineNr].start;
		uint32 length = mText.GetSize() - start;
		if (inLineNr < mLineInfo.size() - 1)
			length = mLineInfo[inLineNr + 1].start - start;
		
		assert(start + length <= mText.GetSize());
		
		auto_array<char> b(new char[length]);
		mText.GetText(start, b.get(), length);
		
		outText.assign(b.get(), b.get() + length);
	}
}

void MDocument::UpdateDirtyLines()
{
	if (mLanguage != nil)
		RestyleDirtyLines(0);

	eInvalidateDirtyLines();
	
	for (uint32 line = 0; line < mLineInfo.size(); ++line)
		mLineInfo[line].dirty = false;
}

void MDocument::GetSelectionBounds(
	MRect&		outBounds) const
{
	if (mSelection.IsBlock())
	{
		assert(false);
	}
	else
	{
		uint32 anchor = mSelection.GetAnchor();
		uint32 caret = mSelection.GetCaret();
		
		if (caret < anchor)
			swap(caret, anchor);
		
		uint32 anchorLine = OffsetToLine(anchor);
		uint32 caretLine = OffsetToLine(caret);
		
		if (anchorLine != caretLine)
		{
			outBounds.y = anchorLine * mLineHeight;
			outBounds.height = (caretLine - anchorLine + 1) * mLineHeight;
			outBounds.x = 0;
			outBounds.width = /*kMaxLineWidth*/100000;
		}
		else
		{
			outBounds.y = anchorLine * mLineHeight;
			outBounds.height = mLineHeight;
			
			int32 anchorPos;
			OffsetToPosition(anchor, anchorLine, anchorPos);

			outBounds.x = anchorPos;

			int32 caretPos;
			OffsetToPosition(caret, caretLine, caretPos);

			outBounds.width = caretPos - anchorPos + 1;
		}
	}
}

// -----------------------------------------------------------------------------
// GetSelectedText

void MDocument::GetSelectedText(
	string&			outText) const
{
	if (not mSelection.IsEmpty())
	{
		if (mSelection.IsBlock())
			assert(false);
		else
		{
			uint32 anchor = mSelection.GetAnchor();
			uint32 caret = mSelection.GetCaret();
			if (anchor > caret)
				swap(anchor, caret);
			mText.GetText(anchor, caret - anchor, outText);
		}
	}
}

// ---------------------------------------------------------------------------
//	Select

void MDocument::Select(
	MSelection	inSelection)
{
	FinishAction();
	ChangeSelection(inSelection);
}

void MDocument::Select(
	uint32			inAnchor,
	uint32			inCaret,
	MScrollMessage	inScrollMessage)
{
	FinishAction();
	ChangeSelection(MSelection(inAnchor, inCaret));
	if (inScrollMessage != kScrollNone)
		eScroll(inScrollMessage);
}

void MDocument::ChangeSelection(
	MSelection	inSelection)
{
	if (inSelection != mSelection)
	{
		uint32 a, c, sl1, sl2, el1, el2, l;

		if (not mSelection.IsBlock())
		{
			a = mSelection.GetAnchor();
			c = mSelection.GetCaret();
			if (a > c)
				swap(a, c);
				
			sl1 = OffsetToLine(a);
			el1 = OffsetToLine(c);
		}

		if (not inSelection.IsBlock())
		{
			a = inSelection.GetAnchor();
			c = inSelection.GetCaret();
			if (a > c)
				swap(a, c);
				
			sl2 = OffsetToLine(a);
			el2 = OffsetToLine(c);
		}
		
		// check for overlap
		if (sl1 < el2 and sl2 < el1)
		{
			for (l = min(sl1, sl2); l <= max(sl1, sl2); ++l)
				mLineInfo[l].dirty = true;
			for (l = min(el1, el2); l <= max(el1, el2); ++l)
				mLineInfo[l].dirty = true;
		}
		else
		{
			for (l = sl1; l <= el1; ++l)
				mLineInfo[l].dirty = true;
			for (l = sl2; l <= el2; ++l)
				mLineInfo[l].dirty = true;
		}
	
		mSelection = inSelection;

//		for (l = 0; l < mLineInfo.size(); ++l)
//			mLineInfo[l].dirty = false;
		
		uint32 line;
		OffsetToPosition(inSelection.GetCaret(), line, mWalkOffset);
	}

	SendSelectionChangedEvent();
}

// ---------------------------------------------------------------------------
//	SendSelectionChangedEvent

void MDocument::SendSelectionChangedEvent()
{
	string name;
	
	if (mLanguage != nil and mNamedRange != nil)
	{
		name = mLanguage->NameForPosition(*mNamedRange, mSelection.GetCaret());
		if (name.length() == 0)
			name = GetLocalisedString("no symbol");
	}
	
	eSelectionChanged(mSelection, name);
}

// ---------------------------------------------------------------------------
//	OffsetToLine

uint32 MDocument::OffsetToLine(
	uint32		inOffset) const
{
	int32 L = 0, R = mLineInfo.size() - 1;
	
	while (L <= R)
	{
		int32 i = (L + R) / 2;
		if (mLineInfo[i].start > inOffset)
			R = i - 1;
		else
			L = i + 1;
	}
	
	if (R < 0)
		R = 0;
	
	return R;
}

// ---------------------------------------------------------------------------
//	OffsetToPosition

void MDocument::OffsetToPosition(
	uint32		inOffset,
	uint32&		outLine,
	int32&		outX) const
{
	outLine = OffsetToLine(inOffset);
	
	uint32 column = inOffset - LineStart(outLine);
	if (column == 0)
		outX = 0;
	else
	{
		string text;
		MDevice device;
		
		GetStyledText(outLine, device, text);

		device.IndexToPosition(inOffset - LineStart(outLine), false, outX);

		if (GetSoftwrap())
			outX += GetLineIndentWidth(outLine);
	}
}

uint32 MDocument::OffsetToColumn(
	uint32		inOffset) const
{
	uint32 line = OffsetToLine(inOffset);
	uint32 start = LineStart(line);
	
	uint32 column = 0;
	
	uint32 n = mText.NextCursorPosition(start, eMoveOneCharacter);
	while (start < inOffset and start < mText.GetSize())
	{
		if (mText.GetChar(start) == '\t')
		{
			uint32 d = mCharsPerTab - (column % mCharsPerTab);
			column += d;
		}
		else
			++column;

		start = n;
		n = mText.NextCursorPosition(start, eMoveOneCharacter);
	}
	
	return static_cast<uint32>(column + GetLineIndentWidth(line));
}

// ---------------------------------------------------------------------------
//	PositionToOffset

void MDocument::PositionToOffset(
	int32			inLocationX,
	int32			inLocationY,
	uint32&			outOffset) const
{
	uint32 line = 0;
	if (inLocationY > 0)
		line = static_cast<uint32>(inLocationY / mLineHeight);
	
	if (line >= mLineInfo.size())
		outOffset = mText.GetSize();
	else
	{
		string text;
		MDevice device;
		
		GetStyledText(line, device, text);
		
		if (GetSoftwrap())
			inLocationX -= GetLineIndentWidth(line);
		
		if (inLocationX > 0)
		{
			bool trailing;
			/*bool hit = */device.PositionToIndex(inLocationX, outOffset, trailing);
			outOffset += mLineInfo[line].start;
		}
		else
			outOffset = mLineInfo[line].start;
	}
}

// -----------------------------------------------------------------------------
// StartAction

void MDocument::StartAction(
	const char*		inTitle)
{
	mFastFindMode = false;
	if (mCurrentAction != inTitle)
	{
		mText.StartAction(inTitle, mSelection);
		mCurrentAction = inTitle;
		mLastAction = kNoAction;
	}
}

void MDocument::FinishAction()
{
	if (mCurrentAction == kTypeAction)
		mText.SetSelectionAfter(mSelection);

	mLastAction = mCurrentAction;
	mCurrentAction = kNoAction;

	UpdateDirtyLines();
	mCompletionStrings.clear();

	mCompletionIndex = -1;
}

void MDocument::Insert(
	uint32			inOffset,
	const char*		inText,
	uint32			inLength)
{
	if (inLength > 0)
	{
		uint32 lineCount = mLineInfo.size();
		
		mText.Insert(inOffset, inText, inLength);
		if (not mDirty)
			SetModified(true);
	
		uint32 line = OffsetToLine(inOffset);
		mLineInfo[line].dirty = true;
	
		for (uint32 l = line + 1; l < mLineInfo.size(); ++l)
			mLineInfo[l].start += inLength;

		if (not mSelection.IsBlock())
		{
			uint32 anchor = mSelection.GetAnchor();
			if (inOffset <= anchor)
				anchor += inLength;
			
			uint32 caret = mSelection.GetCaret();
			if (inOffset <= caret)
				caret += inLength;
			
			mSelection.Set(anchor, caret);
		}
	
		int32 delta = RewrapLines(inOffset, inOffset + inLength);
		if (delta)
			eShiftLines(line, delta);

		if (lineCount != mLineInfo.size())
			eLineCountChanged();
	}
}

void MDocument::Delete(
	uint32			inOffset,
	uint32			inLength)
{
	if (inLength > 0)
	{
		assert(inOffset + inLength <= mText.GetSize());

		mText.Delete(inOffset, inLength);
		if (not mDirty)
			SetModified(true);

		if (not mSelection.IsBlock())
		{
			uint32 anchor = mSelection.GetAnchor();
			if (inOffset + inLength <= anchor)
				anchor -= inLength;
			else if (inOffset < anchor)
				anchor = inOffset;
			
			uint32 caret = mSelection.GetCaret();
			if (inOffset + inLength <= caret)
				caret -= inLength;
			else if (inOffset < caret)
				caret = inOffset;
			
			mSelection.Set(anchor, caret);
		}

		uint32 firstLine = OffsetToLine(inOffset);
		uint32 lastLine = OffsetToLine(inOffset + inLength);

		mLineInfo[firstLine].dirty = true;
		for (uint32 line = firstLine + 1; line < mLineInfo.size(); ++line)
			mLineInfo[line].start -= inLength;
		
		if (firstLine != lastLine)
			mLineInfo.erase(mLineInfo.begin() + firstLine + 1, mLineInfo.begin() + lastLine + 1);

		int32 delta = 0;
		if (GetSoftwrap())
			delta = RewrapLines(inOffset, inOffset);

		if (delta + (lastLine - firstLine) != 0)
		{
			eLineCountChanged();
			eShiftLines(firstLine, delta - static_cast<int32>(lastLine - firstLine));
		}
	}
}

bool MDocument::CanUndo(string& outAction)
{
	return mText.CanUndo(outAction);
}

void MDocument::DoUndo()
{
	MSelection s;
	uint32 offset, length;
	int32 delta;
	
	mLastAction = mCurrentAction = kNoAction;
	mText.Undo(s, offset, length, delta);
	RepairAfterUndo(offset, length, delta);
	ChangeSelection(s);
	UpdateDirtyLines();
	eLineCountChanged();
	eScroll(kScrollToSelection);
	
	SetModified(true);
}

bool MDocument::CanRedo(string& outAction)
{
	return mText.CanRedo(outAction);
}

void MDocument::DoRedo()
{
	MSelection s;
	uint32 offset, length;
	int32 delta;
	
	mLastAction = mCurrentAction = kNoAction;
	mText.Redo(s, offset, length, delta);
	RepairAfterUndo(offset, length, delta);
	ChangeSelection(s);
	UpdateDirtyLines();
	eLineCountChanged();
	eScroll(kScrollToSelection);

	SetModified(true);
}

void MDocument::RepairAfterUndo(
	uint32		inOffset,
	uint32		inLength,
	int32		inDelta)
{
	uint32 line = OffsetToLine(inOffset);
	uint32 lastLine = OffsetToLine(inOffset + inLength);
	int32 lineDelta = 0;

	if (line < lastLine)
	{
		lineDelta = -static_cast<int32>(lastLine - line);
		mLineInfo.erase(mLineInfo.begin() + line + 1,
			mLineInfo.begin() + lastLine + 1);
	}

	for (line += 1; line < mLineInfo.size(); ++line)
		mLineInfo[line].start += inDelta;

	lineDelta += RewrapLines(inOffset, inOffset + inLength);
	
	eShiftLines(OffsetToLine(inOffset), lineDelta);
}

// -----------------------------------------------------------------------------
// Text Menu Command Handlers

void MDocument::DoBalance()
{
	uint32 offset = mSelection.GetMinOffset(*this);
	uint32 length = mSelection.GetMaxOffset(*this) - offset;
	
	uint32 newOffset = offset;
	uint32 newLength = length;
	
	if (mLanguage and mLanguage->Balance(mText, newOffset, newLength))
	{
		if (newOffset == offset and newLength == length)
		{
			--newOffset;
			newLength += 2;
			if (mLanguage->Balance(mText, newOffset, newLength))
				Select(newOffset, newOffset + newLength);
			else
				Beep();
		}
		else
			Select(newOffset, newOffset + newLength);
	}
	else
		Beep();
}
								
void MDocument::DoShiftLeft()
{
	Select(mSelection.SelectLines(*this));
	
	StartAction("Shift left");
	
	uint32 minLine = mSelection.GetMinLine(*this);
	uint32 maxLine = mSelection.GetMaxLine(*this);
	
	for (uint32 line = minLine; line <= maxLine; ++line)
	{
		uint32 offset = LineStart(line);
		MTextBuffer::iterator txt = mText.begin() + offset;
		uint32 lineLength = LineStart(line + 1) - offset;
		
		uint32 n = 0;
		while (n < lineLength and *txt == ' ' and n < mCharsPerTab)
		{
			++txt;
			++n;
		}
		
		if (n < lineLength and *txt == '\t' and n < mCharsPerTab)
			++n;
		
		if (n > 0)
			Delete(offset, n);
	}
	
	FinishAction();
}

void MDocument::DoShiftRight()
{
	Select(mSelection.SelectLines(*this));
	
	StartAction("Shift right");
	
	uint32 anchor = mSelection.GetAnchor();
	uint32 caret = mSelection.GetCaret();

	uint32 minLine = mSelection.GetMinLine(*this);
	uint32 maxLine = mSelection.GetMaxLine(*this);
	
	for (uint32 line = minLine; line <= maxLine; ++line)
	{
		Insert(LineStart(line), "\t", 1);
		caret += 1;
	}
	
	mSelection.Set(anchor, caret);
	FinishAction();
}
								
void MDocument::DoComment()
{
	if (mLanguage and not mSelection.IsBlock())
	{
		if (mSelection.IsEmpty())
			Select(mSelection.SelectLines(*this));
		
		uint32 selectionStart, selectionEnd;
		selectionStart = mSelection.GetMinOffset();
		selectionEnd = mSelection.GetMaxOffset();
		
		StartAction("Comment");

		uint32 minLine = mSelection.GetMinLine(*this);
		uint32 maxLine = mSelection.GetMaxLine(*this);
		
		for (uint32 line = minLine; line <= maxLine; ++line)
		{
			uint32 offset;
			if (line == minLine)
				offset = mSelection.GetMinOffset(*this);
			else
				offset = LineStart(line);
			
			uint32 length;
			if (line == maxLine)
				length = mSelection.GetMaxOffset(*this) - offset;
			else
				length = LineEnd(line) - offset;
			
			string text;
			mText.GetText(offset, length, text);
			
			if (length > 0 and text[length - 1] == '\n')
			{
				--length;
				text.erase(length, 1);
			}
			
			mLanguage->CommentLine(text);
			
			Delete(offset, length);
			Insert(offset, text.c_str(), text.length());
			
			selectionEnd += text.length() - length;
		}
		
		Select(selectionStart, selectionEnd);
		mText.SetSelectionAfter(mSelection);

		FinishAction();
	}
}
								
void MDocument::DoUncomment()
{
	if (mLanguage and not mSelection.IsBlock())
	{
		if (mSelection.IsEmpty())
			Select(mSelection.SelectLines(*this));
		
		uint32 selectionStart, selectionEnd;
		selectionStart = mSelection.GetMinOffset();
		selectionEnd = mSelection.GetMaxOffset();
		
		StartAction("Uncomment");

		uint32 minLine = mSelection.GetMinLine(*this);
		uint32 maxLine = mSelection.GetMaxLine(*this);
		
		for (uint32 line = minLine; line <= maxLine; ++line)
		{
			uint32 offset;
			if (line == minLine)
				offset = mSelection.GetMinOffset(*this);
			else
				offset = LineStart(line);
			
			uint32 length;
			if (line == maxLine)
				length = mSelection.GetMaxOffset(*this) - offset;
			else
				length = LineEnd(line) - offset;
			
			string text;
			mText.GetText(offset, length, text);

			if (length > 0 and text[length - 1] == '\n')
			{
				--length;
				text.erase(length, 1);
			}
			
			mLanguage->UncommentLine(text);
			
			Delete(offset, length);
			Insert(offset, text.c_str(), text.length());
			
			selectionEnd += text.length() - length;
		}
		
		Select(selectionStart, selectionEnd);
		mText.SetSelectionAfter(mSelection);
		
		FinishAction();
	}
}
								
void MDocument::DoEntab()
{
	StartAction("Entab");
	
	if (mSelection.IsEmpty())
		Select(0, mText.GetSize());

	uint32 offset = mSelection.GetMinOffset();
	bool block = mSelection.IsBlock();

	string text;
	GetSelectedText(text);
	DeleteSelectedText();

	int startColumn = OffsetToColumn(offset);
	int column = startColumn;
	string::iterator i = text.begin();
	
	while (i != text.end())
	{
		switch (*i)
		{
			case ' ':
			{
				string::iterator s = i;

				do
				{
					++i;
					++column;
				}
				while (i != text.end() and *i == ' ' and (column % mCharsPerTab) != 0);
				
				if (i - s > 1 and (column % mCharsPerTab) == 0)
				{
					text.erase(s, i);
					i = text.insert(s, '\t') + 1;
				}
				else if (*i == '\t')
				{
					text.erase(s, i);
					i = s + 1;
					column = mCharsPerTab * ((column / mCharsPerTab) + 1);
				}
				break;
			}

			case '\t':
				column = mCharsPerTab * ((column / mCharsPerTab) + 1);
				++i;
				break;

			case '\n':
				if (block)
					column = startColumn;
				else
					column = 0;
				++i;
				break;

			default:
				i = next_cursor_position(i, text.end());
				++column;
				break;
		}
	}
	
	if (block)
	{
	}
	else
	{
		Insert(offset, text);
		Select(offset, offset + text.length());
	}
	
	FinishAction();
}
								
void MDocument::DoDetab()
{
	StartAction("Detab");

	if (mSelection.IsEmpty())
		Select(0, mText.GetSize());

	uint32 offset = mSelection.GetMinOffset();
	bool block = mSelection.IsBlock();

	string text;
	GetSelectedText(text);
	DeleteSelectedText();

    string::iterator i = text.begin();
	int startColumn = OffsetToColumn(offset);
	int column = startColumn;
    
    while (i != text.end())
    {
		switch (*i)
		{
			case '\t':
			{
				int toInsert = mCharsPerTab - (column % mCharsPerTab);

				column += toInsert;
			
				text.erase(i);
				while (toInsert-- > 0)
					i = text.insert(i, ' ') + 1;
				break;
			}
			case '\n':
				if (block)
					column = startColumn;
				else
					column = 0;
				++i;
				break;
			
			default:
				i = next_cursor_position(i, text.end());
				++column;
				break;
		}
	}
	
	if (block)
	{
	}
	else
	{
		Insert(offset, text);
		Select(offset, offset + text.length());
	}
	
	FinishAction();
}
								
void MDocument::DoCut(bool inAppend)
{
	StartAction("Cut");
	
	string text;
	GetSelectedText(text);
	if (inAppend)
		MClipboard::Instance().AddData(text);
	else
		MClipboard::Instance().SetData(text, mSelection.IsBlock());
	
	DeleteSelectedText();

	FinishAction();
}

void MDocument::DoCopy(bool inAppend)
{
	string text;
	GetSelectedText(text);
	if (inAppend)
		MClipboard::Instance().AddData(text);
	else
		MClipboard::Instance().SetData(text, mSelection.IsBlock());
}

void MDocument::DoPaste()
{
	if (MClipboard::Instance().HasData())
	{
		string text;
		bool isBlock;
		
		MClipboard::Instance().GetData(text, isBlock);
		
		StartAction(kPasteAction);
		
		if (not mSelection.IsEmpty())
			DeleteSelectedText();
		
		uint32 offset = mSelection.GetCaret();
		Insert(offset, text);
//		ChangeSelection(Selection(offset, offset + textLength));
		
		FinishAction();

		ChangeSelection(MSelection(offset + text.length(), offset + text.length()));
	}
	else
		assert(false);
}

void MDocument::DoClear()
{
	StartAction("Clear");
	
	DeleteSelectedText();

	FinishAction();
}

void MDocument::DoPasteNext()
{
	if (mLastAction == kPasteAction and mCurrentAction == kNoAction)
	{
		DoUndo();
		MClipboard::Instance().NextInRing();
	}
	DoPaste();
}

void MDocument::DoFastFind(MDirection inDirection)
{
	mFastFindStartOffset = mSelection.GetMaxOffset(*this);
	
	if (not mFastFindMode)
	{
		mFastFindMode = true;
		mFastFindInited = false;
		mFastFindDirection = inDirection;
		mFastFindWhat.clear();
	}
	else if (not mFastFindInited)
	{
		mFastFindInited = true;
		mFastFindWhat = MFindDialog::Instance().GetFindString();
		MFindDialog::Instance().SetFindString(mFastFindWhat, false);
		FastFind(inDirection);
	}
	else
		FastFind(inDirection);
}

// -----------------------------------------------------------------------
// FastFind

void MDocument::FastFindType(const char* inText, uint32 inTextLength)
{
	if (mTargetTextView != nil)
		mTargetTextView->ObscureCursor();
	
	string what = mFastFindWhat;
	
	if (inText != nil)
		mFastFindWhat.append(inText, inTextLength);
	else
	{
#pragma warning("fix me!")
//		uint32 offset = mFastFindWhat.length();
//		OSStatus err = ::UCFindTextBreak(
//			MBreakLocator::Instance(),
//			kUCTextBreakClusterMask,
//			kUCTextBreakGoBackwardsMask,
//			mFastFindWhat.c_str(), offset, offset, &offset);
		uint32 offset = mFastFindWhat.length() - 1;
		
		mFastFindWhat.erase(offset, mFastFindWhat.length() - offset);
	}
	
	if (not mFastFindInited)
		MFindDialog::Instance().SetFindString(mFastFindWhat, false);
	
	if (FastFind(mFastFindDirection))
		mFastFindInited = true;
	else
		mFastFindWhat = what;
}

bool MDocument::FastFind(MDirection inDirection)
{
	uint32 offset = mFastFindStartOffset;
	if (inDirection == kDirectionBackward and offset > 0)
		offset -= mFastFindWhat.length();
	
	MSelection found;
	bool result = mText.Find(offset, mFastFindWhat, inDirection, true, false, found);
	
	if (result)
	{
		MFindDialog::Instance().SetFindString(mFastFindWhat, true);
		Select(found.GetMinOffset(*this), found.GetMaxOffset(*this), kScrollToSelection);
	}
	else
		Beep();
	
	return result;
}

bool MDocument::CanReplace()
{
	return mText.CanReplace(MFindDialog::Instance().GetFindString(),
		MFindDialog::Instance().GetRegex(),
		MFindDialog::Instance().GetIgnoreCase(), mSelection);
}

void MDocument::DoMarkLine()
{
	uint32 line = mSelection.GetMinLine(*this);
	MarkLine(line, not mLineInfo[line].marked);
	eInvalidateDirtyLines();
}

void MDocument::HandleFindDialogCommand(uint32 inCommand)
{
	if (mTargetTextView and mTargetTextView->GetWindow())
		mTargetTextView->GetWindow()->Select();
	
	switch (inCommand)
	{
		case cmd_FindNext:
			if (not DoFindNext(kDirectionForward))
				Beep();
			break;

		case cmd_Replace:
			DoReplace(false, kDirectionForward);
			break;

		case cmd_ReplaceFindNext:
			DoReplace(true, kDirectionForward);
			break;

		case cmd_ReplaceAll:
			DoReplaceAll();
			break;
	}
}

bool MDocument::DoFindNext(MDirection inDirection)
{
	string what = MFindDialog::Instance().GetFindString();
	bool ignoreCase = MFindDialog::Instance().GetIgnoreCase();
	bool regex = MFindDialog::Instance().GetRegex();
	uint32 offset;
	
	if (inDirection == kDirectionBackward)
		offset = mSelection.GetMinOffset(*this);
	else
		offset = mSelection.GetMaxOffset(*this);
	
	MSelection found;
	bool result = mText.Find(offset, what, inDirection, ignoreCase, regex, found);
	
	if (result)
		Select(found.GetMinOffset(*this), found.GetMaxOffset(*this), kScrollToSelection);
	
	return result;
}

void MDocument::FindAll(string inWhat, bool inIgnoreCase, 
	bool inRegex, bool inSelection, MMessageList&outHits)
{
	if (mSelection.IsBlock())
		THROW(("block selection not supported in Find All"));

	uint32 minOffset = 0;
	uint32 maxOffset = mText.GetSize();

	if (inSelection)
	{
		minOffset = mSelection.GetMinOffset(*this);
		maxOffset = mSelection.GetMaxOffset(*this);
	}
	
	MSelection sel;
	while (mText.Find(minOffset, inWhat, kDirectionForward, inIgnoreCase, inRegex, sel) and
		sel.GetMaxOffset() <= maxOffset)
	{
		uint32 lineNr = sel.GetMinLine(*this);
		
		string s;
		GetLine(lineNr, s);
		
		outHits.AddMessage(kMsgKindNone, mURL.GetPath(), lineNr + 1,
			sel.GetMinOffset(*this), sel.GetMaxOffset(*this), s);
		
		minOffset = sel.GetMaxOffset();
	}
}

void MDocument::DoReplace(bool inFindNext, MDirection inDirection)
{
	if (CanReplace())
	{
		StartAction(kReplaceAction);
		
		string what = MFindDialog::Instance().GetFindString();
		string replace = MFindDialog::Instance().GetReplaceString();
		
		uint32 offset = mSelection.GetMinOffset(*this);
		
		if (MFindDialog::Instance().GetRegex())
		{
			mText.ReplaceExpression(mSelection, what, MFindDialog::Instance().GetIgnoreCase(),
				replace, replace);
		}

		DeleteSelectedText();
		Insert(offset, replace);
		
		if (inFindNext)
		{
			bool ignoreCase = MFindDialog::Instance().GetIgnoreCase();
			bool regex = MFindDialog::Instance().GetRegex();
			uint32 nextOffset = offset;

			if (inDirection == kDirectionForward)
				nextOffset += replace.length();
		
			MSelection found;
			bool result = mText.Find(nextOffset, what, inDirection, ignoreCase, regex, found);
			
			if (result)
				ChangeSelection(found);
			else
			{
				ChangeSelection(offset, offset + replace.length());
				Beep();
			}
	
			eScroll(kScrollToSelection);
			FinishAction();
		}
	}
	else
		Beep();
}

void MDocument::DoReplaceAll()
{
	uint32 offset = 0, lastMatch = 0;
	string what = MFindDialog::Instance().GetFindString();
	string replace;
	bool ignoreCase = MFindDialog::Instance().GetIgnoreCase();
	bool replacedAny = false;
	bool regex = MFindDialog::Instance().GetRegex();
	MSelection found;
	
	while (mText.Find(offset, what, kDirectionForward, ignoreCase, regex, found))
	{
		if (not replacedAny)
		{
			StartAction(kReplaceAction);
			replacedAny = true;
		}
		
		offset = found.GetMinOffset();
		replace = MFindDialog::Instance().GetReplaceString();
		
		if (regex)
		{
			mText.ReplaceExpression(found, what, MFindDialog::Instance().GetIgnoreCase(),
				replace, replace);
		}
		
		Delete(offset, found.GetMaxOffset() - offset);
		Insert(offset, replace);
		
		lastMatch = offset;
		offset += replace.length();
	}
	
	if (replacedAny)
		Select(lastMatch, lastMatch + replace.length(), kScrollToSelection);
	else
		Beep();
}

void MDocument::DoComplete(MDirection inDirection)
{
	if (mCompletionIndex != -1 and mCurrentAction == kTypeAction)
	{
		int32 remove = mSelection.GetCaret() - mCompletionStartOffset;
		if (remove > 0)
			Delete(mCompletionStartOffset, mSelection.GetCaret() - mCompletionStartOffset);
	}
	else
	{
		mCompletionStrings.clear();
		mCompletionStartOffset = mSelection.GetCaret();
		
		int32 startOffset = FindWord(mCompletionStartOffset, kDirectionBackward);
		int32 length = mCompletionStartOffset - startOffset;
		
		if (length == 0 or OffsetToLine(mCompletionStartOffset) != OffsetToLine(startOffset))
		{
			Beep();
			return;
		}
		
		string key;
		mText.GetText(startOffset, length, key);
		mText.CollectWordsBeginningWith(startOffset, inDirection, key, mCompletionStrings);
	
//		boost::mutex::scoped_lock lock(sDocListMutex);

		MDocument* doc = sFirst;
		while (doc != nil)
		{
			if (doc != this)
				doc->mText.CollectWordsBeginningWith(
					0, kDirectionForward, key, mCompletionStrings);
			doc = doc->mNext;
		}
		
//		lock.unlock();
		
		if (inDirection == kDirectionForward)
			mCompletionIndex = -1;
		else
		{
			mCompletionIndex = mCompletionStrings.size();
			reverse(mCompletionStrings.begin(), mCompletionStrings.end());
		}
	}
	
	if (inDirection == kDirectionForward)
		++mCompletionIndex;
	else
		--mCompletionIndex;
	
	if (mCompletionIndex < 0 or mCompletionIndex >= static_cast<int32>(mCompletionStrings.size()))
	{
		mCompletionIndex = -1;
		Beep();
	}
	else
	{
		Type(mCompletionStrings[mCompletionIndex].c_str(),
			mCompletionStrings[mCompletionIndex].length());
	}
	
	TouchLine(OffsetToLine(mCompletionStartOffset));
	UpdateDirtyLines();
}

void MDocument::DoSoftwrap()
{
	mSoftwrap = not mSoftwrap;
	Rewrap();
	UpdateDirtyLines();
}

void MDocument::GetStyledText(
	uint32		inLine,
	MDevice&	inDevice,
	string&		outText) const
{
	uint32 offset = LineStart(inLine);
	uint32 length = LineStart(inLine + 1) - offset;
	return GetStyledText(offset, length, mLineInfo[inLine].state, inDevice, outText);
}

void MDocument::GetStyledText(
	uint32		inOffset,
	uint32		inLength,
	uint16		inState,
	MDevice&	inDevice,
	string&		outText) const
{
	inDevice.SetFont(mFont);
	
	mText.GetText(inOffset, inLength, outText);

	if (inLength > 0 and outText[outText.length() - 1] == '\n')
		outText.erase(outText.length() - 1);

	inDevice.SetText(outText);
	
	if (mLanguage)
	{
		uint32 styles[kMaxStyles] = { 0 };
		uint32 offsets[kMaxStyles] = { 0 };

		uint32 count = mLanguage->StyleLine(
			mText, inOffset, inLength, inState, styles, offsets);

		inDevice.SetTextColors(count, styles, offsets);
	}

	inDevice.SetTabStops(mTabWidth);
	
//	if (mTextInputAreaInfo.fOffset[kActiveInputArea] >= 0)
//	{
//		uint32 line = OffsetToLine(inOffset);
//
//		if (OffsetToLine(mTextInputAreaInfo.fOffset[kActiveInputArea]) == line)
//		{
//			for (uint32 i = kRawText; i <= kSelectedConvertedText; ++i)
//			{
//				if (mTextInputAreaInfo.fOffset[i] < 0 or mTextInputAreaInfo.fLength[i] <= 0)
//					continue;
//				
//				uint32 runOffset = mTextInputAreaInfo.fOffset[i] - inOffset;
//				uint32 runLength = mTextInputAreaInfo.fLength[i];
//				
//				THROW_IF_OSERROR(::ATSUSetRunStyle(mTextLayout,
//					styleArray.GetInputStyle(i), runOffset, runLength));
//			}
//		}
//	}
	
//	for (uint32 ix = 0; ix < outText.length(); ++ix)
//	{
//		char ch = outText[ix];
//		if ((ch < 0x0020 and not isspace(ch)) or ch == 0x007f)
//		{
//			outText[ix] = 0x00BF; //	'¿'
//			
//			THROW_IF_OSERROR(::ATSUSetRunStyle(mTextLayout,
//				styleArray.GetInvisiblesStyle(), ix, 1));
//		}
//	}

//	if (gShowInvisibles)
//	{
//		ATSULayoutOperationOverrideSpecifier overrideSpec;
//	
//		// setup the post layout adjustment
//		overrideSpec.operationSelector = kATSULayoutOperationPostLayoutAdjustment;
//		overrideSpec.overrideUPP = &MDocument::GetReplacementGlyphCallback;
//		
//		// set the override spec in the layout object
//		ATSUAttributeTag attrTag[] = { kATSULayoutOperationOverrideTag };
//		ByteCount attrSize[] = { sizeof(ATSULayoutOperationOverrideSpecifier) };
//		ATSUAttributeValuePtr attrPtr[] = { &overrideSpec };
//		
//		// set the override spec in the layout
//		THROW_IF_OSERROR(::ATSUSetLayoutControls(mTextLayout, 1, attrTag, attrSize, attrPtr));
//	}
//	else
//	{
//		
//	}
}

// ---------------------------------------------------------------------------
//	HashLines, used for creating diffs

void MDocument::HashLines(vector<uint32>& outHashes) const
{
	outHashes.clear();
	outHashes.reserve(mLineInfo.size());
	
	for (uint32 line = 0; line < mLineInfo.size(); ++line)
	{
		MTextBuffer::const_iterator b(&mText, LineStart(line));
		MTextBuffer::const_iterator e(&mText, LineStart(line + 1));
		
		uint32 hash = boost::hash_range(b, e);
		
		outHashes.push_back(hash);
	}
}

// -- text input methods

// ---------------------------------------------------------------------------
//	OnKeyPressEvent

void MDocument::OnKeyPressEvent(
	GdkEventKey*		inEvent)
{
	bool handled = false;

    uint32 modifiers = inEvent->state;
	uint32 keyValue = inEvent->keyval;
	
	handled = HandleRawKeydown(keyValue, modifiers);
	
	if (not handled and modifiers == 0)
	{
		wchar_t ch = gdk_keyval_to_unicode(keyValue);
		
		if (ch != 0)
		{
			char s[8] = {};
			char* sp = s;
			uint32 length = MEncodingTraits<kEncodingUTF8>::WriteUnicode(sp, ch);

cout << "Writing a result from gdk_keyval_to_unicode: " << s << endl;

			Type(s, length);
		}
	}
}

// ----------------------------------------------------------------------------
//	OnCommit

void MDocument::OnCommit(
	const char*			inText,
	uint32				inLength)
{
	if (mFastFindMode)
		FastFindType(inText, inLength);
	else
	{
		Type(inText, inLength);
		mCompletionIndex = -1;
	}
	
	UpdateDirtyLines();
}

//OSStatus MDocument::DoTextInputUnicodeForKeyEvent(EventRef ioEvent)
//{
//	uint32 dataSize; 
//	auto_array<UniChar> buffer;
//	
//	OSStatus status = ::GetEventParameter(ioEvent, kEventParamTextInputSendText,
//		typeUnicodeText, nil, 0, &dataSize, nil);
//	
//	if (status == noErr and dataSize > 0) 
//	{ 
//		buffer.reset(new UniChar[dataSize / sizeof(UniChar)]);
//		
//		status = ::GetEventParameter(ioEvent, kEventParamTextInputSendText,
//			typeUnicodeText, nil, dataSize, nil, buffer.get());
//	}
//	
//	UniChar* chars = buffer.get();
//	uint32 charCount = dataSize / sizeof(UniChar);
//	
//	THROW_IF_OSERROR(status); 
//	
//	bool handled = false;
//	mCompletionStrings.clear();
//
//	EventRef origEvent;
//	if (::GetEventParameter(ioEvent, kEventParamTextInputSendKeyboardEvent,
//	    typeEventRef, nil, sizeof(EventRef), nil, &origEvent) == noErr)
//	{
//	    uint32 modifiers, keyCode;
//
//	    ::GetEventParameter(origEvent, kEventParamKeyModifiers,
//	        typeuint32, nil, sizeof(uint32), nil, &modifiers);
//	    ::GetEventParameter(origEvent, kEventParamKeyCode,
//			typeuint32, nil, sizeof(uint32), nil, &keyCode);
//
//		uint32 state = 0;    // we don't want to save the state
//		void* transData = (void*)::GetScriptManagerVariable(smKCHRCache);
//		uint32 charCode = static_cast<uint32>(
//			toupper(static_cast<int>(::KeyTranslate(transData,
//			static_cast<uint32>(keyCode & 0x000000FF), &state))));
//		
//		handled = HandleRawKeydown(keyCode, charCode, modifiers);
//		
//		if (modifiers & cmdKey)
//		{
//			handled = true;		// don't type command key's
//			mCompletionIndex = -1;
//		}
//	}
//	
//	if (not handled and dataSize > 0)
//	{
//		for (UniChar* text = chars; text != chars + charCount; ++text)
//		{
//			if (*text == '\r')
//				*text = '\n';
//		}
//		
//		if (mFastFindMode)
//			FastFindType(chars, charCount);
//		else
//		{
//			Type(chars, charCount);
//			mCompletionIndex = -1;
//		}
//		
//		handled = true;
//	}
//	
//	UpdateDirtyLines();
//	
//	OSStatus err = noErr;
//	if (not handled)
//		err = eventNotHandledErr;
//	return err;
//}

bool MDocument::HandleKeyCommand(MKeyCommand inKeyCommand)
{
	bool handled = true;
	bool updateWalkOffset = true;
	bool scrollToCaret = true;
	bool updateSelection = true;
	int32 oldWalkOffset = mWalkOffset;

	uint32 caret = mSelection.GetCaret();
	uint32 anchor = mSelection.GetAnchor();
	uint32 caretLine = OffsetToLine(caret);

	uint32 minOffset = caret;
	uint32 maxOffset = anchor;
	if (minOffset > maxOffset)
		swap(minOffset, maxOffset);
	
	uint32 minLine = OffsetToLine(minOffset);
	uint32 maxLine = OffsetToLine(maxOffset);

	int32 x, y;
	x = mWalkOffset;
	y = caretLine * mLineHeight;
	
	uint32 firstLine, lastLine;
	mTargetTextView->GetVisibleLineSpan(firstLine, lastLine);
	uint32 linesPerPage = lastLine - firstLine;

	switch (inKeyCommand)
	{
		case kcmd_MoveCharacterLeft:
			mFastFindMode = false;
			if (mSelection.IsEmpty())
				caret = anchor = mText.PreviousCursorPosition(caret, eMoveOneCharacter);
			else
				caret = anchor = mSelection.GetMinOffset(*this);
			break;

		case kcmd_MoveCharacterRight:
			mFastFindMode = false;
			if (mSelection.IsEmpty())
				caret = anchor = mText.NextCursorPosition(caret, eMoveOneCharacter);
			else
				caret = anchor = mSelection.GetMaxOffset(*this);
			break;

		case kcmd_MoveWordLeft:
			mFastFindMode = false;
			if (mSelection.IsEmpty())
				caret = anchor = FindWord(caret, kDirectionBackward);
			else
				caret = anchor = mSelection.GetMinOffset(*this);
			break;

		case kcmd_MoveWordRight:
			mFastFindMode = false;
			if (mSelection.IsEmpty())
				caret = anchor = FindWord(caret, kDirectionForward);
			else
				caret = anchor = mSelection.GetMaxOffset(*this);
			break;

		case kcmd_MoveToBeginningOfLine:
			mFastFindMode = false;
			if (mSelection.IsEmpty())
				caret = anchor = LineStart(minLine);
			else
				caret = anchor = mSelection.GetMinOffset(*this);
			break;

		case kcmd_MoveToEndOfLine:
			mFastFindMode = false;
			if (mSelection.IsEmpty())
				caret = anchor = LineEnd(minLine);
			else
				caret = anchor = mSelection.GetMaxOffset(*this);
			break;

		case kcmd_MoveToPreviousLine:
			mFastFindMode = false;
			if (mSelection.IsEmpty())
			{
				if (minLine > 0)
				{
					y -= mLineHeight;
					PositionToOffset(x, y, caret);
				}
				else
					caret = 0;
			}
			else
				caret = mSelection.GetMinOffset(*this);
			anchor = caret;
			updateWalkOffset = false;
			break;

		case kcmd_MoveToNextLine:
			mFastFindMode = false;
			if (mSelection.IsEmpty())
			{
				y += mLineHeight;
				PositionToOffset(x, y, caret);
			}
			else
				caret = mSelection.GetMaxOffset(*this);
			anchor = caret;
			updateWalkOffset = false;
			break;

		case kcmd_MoveToTopOfPage:
			mFastFindMode = false;
			if (minLine == 0)
				caret = 0;
			else if (minLine > firstLine)
			{
				y = firstLine * mLineHeight;
				PositionToOffset(x, y, caret);
			}
			else
			{
				if (caretLine > linesPerPage)
					caretLine -= linesPerPage;
				else
					caretLine = 0;
				y = caretLine * mLineHeight;
				PositionToOffset(x, y, caret);
			}
			anchor = caret;
			updateWalkOffset = false;
			break;

		case kcmd_MoveToEndOfPage:
			mFastFindMode = false;
			if (maxLine < lastLine)
			{
				y = lastLine * mLineHeight;
				PositionToOffset(x, y, caret);
			}
			else
			{
				maxLine = lastLine + linesPerPage;
				if (maxLine >= mLineInfo.size())
					maxLine = mLineInfo.size() - 1;
				
				y = maxLine * mLineHeight;
				PositionToOffset(x, y, caret);
			}
			anchor = caret;
			updateWalkOffset = false;
			break;

		case kcmd_MoveToBeginningOfFile:
			mFastFindMode = false;
			caret = anchor = 0;		
			updateWalkOffset = false;
			break;

		case kcmd_MoveToEndOfFile:
			mFastFindMode = false;
			caret = anchor = mText.GetSize();
			updateWalkOffset = false;
			break;


		case kcmd_DeleteCharacterLeft:
			if (mFastFindMode)
			{
#pragma warning("fix me")
				if (mFastFindWhat.length())
					FastFindType(nil, 0);		// lame
				else
					Beep();
			}
			else
				HandleDeleteKey(kDirectionBackward);
			updateSelection = false;
			break;

		case kcmd_DeleteCharacterRight:
			mFastFindMode = false;
			HandleDeleteKey(kDirectionForward);
			updateSelection = false;
			break;

		case kcmd_DeleteWordLeft:
			mFastFindMode = false;
			FinishAction();
			if (mSelection.IsEmpty())
				mSelection.Set(FindWord(caret, kDirectionBackward), caret);
			HandleDeleteKey(kDirectionBackward);
			updateSelection = false;
			break;

		case kcmd_DeleteWordRight:
			mFastFindMode = false;
			FinishAction();
			if (mSelection.IsEmpty())
				mSelection.Set(caret, FindWord(caret, kDirectionForward));
			HandleDeleteKey(kDirectionForward);
			updateSelection = false;
			break;

		case kcmd_DeleteToEndOfLine:
			mFastFindMode = false;
			if (mSelection.IsEmpty())
				mSelection.Set(caret, LineEnd(minLine));
			HandleDeleteKey(kDirectionForward);
			updateSelection = false;
			break;

		case kcmd_DeleteToEndOfFile:
			mFastFindMode = false;
			if (mSelection.IsEmpty())
				mSelection.Set(caret, mText.GetSize());
			HandleDeleteKey(kDirectionForward);
			updateSelection = false;
			break;


		case kcmd_ExtendSelectionWithCharacterLeft:
			mFastFindMode = false;
			caret = mText.PreviousCursorPosition(caret, eMoveOneCharacter);
			break;

		case kcmd_ExtendSelectionWithCharacterRight:
			mFastFindMode = false;
			caret = mText.NextCursorPosition(caret, eMoveOneCharacter);
			break;

		case kcmd_ExtendSelectionWithPreviousWord:
			mFastFindMode = false;
			caret = FindWord(caret, kDirectionBackward);
			break;

		case kcmd_ExtendSelectionWithNextWord:
			mFastFindMode = false;
			caret = FindWord(caret, kDirectionForward);
			break;

		case kcmd_ExtendSelectionToCurrentLine:
			mFastFindMode = false;
			caret = LineStart(minLine);
			anchor = LineEnd(maxLine);
			break;

		case kcmd_ExtendSelectionToPreviousLine:
			mFastFindMode = false;
			if (caretLine > 0)
			{
				y -= mLineHeight;
				PositionToOffset(x, y, caret);
			}
			else
				caret = 0;
			updateWalkOffset = false;
			break;

		case kcmd_ExtendSelectionToNextLine:
			mFastFindMode = false;
			y += mLineHeight;
			PositionToOffset(x, y, caret);
			updateWalkOffset = false;
			break;

		case kcmd_ExtendSelectionToBeginningOfLine:
			mFastFindMode = false;
			caret = LineStart(caretLine);
			break;

		case kcmd_ExtendSelectionToEndOfLine:
			mFastFindMode = false;
			caret = LineEnd(caretLine);
			break;

		case kcmd_ExtendSelectionToBeginningOfPage:
			mFastFindMode = false;
			if (caretLine == 0)
				caret = 0;
			else if (caretLine - 1 > firstLine)
			{
				y = firstLine * mLineHeight;
				PositionToOffset(x, y, caret);
			}
			else
			{
				if (caretLine > linesPerPage)
					caretLine -= linesPerPage;
				else
					caretLine = 0;
				y = caretLine * mLineHeight;
				PositionToOffset(x, y, caret);
			}
			updateWalkOffset = false;
			break;

		case kcmd_ExtendSelectionToEndOfPage:
			mFastFindMode = false;
			if (caretLine < lastLine)
			{
				y = lastLine * mLineHeight;
				PositionToOffset(x, y, caret);
			}
			else
			{
				maxLine = lastLine + linesPerPage;
				if (maxLine >= mLineInfo.size())
					maxLine = mLineInfo.size() - 1;
				
				y = maxLine * mLineHeight;
				PositionToOffset(x, y, caret);
			}
			updateWalkOffset = false;
			break;

		case kcmd_ExtendSelectionToBeginningOfFile:
			mFastFindMode = false;
			caret = 0;
			updateWalkOffset = false;
			break;

		case kcmd_ExtendSelectionToEndOfFile:
			mFastFindMode = false;
			caret = mText.GetSize();
			updateWalkOffset = false;
			break;


		case kcmd_ScrollOneLineUp:
			eScroll(kScrollLineUp);
			scrollToCaret = false;
			updateSelection = false;
			updateWalkOffset = false;
			break;

		case kcmd_ScrollOneLineDown:
			eScroll(kScrollLineDown);
			scrollToCaret = false;
			updateSelection = false;
			updateWalkOffset = false;
			break;
		
		case kcmd_MoveLineUp:
			if (minLine > 0)
			{
				Select(mSelection.SelectLines(*this));

				anchor = mSelection.GetAnchor();
				caret = mSelection.GetCaret();
				
				minLine = mSelection.GetMinLine(*this);

				StartAction("Shift lines up");
			
				string txt;
				mText.GetText(LineStart(minLine - 1), LineStart(minLine) - LineStart(minLine - 1), txt);
				Delete(LineStart(minLine - 1), LineStart(minLine) - LineStart(minLine - 1));
				
				anchor -= txt.length();
				caret -= txt.length();
				
				Insert(caret, txt);

				mSelection.Set(anchor, caret);
				FinishAction();
				
				eScroll(kScrollToSelection);
				scrollToCaret = false;
				updateSelection = false;
				updateWalkOffset = false;
			}
			break;
		
		case kcmd_MoveLineDown:
			if (maxLine < mLineInfo.size() - 1)
			{
				Select(mSelection.SelectLines(*this));

				anchor = mSelection.GetAnchor();
				caret = mSelection.GetCaret();
				
				maxLine = mSelection.GetMaxLine(*this);

				StartAction("Shift lines down");
			
				string txt;
				mText.GetText(LineStart(maxLine + 1), LineStart(maxLine + 2) - LineStart(maxLine + 1), txt);
				Delete(LineStart(maxLine + 1), LineStart(maxLine + 2) - LineStart(maxLine + 1));
				
				Insert(anchor, txt);

				anchor += txt.length();
				caret += txt.length();

				mSelection.Set(anchor, caret);
				FinishAction();
				
				eScroll(kScrollToSelection);
				scrollToCaret = false;
				updateSelection = false;
				updateWalkOffset = false;
			}
			break;
		
		case kcmd_ScrollPageUp:
			eScroll(kScrollPageUp);
			scrollToCaret = false;
			updateSelection = false;
			updateWalkOffset = false;
			break;

		case kcmd_ScrollPageDown:
			eScroll(kScrollPageDown);
			scrollToCaret = false;
			updateSelection = false;
			updateWalkOffset = false;
			break;

		case kcmd_ScrollToStartOfFile:
			eScroll(kScrollToStart);
			scrollToCaret = false;
			updateSelection = false;
			updateWalkOffset = false;
			break;

		case kcmd_ScrollToEndOfFile:
			eScroll(kScrollToEnd);
			scrollToCaret = false;
			updateSelection = false;
			updateWalkOffset = false;
			break;

		default:
			handled = false;
			updateSelection = false;
			updateWalkOffset = false;
			break;
	}
	
	if (updateSelection)
		Select(anchor, caret);
	
	if (not updateWalkOffset)
		mWalkOffset = oldWalkOffset;
	
	if (scrollToCaret)
		eScroll(kScrollToCaret);
	
//	if (firstLine > 0)
//		--firstLine;
	
	if (handled and mTargetTextView != nil)
		mTargetTextView->ObscureCursor();
	
	return handled;
}

bool MDocument::HandleRawKeydown(
	uint32		inKeyValue,
	uint32		inModifiers)
{
	MKeyCommand keyCommand = kcmd_None;
	bool handled = false;

	if (MAcceleratorTable::EditKeysInstance().IsNavigationKey(
			inKeyValue, inModifiers, keyCommand))
	{
		HandleKeyCommand(keyCommand);
		handled = true;
	}
	else
	{
		switch (inKeyValue)
		{
			case GDK_KP_Enter:
				Reset();
				Execute();
				handled = true;
				break;		
	
			case GDK_Return:
				if (mFastFindMode)
					mFastFindMode = false;
				else if (inModifiers & GDK_MOD2_MASK or inModifiers & GDK_CONTROL_MASK)
				{
					Reset();
					Execute();
					handled = true;
	//				updateSelection = false;
				}
				else
				{
					// Enter a return, optionally auto indented
					uint32 minOffset = mSelection.GetMinOffset(*this);
	
					string s;
					s += '\n';
	
					if (gAutoIndent)
					{
						uint32 line = OffsetToLine(minOffset);
						MTextBuffer::iterator c = mText.begin() + LineStart(line);
						while (c.GetOffset() < minOffset and IsSpace(*c))
						{
							s += *c;
							++c;
						}
					}
					
					if (gSmartIndent and
						mLanguage and
						mLanguage->IsSmartIndentLocation(mText, minOffset))
					{
						if (gTabEntersSpaces)
						{
							uint32 o = gSpacesPerTab - (OffsetToColumn(minOffset) % gSpacesPerTab);
							while (o-- > 0)
								s += ' ';
						}
						else
							s += '\t';
					}
	
					Type(s.c_str(), s.length());
				}
				handled = true;
				break;
			
			case GDK_Tab:
				if (inModifiers == 0)
				{
					if (not mSelection.IsEmpty() and
						not mFastFindMode and
						mSelection.GetMinLine(*this) != mSelection.GetMaxLine(*this))
					{
						if (inModifiers & GDK_SHIFT_MASK)
							DoShiftLeft();
						else
							DoShiftRight();
						handled = true;
					}
					else
					{
						Type("\t", 1);
						handled = true;
					}
				}
				break;
			
			case GDK_Escape:
				mFastFindMode = false;
	//			updateSelection = false;
				if (mShell.get() != nil and mShell->IsRunning())
					mShell->Kill();
				handled = true;
				break;
			
			case GDK_period:
				if (inModifiers & GDK_CONTROL_MASK and mShell.get() != nil and mShell->IsRunning())
					mShell->Kill();
				else
					handled = false;
				break;
			
			default:
				handled = false;
	//			updateSelection = false;
				break;
		}
	
		if (not handled and keyCommand != kcmd_None)
			handled = HandleKeyCommand(keyCommand);
	}
	
	return handled;
}

//OSStatus MDocument::DoTextInputOffsetToPos(EventRef ioEvent)
//{
//	OSStatus err = eventNotHandledErr;
//
//	SInt32 textOffset;
//	::GetEventParameter(ioEvent, kEventParamTextInputSendTextOffset,
//		typeSInt32, nil, sizeof(textOffset), nil, &textOffset);
//	
//	if (textOffset == 0)
//	{
//		uint32 line;
//		int32 x;
//		OffsetToPosition(mSelection.GetCaret(), line, x);
//		
//		HIPoint pt;
//		pt.x = x;
//		pt.y = mLineHeight * line;
//		
//		mTargetTextView->ConvertToGlobal(pt);
//		
//		err = ::SetEventParameter(ioEvent, kEventParamTextInputReplyPoint,
//			typeHIPoint, sizeof(pt), &pt);
//	}
//	
//	return err;
//}
//
//OSStatus MDocument::DoTextInputUpdateActiveInputArea(EventRef ioEvent)
//{
//	OSStatus err = eventNotHandledErr;
//	
//	try
//	{
//		uint32 size;
//		int32 fix = 0;
//		UniChar buf[256];
//
//		(void)::GetEventParameter(ioEvent, kEventParamTextInputSendFixLen,
//				typeLongInteger, nil, sizeof(long), nil, &fix);
//		fix /= 2;
//
//		THROW_IF_OSERROR(::GetEventParameter(ioEvent, kEventParamTextInputSendText,
//			typeUnicodeText, nil, sizeof(buf), &size, buf));
//		size /= 2;
//		
//		struct MyTextRangeArray
//		{
//		    short		fNumOfRanges;	/* specify the size of the fRange array */
//		    TextRange	fRange[9];		/* when fNumOfRanges > 1, the size of this array has to be calculated */
//		};
//
//		MyTextRangeArray tra;
//		tra.fNumOfRanges = 0;
//
//		(void)::GetEventParameter(ioEvent, kEventParamTextInputSendHiliteRng, typeTextRangeArray,
//			nil, sizeof(MyTextRangeArray), nil, &tra);
//
////		StHideCaret hide(fTargetTextView);
////	
//		for (int i = kCaretPosition; i <= kSelectedText; ++i)
//			mTextInputAreaInfo.fOffset[i] = -1;
//
//		StartAction(kTypeAction);
//	
//		if (mTextInputAreaInfo.fOffset[kActiveInputArea] >= 0)
//		{
//			Delete(mTextInputAreaInfo.fOffset[kActiveInputArea],
//				mTextInputAreaInfo.fLength[kActiveInputArea]);
//		}
//	
//		int insertAt = mSelection.GetCaret();
//	
//		if (size > 0)
//		{
//			Insert(insertAt, buf, size);
//
//			MSelection s = mText.GetSelectionAfter();
//			s.SetCaret(mSelection.GetCaret());
//			mText.SetSelectionAfter(s);
//	
//			if (fix == -1 or fix == static_cast<int32>(size))
//				mTextInputAreaInfo.fOffset[kActiveInputArea] = -1;
//			else
//			{
//				mTextInputAreaInfo.fOffset[kActiveInputArea] = insertAt;
//				mTextInputAreaInfo.fLength[kActiveInputArea] = size;
//		
//				for (int i = 0; i < tra.fNumOfRanges; ++i)
//				{
//					TextRangePtr trp = &tra.fRange[i];
//					if (trp->fStart >= 0)
//					{
//						mTextInputAreaInfo.fOffset[trp->fHiliteStyle] = insertAt + trp->fStart / 2;
//						mTextInputAreaInfo.fLength[trp->fHiliteStyle] = (trp->fEnd - trp->fStart) / 2;
//					}
//				}
//		
//				if (mTextInputAreaInfo.fOffset[kSelectedText] >= 0)
//				{
//					ChangeSelection(mTextInputAreaInfo.fOffset[kSelectedText],
//						mTextInputAreaInfo.fOffset[kSelectedText] +
//						mTextInputAreaInfo.fLength[kSelectedText]);
//				}
//				else if (mTextInputAreaInfo.fOffset[kCaretPosition] >= 0)
//				{
//					ChangeSelection(mTextInputAreaInfo.fOffset[kCaretPosition],
//						mTextInputAreaInfo.fOffset[kCaretPosition]);
//				}
//			}
//		}
//		else
//			mTextInputAreaInfo.fOffset[kActiveInputArea] = -1;
//
//		TouchLine(OffsetToLine(insertAt));
//		UpdateDirtyLines();
//		eScroll(kScrollToCaret);
//		
//		uint32 line;
//		OffsetToPosition(mSelection.GetCaret(), line, mWalkOffset);
//		
//		err = noErr;
//	}
//	catch (exception& e)
//	{
//		MError::DisplayError(e);
//	}
//
//	return err;
//}

void MDocument::PrefsChanged()
{
	ReInit();
	Rewrap();
	UpdateDirtyLines();
}

void MDocument::ShellStatusIn(bool inActive)
{
	eShellStatus(inActive);
}

void MDocument::StdOut(const char* inText, uint32 inSize)
{
	if (not mPreparedForStdOut)
	{
		StartAction(kTypeAction);
		
		uint32 line = mSelection.GetMaxLine(*this);
		ChangeSelection(LineEnd(line), LineEnd(line));
		
		Type("\n", 1);
		mPreparedForStdOut = true;
	}
	
	Type(inText, inSize);
	
	eLineCountChanged();
	eScroll(kScrollToCaret);
}

void MDocument::StdErr(const char* inText, uint32 inSize)
{
	if (mStdErrWindow == nil)
	{
		mStdErrWindow = new MMessageWindow;
		AddRoute(mStdErrWindow->eWindowClosed, eMsgWindowClosed);
	}

	mStdErrWindow->SetBaseDirectory(MPath(mShell->GetCWD()));
	
//	StdOut(inText, inSize);

	mStdErrWindow->AddStdErr(inText, inSize);
}

void MDocument::MsgWindowClosed(MWindow* inWindow)
{
	assert(inWindow == mStdErrWindow);
	mStdErrWindow = nil;
}

void MDocument::Execute()
{
	FinishAction();

	if (mSelection.IsEmpty())
	{
		uint32 line = mSelection.GetMinLine(*this);
		ChangeSelection(LineStart(line), LineEnd(line));
	}
	
	string s;
	GetSelectedText(s);
	
	if (mShell.get() == nil)
	{
		mShell.reset(new MShell(true));

		if (IsSpecified())
		{
			char cwd[1024] = { 0 };
			ssize_t size = read_attribute(mURL.GetPath(), kJapieCWD, cwd, sizeof(cwd));
			if (size > 0)
			{
				string d(cwd, size);
				mShell->SetCWD(d);
			}
		}
		
		AddRoute(mShell->eStdOut, eStdOut);
		AddRoute(mShell->eStdErr, eStdErr);
		AddRoute(mShell->eShellStatus, eShellStatusIn);
	}

	mPreparedForStdOut = false;
	
	if (mStdErrWindow != nil)
		mStdErrWindow->ClearList();
	
	mShell->Execute(s);
}

MController* MDocument::GetFirstController() const
{
	MController* controller = nil;
	
	if (mTargetTextView != nil)
		controller = mTargetTextView->GetController();
	
	if (controller == nil)
		controller = mControllers.front();
	
	return controller;
}

void MDocument::AddNotifier(
	MDocClosedNotifier&		inNotifier)
{
	mNotifiers.push_back(inNotifier);
}

void MDocument::Idle(
	double		inSystemTime)
{
	if (mNeedReparse)
	{
		if (mLanguage and mNamedRange)
		{
			mLanguage->Parse(mText, *mNamedRange, *mIncludeFiles);
			SendSelectionChangedEvent();
		}
		
		mNeedReparse = false;
	}
}

// ---------------------------------------------------------------------------
//	MDocument::GetParsePopupItems

bool MDocument::GetParsePopupItems(MMenu& inMenu)
{
	bool result = false;
	uint32 ix = 0;
	
	if (mLanguage and mNamedRange)
	{
		if (mNeedReparse)
			Idle(0);		// force reparse

		mLanguage->GetParsePopupItems(*mNamedRange, "", inMenu, ix);
		result = true;
	}
	
	return result;
}

// ---------------------------------------------------------------------------
//	MDocument::SelectParsePopupItem

void MDocument::SelectParsePopupItem(uint32 inItem)
{
	if (mLanguage and mNamedRange)
	{
		uint32 anchor, caret;
		if (mLanguage->GetSelectionForParseItem(*mNamedRange, inItem, anchor, caret))
		{
			Select(anchor, caret);
			eScroll(kScrollCenterSelection);
		}
	}
}

// ---------------------------------------------------------------------------
//	MDocument::GetIncludePopupItems

bool MDocument::GetIncludePopupItems(MMenu& inMenu)
{
	bool result = false;
	
	if (mLanguage and mIncludeFiles)
	{
		if (mNeedReparse)
			Idle(0);		// force reparse

		for (MIncludeFileList::iterator i = mIncludeFiles->begin(); i != mIncludeFiles->end(); ++i)
			inMenu.AppendItem(i->name, cmd_OpenIncludeFromMenu);
		
		result = mIncludeFiles->size() > 0;
	}
	
	return result;
}

// ---------------------------------------------------------------------------
//	MDocument::SelectIncludePopupItem

void MDocument::SelectIncludePopupItem(uint32 inItem)
{
	if (mLanguage)
	{
		MIncludeFile file = mIncludeFiles->at(inItem);

		if (mSpecified and mURL.IsLocal())
		{
			MPath path = mURL.GetPath().branch_path() / file.name;
			if (fs::exists(path))
				gApp->OpenOneDocument(MUrl(path));
		}

		MProject* project = MProject::Instance();
		MPath p;
		
		if (project != nil and project->LocateFile(file.name, file.isQuoted, p))
			gApp->OpenOneDocument(MUrl(p));
	}
}

// ---------------------------------------------------------------------------
//	MDocument::SelectIncludePopupItem

void MDocument::NotifyPut(
	bool		inPutting)
{
	if (inPutting)
		++mPutCount;
	else
		--mPutCount;
}
