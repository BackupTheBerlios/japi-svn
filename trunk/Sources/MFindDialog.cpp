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

/*	$Id: MFindDialog.cpp 170 2007-06-12 20:55:53Z maarten $
	Copyright Maarten L. Hekkelman
	Created Sunday August 15 2004 20:47:19
*/

#include "MJapieG.h"

#include <boost/bind.hpp>

#include "MFindDialog.h"
#include "MDocument.h"
#include "MEditWindow.h"
#include "MPreferences.h"
#include "MUtils.h"
#include "MMessageWindow.h"
#include "MCommands.h"
#include "MGlobals.h"
//#include "MApplication.h"
//#include "MProject.h"

using namespace std;

namespace {
	
const uint32
	kMaxComboListSize =			10;

enum {
	kFindButtonID =				0,
	kReplaceAndFindButtonID =	2,
	kReplaceButtonID =			3,
	kReplaceAllButtonID =		4,
	kFindComboboxID =			5,
	kReplaceComboboxID =	 	6,
	kInSelectionCheckboxID = 	7,
	kWrapCheckboxID =			8,
	kIgnoreCaseCheckboxID =		9,
	kRegexCheckboxID = 			10,
	kEntireWordCheckboxID =		11,
	kBatchCheckboxID =			12,
	
	// --- multi
	
	kShowHideMultiTriangleID =	201,
	kMethodPopupID = 			202,
	kRecursiveCheckboxID =		203,
	kStartDirComboboxID =		204,
	kTextFilesOnlyCheckboxID =	205,
	kEnableFilterCheckboxID =	206,
	kNameFilterEditboxID =		207,
	kBrowseStartDirButtonID =	208,
	
	// --- status
	
	kChasingArrowsID =			301,
	kStatusPanelID =			302
};

enum {
	kMethodDirectory = 			1,
	kMethodOpenWindows = 		2,
	kMethodIncludeFiles =		3
};

const int16
	kFindDialogCollapsedHeight	= 179,
	kFindDialogExpandedHeight	= 281;

}

MFindDialog& MFindDialog::Instance()
{
	static std::auto_ptr<MFindDialog> sInstance;
	if (sInstance.get() == nil)
	{
		sInstance.reset(new MFindDialog());
		sInstance->Initialize();
	}
	return *sInstance;
}

MFindDialog::MFindDialog()
	: eIdle(this, &MFindDialog::Idle)
	, mFindAllThread(nil)
	, mFindAllResult(nil)
//	, mScrap(kScrapFindScrap)
{
}

void MFindDialog::Close()
{//
//	Preferences::SetInteger("find in selection", IsChecked(kInSelectionCheckboxID));
//	Preferences::SetInteger("find wrap around", IsChecked(kWrapCheckboxID));
//	Preferences::SetInteger("find ignore case", IsChecked(kIgnoreCaseCheckboxID));
//	Preferences::SetInteger("find regular expression", IsChecked(kRegexCheckboxID));
//	
//	SavePosition("find dialog position");
//	
//	Preferences::SetArray("find find strings", mFindStrings);
//	Preferences::SetArray("find replace strings", mReplaceStrings);
//	Preferences::SetArray("find directories", mStartDirectories);
//
////	Preferences::SetInteger("find multi", GetValue(kShowHideMultiTriangleID));
//	Preferences::SetInteger("find multi method", GetValue(kMethodPopupID));
//	Preferences::SetInteger("find only TEXT", IsChecked(kTextFilesOnlyCheckboxID));
//	Preferences::SetInteger("find recursive", IsChecked(kRecursiveCheckboxID));
//
//	::HideWindow(GetSysWindow());
}

void MFindDialog::Initialize()
{//
//	MDialog::Initialize(CFSTR("Find"));
//
//	Install(kEventClassWindow, kEventWindowHandleActivate,
//		this, &MFindDialog::DoWindowActivate);
//
//	RestorePosition("find dialog position");
//	
//	SetChecked(kInSelectionCheckboxID, Preferences::GetInteger("find in selection", 0));
//	SetChecked(kWrapCheckboxID, Preferences::GetInteger("find wrap around", 0));
//	SetChecked(kIgnoreCaseCheckboxID, Preferences::GetInteger("find ignore case", 0));
//	SetChecked(kRegexCheckboxID, Preferences::GetInteger("find regular expression", 0));
//	SetChecked(kEntireWordCheckboxID, Preferences::GetInteger("find entire word", 0));
////	SetChecked(kBatchCheckboxID, Preferences::GetInteger("find batch", 0));
//	SetChecked(kRecursiveCheckboxID, Preferences::GetInteger("find recursive", 0));
//	SetChecked(kTextFilesOnlyCheckboxID, Preferences::GetInteger("find only TEXT", 1));
//	SetChecked(kRecursiveCheckboxID, Preferences::GetInteger("find recursive", 1));
//
//	SetValue(kMethodPopupID, Preferences::GetInteger("find multi method", kMethodDirectory));
//
//	// never set multi-mode automatically
//	mMultiMode = false;
//	SetValue(kShowHideMultiTriangleID, mMultiMode);
//	ShowHideMultiPanel(mMultiMode);
//
//	Preferences::GetArray("find find strings", mFindStrings);
//	SetValues(kFindComboboxID, mFindStrings);
//	
//	Preferences::GetArray("find replace strings", mReplaceStrings);
//	SetValues(kReplaceComboboxID, mReplaceStrings);
//
//	Preferences::GetArray("find directories", mStartDirectories);
//	SetValues(kStartDirComboboxID, mStartDirectories);
//	
//	AddRoute(eIdle, MApplication::Instance().eIdle);
//	
//	SetVisible(kChasingArrowsID, false);
//	SetVisible(kStatusPanelID, false);
}

void MFindDialog::Select()
{//
//	Show(nil);
//	::SelectWindow(GetSysWindow());
//	SetFocus(kFindComboboxID);
}

bool MFindDialog::OKClicked()
{//
//	ustring s;
//	GetText(kFindComboboxID, s);
//	Preferences::SetString("last find", s);
//
//	if (mFindStrings.size() > 0)
//		mFindStrings.erase(mFindStrings.begin());
//		
//	StoreComboText(kFindComboboxID, mFindStrings);
//
//	return false;
}

void MFindDialog::DoFindCommand(
	uint32		inCommand)
{//
//	// check regular expression first?
//	
//	if (mMultiMode and IsChecked(kBatchCheckboxID) and mFindAllThread != nil)
//	{
//		::AlertSoundPlay();
//		return;
//	}
//	
//	mInSelection = IsChecked(kInSelectionCheckboxID);
//	
//	StoreComboText(kFindComboboxID, mFindStrings);
//	if (inCommand != cmd_FindNext)
//		StoreComboText(kReplaceComboboxID, mReplaceStrings);
//
//	ustring what;
//	GetText(kFindComboboxID, what);
//
//	mScrap.PutScrap(what);
//
//	MDocument* doc = MDocument::GetFirstDocument();
//
//	if (mMultiMode)
//	{
//		MURL dir;
//		bool recursive, textFilesOnly;
//		string filter;
//		MMultiMethod method;
//
//		recursive = IsChecked(kRecursiveCheckboxID);
//		textFilesOnly = IsChecked(kTextFilesOnlyCheckboxID);
//		
//		if (IsChecked(kEnableFilterCheckboxID))
//			GetText(kNameFilterEditboxID, filter);
//		
//		switch (GetValue(kMethodPopupID))
//		{
//			case kMethodDirectory:
//			{
//				StoreComboText(kStartDirComboboxID, mStartDirectories);
//				
//				string dirStr;
//				GetText(kStartDirComboboxID, dirStr);
//				dir = dirStr;
//
//				if (not exists(dir) or not is_directory(dir))
//					THROW(("Start directory does not exist or is not a directory"));
//				
//				method = eMMDirectory;
//				break;
//			}
//			
//			case kMethodIncludeFiles:
//				method = eMMIncludes;
//				break;
//			
//			case kMethodOpenWindows:
//				method = eMMOpenWindows;
//				break;
//		}
//
//		if (IsChecked(kBatchCheckboxID))
//		{
//			mFindAllThread = new boost::thread(
//				boost::bind(&MFindDialog::FindAll, this, what,
//					IsChecked(kIgnoreCaseCheckboxID),
//					IsChecked(kRegexCheckboxID),
//					method, dir, recursive, textFilesOnly, filter));
//		}
//		else
//		{
//			mMultiFiles.clear();
//			
//			GetFilesForFindAll(method, dir,
//				recursive, textFilesOnly, filter, mMultiFiles);
//
//			FindNext();
//		}
//	}
//	else if (IsChecked(kBatchCheckboxID))
//	{
//		if (doc != nil)
//		{
//			MMessageList list;
//			
//			doc->FindAll(what, IsChecked(kIgnoreCaseCheckboxID),
//				IsChecked(kRegexCheckboxID), IsChecked(kInSelectionCheckboxID), list);
//
//			if (list.GetCount())
//			{
//				MMessageWindow* w = new MMessageWindow;
//				w->AddMessages(list);
//			}
//		}
//	}
//	else
//	{
//		if (doc != nil)
//			doc->HandleFindDialogCommand(inCommand);
//	}
}

void
MFindDialog::SetFindString(
	const string&	inString,
	bool			inReplaceFirst)
{
	string s = Escape(inString);
	
	if (s != GetFindString())
	{
		if (inReplaceFirst and mFindStrings.size() > 0)
			mFindStrings.erase(mFindStrings.begin());
		
//		SetText(kFindComboboxID, inString);
//		StoreComboText(kFindComboboxID, mFindStrings);
		mFindStrings.push_front(s);
//
//		SetChecked(kRegexCheckboxID, false);
//		SetChecked(kBatchCheckboxID, false);

		mMultiMode = false;
//		SetValue(kShowHideMultiTriangleID, mMultiMode);
//		ShowHideMultiPanel(mMultiMode);
		
//		mScrap.PutScrap(inString);
	}
}

string MFindDialog::GetFindString()
{
	string result;
	
	if (not mFindStrings.empty())
		result = mFindStrings.front();
	
	if (not GetRegex())
		result = Unescape(result);
	
	return result;
}

void MFindDialog::SetReplaceString(
	const string&	inString)
{
	string s = Escape(inString);

	if (s != GetReplaceString())
	{
//		SetText(kReplaceComboboxID, inString);
		mReplaceStrings.push_front(s);
		if (mReplaceStrings.size() > 10)
			mReplaceStrings.pop_back();
	}
}

string MFindDialog::GetReplaceString()
{
	string result;
	
	if (not mReplaceStrings.empty())
		result = mReplaceStrings.front();
	
	if (not GetRegex())
		result = Unescape(result);

	return result;
}

//OSStatus MFindDialog::DoControlHit(EventRef inEvent)
//{
//	OSStatus err = noErr;
//	
//	ControlRef theControl;
//	::GetEventParameter(inEvent, kEventParamDirectObject,
//		typeControlRef, nil, sizeof(theControl), nil, &theControl);
//
//	MCarbonEvent event;
//	
//	ControlID id;
//	if (::GetControlID(theControl, &id) == noErr and id.signature == kJapieSignature)
//	{
//		switch (id.id)
//		{
//			case kFindButtonID:
//				DoFindCommand(cmd_FindNext);
//				break;
//			
//			case kReplaceButtonID:
//				DoFindCommand(cmd_Replace);
//				break;
//			
//			case kReplaceAndFindButtonID:
//				DoFindCommand(cmd_ReplaceFindNext);
//				break;
//			
//			case kReplaceAllButtonID:
//				DoFindCommand(cmd_ReplaceAll);
//				break;
//			
//			case kShowHideMultiTriangleID:
//				ShowHideMultiPanel(GetValue(kShowHideMultiTriangleID) == 1);
//				break;
//			
//			default:
//				err = MDialog::DoControlHit(inEvent);
//				break;
//		}
//	}
//	
//	return err;
//}
//
//OSStatus MFindDialog::DoWindowActivate(
//	EventRef		inEvent)
//{
//	OSStatus result = eventNotHandledErr;
//	
//	ustring txt;
//	if (mScrap.LoadOSScrapIfNewer(txt))
//		SetText(kFindComboboxID, txt);
//	
//	return result;
//}

bool MFindDialog::GetInSelection() const
{
//	return IsChecked(kInSelectionCheckboxID);
	return false;
}

bool MFindDialog::GetWrap() const
{
//	return IsChecked(kWrapCheckboxID);
	return false;
}

bool MFindDialog::GetIgnoreCase() const
{
//	return IsChecked(kIgnoreCaseCheckboxID);
	return false;
}

bool MFindDialog::GetRegex() const
{
//	return IsChecked(kRegexCheckboxID);
	return false;
}

void MFindDialog::ShowHideMultiPanel(
	bool		inShow)
{//
//	Rect sr, cr;
//	::GetWindowBounds(GetSysWindow(), kWindowContentRgn, &cr);
//	::GetWindowBounds(GetSysWindow(), kWindowStructureRgn, &sr);
//	
//	int16 d = (sr.bottom - sr.top) - (cr.bottom - cr.top);
//	
//	mMultiMode = inShow;
//	SetValue(kShowHideMultiTriangleID, inShow);
//	
//	if (mMultiMode)
//	{
//		SetVisible(kMethodPopupID, mMultiMode);
//		SetVisible(kRecursiveCheckboxID, mMultiMode);
//		SetVisible(kStartDirComboboxID, mMultiMode);
//		SetVisible(kTextFilesOnlyCheckboxID, mMultiMode);
//		SetVisible(kEnableFilterCheckboxID, mMultiMode);
//		SetVisible(kNameFilterEditboxID, mMultiMode);
//	}
//	
//	if (mMultiMode)
//		sr.bottom = sr.top + kFindDialogExpandedHeight + d;
//	else
//		sr.bottom = sr.top + kFindDialogCollapsedHeight + d;
//
//	::TransitionWindow(GetSysWindow(), kWindowSlideTransitionEffect,
//		kWindowResizeTransitionAction, &sr);
//	
//	SetFocus(kFindComboboxID);
//
//	if (not mMultiMode)
//	{
//		SetVisible(kMethodPopupID, mMultiMode);
//		SetVisible(kRecursiveCheckboxID, mMultiMode);
//		SetVisible(kStartDirComboboxID, mMultiMode);
//		SetVisible(kTextFilesOnlyCheckboxID, mMultiMode);
//		SetVisible(kEnableFilterCheckboxID, mMultiMode);
//		SetVisible(kNameFilterEditboxID, mMultiMode);
//	}
}

void MFindDialog::StoreComboText(
	uint32			inID,
	StringArray&	inArray)
{//
//	ustring s;
//	GetText(inID, s);
//	
//	if (s.length())	// don't store empty strings
//	{
//		// see if s is in inArray already, if so, remove it.
//		for (StringArray::iterator i = inArray.begin(); i != inArray.end(); ++i)
//		{
//			if (s == *i)
//			{
//				inArray.erase(i);
//				break;
//			}
//		}
//		
//		inArray.insert(inArray.begin(), s);
//		
//		if (inArray.size() > kMaxComboListSize)
//			inArray.erase(inArray.begin() + kMaxComboListSize, inArray.end());
//		
//		SetValues(inID, inArray);
//	}
}

void MFindDialog::FindNext()
{//
//	if (mMultiFiles.size() == 0)
//	{
//		::SysBeep(10);
//		return;
//	}
//	
//	bool found = false;
//	
//	while (not found and mMultiFiles.size() > 0)
//	{
//		try
//		{
//			MURL file = mMultiFiles.front();
//			mMultiFiles.pop_front();
//		
//			MDocument* doc = MDocument::GetDocForFile(file);
//			
//			if (doc != nil and doc->DoFindNext(kDirectionForward))
//				found = true;
//			else
//			{
//				auto_ptr<MDocument> newDoc(new MDocument(&file));
//
//				if (newDoc->DoFindNext(kDirectionForward))
//				{
//					doc = newDoc.release();
//					MEditWindow::DisplayDocument(doc);
//					found = true;
//				}
//			}
//		}
//		catch (...)
//		{
//			found = false;
//		}		
//	}
}

void MFindDialog::FindAll(
	const string&	inWhat,
	bool			inIgnoreCase,
	bool			inRegex,
	MMultiMethod	inMethod,
	MURL			inDirectory,
	bool			inRecursive,
	bool			inTextFilesOnly,
	const string&	inFileNameFilter)
{//
//	try
//	{
//		FileArray files;
//		auto_ptr<MMessageList> list(new MMessageList);
//		
//		GetFilesForFindAll(inMethod, inDirectory,
//			inRecursive, inTextFilesOnly, inFileNameFilter, files);
//		
//		while (files.size() > 0)
//		{
//			SetStatusString(files.front().string());
//			
////			MDocument* doc = MDocument::GetDocForFile(files.front());
////			
////			if (doc != nil)
////				doc->FindAll(inWhat, inIgnoreCase, inRegex, false, *list.get());
////			else
////			{
//				MDocument newDoc(files.front());
//				newDoc.FindAll(inWhat, inIgnoreCase, inRegex, false, *list.get());
////			}
//
//			files.pop_front();
//		}
//		
//		mFindAllResult = list.release();
//	}
//	catch (...)
//	{
//		mFindAllResult = new MMessageList;	// flag failure... sucks.. I know
//		mFindAllResult->AddMessage(kMsgKindError, MURL(), 0, 0, 0, "Error in find all, sorry");
//	}	
}

void MFindDialog::GetFilesForFindAll(
	MMultiMethod	inMethod,
	const MURL&		inDirectory,
	bool			inRecursive,
	bool			inTextFilesOnly,
	const string&	inFileNameFilter,
	FileArray&		outFiles)
{//
//	SetStatusString(inDirectory.string());
//	
//	switch (inMethod)
//	{
//		case eMMDirectory:
//		{
//			uint32 flags;
//			
//			if (inRecursive)
//				flags |= kFileIter_Deep;
//			
//			if (inTextFilesOnly)
//				flags |= kFileIter_TEXTFilesOnly;
//			
//			MFileIterator iter(inDirectory, flags);
//		
//			if (inFileNameFilter.c_str())
//				iter.SetFilter(inFileNameFilter);
//		
//			MURL file;
//			while (iter.Next(file))
//				outFiles.push_back(file);
//			break;
//		}
//		
//		case eMMOpenWindows:
//		{
//			MDocument* doc = MDocument::GetFirstDocument();
//			while (doc != nil)
//			{
//				MURL file = doc->GetFilePath();
//				if (exists(file))
//					outFiles.push_back(file);
//				doc = doc->GetNextDocument();
//			}
//			break;
//		}
//		
//		case eMMIncludes:
//		{
//			MProject* project = MProject::Instance();
//			if (project != nil)
//			{
//				vector<MURL> includePaths;
//				project->GetIncludePaths(includePaths);
//				for (vector<MURL>::iterator p = includePaths.begin(); p != includePaths.end(); ++p)
//					GetFilesForFindAll(eMMDirectory, *p, false, false, "", outFiles);
//			}
//			break;
//		}
//	}
}

void MFindDialog::SetStatusString(
	const string&		inMessage)
{//
//	if (mFindAllThread != nil)
//	{
//		boost::mutex::scoped_lock lock(mFindDialogMutex);
//		mCurrentMultiFile = inMessage;
//	}
//	else
//		SetText(kStatusPanelID, mCurrentMultiFile);
}

void MFindDialog::Idle()
{//
//	if (mFindAllThread != nil)
//	{
//		boost::mutex::scoped_lock lock(mFindDialogMutex);
//	
//		if (mFindAllResult != nil)
//		{
//			auto_ptr<MMessageList> list(mFindAllResult);
//			mFindAllResult = nil;
//
//			SetVisible(kChasingArrowsID, false);
//			SetVisible(kStatusPanelID, false);
//
//			mFindAllThread->join();
//			delete mFindAllThread;
//			
//			mFindAllThread = nil;
//			
//			if (list->GetCount() > 0)
//			{
//				MMessageWindow* w = new MMessageWindow;
//				w->AddMessages(*list.get());
//			}
//			else
//				::AlertSoundPlay();
//		}
//		else
//		{
//			SetVisible(kChasingArrowsID, true);
//			SetVisible(kStatusPanelID, true);
//
//			SetText(kStatusPanelID, mCurrentMultiFile);
//		}
//	}
}

