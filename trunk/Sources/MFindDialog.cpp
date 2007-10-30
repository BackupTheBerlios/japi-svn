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
	kFindButtonID =				1001,
	kReplaceAndFindButtonID =	1002,
	kReplaceButtonID =			1003,
	kReplaceAllButtonID =		1004,
	kFindComboboxID =			'find',
	kReplaceComboboxID =	 	'repl',
	kInSelectionCheckboxID = 	'insl',
	kWrapCheckboxID =			'wrap',
	kIgnoreCaseCheckboxID =		'ignc',
	kRegexCheckboxID = 			'regx',
	kEntireWordCheckboxID =		'word',
	kBatchCheckboxID =			'btch',
	
	// --- multi
	
	kMultiFileExpanderID	 =	201,
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
//	, mFindAllThread(nil)
	, mFindAllResult(nil)
//	, mScrap(kScrapFindScrap)
{
	SetTitle("Find");
	
	AddTable('tbl1', 2, 2);
	AddStaticText('lbl1', "Find", 'tbl1');
	AddComboBoxEntry(kFindComboboxID, 'tbl1');
	AddStaticText('lbl2', "Replace", 'tbl1');
	AddComboBoxEntry(kReplaceComboboxID, 'tbl1');

//	AddHSeparator('sep1');

	AddAlignment('ali2', 0.5, 0, 0.0, 0.0);
	
	AddTable('tbl2', 3, 2, 'ali2');
	AddCheckBox(kIgnoreCaseCheckboxID, "Ignore Case", 'tbl2');
	AddCheckBox(kInSelectionCheckboxID, "In Selection", 'tbl2');
	AddCheckBox(kRegexCheckboxID, "Regular Expression", 'tbl2');
	AddCheckBox(kWrapCheckboxID, "Wrap Around", 'tbl2');
	AddCheckBox(kEntireWordCheckboxID, "Entire Word", 'tbl2');
	AddCheckBox(kBatchCheckboxID, "Batch", 'tbl2');

	AddHSeparator('sep2');
	
	AddExpander(kMultiFileExpanderID, "Multi File Search");
	
	AddVBox('vbx1', false, 0, kMultiFileExpanderID);

	AddTable('tbl3', 3, 2, 'vbx1');
	AddStaticText('lbl3', "Method", 'tbl3');
	vector<string> methods;
	methods.push_back("Directory Search");
	methods.push_back("Open Windows");
	methods.push_back("Include Files");
	AddComboBox(kMethodPopupID, methods, 'tbl3');
	AddCheckBox(kRecursiveCheckboxID, "Recursive", 'tbl3');

	AddStaticText('lbl4', "Starting Directory", 'tbl3');
	AddComboBoxEntry(kStartDirComboboxID, 'tbl3');
	AddButton('xxxx', "...", 'tbl3');
	
	AddHBox('hbx1', false, 0, 'vbx1');
	AddCheckBox(kTextFilesOnlyCheckboxID, "Text Files Only", 'hbx1');
	AddCheckBox(kEnableFilterCheckboxID, "Filter File Name", 'hbx1');
	AddEditField(kNameFilterEditboxID, "", 'hbx1');
	
	// sep3
	
	AddButton(kReplaceAllButtonID, "Replace All");
	AddButton(kReplaceButtonID, "Replace");
	AddButton(kReplaceAndFindButtonID, "Replace & Find");
	AddButton(kFindButtonID, "Find");
}

bool MFindDialog::DoClose()
{
	Preferences::SetInteger("find in selection", IsChecked(kInSelectionCheckboxID));
	Preferences::SetInteger("find wrap around", IsChecked(kWrapCheckboxID));
	Preferences::SetInteger("find ignore case", IsChecked(kIgnoreCaseCheckboxID));
	Preferences::SetInteger("find regular expression", IsChecked(kRegexCheckboxID));
	
	SavePosition("find dialog position");
	
	Preferences::SetArray("find find strings", mFindStrings);
	Preferences::SetArray("find replace strings", mReplaceStrings);
	Preferences::SetArray("find directories", mStartDirectories);

////	Preferences::SetInteger("find multi", GetValue(kMultiFileExpanderID));
//	Preferences::SetInteger("find multi method", GetValue(kMethodPopupID));
//	Preferences::SetInteger("find only TEXT", IsChecked(kTextFilesOnlyCheckboxID));
//	Preferences::SetInteger("find recursive", IsChecked(kRecursiveCheckboxID));

	Hide();
	
	return false;
}

void MFindDialog::Initialize()
{//
//	MDialog::Initialize(CFSTR("Find"));
//
//	Install(kEventClassWindow, kEventWindowHandleActivate,
//		this, &MFindDialog::DoWindowActivate);

	RestorePosition("find dialog position");
	
	SetChecked(kInSelectionCheckboxID, Preferences::GetInteger("find in selection", 0));
	SetChecked(kWrapCheckboxID, Preferences::GetInteger("find wrap around", 0));
	SetChecked(kIgnoreCaseCheckboxID, Preferences::GetInteger("find ignore case", 0));
	SetChecked(kRegexCheckboxID, Preferences::GetInteger("find regular expression", 0));
	SetChecked(kEntireWordCheckboxID, Preferences::GetInteger("find entire word", 0));
//	SetChecked(kBatchCheckboxID, Preferences::GetInteger("find batch", 0));
	SetChecked(kRecursiveCheckboxID, Preferences::GetInteger("find recursive", 0));
	SetChecked(kTextFilesOnlyCheckboxID, Preferences::GetInteger("find only TEXT", 1));
	SetChecked(kRecursiveCheckboxID, Preferences::GetInteger("find recursive", 1));

	SetValue(kMethodPopupID, Preferences::GetInteger("find multi method", kMethodDirectory));

	// never set multi-mode automatically
	mMultiMode = false;
	SetExpanded(kMultiFileExpanderID, mMultiMode);
//	SetExpanded(mMultiMode);

	Preferences::GetArray("find find strings", mFindStrings);
	SetValues(kFindComboboxID, mFindStrings);
	
	Preferences::GetArray("find replace strings", mReplaceStrings);
	SetValues(kReplaceComboboxID, mReplaceStrings);

	Preferences::GetArray("find directories", mStartDirectories);
	SetValues(kStartDirComboboxID, mStartDirectories);
	
//	AddRoute(eIdle, MApplication::Instance().eIdle);
//	
//	SetVisible(kChasingArrowsID, false);
//	SetVisible(kStatusPanelID, false);
}

void MFindDialog::Select()
{
	RestorePosition("find dialog position");
	Show(nil);
	MWindow::Select();
	SetFocus(kFindComboboxID);
}

bool MFindDialog::OKClicked()
{
	string s;
	GetText(kFindComboboxID, s);
	Preferences::SetString("last find", s);

	StoreComboText(kFindComboboxID, s, mFindStrings);

	return false;
}

void MFindDialog::DoFindCommand(
	uint32		inCommand)
{
	// check regular expression first?
	
//	if (mMultiMode and IsChecked(kBatchCheckboxID) and mFindAllThread != nil)
//	{
//		::AlertSoundPlay();
//		return;
//	}
	
	mInSelection = IsChecked(kInSelectionCheckboxID);

	string what;
	GetText(kFindComboboxID, what);
	
	StoreComboText(kFindComboboxID, what, mFindStrings);
	if (inCommand != cmd_FindNext)
	{
		string with;
		GetText(kReplaceComboboxID, with);
		StoreComboText(kReplaceComboboxID, with, mReplaceStrings);
	}

	MDocument* doc = MDocument::GetFirstDocument();

	if (IsExpanded(kMultiFileExpanderID))
	{
//		MPath dir;
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
	}
	else if (IsChecked(kBatchCheckboxID))
	{
		if (doc != nil)
		{
			MMessageList list;
			
			doc->FindAll(what, IsChecked(kIgnoreCaseCheckboxID),
				IsChecked(kRegexCheckboxID), IsChecked(kInSelectionCheckboxID), list);

			if (list.GetCount())
			{
				MMessageWindow* w = new MMessageWindow;
				w->AddMessages(list);
			}
		}
	}
	else
	{
		if (doc != nil)
			doc->HandleFindDialogCommand(inCommand);
	}
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
		
		StoreComboText(kFindComboboxID, inString, mFindStrings);

		SetChecked(kRegexCheckboxID, false);
		SetChecked(kBatchCheckboxID, false);

		mMultiMode = false;
		SetExpanded(kMultiFileExpanderID, mMultiMode);
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
		if (mReplaceStrings.size() > 10)
			mReplaceStrings.pop_back();

		StoreComboText(kReplaceComboboxID, inString, mReplaceStrings);
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

void MFindDialog::ButtonClicked(
	uint32		inButonID)
{
	switch (inButonID)
	{
		case kFindButtonID:
			DoFindCommand(cmd_FindNext);
			break;
		
		case kReplaceButtonID:
			DoFindCommand(cmd_Replace);
			break;
		
		case kReplaceAndFindButtonID:
			DoFindCommand(cmd_ReplaceFindNext);
			break;
		
		case kReplaceAllButtonID:
			DoFindCommand(cmd_ReplaceAll);
			break;
		
//		case kMultiFileExpanderID:
//			ShowHideMultiPanel(GetValue(kMultiFileExpanderID) == 1);
//			break;
	}
}

bool MFindDialog::GetInSelection() const
{
	return IsChecked(kInSelectionCheckboxID);
}

bool MFindDialog::GetWrap() const
{
	return IsChecked(kWrapCheckboxID);
}

bool MFindDialog::GetIgnoreCase() const
{
	return IsChecked(kIgnoreCaseCheckboxID);
}

bool MFindDialog::GetRegex() const
{
	return IsChecked(kRegexCheckboxID);
}

void MFindDialog::StoreComboText(
	uint32			inID,
	const string&	inActiveString,
	StringArray&	inArray)
{
	inArray.erase(remove(inArray.begin(), inArray.end(), inActiveString), inArray.end());
	inArray.insert(inArray.begin(), inActiveString);
	
	if (inArray.size() > kMaxComboListSize)
		inArray.erase(inArray.begin() + kMaxComboListSize, inArray.end());
	
	SetValues(inID, inArray);
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
//			MPath file = mMultiFiles.front();
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
	MPath			inDirectory,
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
//		mFindAllResult->AddMessage(kMsgKindError, MPath(), 0, 0, 0, "Error in find all, sorry");
//	}	
}

void MFindDialog::GetFilesForFindAll(
	MMultiMethod	inMethod,
	const MPath&		inDirectory,
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
//			MPath file;
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
//				MPath file = doc->GetFilePath();
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
//				vector<MPath> includePaths;
//				project->GetIncludePaths(includePaths);
//				for (vector<MPath>::iterator p = includePaths.begin(); p != includePaths.end(); ++p)
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

