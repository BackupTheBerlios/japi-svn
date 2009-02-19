//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "MJapi.h"

#include <limits>
#include <cmath>
#include <fcntl.h>
#include <iostream>

#include <gdk/gdkkeysyms.h>

#include <boost/functional/hash.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/iostreams/filtering_stream.hpp>

#include "MTextDocument.h"
#include "MTextView.h"
#include "MClipboard.h"
#include "MUnicode.h"
#include "MGlobals.h"
#include "MLanguage.h"
#include "MFindDialog.h"
#include "MError.h"
#include "MController.h"
#include "MStyles.h"
#include "MPreferences.h"
#include "MPrefsDialog.h"
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
#include "MStrings.h"
#include "MAcceleratorTable.h"
#include "MSound.h"
#include "MAlerts.h"
#include "MDiffWindow.h"
#include "MJapiApp.h"
#include "MPrinter.h"

using namespace std;
namespace io = boost::iostreams;

MTextDocument* MTextDocument::sWorksheet;

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
	kJapieDocState[] = "com.hekkelman.japi.State",
	kJapieCWD[] = "com.hekkelman.japi.CWD";

const uint32
	kMDocStateSize = 36;	// sizeof(MDocState)

}

// ---------------------------------------------------------------------------
//	MDocState

void MDocState::Swap()
{
	net_swapper swap;
	
	mSelection[0] = swap(mSelection[0]);
	mSelection[1] = swap(mSelection[1]);
	mSelection[2] = swap(mSelection[2]);
	mSelection[3] = swap(mSelection[3]);
	mScrollPosition[0] = swap(mScrollPosition[0]);
	mScrollPosition[1] = swap(mScrollPosition[1]);
	mWindowPosition[0] = swap(mWindowPosition[0]);
	mWindowPosition[1] = swap(mWindowPosition[1]);
	mWindowSize[0] = swap(mWindowSize[0]);
	mWindowSize[1] = swap(mWindowSize[1]);
	mSwapHelper = swap(mSwapHelper);
}

// ---------------------------------------------------------------------------
//	MTextDocument

MTextDocument::MTextDocument(
	const MFile&		inFile)
	: MDocument(inFile)
	, eBoundsChanged(this, &MTextDocument::BoundsChanged)
	, ePrefsChanged(this, &MTextDocument::PrefsChanged)
	, eMsgWindowClosed(this, &MTextDocument::MsgWindowClosed)
	, eIdle(this, &MTextDocument::Idle)
	, mSelection(this)
{
	Init();
	
	AddRoute(ePrefsChanged, MPrefsDialog::ePrefsChanged);
	AddRoute(eIdle, gApp->eIdle);
	
	ReInit();
}

MTextDocument::MTextDocument()
	: MDocument(MFile())
	, eBoundsChanged(this, &MTextDocument::BoundsChanged)
	, ePrefsChanged(this, &MTextDocument::PrefsChanged)
	, eMsgWindowClosed(this, &MTextDocument::MsgWindowClosed)
	, eIdle(this, &MTextDocument::Idle)
	, mSelection(this)
{
	Init();
}

void MTextDocument::Init()
{
	mTargetTextView = nil;
	mWrapWidth = 0;
	mWalkOffset = 0;
	mLastAction = kNoAction;
	mCurrentAction = kNoAction;
	mLanguage = nil;
	mNamedRange = nil;
	mIncludeFiles = nil;
	mNeedReparse = false;
	mSoftwrap = false;
	mShowWhiteSpace = false;
	mFastFindMode = false;
	mCompletionIndex = -1;
	mStdErrWindow = nil;
	mPCLine = numeric_limits<uint32>::max();
	mDataFD = -1;
 
	mCharsPerTab = gCharsPerTab;

	mLineInfo.push_back(MLineInfo());

	for (int i = kActiveInputArea; i <= kSelectedText; ++i)
		mTextInputAreaInfo.fOffset[i] = -1;
}

MTextDocument::~MTextDocument()
{
	if (sWorksheet == this)
		sWorksheet = nil;
	
	delete mNamedRange;
	delete mIncludeFiles;
	
	eDocumentClosed(this);
}

MTextDocument* MTextDocument::GetFirstTextDocument()
{
	MDocument* doc = GetFirstDocument();
	
	while (doc != nil and dynamic_cast<MTextDocument*>(doc) == nil)
		doc = doc->GetNextDocument();
	
	return dynamic_cast<MTextDocument*>(doc);
}

void MTextDocument::SetFileNameHint(
	const string&	inNameHint)
{
	MDocument::SetFile(MFile(fs::path(inNameHint)));
	
	delete mNamedRange;
	mNamedRange = nil;
	
	delete mIncludeFiles;
	mIncludeFiles = nil;

	mLanguage = MLanguage::GetLanguageForDocument(inNameHint, mText);
	if (mLanguage != nil)
	{
		mNamedRange = new MNamedRange;
		mIncludeFiles = new MIncludeFileList;
	}
	
	Rewrap();
}

bool MTextDocument::DoSave()
{
	bool result = MDocument::DoSave();
	MProject::RecheckFiles();
	return result;
}

bool MTextDocument::DoSaveAs(
	const MFile&		inFile)
{
	bool result = false;

	if (MDocument::DoSaveAs(inFile))
	{
		if (mLanguage == nil)
		{
			mLanguage = MLanguage::GetLanguageForDocument(
							mFile.GetFileName(), mText);
	
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

void MTextDocument::AddNotifier(
	MDocClosedNotifier&		inNotifier,
	bool					inRead)
{
	MDocument::AddNotifier(inNotifier, inRead);
	
	if (inRead)
	{
		mPreparedForStdOut = true;
		mDataFD = inNotifier.GetFD();

		int flags = fcntl(mDataFD, F_GETFL, 0);
		if (fcntl(mDataFD, F_SETFL, flags | O_NONBLOCK))
			cerr << _("Failed to set fd non blocking: ") << strerror(errno) << endl;
	}
}

MController* MTextDocument::GetFirstController() const
{
	MController* controller = nil;
	
	if (mTargetTextView != nil)
		controller = mTargetTextView->GetController();
	else
		controller = MDocument::GetFirstController();
	
	return controller;
}

// ---------------------------------------------------------------------------
//	SetModified

void MTextDocument::SetModified(bool inModified)
{
	MDocument::SetModified(inModified);
	
	if (inModified)
		mNeedReparse = true;
}

// ---------------------------------------------------------------------------
//	ReadFile, the hard work

void MTextDocument::ReadFile(
	istream&		inFile)
{
	mText.ReadFromFile(inFile);
	
	mLanguage = MLanguage::GetLanguageForDocument(mFile.GetFileName(), mText);
	
	if (mLanguage != nil)
	{
		mSoftwrap = mLanguage->Softwrap();

		if (mNamedRange == nil)
			mNamedRange = new MNamedRange;
		
		if (mIncludeFiles == nil)
			mIncludeFiles = new MIncludeFileList;
	}

	mNeedReparse = true;

	ReInit();
	Rewrap();
	UpdateDirtyLines();
	
	eSSHProgress(-1.f, "");
}

// ---------------------------------------------------------------------------
//	WriteFile

void MTextDocument::WriteFile(
	ostream&		inFile)
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

	mText.WriteToFile(inFile);
}

// ---------------------------------------------------------------------------
//	SaveState

void MTextDocument::SaveState()
{
	MDocState state = { };

	mFile.ReadAttribute(kJapieDocState, &state, kMDocStateSize);
	
	state.Swap();
	
	if (mSelection.IsBlock())
	{
		mSelection.GetAnchorLineAndColumn(
			state.mSelection[0], state.mSelection[1]);
		mSelection.GetCaretLineAndColumn(
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
	
	mFile.WriteAttribute(kJapieDocState, &state, kMDocStateSize);
	
	if (mShell.get() != nil)
	{
		string cwd = mShell->GetCWD();
		
		mFile.WriteAttribute(kJapieCWD, cwd.c_str(), cwd.length());
			
		if (IsWorksheet())
			Preferences::SetString("worksheet wd", cwd);
	}
}

void MTextDocument::SetText(
	const char*		inText,
	uint32			inTextLength)
{
	mText.SetText(inText, inTextLength);
	
	ReInit();

	mLanguage = MLanguage::GetLanguageForDocument(mFile.GetFileName(), mText);
	if (mLanguage != nil)
	{
		mNamedRange = new MNamedRange;
		mIncludeFiles = new MIncludeFileList;
	}

	Rewrap();
}

// --------------------------------------------------------------------
//	SetTargetTextView

void MTextDocument::SetTargetTextView(
	MTextView*		inTextView)
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

// --------------------------------------------------------------------

bool MTextDocument::IsWorksheet() const
{
	return sWorksheet == this;
}

// --------------------------------------------------------------------

MDocument* MTextDocument::GetWorksheet()
{
	return sWorksheet;
}

// --------------------------------------------------------------------

void MTextDocument::SetWorksheet(
	MTextDocument*		inDocument)
{
	if (sWorksheet != inDocument)
	{
		if (sWorksheet != nil)
			sWorksheet->SetWorksheet(false);
	
		sWorksheet = inDocument;
	
		string cwd = Preferences::GetString("worksheet wd", "");
		if (cwd.length() > 0)
		{
			inDocument->mShell.reset(new MShell(true));

			inDocument->mShell->SetCWD(cwd);

			SetCallback(inDocument->mShell->eStdOut, inDocument, &MTextDocument::StdOut);
			SetCallback(inDocument->mShell->eStdErr, inDocument, &MTextDocument::StdErr);
			SetCallback(inDocument->mShell->eShellStatus, inDocument, &MTextDocument::ShellStatusIn);
		}
	}		
}

// --------------------------------------------------------------------

const char* MTextDocument::GetCWD() const
{
	const char* result = nil;
	if (mShell.get() != nil)
		result = mShell->GetCWD().c_str();
	else if (mFile.IsValid())
	{
		static auto_array<char> cwd(new char[PATH_MAX]);

		int32 r = mFile.ReadAttribute(kJapieCWD, cwd.get(), PATH_MAX);
		if (r > 0 and r < PATH_MAX)
		{
			cwd.get()[r] = 0;
			result = cwd.get();
		}
	}
	return result;
}

bool MTextDocument::StopRunningShellCommand()
{
	bool result = false;

	if (mShell.get() != nil and mShell->IsRunning())
	{
		mShell->Kill();
		result = true;
	}
	
	return result;
}

void MTextDocument::Reset()
{
	if (mCurrentAction != kNoAction)
		FinishAction();

	mCompletionIndex = -1;
	mFastFindMode = false;
	
	for (int i = kActiveInputArea; i <= kSelectedText; ++i)
		mTextInputAreaInfo.fOffset[i] = -1;
}

// ---------------------------------------------------------------------------
//	ReInit, reset all font related member values

void MTextDocument::ReInit()
{
	mFont = Preferences::GetString("font", "monospace 9");
	
	MDevice device;
	
	device.SetFont(mFont);

	mLineHeight = device.GetLineHeight();
	mCharWidth = device.GetStringWidth("          ") / 10;
	mTabWidth = mCharWidth * mCharsPerTab;
}

// ---------------------------------------------------------------------------
//	SetLanguage

void MTextDocument::SetLanguage(
	const string&		inLanguage)
{
	mLanguage = MLanguage::GetLanguage(inLanguage);

	ReInit();
	Rewrap();
	UpdateDirtyLines();
}

// ---------------------------------------------------------------------------
//	SetCharsPerTab

void MTextDocument::SetCharsPerTab(
	uint32				inCharsPerTab)
{
	mCharsPerTab = inCharsPerTab;
	
	ReInit();
	Rewrap();
	UpdateDirtyLines();
}

// ---------------------------------------------------------------------------
//	SetDocInfo

void MTextDocument::SetDocInfo(
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

bool MTextDocument::ReadDocState(
	MDocState&		ioDocState)
{
	bool result = false;
	
	if (IsSpecified() and Preferences::GetInteger("save state", 1))
	{
		ssize_t r = mFile.ReadAttribute(kJapieDocState, &ioDocState, kMDocStateSize);
		if (r > 0 and static_cast<uint32>(r) == kMDocStateSize)
		{
			ioDocState.Swap();

			if (ioDocState.mFlags.mSelectionIsBlock)
			{
				if (ioDocState.mSelection[0] <= mLineInfo.size() and
					ioDocState.mSelection[2] <= mLineInfo.size() and
					ioDocState.mSelection[1] <= 1000 and
					ioDocState.mSelection[3] <= 1000)
				{
					mSelection.Set(ioDocState.mSelection[0], ioDocState.mSelection[1],
						ioDocState.mSelection[2], ioDocState.mSelection[3]);
				}
			}
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

uint32 MTextDocument::FindWord(
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

void MTextDocument::FindWord(uint32 inOffset, uint32& outMinAnchor, uint32& outMaxAnchor)
{
	outMaxAnchor = mText.NextCursorPosition(inOffset, eMoveOneWordForKeyboard);
	outMinAnchor = mText.PreviousCursorPosition(outMaxAnchor, eMoveOneWordForKeyboard);
	
	if (outMinAnchor > inOffset or outMaxAnchor < inOffset)
		outMinAnchor = outMaxAnchor = inOffset;
}

// ---------------------------------------------------------------------------
//	HandleDeleteKey

void MTextDocument::HandleDeleteKey(MDirection inDirection)
{
//	FinishAction();
	StartAction(kTypeAction);
	
	if (mSelection.IsEmpty())
	{
		uint32 caret = mSelection.GetCaret();
		uint32 anchor = caret;
		
		if ((caret > 0 and inDirection == kDirectionBackward) or
			(caret < mText.GetSize() and inDirection == kDirectionForward))
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
	
		ChangeSelection(MSelection(this, anchor, anchor));
	}
	else
		DeleteSelectedText();

	uint32 line;
	OffsetToPosition(mSelection.GetCaret(), line, mWalkOffset);
	SendSelectionChangedEvent();

	UpdateDirtyLines();
}

// ---------------------------------------------------------------------------
// DeleteSelectedText

void MTextDocument::DeleteSelectedText()
{
	if (mSelection.IsBlock())
	{
		uint32 anchor = 0;
		
		uint32 minLine = mSelection.GetMinLine();
		uint32 maxLine = mSelection.GetMaxLine();
		
		uint32 minColumn = mSelection.GetMinColumn();
		uint32 maxColumn = mSelection.GetMaxColumn();
		
		for (uint32 line = minLine; line <= maxLine; ++line)
		{
			uint32 start = LineColumnToOffsetBreakingTabs(line, minColumn, false);
			uint32 end = LineColumnToOffsetBreakingTabs(line, maxColumn, false);
			
			Delete(start, end - start);
			
			if (line == minLine)
				anchor = start;
		}

		ChangeSelection(MSelection(this, anchor, anchor));
	}
	else
	{
		uint32 anchor = mSelection.GetAnchor();
		uint32 caret = mSelection.GetCaret();
		
		if (anchor > caret)
			swap(anchor, caret);
	
		if (anchor != caret)
			Delete(anchor, caret - anchor);
	
		ChangeSelection(MSelection(this, anchor, anchor));
	}
}

// ---------------------------------------------------------------------------
// ReplaceSelectedText

void MTextDocument::ReplaceSelectedText(
	const string&		inText,
	bool				isBlock,
	bool				inSelectPastedText)
{
	DeleteSelectedText();
	
	uint32 anchor = mSelection.GetAnchor();
	
	if (isBlock)
	{
		string::const_iterator s = inText.begin();
		
		uint32 anchor = mSelection.GetAnchor();
		uint32 line = OffsetToLine(anchor);
		uint32 column = OffsetToColumn(anchor);
		uint32 caret = anchor;
		
		while (s != inText.end())
		{
			string::const_iterator e = s;
			while (e != inText.end() and *e != '\n')
				++e;
			
			if (line >= mLineInfo.size())
				Insert(mText.GetSize(), "\n", 1);
			
			uint32 offset = LineColumnToOffsetBreakingTabs(line, column, true);
			Insert(offset, string(s, e));
			caret = offset + (e - s);
			
			if (e == inText.end())	// should never happen
				break;

			s = e + 1;
			++line;
		}
		
		if (inSelectPastedText)
		{
			ChangeSelection(MSelection(this,
				OffsetToLine(anchor), column,
				line - 1, OffsetToColumn(caret)));
		}
		else
			ChangeSelection(caret, caret);
	}
	else
	{
		Insert(anchor, inText);
		
		uint32 caret = anchor + inText.length();
		
		if (inSelectPastedText)
			ChangeSelection(MSelection(this, anchor, caret));
		else
			ChangeSelection(caret, caret);
	}
}

// ---------------------------------------------------------------------------
// LineColumnToOffsetBreakingTabs

uint32 MTextDocument::LineColumnToOffsetBreakingTabs(
	uint32		inLine,
	uint32		inColumn,
	bool		inAddSpacesExtendingLine)
{
	MTextBuffer::iterator text(&mText, LineStart(inLine));
	MTextBuffer::iterator end(&mText, LineEnd(inLine));
	
	uint32 column = 0;
	
	while (column < inColumn and text < end)
	{
		if (*text == '\t')
		{
			uint32 d = mCharsPerTab - (column % mCharsPerTab);
			
			if (column + d > inColumn)
			{
				// we have to split this tab
				Delete(text.GetOffset(), 1);
				vector<char> s(d, ' ');
				Insert(text.GetOffset(), &s[0], d);
				
				text += inColumn - column;
				column = inColumn;
				break;
			}
			else
				column += d;
		}
		else
			++column;
		++text;
	}
	
	if (inAddSpacesExtendingLine and column < inColumn)
	{
		uint32 d = inColumn - column;
		vector<char> s(d, ' ');
		Insert(text.GetOffset(), &s[0], d);
		
		text += d;
	}
	
	return text.GetOffset();
}

// ---------------------------------------------------------------------------
// Type

void MTextDocument::Type(
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
	ChangeSelection(MSelection(this, offset + inLength, offset + inLength));
	mText.SetSelectionAfter(s);

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
			ChangeSelection(MSelection(this, offset - 1, offset));
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

void MTextDocument::Drop(
	uint32			inOffset,
	const char*		inText,
	uint32			inSize,
	bool			inDragMove)
{
	StartAction(kDropAction);
	
	if (inDragMove)
	{
		uint32 start = mSelection.GetMinOffset();
		uint32 length = mSelection.GetMaxOffset() - start;
		
		Delete(start, length);
		if (inOffset > mSelection.GetMaxOffset())
			inOffset -= length;
	}
	
	Insert(inOffset, inText, inSize);
	ChangeSelection(inOffset, inOffset + inSize);

	FinishAction();

	SendSelectionChangedEvent();
}

// ---------------------------------------------------------------------------
//	MarkLine

void MTextDocument::MarkLine(
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

void MTextDocument::MarkMatching(
	const string&	inMatch,
	bool			inIgnoreCase,
	bool			inRegEx)
{
	MSelection savedSelection(mSelection);
	
	MSelection found(this);
	uint32 offset = 0;
	
	while (mText.Find(offset, inMatch, kDirectionForward, inIgnoreCase, inRegEx, found))
	{
		uint32 line = found.GetMinLine();

		mLineInfo[line].marked = true;
		mLineInfo[line].dirty = true;

		offset = LineStart(line + 1);
	}
	
	mSelection = savedSelection;
	mSelection.SetDocument(this);
	
	eInvalidateDirtyLines();
}

// ---------------------------------------------------------------------------
//	ClearAllMarkers

void MTextDocument::ClearAllMarkers()
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

void MTextDocument::ClearBreakpointsAndStatements()
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

void MTextDocument::SetIsStatement(
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

void MTextDocument::SetIsBreakpoint(
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

void MTextDocument::SetPCLine(
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

void MTextDocument::CCCMarkedLines(bool inCopy, bool inClear)
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
		
		uint32 offset = mSelection.GetMinOffset();
		
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
		
		ChangeSelection(MSelection(this, offset, offset));
		
		FinishAction();
	}
}

// ---------------------------------------------------------------------------
//	ClearDiffs

void MTextDocument::ClearDiffs()
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

void MTextDocument::DoJumpToNextMark(MDirection inDirection)
{
	uint32 currentLine;
	if (inDirection == kDirectionForward)
		currentLine = OffsetToLine(mSelection.GetMaxOffset());
	else
		currentLine = OffsetToLine(mSelection.GetMinOffset());
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

void MTextDocument::GoToLine(
	uint32	inLineNr)
{
	if (inLineNr < CountLines())
		Select(LineStart(inLineNr), LineStart(inLineNr + 1), kScrollToSelection);
}

// ---------------------------------------------------------------------------
//	TouchLine

void MTextDocument::TouchLine(
	uint32		inLineNr,
	bool		inDirty)
{
	assert(inLineNr < mLineInfo.size());
	if (inLineNr < mLineInfo.size())
		mLineInfo[inLineNr].dirty = inDirty;
}

// ---------------------------------------------------------------------------
//	TouchLine

void MTextDocument::TouchAllLines()
{
	for (uint32 line = 0; line < mLineInfo.size(); ++line)
		mLineInfo[line].dirty = true;
}

void MTextDocument::SetSoftwrap(bool inSoftwrap)
{
	if (mSoftwrap != inSoftwrap)
	{
		mSoftwrap = inSoftwrap;
		Rewrap();
	}
}

bool MTextDocument::GetSoftwrap() const
{
	return mSoftwrap and mWrapWidth > 0;
}

uint32 MTextDocument::GetIndent(uint32 inOffset) const
{
	uint32 indent = 0;
	
	uint32 maxWidth = numeric_limits<uint32>::max();
	if (mWrapWidth > 0)
		maxWidth = mWrapWidth;

	for (MTextBuffer::const_iterator i(&mText, inOffset); i != mText.end(); ++i)
	{
		char ch = *i;
		
		if (ch == '\t')
		{
			uint32 d = mCharsPerTab - (indent % mCharsPerTab);
			indent += d;
		}
		else if (ch == ' ')
			++indent;
		else
			break;

		if ((indent + 1) * mCharWidth >= maxWidth)
		{
			indent = 0;
			break;
		}
	}
	
	return indent;
}

uint32 MTextDocument::GetLineIndent(uint32 inLine) const
{
	// short cut
	if (not GetSoftwrap() or mLineInfo[inLine].nl or inLine == 0)
		return 0;
	
	while (inLine > 0 and not mLineInfo[inLine].nl)
		--inLine;
	
	return GetIndent(mLineInfo[inLine].start);
//	return OffsetToColumn(mLineInfo[inLine].start);
}

uint32 MTextDocument::GetLineIndentWidth(uint32 inLine) const
{
	return GetLineIndent(inLine) * mCharWidth;
}

// ---------------------------------------------------------------------------
//	FindLineBreak

uint32 MTextDocument::FindLineBreak(
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
	
	if (GetSoftwrap() and mWrapWidth > 0 and result > inFromOffset + 1)
	{
		uint32 width = mWrapWidth;

		width -= inIndent * mCharWidth;

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

int32 MTextDocument::RewrapLines(
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

	if (lineInfoStart->nl == false and lineInfoStart != mLineInfo.begin())
		--lineInfoStart;
	
	MLineInfoArray::iterator lineInfoEnd = lineInfoStart + 1;

	while (lineInfoEnd != mLineInfo.end() and lineInfoEnd->start < inTo)
		++lineInfoEnd;

	while (lineInfoEnd != mLineInfo.end() and lineInfoEnd->nl == false)
		++lineInfoEnd;
	
	if (lineInfoEnd == mLineInfo.end())
		inTo = mText.GetSize();
	else
		inTo = lineInfoEnd->start - 1;
	
	// start by marking the first line dirty
	
	lineInfoStart->dirty = true;
	
	// now if we have more than one line we will erase the old info
	int32 cnt = lineInfoEnd - lineInfoStart;
	if (cnt > 1)
	{
		for (MLineInfoArray::iterator i = lineInfoStart; i != lineInfoEnd; ++i)
		{
			if (i->marked)
				markOffsets.push_back(i->start);
		}
		
		mLineInfo.erase(lineInfoStart + 1, lineInfoEnd);
	}
	cnt = 1 - cnt;
	
	lineInfoEnd = lineInfoStart + 1;
	
	if (mLanguage and lineInfoStart == mLineInfo.begin())
		lineInfoStart->state = mLanguage->GetInitialState(mFile.GetFileName(), mText);
	
	uint16 state = lineInfoStart->state;
	uint32 start = lineInfoStart->start;
	
	uint32 indent = 0;

	bool isHardBreak = lineInfoStart->nl or lineInfoStart == mLineInfo.begin();
	
	if (GetSoftwrap())
	{
		MLineInfoArray::iterator li = lineInfoStart;
		while (li != mLineInfo.begin() and li->nl == false)
			--li;
		indent = GetIndent(li->start);
	}
	
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
			lineInfoEnd->indent = true;

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

void MTextDocument::BoundsChanged()
{
	mWrapWidth = mTargetTextView->GetWrapWidth();	
	
	if (GetSoftwrap())
		Rewrap();
}

void MTextDocument::SetWrapWidth(
	uint32			inWrapWidth)
{
	if (mWrapWidth != inWrapWidth)
	{
		mWrapWidth = inWrapWidth;
		Rewrap();
	}
}

// ---------------------------------------------------------------------------
//	Rewrap

void MTextDocument::Rewrap()
{
	RewrapLines(0, mText.GetSize());
	eLineCountChanged();
}

// ---------------------------------------------------------------------------
//  RestyleDirtyLines

void MTextDocument::RestyleDirtyLines(
	uint32	inFrom)
{
	assert(mLanguage != nil);

	for (uint32 line = inFrom; line < mLineInfo.size(); ++line)
	{
		if (mLineInfo[line].dirty)
		{
			uint16 state;
			if (line == 0)
				state = mLanguage->GetInitialState(mFile.GetFileName(), mText);
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

void MTextDocument::GetLine(
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

void MTextDocument::UpdateDirtyLines()
{
	if (mLanguage != nil)
		RestyleDirtyLines(0);

	eInvalidateDirtyLines();
	
	for (uint32 line = 0; line < mLineInfo.size(); ++line)
		mLineInfo[line].dirty = false;
}

void MTextDocument::GetSelectionRegion(
	MRegion&		outRegion) const
{
	if (mSelection.IsBlock())
	{
		MRect r;
		
		r.y = mSelection.GetMinLine() * mLineHeight;
		r.height = mSelection.CountLines() * mLineHeight;
		r.x = mSelection.GetMinColumn() * mCharWidth;
		r.width = (mSelection.GetMaxColumn() - mSelection.GetMinColumn() + 1) * mCharWidth;
		
		outRegion += r;
	}
	else
	{
		uint32 anchor = mSelection.GetAnchor();
		uint32 caret = mSelection.GetCaret();
		
		if (caret < anchor)
			swap(caret, anchor);
		
		uint32 anchorLine = OffsetToLine(anchor);
		uint32 caretLine = OffsetToLine(caret);
		
		for (uint32 line = anchorLine; line <= caretLine; ++line)
		{
			MRect r;
			
			r.y = line * mLineHeight;
			r.height = mLineHeight;
			
			int32 anchorPos = 0;
			if (line == anchorLine)
				OffsetToPosition(anchor, anchorLine, anchorPos);
	
			r.x = anchorPos;
			
			int32 caretPos = 10000;
			if (mWrapWidth > 0)
				caretPos = mWrapWidth;
			if (line == caretLine)
				OffsetToPosition(caret, caretLine, caretPos);
	
			r.width = caretPos - anchorPos + 1;
			
			outRegion += r;
		}
	}
}

// -----------------------------------------------------------------------------
// GetSelectedText

void MTextDocument::GetSelectedText(
	string&			outText) const
{
	if (not mSelection.IsEmpty())
	{
		if (mSelection.IsBlock())
		{
			outText.clear();
			
			uint32 minLine = mSelection.GetMinLine();
			uint32 maxLine = mSelection.GetMaxLine();
			
			uint32 minColumn = mSelection.GetMinColumn();
			uint32 maxColumn = mSelection.GetMaxColumn();
			
			if (minColumn == maxColumn)
				outText.insert(outText.begin(), maxLine - minLine + 1, '\n');
			else
			{
				outText.reserve((maxLine - minLine + 1) * (maxColumn - minColumn + 2));
				
				for (uint32 line = minLine; line <= maxLine; ++line)
				{
					MTextBuffer::const_iterator text(&mText, LineStart(line));
					MTextBuffer::const_iterator end(&mText, LineEnd(line));
					
					uint32 column = 0;
					
					while (column < minColumn and text < end)
					{
						if (*text == '\t')
						{
							uint32 d = mCharsPerTab - (column % mCharsPerTab);
							column += d;
						}
						else
							++column;
						++text;
					}
					
					if (column > minColumn)
						outText.insert(outText.end(), column - minColumn, ' ');
					uint32 startOffset = text.GetOffset();
					
					uint32 d = 0;
					while (column < maxColumn and text < end)
					{
						if (*text == '\t')
						{
							d = mCharsPerTab - (column % mCharsPerTab);
							column += d;
						}
						else
							++column;
						++text;
					}
					
					string copy;
					if (column > maxColumn)
					{
						mText.GetText(startOffset, text.GetOffset() - startOffset - 1, copy);
						copy.insert(copy.end(), maxColumn - (column - d), ' ');
					}
					else
					{
						mText.GetText(startOffset, text.GetOffset() - startOffset, copy);
						if (column < maxColumn)
							copy.insert(copy.end(), maxColumn - column, ' ');
					}
					
					outText += copy;
					outText += '\n';
				}
			}
		}
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

void MTextDocument::Select(
	MSelection	inSelection)
{
	FinishAction();
	ChangeSelection(inSelection);
}

void MTextDocument::Select(
	uint32			inAnchor,
	uint32			inCaret,
	bool			inBlock)
{
	if (inBlock)
	{
		MSelection selection(this,
			OffsetToLine(inAnchor), OffsetToColumn(inAnchor),
			OffsetToLine(inCaret), OffsetToColumn(inCaret));
		Select(selection);
	}
	else
		Select(inAnchor, inCaret, kScrollNone);
}

void MTextDocument::Select(
	uint32			inAnchor,
	uint32			inCaret,
	MScrollMessage	inScrollMessage)
{
	FinishAction();
	ChangeSelection(MSelection(this, inAnchor, inCaret));
	if (inScrollMessage != kScrollNone)
		eScroll(inScrollMessage);
}

void MTextDocument::ChangeSelection(
	MSelection	inSelection)
{
	inSelection.SetDocument(this);
	
	if (inSelection != mSelection)
	{
		uint32 a, c, sl1, sl2, el1, el2, l;

		if (mSelection.IsBlock())
		{
			sl1 = mSelection.GetMinLine();
			el1 = mSelection.GetMaxLine();
		}
		else
		{
			a = mSelection.GetAnchor();
			c = mSelection.GetCaret();
			if (a > c)
				swap(a, c);
				
			sl1 = OffsetToLine(a);
			el1 = OffsetToLine(c);
		}

		if (inSelection.IsBlock())
		{
			sl2 = inSelection.GetMinLine();
			el2 = inSelection.GetMaxLine();
		}
		else
		{
			a = inSelection.GetAnchor();
			c = inSelection.GetCaret();
			if (a > c)
				swap(a, c);
				
			sl2 = OffsetToLine(a);
			el2 = OffsetToLine(c);
		}
		
		// check for overlap
		if (not mSelection.IsBlock() and not inSelection.IsBlock() and
			sl1 < el2 and sl2 < el1)
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
		
		if (not mSelection.IsBlock())
		{
			uint32 line;
			OffsetToPosition(inSelection.GetCaret(), line, mWalkOffset);
		}
	}

	if (mCurrentAction != kTypeAction)
		mText.SetSelectionAfter(mSelection);
	SendSelectionChangedEvent();
}

// ---------------------------------------------------------------------------
//	SendSelectionChangedEvent

void MTextDocument::SendSelectionChangedEvent()
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

uint32 MTextDocument::OffsetToLine(
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

void MTextDocument::OffsetToPosition(
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

// ---------------------------------------------------------------------------
//	OffsetToColumn

uint32 MTextDocument::OffsetToColumn(
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

void MTextDocument::PositionToOffset(
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
			device.PositionToIndex(inLocationX, outOffset);
			outOffset += mLineInfo[line].start;
			if (outOffset > LineEnd(line))
				outOffset = LineEnd(line);
		}
		else
			outOffset = mLineInfo[line].start;
	}
}

// ---------------------------------------------------------------------------
//	PositionToLineColumn

void MTextDocument::PositionToLineColumn(
	int32			inLocationX,
	int32			inLocationY,
	uint32&			outLine,
	uint32&			outColumn) const
{
	outLine = 0;
	if (inLocationY > 0)
		outLine = static_cast<uint32>(inLocationY / mLineHeight);
	
	if (outLine >= mLineInfo.size())
		outLine = mLineInfo.size() - 1;

	string text;
	MDevice device;
	
	GetStyledText(outLine, device, text);
	
	if (GetSoftwrap())
		inLocationX -= GetLineIndentWidth(outLine);
	
	if (inLocationX > 0)
	{
//		bool trailing;
//		/*bool hit = */device.PositionToIndex(inLocationX, outColumn, trailing);
		outColumn = static_cast<uint32>((inLocationX + (mCharWidth / 2)) / mCharWidth);
	}
	else
		outColumn = 0;
}

// ---------------------------------------------------------------------------
//	LineAndColumnToOffset

uint32 MTextDocument::LineAndColumnToOffset(
	uint32			inLine,
	uint32			inColumn) const
{
	uint32 start = LineStart(inLine);
	uint32 end = LineStart(inLine + 1);
	MTextBuffer::const_iterator text = mText.begin() + start;
	
	uint32 column = 0;

	while (column < inColumn and text != mText.end() and text.GetOffset() < end)
	{
		if (*text == '\t')
		{
			uint32 d = mCharsPerTab - (column % mCharsPerTab);
			column += d;
		}
		else
			++column;

		++text;
	}
	
	return text.GetOffset();
}

// ---------------------------------------------------------------------------
//	GuessMaxWidth

uint32 MTextDocument::GuessMaxWidth() const
{
	uint32 maxWidth = 0;

	if (not mSoftwrap)
	{
		uint32 offset = 0;
		for (uint32 line = 1; line < mLineInfo.size(); ++line)
		{
			if (maxWidth < mLineInfo[line].start - offset)
				maxWidth = mLineInfo[line].start - offset;
			offset = mLineInfo[line].start;
		}
		
		maxWidth *= mCharWidth;
	}
	
	return maxWidth;
}

// -----------------------------------------------------------------------------
// StartAction

void MTextDocument::StartAction(
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

void MTextDocument::FinishAction()
{
	if (mCurrentAction != kNoAction)
	{
		mText.ActionFinished();
	
		mLastAction = mCurrentAction;
		mCurrentAction = kNoAction;
	
		UpdateDirtyLines();
		mCompletionStrings.clear();
	
		mCompletionIndex = -1;
	}
}

void MTextDocument::Insert(
	uint32			inOffset,
	const char*		inText,
	uint32			inLength)
{
	if (mFile.ReadOnly() and not mWarnedReadOnly)
	{
		DisplayAlert("read-only-alert");
		mWarnedReadOnly = true;
	}
	
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

		uint32 anchor = mSelection.GetAnchor();
		if (inOffset <= anchor)
			anchor += inLength;
		
		uint32 caret = mSelection.GetCaret();
		if (inOffset <= caret)
			caret += inLength;
		
		mSelection.Set(anchor, caret);
	
		int32 delta = RewrapLines(inOffset, inOffset + inLength);
		if (delta)
			eShiftLines(line, delta);

		if (lineCount != mLineInfo.size())
			eLineCountChanged();
	}
}

void MTextDocument::Delete(
	uint32			inOffset,
	uint32			inLength)
{
	if (mFile.ReadOnly() and not mWarnedReadOnly)
	{
		DisplayAlert("read-only-alert");
		mWarnedReadOnly = true;
	}

	if (inLength > 0)
	{
		assert(inOffset + inLength <= mText.GetSize());

		mText.Delete(inOffset, inLength);
		if (not mDirty)
			SetModified(true);

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

bool MTextDocument::CanUndo(string& outAction)
{
	return mText.CanUndo(outAction);
}

void MTextDocument::DoUndo()
{
	MSelection s(this);
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

bool MTextDocument::CanRedo(string& outAction)
{
	return mText.CanRedo(outAction);
}

void MTextDocument::DoRedo()
{
	MSelection s(this);
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

void MTextDocument::RepairAfterUndo(
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

void MTextDocument::DoBalance()
{
	uint32 offset = mSelection.GetMinOffset();
	uint32 length = mSelection.GetMaxOffset() - offset;
	
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
				PlaySound("warning");
		}
		else
			Select(newOffset, newOffset + newLength);
	}
	else
		PlaySound("warning");
}
								
void MTextDocument::DoShiftLeft()
{
	Select(mSelection.SelectLines());
	
	StartAction("Shift left");
	
	uint32 minLine = mSelection.GetMinLine();
	uint32 maxLine = mSelection.GetMaxLine();
	
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

void MTextDocument::DoShiftRight()
{
	Select(mSelection.SelectLines());
	
	StartAction("Shift right");
	
	uint32 anchor = mSelection.GetAnchor();
	uint32 caret = mSelection.GetCaret();

	uint32 minLine = mSelection.GetMinLine();
	uint32 maxLine = mSelection.GetMaxLine();
	
	for (uint32 line = minLine; line <= maxLine; ++line)
	{
		Insert(LineStart(line), "\t", 1);
		caret += 1;
	}
	
	mSelection.Set(anchor, caret);
	FinishAction();
}
								
void MTextDocument::DoComment()
{
	if (mLanguage and not mSelection.IsBlock())
	{
		if (mSelection.IsEmpty())
			Select(mSelection.SelectLines());
		
		uint32 selectionStart, selectionEnd;
		selectionStart = mSelection.GetMinOffset();
		selectionEnd = mSelection.GetMaxOffset();
		
		StartAction("Comment");

		uint32 minLine = mSelection.GetMinLine();
		uint32 maxLine = mSelection.GetMaxLine();
		
		for (uint32 line = minLine; line <= maxLine; ++line)
		{
			uint32 offset;
			if (line == minLine)
				offset = mSelection.GetMinOffset();
			else
				offset = LineStart(line);
			
			uint32 length;
			if (line == maxLine)
				length = mSelection.GetMaxOffset() - offset;
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
								
void MTextDocument::DoUncomment()
{
	if (mLanguage and not mSelection.IsBlock())
	{
		if (mSelection.IsEmpty())
			Select(mSelection.SelectLines());
		
		uint32 selectionStart, selectionEnd;
		selectionStart = mSelection.GetMinOffset();
		selectionEnd = mSelection.GetMaxOffset();
		
		StartAction("Uncomment");

		uint32 minLine = mSelection.GetMinLine();
		uint32 maxLine = mSelection.GetMaxLine();
		
		for (uint32 line = minLine; line <= maxLine; ++line)
		{
			uint32 offset;
			if (line == minLine)
				offset = mSelection.GetMinOffset();
			else
				offset = LineStart(line);
			
			uint32 length;
			if (line == maxLine)
				length = mSelection.GetMaxOffset() - offset;
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
								
void MTextDocument::DoEntab()
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
								
void MTextDocument::DoDetab()
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
								
void MTextDocument::DoCut(bool inAppend)
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

void MTextDocument::DoCopy(bool inAppend)
{
	string text;
	GetSelectedText(text);
	if (inAppend)
		MClipboard::Instance().AddData(text);
	else
		MClipboard::Instance().SetData(text, mSelection.IsBlock());
}

void MTextDocument::DoPaste()
{
	if (MClipboard::Instance().HasData())
	{
		string text;
		bool isBlock;
		
		MClipboard::Instance().GetData(text, isBlock);
		
		StartAction(kPasteAction);
		ReplaceSelectedText(text, isBlock, true);
		FinishAction();
		
		Select(mSelection.GetCaret(), mSelection.GetCaret(), false);
	}
	else
		assert(false);
}

void MTextDocument::DoClear()
{
	StartAction("Clear");
	
	DeleteSelectedText();

	FinishAction();
}

void MTextDocument::DoPasteNext()
{
	if (mLastAction == kPasteAction and mCurrentAction == kNoAction)
	{
		DoUndo();
		MClipboard::Instance().NextInRing();
	}
	DoPaste();
}

void MTextDocument::DoFastFind(MDirection inDirection)
{
	mFastFindStartOffset = mSelection.GetMaxOffset();
	
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

void MTextDocument::FastFindType(const char* inText, uint32 inTextLength)
{
	if (mTargetTextView != nil)
		mTargetTextView->ObscureCursor();
	
	string what = mFastFindWhat;
	
	if (inText != nil)
		mFastFindWhat.append(inText, inTextLength);
	else
	{
		uint32 l = MEncodingTraits<kEncodingUTF8>::GetPrevCharLength(
						mFastFindWhat.end());
		
		uint32 offset = mFastFindWhat.length() - l;
		
		mFastFindWhat.erase(offset, mFastFindWhat.length() - offset);
	}
	
	if (not mFastFindInited)
		MFindDialog::Instance().SetFindString(mFastFindWhat, false);
	
	if (FastFind(mFastFindDirection))
		mFastFindInited = true;
	else
		mFastFindWhat = what;
}

bool MTextDocument::FastFind(MDirection inDirection)
{
	uint32 offset = mFastFindStartOffset;
	if (inDirection == kDirectionBackward and offset > 0)
		offset -= mFastFindWhat.length();
	
	MSelection found(this);
	bool result = mText.Find(offset, mFastFindWhat, inDirection, true, false, found);
	
	if (result)
	{
		MFindDialog::Instance().SetFindString(mFastFindWhat, true);
		Select(found.GetMinOffset(), found.GetMaxOffset(), kScrollToSelection);
	}
	else
		PlaySound("warning");
	
	return result;
}

bool MTextDocument::CanReplace()
{
	return mText.CanReplace(MFindDialog::Instance().GetFindString(),
		MFindDialog::Instance().GetRegex(),
		MFindDialog::Instance().GetIgnoreCase(), mSelection);
}

void MTextDocument::DoMarkLine()
{
	uint32 line = mSelection.GetMinLine();
	MarkLine(line, not mLineInfo[line].marked);
	eInvalidateDirtyLines();
}

void MTextDocument::HandleFindDialogCommand(uint32 inCommand)
{
	if (mTargetTextView and mTargetTextView->GetWindow())
		mTargetTextView->GetWindow()->Select();
	
	switch (inCommand)
	{
		case cmd_FindNext:
			if (not DoFindNext(kDirectionForward))
				PlaySound("warning");
			break;

		case cmd_Replace:
			DoReplace(false, kDirectionForward);
			break;

		case cmd_ReplaceFindNext:
			if (not DoReplace(true, kDirectionForward))
				PlaySound("warning");
			break;

		case cmd_ReplaceAll:
			DoReplaceAll();
			break;
	}
}

bool MTextDocument::DoFindFirst()
{
	string what = MFindDialog::Instance().GetFindString();
	bool ignoreCase = MFindDialog::Instance().GetIgnoreCase();
	bool regex = MFindDialog::Instance().GetRegex();
	uint32 offset = 0;
	
	MSelection found(this);
	bool result = mText.Find(offset, what, kDirectionForward, ignoreCase, regex, found);
	
	if (result)
		Select(found.GetMinOffset(), found.GetMaxOffset(), kScrollToSelection);
	
	return result;
}

bool MTextDocument::DoFindNext(MDirection inDirection)
{
	string what = MFindDialog::Instance().GetFindString();
	bool ignoreCase = MFindDialog::Instance().GetIgnoreCase();
	bool regex = MFindDialog::Instance().GetRegex();
	uint32 offset;
	
	if (inDirection == kDirectionBackward)
		offset = mSelection.GetMinOffset();
	else
		offset = mSelection.GetMaxOffset();
	
	MSelection found(this);
	bool result = mText.Find(offset, what, inDirection, ignoreCase, regex, found);
	
	if (result)
		Select(found.GetMinOffset(), found.GetMaxOffset(), kScrollToSelection);
	
	return result;
}

void MTextDocument::FindAll(string inWhat, bool inIgnoreCase, 
	bool inRegex, bool inSelection, MMessageList&outHits)
{
	if (inSelection and mSelection.IsBlock())
		THROW(("block selection not supported in Find All"));

	uint32 minOffset = 0;
	uint32 maxOffset = mText.GetSize();

	if (inSelection)
	{
		minOffset = mSelection.GetMinOffset();
		maxOffset = mSelection.GetMaxOffset();
	}
	
	MSelection sel(this);
	while (mText.Find(minOffset, inWhat, kDirectionForward, inIgnoreCase, inRegex, sel) and
		sel.GetMaxOffset() <= maxOffset)
	{
		uint32 lineNr = sel.GetMinLine();
		
		string s;
		GetLine(lineNr, s);
		boost::trim_right(s);
		
		outHits.AddMessage(kMsgKindNone, mFile.GetPath(), lineNr + 1,
			sel.GetMinOffset(), sel.GetMaxOffset(), s);
		
		minOffset = sel.GetMaxOffset();
	}
}

void MTextDocument::FindAll(
	const fs::path&		inPath,
	const std::string&	inWhat,
	bool				inIgnoreCase, 
	bool				inRegex,
	bool				inSelection,
	MMessageList&		outHits)
{
	MTextDocument doc;
	fs::ifstream file(inPath, ios::binary);

	doc.SetFile(MFile(inPath));
	doc.mText.ReadFromFile(file);
	doc.Rewrap();
	doc.FindAll(inWhat, inIgnoreCase, inRegex, inSelection, outHits);
}

bool MTextDocument::DoReplace(bool inFindNext, MDirection inDirection)
{
	bool result = false;
	
	if (CanReplace())
	{
		StartAction(kReplaceAction);
		
		string what = MFindDialog::Instance().GetFindString();
		string replace = MFindDialog::Instance().GetReplaceString();
		
		uint32 offset = mSelection.GetMinOffset();
		
		if (MFindDialog::Instance().GetRegex())
		{
			mText.ReplaceExpression(mSelection, what, MFindDialog::Instance().GetIgnoreCase(),
				replace, replace);
		}

		ReplaceSelectedText(replace, false, true);
		FinishAction();
		
		if (inFindNext)
		{
			bool ignoreCase = MFindDialog::Instance().GetIgnoreCase();
			bool regex = MFindDialog::Instance().GetRegex();
			uint32 nextOffset = offset;

			if (inDirection == kDirectionForward)
				nextOffset += replace.length();
		
			MSelection found(this);
			result = mText.Find(nextOffset, what, inDirection, ignoreCase, regex, found);
			
			if (result)
				ChangeSelection(found);
			else
				ChangeSelection(offset, offset + replace.length());
	
			eScroll(kScrollToSelection);
		}
	}

	return result;
}

void MTextDocument::DoReplaceAll()
{
	uint32 offset = 0, lastMatch = 0, lastOffset = mText.GetSize();
	string what = MFindDialog::Instance().GetFindString();
	string replace;
	bool ignoreCase = MFindDialog::Instance().GetIgnoreCase();
	bool replacedAny = false;
	bool regex = MFindDialog::Instance().GetRegex();
	MSelection found(this);

	if (MFindDialog::Instance().GetInSelection())
	{
		offset = mSelection.GetMinOffset();
		lastOffset = mSelection.GetMaxOffset();
	}
	
	while (mText.Find(offset, what, kDirectionForward, ignoreCase, regex, found)
		and found.GetMaxOffset() <= lastOffset)
	{
		if (not replacedAny)
		{
			StartAction(kReplaceAction);
			replacedAny = true;
		}
		
		offset = found.GetMinOffset();
		int32 lengthOfSelection = found.GetMaxOffset() - offset;
		
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
		lastOffset += replace.length() - lengthOfSelection;
	}
	
	if (replacedAny)
		Select(lastMatch, lastMatch + replace.length(), kScrollToSelection);
	else
		PlaySound("warning");
}

void MTextDocument::DoComplete(MDirection inDirection)
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
			PlaySound("warning");
			return;
		}
		
		string key;
		mText.GetText(startOffset, length, key);
		mText.CollectWordsBeginningWith(startOffset, inDirection, key, mCompletionStrings);
	
		if (mLanguage != nil)
			mLanguage->CollectKeyWordsBeginningWith(key, mCompletionStrings);
	
		MDocument* doc = GetFirstDocument();
		while (doc != nil)
		{
			if (doc != this and dynamic_cast<MTextDocument*>(doc) != nil)
				static_cast<MTextDocument*>(doc)->mText.CollectWordsBeginningWith(
					0, kDirectionForward, key, mCompletionStrings);
			doc = doc->GetNextDocument();
		}
		
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
		PlaySound("warning");
	}
	else
	{
		Type(mCompletionStrings[mCompletionIndex].c_str(),
			mCompletionStrings[mCompletionIndex].length());
	}
	
	TouchLine(OffsetToLine(mCompletionStartOffset));
	UpdateDirtyLines();
}

void MTextDocument::DoSoftwrap()
{
	mSoftwrap = not mSoftwrap;
	Rewrap();
	UpdateDirtyLines();
}

void MTextDocument::QuotedRewrap(
	const string&	inQuoteCharacters,
	uint32			inWidth)
{
	if (mSelection.IsEmpty())
		SelectAll();
	
	string text;
	GetSelectedText(text);
	
	StartAction(_("Quoted Rewrap"));
	
	string::iterator ch = text.begin();
	string quote, line, result;
	
	result.reserve(text.length());
	typedef MEncodingTraits<kEncodingUTF8> enc;

	for (;;)
	{
		// collect the next quote and text string
		string nextQuote, nextLine;
		
		while (ch != text.end() and 
			(*ch == ' ' or inQuoteCharacters.find(*ch) != string::npos))
		{
			if (*ch != ' ')
				nextQuote += *ch;
			++ch;
		}
		
		while (ch != text.end() and *ch != '\n')
		{
			if (*ch != ' ' or nextLine.empty() or nextLine[nextLine.length() - 1] != ' ')
				nextLine += *ch;
			++ch;
		}
		
		if (ch != text.end())
			++ch;
		
		// same as previous, add to line
		if (nextQuote == quote and not nextLine.empty())
		{
			if (line.empty() or line[line.length() - 1] == ' ')
				line += nextLine;
			else
				line = line + ' ' + nextLine;
			continue;
		}
		
		// detected new quote level, write out previous line
		string::iterator lc = line.begin();
		while (lc != line.end())
		{
			if (not quote.empty())
				result += quote + ' ';
			
			uint32 width = inWidth - quote.length();
			if (width < 8)
				THROW(("Too much quote characters to rewrap"));
			
			string::iterator slc = lc;
			while (lc != line.end())
			{
				string::iterator e = next_line_break(lc, line.end());
				uint32 lcl = e - lc;
				if (width < lcl + 1)
					break;
				width -= lcl;
				while (lcl-- > 0)
					result += *lc++;
			}

			if (slc == lc)	// could not break... force
			{
				result.append(line);
				lc = line.end();
			}
			
			result += '\n';
		}
		
		line.clear();
		
		// flushed line, now reset. Perhaps we need to enter a newline
		if (nextLine.empty())
		{
			if (not quote.empty())
				result += quote;
			result += '\n';
		}
		
		if (ch == text.end())
			break;
		
		quote = nextQuote;
		line = nextLine;
	}
	
	ReplaceSelectedText(result, false, true);
	FinishAction();
}

void MTextDocument::DoApplyScript(const std::string& inScript)
{
	if (mShell.get() != nil and mShell->IsRunning())
		return;
	
	if (mShell.get() == nil)
	{
		mShell.reset(new MShell(true));

		SetCallback(mShell->eStdOut, this, &MTextDocument::StdOut);
		SetCallback(mShell->eStdErr, this, &MTextDocument::StdErr);
		SetCallback(mShell->eShellStatus, this, &MTextDocument::ShellStatusIn);
	}
	
	if (mSelection.IsEmpty())
		SelectAll();

	string text;
	GetSelectedText(text);
	
	mPreparedForStdOut = true;
	StartAction(inScript.c_str());
	mShell->ExecuteScript((gScriptsDir / inScript).string(), text);
}

void MTextDocument::GetStyledText(
	uint32		inLine,
	MDevice&	inDevice,
	string&		outText) const
{
	uint32 offset = LineStart(inLine);
	uint32 length = LineStart(inLine + 1) - offset;
	return GetStyledText(offset, length, mLineInfo[inLine].state, inDevice, outText);
}

void MTextDocument::GetStyledText(
	uint32		inOffset,
	uint32		inLength,
	uint16		inState,
	MDevice&	inDevice,
	string&		outText) const
{
	inDevice.SetFont(mFont);
	
	mText.GetText(inOffset, inLength, outText);

	inDevice.SetText(outText);
	
	if (inLength > 0 and outText[outText.length() - 1] == '\n')
		outText.erase(outText.length() - 1);

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
//		overrideSpec.overrideUPP = &MTextDocument::GetReplacementGlyphCallback;
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

void MTextDocument::HashLines(vector<uint32>& outHashes) const
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

void MTextDocument::OnKeyPressEvent(
	GdkEventKey*		inEvent)
{
	bool handled = false;

    uint32 modifiers = inEvent->state & kValidModifiersMask;
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
			Type(s, length);
		}
	}
}

// ----------------------------------------------------------------------------
//	OnCommit

void MTextDocument::OnCommit(
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

bool MTextDocument::HandleKeyCommand(MKeyCommand inKeyCommand)
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
				caret = anchor = mSelection.GetMinOffset();
			break;

		case kcmd_MoveCharacterRight:
			mFastFindMode = false;
			if (mSelection.IsEmpty())
				caret = anchor = mText.NextCursorPosition(caret, eMoveOneCharacter);
			else
				caret = anchor = mSelection.GetMaxOffset();
			break;

		case kcmd_MoveWordLeft:
			mFastFindMode = false;
			if (mSelection.IsEmpty())
				caret = anchor = FindWord(caret, kDirectionBackward);
			else
				caret = anchor = mSelection.GetMinOffset();
			break;

		case kcmd_MoveWordRight:
			mFastFindMode = false;
			if (mSelection.IsEmpty())
				caret = anchor = FindWord(caret, kDirectionForward);
			else
				caret = anchor = mSelection.GetMaxOffset();
			break;

		case kcmd_MoveToBeginningOfLine:
			mFastFindMode = false;
			if (mSelection.IsEmpty())
				caret = anchor = LineStart(minLine);
			else
				caret = anchor = mSelection.GetMinOffset();
			break;

		case kcmd_MoveToEndOfLine:
			mFastFindMode = false;
			if (mSelection.IsEmpty())
				caret = anchor = LineEnd(minLine);
			else
				caret = anchor = mSelection.GetMaxOffset();
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
				caret = mSelection.GetMinOffset();
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
				caret = mSelection.GetMaxOffset();
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
				if (mFastFindWhat.length())
					FastFindType(nil, 0);
				else
					PlaySound("warning");
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
				Select(mSelection.SelectLines());

				anchor = mSelection.GetAnchor();
				caret = mSelection.GetCaret();
				
				minLine = mSelection.GetMinLine();

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
				Select(mSelection.SelectLines());

				anchor = mSelection.GetAnchor();
				caret = mSelection.GetCaret();
				
				maxLine = mSelection.GetMaxLine();

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

bool MTextDocument::HandleRawKeydown(
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
				else if (inModifiers & GDK_CONTROL_MASK)
				{
					Reset();
					Execute();
					handled = true;
	//				updateSelection = false;
				}
				else
				{
					// Enter a return, optionally auto indented
					uint32 minOffset = mSelection.GetMinOffset();
	
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
			{
				int minLine = mSelection.GetMinLine();
				int maxLine = mSelection.GetMaxLine();

				bool shift = minLine < maxLine;
				if (not shift)
				{
					shift =
						mSelection.GetMinOffset() == LineStart(minLine) and
						mSelection.GetMaxOffset() == LineStart(minLine + 1);
				}
				
				if (shift)
				{
					DoShiftRight();
					handled = true;
				}
				else
				{
					Type("\t", 1);
					handled = true;
				}
				break;
			}

			case GDK_ISO_Left_Tab:
			{
				int minLine = mSelection.GetMinLine();
				int maxLine = mSelection.GetMaxLine();

				bool shift = minLine < maxLine;
				if (not shift)
				{
					shift =
						mSelection.GetMinOffset() == LineStart(minLine) and
						mSelection.GetMaxOffset() == LineStart(minLine + 1);
				}
				
				if (shift)
				{
					DoShiftLeft();
					handled = true;
				}
				break;
			}
			
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

void MTextDocument::PrefsChanged()
{
	ReInit();
	Rewrap();
	UpdateDirtyLines();
}

void MTextDocument::ShellStatusIn(bool inActive)
{
	eShellStatus(inActive);
	
	if (not inActive)
	{
		string cwd = mShell->GetCWD();
		eBaseDirChanged(fs::path(cwd));
	}
}

void MTextDocument::StdOut(const char* inText, uint32 inSize)
{
	bool findLanguage = mDirty == false and mLanguage == nil and mText.GetSize() == 0;
	
	if (not mPreparedForStdOut)
	{
		StartAction(kTypeAction);
		
		uint32 line = mSelection.GetMaxLine();
		ChangeSelection(LineEnd(line), LineEnd(line));
		
		Type("\n", 1);
		mPreparedForStdOut = true;
	}
	
	Type(inText, inSize);
	
	eLineCountChanged();
	eScroll(kScrollToCaret);
	
	if (findLanguage)
	{
		mLanguage = MLanguage::GetLanguageForDocument(mFile.GetFileName(), mText);
		if (mLanguage != nil)
		{
			mNamedRange = new MNamedRange;
			mIncludeFiles = new MIncludeFileList;
		}

		Rewrap();
		UpdateDirtyLines();
	}
}

void MTextDocument::StdErr(const char* inText, uint32 inSize)
{
	if (mStdErrWindow == nil)
	{
		mStdErrWindow = new MMessageWindow(_("Output from stderr"));
		AddRoute(mStdErrWindow->eWindowClosed, eMsgWindowClosed);
	}
	else if (not mStdErrWindowSelected)
		mStdErrWindow->Select();

	mStdErrWindow->SetBaseDirectory(fs::path(mShell->GetCWD()));

	mStdErrWindow->AddStdErr(inText, inSize);
}

void MTextDocument::MsgWindowClosed(MWindow* inWindow)
{
	assert(inWindow == mStdErrWindow);
	mStdErrWindow = nil;
}

void MTextDocument::Execute()
{
	FinishAction();
	
	if (mSelection.IsEmpty())
	{
		uint32 line = mSelection.GetMinLine();
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
			ssize_t size = mFile.ReadAttribute(kJapieCWD, cwd, sizeof(cwd));
			if (size > 0)
			{
				string d(cwd, size);
				mShell->SetCWD(d);
			}
			else
				mShell->SetCWD(mFile.GetPath().branch_path().string());
		}
		
		SetCallback(mShell->eStdOut, this, &MTextDocument::StdOut);
		SetCallback(mShell->eStdErr, this, &MTextDocument::StdErr);
		SetCallback(mShell->eShellStatus, this, &MTextDocument::ShellStatusIn);
	}

	mPreparedForStdOut = false;
	mStdErrWindowSelected = false;
	
	if (mStdErrWindow != nil)
		mStdErrWindow->ClearList();
	
	mShell->Execute(s);
}

void MTextDocument::Idle(
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
	
	if (mDataFD >= 0)
	{
		char buffer[10240];
		int r = read(mDataFD, buffer, sizeof(buffer));
		if (r == 0 or (r < 0 and errno != EAGAIN))
			mDataFD = -1;
		else if (r > 0)
			StdOut(buffer, r);
	}
}

// ---------------------------------------------------------------------------
//	MDocument::GetParsePopupItems

bool MTextDocument::GetParsePopupItems(MMenu& inMenu)
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

void MTextDocument::SelectParsePopupItem(uint32 inItem)
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

bool MTextDocument::GetIncludePopupItems(MMenu& inMenu)
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

void MTextDocument::SelectIncludePopupItem(uint32 inItem)
{
	if (mLanguage)
	{
		MIncludeFile file = mIncludeFiles->at(inItem);
		
		bool found = false;

		if (mFile.IsLocal())
		{
			MFile path = mFile.GetParent() / file.name;
			if (path.Exists())
			{
				gApp->OpenOneDocument(path);
				found = true;
			}
		}
		
		if (not found)
		{
			MProject* project = MProject::Instance();
			fs::path p;
			
			if (project != nil and project->LocateFile(file.name, file.isQuoted, p))
				gApp->OpenOneDocument(MFile(p));
			else if (mFile.IsValid())
			{
				MFile path = mFile.GetParent() / file.name;
				
				if (path.Exists())
					gApp->OpenOneDocument(path);
			}
		}
	}
}

// ---------------------------------------------------------------------------
//	MDocument::ProcessCommand

bool MTextDocument::ProcessCommand(
	uint32			inCommand,
	const MMenu*	inMenu,
	uint32			inItemIndex,
	uint32			inModifiers)
{
	bool result = true;
	MProject* project = MProject::Instance();

	string s;
	
	switch (inCommand)
	{
		case cmd_Undo:
			DoUndo();
			break;

		case cmd_Redo:
			DoRedo();
			break;

		case cmd_Cut:
			DoCut(false);
			break;

		case cmd_Copy:
			DoCopy(false);
			break;

		case cmd_Paste:
			DoPaste();
			break;

		case cmd_Clear:
			DoClear();
			break;

		case cmd_SelectAll:
			SelectAll();
			break;

		case cmd_Balance:
			DoBalance();
			break;

		case cmd_ShiftLeft:
			DoShiftLeft();
			break;

		case cmd_ShiftRight:
			DoShiftRight();
			break;

		case cmd_Entab:
			DoEntab();
			break;

		case cmd_Detab:
			DoDetab();
			break;

		case cmd_Comment:
			DoComment();
			break;

		case cmd_Uncomment:
			DoUncomment();
			break;

		case cmd_PasteNext:
			DoPasteNext();
			break;

		case cmd_CopyAppend:
			DoCopy(true);
			break;

		case cmd_CutAppend:
			DoCut(true);
			break;
		
		case cmd_FastFind:
			DoFastFind(kDirectionForward);
			break;

		case cmd_FastFindBW:
			DoFastFind(kDirectionBackward);
			break;

		case cmd_Find:
			MFindDialog::Instance().Select();
			break;

		case cmd_FindNext:
			result = DoFindNext(kDirectionForward);
			break;

		case cmd_FindPrev:
			if (not DoFindNext(kDirectionBackward))
				PlaySound("warning");
			break;
		
		case cmd_EnterSearchString:
			GetSelectedText(s);
			MFindDialog::Instance().SetFindString(s, false);
			break;

		case cmd_EnterReplaceString:
			GetSelectedText(s);
			MFindDialog::Instance().SetReplaceString(s);
			break;

		case cmd_Replace:
			DoReplace(false, kDirectionForward);
			break;

		case cmd_ReplaceAll:
			DoReplaceAll();
			break;

		case cmd_ReplaceFindNext:
			result = DoReplace(true, kDirectionForward);
			break;

		case cmd_ReplaceFindPrev:
			if (not DoReplace(true, kDirectionBackward))
				PlaySound("warning");
			break;

		case cmd_CompleteLookingBack:
			DoComplete(kDirectionBackward);
			break;

		case cmd_CompleteLookingFwd:
			DoComplete(kDirectionForward);
			break;

		case cmd_ClearMarks:
			ClearAllMarkers();
			break;

		case cmd_MarkLine:
			DoMarkLine();
			break;

		case cmd_JumpToNextMark:
			DoJumpToNextMark(kDirectionForward);
			break;

		case cmd_JumpToPrevMark:
			DoJumpToNextMark(kDirectionBackward);
			break;
		
		case cmd_CutMarkedLines:
			CCCMarkedLines(true, true);
			break;

		case cmd_CopyMarkedLines:
			CCCMarkedLines(true, false);
			break;

		case cmd_ClearMarkedLines:
			CCCMarkedLines(false, true);
			break;

		case cmd_Softwrap:
			DoSoftwrap();
			break;

		case cmd_ShowHideWhiteSpace:
			mShowWhiteSpace = not mShowWhiteSpace;
			if (mTargetTextView)
				mTargetTextView->Invalidate();
			break;

		case cmd_ShowDocInfoDialog:
//				if (mDocument != nil)
//				{
//					std::auto_ptr<MDocInfoDialog> dlog(new MDocInfoDialog);
//					dlog->Initialize(mDocument, mWindow);
//					dlog.release();
//				}
			break;
			
		case cmd_SelectFunctionFromMenu:
			SelectParsePopupItem(inItemIndex);
			break;
		
		case cmd_OpenIncludeFromMenu:
			SelectIncludePopupItem(inItemIndex);
			break;
	
		case cmd_Preprocess:
			if (project != nil)
				project->Preprocess(GetFile().GetPath());
			break;
			
		case cmd_CheckSyntax:
			if (project != nil)
				project->CheckSyntax(GetFile().GetPath());
			break;
			
		case cmd_Compile:
			if (project != nil)
				project->Compile(GetFile().GetPath());
			break;

		case cmd_Disassemble:
			if (project != nil)
				project->Disassemble(GetFile().GetPath());
			break;

		case cmd_2CharsPerTab:
			SetCharsPerTab(2);
			break;

		case cmd_4CharsPerTab:
			SetCharsPerTab(4);
			break;

		case cmd_8CharsPerTab:
			SetCharsPerTab(8);
			break;

		case cmd_16CharsPerTab:
			SetCharsPerTab(16);
			break;
		
		case cmd_SyntaxNone:
			SetLanguage("");
			break;
		
		case cmd_SyntaxLanguage:
			SetLanguage(inMenu->GetItemLabel(inItemIndex));
			break;
		
		case cmd_Stop:
			result = StopRunningShellCommand();
			break;
		
		case cmd_ShowDiffWindow:
		{
			auto_ptr<MDiffWindow> w(new MDiffWindow(this));
			w->Select();
			w.release();
			break;
		}

		case cmd_Menu:
			if (mTargetTextView != nil)
				mTargetTextView->OnPopupMenu(nil);
			break;
	
		case cmd_ApplyScript:
			if ((inModifiers & GDK_CONTROL_MASK) == 0)
				DoApplyScript(inMenu->GetItemLabel(inItemIndex));
			else
				result = false;
			break;
	
		default:
			result = false;
			break;
	}
	
	return result;
}

// ---------------------------------------------------------------------------
//	UpdateCommandStatus

bool MTextDocument::UpdateCommandStatus(
	uint32			inCommand,
	MMenu*			inMenu,
	uint32			inItemIndex,
	bool&			outEnabled,
	bool&			outChecked)
{
	bool result = true;

	MProject* project = MProject::Instance();
	MLanguage* lang = GetLanguage();

	string title;
		
	switch (inCommand)
	{
		// always
		case cmd_Close:
		case cmd_SaveAs:
		case cmd_SelectAll:
		case cmd_MarkLine:
		case cmd_CompleteLookingBack:
		case cmd_CompleteLookingFwd:
		case cmd_JumpToNextMark:
		case cmd_JumpToPrevMark:
		case cmd_FastFind:
		case cmd_FastFindBW:
		case cmd_Find:
		case cmd_FindNext:
		case cmd_FindPrev:
		case cmd_ReplaceAll:
		case cmd_Entab:
		case cmd_Detab:
		case cmd_GoToLine:
		case cmd_ShiftLeft:
		case cmd_ShiftRight:
		case cmd_OpenIncludeFile:
		case cmd_ShowDocInfoDialog:
		case cmd_ShowDiffWindow:
			outEnabled = true;
			break;

		// dirty
		case cmd_Save:
			outEnabled = IsModified() and
				(not IsSpecified() or not IsReadOnly());
			break;

		// has selection
		case cmd_Cut:
		case cmd_Copy:
		case cmd_Clear:
		case cmd_CopyAppend:
		case cmd_CutAppend:
		case cmd_EnterSearchString:
		case cmd_EnterReplaceString:
			outEnabled = not mSelection.IsEmpty();
			break;

		// special
		case cmd_Undo:
			outEnabled = CanUndo(title);
			break;

		case cmd_Redo:
			outEnabled = CanRedo(title);
			break;

		case cmd_Revert:
			outEnabled = IsSpecified() and IsModified();
			break;

		case cmd_Paste:
		case cmd_PasteNext:
			outEnabled = MClipboard::Instance().HasData();
			break;

		case cmd_Balance:
		case cmd_Comment:
		case cmd_Uncomment:
			outEnabled = GetLanguage() != nil and
						not mSelection.IsBlock();
			break;

		case cmd_QuotedRewrap:
			outEnabled = mSelection.IsEmpty() or mSelection.IsBlock() == false;
			break;

		case cmd_Replace:
		case cmd_ReplaceFindNext:
		case cmd_ReplaceFindPrev:
			outEnabled = CanReplace();
			break;
		
		case cmd_Softwrap:
			outEnabled = true;
			outChecked = GetSoftwrap();
			break;
		
		case cmd_ShowHideWhiteSpace:
			outEnabled = true;
			outChecked = mShowWhiteSpace;
			break;

		case cmd_Preprocess:
		case cmd_Compile:
		case cmd_CheckSyntax:
		case cmd_Disassemble:
			outEnabled =
				project != nil and
				GetFile().IsLocal() and
				project->IsFileInProject(GetFile().GetPath());
			break;
		
		case cmd_2CharsPerTab:
			outEnabled = true;
			outChecked = GetCharsPerTab() == 2;
			break;
		
		case cmd_4CharsPerTab:
			outEnabled = true;
			outChecked = GetCharsPerTab() == 4;
			break;
		
		case cmd_8CharsPerTab:
			outEnabled = true;
			outChecked = GetCharsPerTab() == 8;
			break;
		
		case cmd_16CharsPerTab:
			outEnabled = true;
			outChecked = GetCharsPerTab() == 16;
			break;
		
		case cmd_SyntaxNone:
			outEnabled = true;
			outChecked = (lang == nil);
			break;
		
		case cmd_SyntaxLanguage:
			outEnabled = true;
			outChecked = (lang != nil and lang->GetName() == inMenu->GetItemLabel(inItemIndex));
			break;

		case cmd_Menu:
			outEnabled = mTargetTextView != nil;
			break;

		default:
			result = false;
			break;
	}
	
	return result;
}

void MTextDocument::IOProgress(
	float			inProgress,
	const string&	inMessage)
{
	eSSHProgress(inProgress, _("Receiving data"));
	MDocument::IOProgress(inProgress, inMessage);
}

void MTextDocument::IOFileLoaded()
{
	MDocument::IOFileLoaded();
	
	MDocState state = {};
	if (not ReadDocState(state))
		Rewrap();
}

void MTextDocument::IOFileWritten()
{
	MDocument::IOFileWritten();
	eSSHProgress(-1.f, "");
}
