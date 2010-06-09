//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

/*	$Id: MFindDialog.cpp 170 2007-06-12 20:55:53Z maarten $
	Copyright Maarten L. Hekkelman
	Created Sunday August 15 2004 20:47:19
*/

#include "MJapi.h"

#include <boost/bind.hpp>

#include "MFindDialog.h"
#include "MTextDocument.h"
#include "MEditWindow.h"
#include "MPreferences.h"
#include "MUtils.h"
//#include "MMessageWindow.h"
#include "MCommands.h"
#include "MStrings.h"
//#include "MProject.h"
#include "MSound.h"
#include "MAlerts.h"
#include "MError.h"
#include "MJapiApp.h"

using namespace std;

namespace {
	
const uint32
	kMaxComboListSize =			10;

enum {
	kFindButtonID =				'btnf',
	kReplaceAndFindButtonID =	'btrf',
	kReplaceButtonID =			'btnr',
	kReplaceAllButtonID =		'btra',
	kFindComboboxID =			'find',
	kReplaceComboboxID =	 	'repl',
	kInSelectionCheckboxID = 	'insl',
	kWrapCheckboxID =			'wrap',
	kIgnoreCaseCheckboxID =		'ignc',
	kRegexCheckboxID = 			'regx',
	kEntireWordCheckboxID =		'word',
	kBatchCheckboxID =			'btch',
	
	// --- multi
	
	kMultiFileExpanderID	 =	'exp1',
	kMethodPopupID = 			'meth',
	kRecursiveCheckboxID =		'recu',
	kStartDirComboboxID =		'sdir',
	kEnableFilterCheckboxID =	'ffnm',
	kNameFilterEditboxID =		'filt',
	kBrowseStartDirButtonID =	'chdr',
	
	// --- status
	
	kChasingArrowsID =			301,
	kStatusPanelID =			'curf'
};

enum {
	kMethodDirectory = 			1,
	kMethodOpenWindows = 		2,
	kMethodIncludeFiles =		3
};

enum {
	kReplaceAll_Save =			1,
	kReplaceAll_LeaveOpen =		2
};

const int16
	kFindDialogCollapsedHeight	= 179,
	kFindDialogExpandedHeight	= 281;

}

MFindDialog* MFindDialog::sInstance = nil;

MFindDialog& MFindDialog::Instance()
{
	if (sInstance == nil)
		sInstance = new MFindDialog();
	return *sInstance;
}

MFindDialog::MFindDialog()
	: MDialog("find-dialog")
	, eIdle(this, &MFindDialog::Idle)
	, mUpdatingComboBox(true)
	, mFindStringChanged(false)
	, mReplaceStringChanged(false)
	, mStartDirectoriesChanged(false)
	, mVisible(false)
	, mFindAllThread(nil)
	, mFindAllResult(nil)
{	
	RestorePosition("find dialog position");
	
	//SetChecked(kInSelectionCheckboxID, Preferences::GetInteger("find in selection", 0));
	//SetChecked(kWrapCheckboxID, Preferences::GetInteger("find wrap around", 0));
	//SetChecked(kIgnoreCaseCheckboxID, Preferences::GetInteger("find ignore case", 0));
	//SetChecked(kRegexCheckboxID, Preferences::GetInteger("find regular expression", 0));
	//SetChecked(kEntireWordCheckboxID, Preferences::GetInteger("find entire word", 0));
	//SetChecked(kRecursiveCheckboxID, Preferences::GetInteger("find recursive", 0));
	//SetChecked(kRecursiveCheckboxID, Preferences::GetInteger("find recursive", 1));

	//SetChecked(kEnableFilterCheckboxID, Preferences::GetInteger("find name filter enabled", 1));
	//SetText(kNameFilterEditboxID, Preferences::GetString("find name filter", "*.h;*.cpp;*.inl"));

	//SetValue(kMethodPopupID, Preferences::GetInteger("find multi method", kMethodDirectory));

	//// never set multi-mode automatically
	//mMultiMode = false;
	//SetExpanded(kMultiFileExpanderID, mMultiMode);

	//Preferences::GetArray("find find strings", mFindStrings);
	//SetValues(kFindComboboxID, mFindStrings);
	//
	//Preferences::GetArray("find replace strings", mReplaceStrings);
	//SetValues(kReplaceComboboxID, mReplaceStrings);

	//Preferences::GetArray("find directories", mStartDirectories);
	//SetValues(kStartDirComboboxID, mStartDirectories);
	
	AddRoute(eIdle, gApp->eIdle);
	
//	SetVisible(kChasingArrowsID, false);
	//SetText(kStatusPanelID, "");
	
	mUpdatingComboBox = false;
}

MFindDialog::~MFindDialog()
{
	sInstance = nil;
	RemoveWindowFromList(this);
}

bool MFindDialog::ProcessCommand(
	uint32			inCommand,
	const MMenu*	inMenu,
	uint32			inItemIndex,
	uint32			inModifiers)
{
	bool result = true;
	
	switch (inCommand)
	{
		case cmd_Stop:
			result = Stop();
			break;
		
		default:
			result = MDialog::ProcessCommand(inCommand, inMenu, inItemIndex, inModifiers);
			break;
	}
	
	return result;
}

bool MFindDialog::DoClose()
{
	if (mVisible)
	{
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
	////	Preferences::SetInteger("find multi", GetValue(kMultiFileExpanderID));
	//	Preferences::SetInteger("find multi method", GetValue(kMethodPopupID));
	//	Preferences::SetInteger("find recursive", IsChecked(kRecursiveCheckboxID));
	//
	//	Preferences::SetInteger("find name filter enabled", IsChecked(kEnableFilterCheckboxID));
	//	
	//	string s;
	//	GetText(kNameFilterEditboxID, s);
	//	Preferences::SetString("find name filter", s);
	
		Hide();
		
		mVisible = false;
	}

	return false;
}

void MFindDialog::Select()
{
	RestorePosition("find dialog position");
	Show(nil);
	MWindow::Select();
	//SetFocus(kFindComboboxID);
	mVisible = true;
}

bool MFindDialog::OKClicked()
{
	DoFindCommand(cmd_FindNext);
	return false;
}

void MFindDialog::DoFindCommand(
	uint32		inCommand)
{
	//// check regular expression first?
	//
	//if (mMultiMode and IsChecked(kBatchCheckboxID) and mFindAllThread != nil)
	//{
	//	PlaySound("warning");
	//	return;
	//}
	//
	//mInSelection = IsChecked(kInSelectionCheckboxID);
	//MTextDocument* doc = MTextDocument::GetFirstTextDocument();

	//if (IsExpanded(kMultiFileExpanderID))
	//{
	//	fs::path dir;
	//	bool recursive;
	//	string filter;
	//	MMultiMethod method = eMMDirectory;

	//	recursive = IsChecked(kRecursiveCheckboxID);
	//	
	//	if (IsChecked(kEnableFilterCheckboxID))
	//		GetText(kNameFilterEditboxID, filter);
	//	
	//	switch (GetValue(kMethodPopupID))
	//	{
	//		case kMethodDirectory:
	//		{
	//			dir = GetStartDirectory();

	//			if (not exists(dir) or not is_directory(dir))
	//				THROW(("Start directory does not exist or is not a directory"));
	//			
	//			method = eMMDirectory;
	//			break;
	//		}
	//		
	//		case kMethodIncludeFiles:
	//			method = eMMIncludes;
	//			break;
	//		
	//		case kMethodOpenWindows:
	//			method = eMMOpenWindows;
	//			break;
	//	}

	//	if (IsChecked(kBatchCheckboxID))
	//	{
	//		mFindAllThread = new boost::thread(
	//			boost::bind(&MFindDialog::FindAll, this, GetFindString(),
	//				IsChecked(kIgnoreCaseCheckboxID),
	//				IsChecked(kRegexCheckboxID),
	//				method, dir, recursive, filter));
	//	}
	//	else
	//	{
	//		mMultiFiles.clear();
	//		
	//		FileSet files;
	//		GetFilesForFindAll(method, dir, recursive, filter, files);
	//		copy(files.begin(), files.end(), back_inserter(mMultiFiles));

	//		switch (inCommand)
	//		{
	//			case cmd_FindNext:
	//				FindNext();
	//				break;
	//			
	//			case cmd_ReplaceAll:
	//				switch (DisplayAlert("replace-all-alert"))
	//				{
	//					case kReplaceAll_Save:
	//						ReplaceAll(true);
	//						break;
	//					
	//					case kReplaceAll_LeaveOpen:
	//						ReplaceAll(false);
	//						break;
	//				}
	//				break;
	//		}
	//	}
	//}
	//else if (IsChecked(kBatchCheckboxID))
	//{
	//	if (doc != nil)
	//	{
	//		MMessageList list;
	//		
	//		doc->FindAll(GetFindString(), IsChecked(kIgnoreCaseCheckboxID),
	//			IsChecked(kRegexCheckboxID), IsChecked(kInSelectionCheckboxID), list);

	//		if (list.GetCount())
	//		{
	//			MMessageWindow* w = new MMessageWindow("");
	//			w->SetMessages(
	//				FormatString("Found ^0 hits for ^1",
	//					list.GetCount(), mFindStrings.front()),
	//				list);
	//		}
	//	}
	//}
	//else
	//{
	//	if (doc != nil)
	//		doc->HandleFindDialogCommand(inCommand);
	//}
}

void
MFindDialog::SetFindString(
	const string&	inString,
	bool			inReplaceFirst)
{
	string s = Escape(inString);
	
	if (mFindStrings.empty() or mFindStrings.front() != s)
	{
		if (inReplaceFirst and not mFindStrings.empty())
			mFindStrings.front() = s;
		else
		{
			mFindStrings.erase(
				remove(mFindStrings.begin(), mFindStrings.end(), s),
				mFindStrings.end());

			mFindStrings.insert(mFindStrings.begin(), s);

			if (mFindStrings.size() > kMaxComboListSize)
				mFindStrings.pop_back();
		}

		//SetValues(kFindComboboxID, mFindStrings);
		//
		//SetChecked(kRegexCheckboxID, false);
		//SetChecked(kBatchCheckboxID, false);

		//mMultiMode = false;
		//SetExpanded(kMultiFileExpanderID, mMultiMode);
	}
}

string MFindDialog::GetFindString()
{
	string result;

	//if (mFindStringChanged)
	//{
	//	GetText(kFindComboboxID, result);

	//	// place in front of list, removing other occurrences if any
	//	mFindStrings.erase(
	//		remove(mFindStrings.begin(), mFindStrings.end(), result),
	//		mFindStrings.end());

	//	mFindStrings.insert(mFindStrings.begin(), result);

	//	if (mFindStrings.size() > kMaxComboListSize)
	//		mFindStrings.pop_back();

	//	mUpdatingComboBox = true;
	//	SetValues(kFindComboboxID, mFindStrings);
	//	mUpdatingComboBox = false;
	//	mFindStringChanged = false;
	//}
	//else 
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

	if (mReplaceStrings.empty() or mReplaceStrings.front() != s)
	{
		mReplaceStrings.erase(
			remove(mReplaceStrings.begin(), mReplaceStrings.end(), s),
			mReplaceStrings.end());

		mReplaceStrings.insert(mReplaceStrings.begin(), s);

		if (mReplaceStrings.size() > kMaxComboListSize)
			mReplaceStrings.pop_back();

		//SetValues(kReplaceComboboxID, mReplaceStrings);
		//
		//SetChecked(kRegexCheckboxID, false);
		//SetChecked(kBatchCheckboxID, false);

		//mMultiMode = false;
		//SetExpanded(kMultiFileExpanderID, mMultiMode);
	}
}

string MFindDialog::GetReplaceString()
{
	string result;

	//if (mReplaceStringChanged)
	//{
	//	GetText(kReplaceComboboxID, result);

	//	// place in front of list, removing other occurrences if any
	//	mReplaceStrings.erase(
	//		remove(mReplaceStrings.begin(), mReplaceStrings.end(), result),
	//		mReplaceStrings.end());

	//	mReplaceStrings.insert(mReplaceStrings.begin(), result);

	//	if (mReplaceStrings.size() > kMaxComboListSize)
	//		mReplaceStrings.pop_back();

	//	mUpdatingComboBox = true;
	//	SetValues(kReplaceComboboxID, mReplaceStrings);
	//	mUpdatingComboBox = false;
	//	mFindStringChanged = false;
	//}
	//else
	if (not mReplaceStrings.empty())
		result = mReplaceStrings.front();
	
	if (not GetRegex())
		result = Unescape(result);
	
	return result;
}

string MFindDialog::GetStartDirectory()
{
	string result;

	//if (mStartDirectoriesChanged)
	//{
	//	GetText(kStartDirComboboxID, result);

	//	// place in front of list, removing other occurrences if any
	//	mStartDirectories.erase(
	//		remove(mStartDirectories.begin(), mStartDirectories.end(), result),
	//		mStartDirectories.end());

	//	mStartDirectories.insert(mStartDirectories.begin(), result);

	//	if (mStartDirectories.size() > kMaxComboListSize)
	//		mStartDirectories.pop_back();

	//	mUpdatingComboBox = true;
	//	SetValues(kStartDirComboboxID, mStartDirectories);
	//	mUpdatingComboBox = false;
	//	mStartDirectoriesChanged = false;
	//}
	//else
	if (not mStartDirectories.empty())
		result = mStartDirectories.front();
	
	return result;
}

bool MFindDialog::GetInSelection() const
{
	//return IsChecked(kInSelectionCheckboxID);
	return false;
}

bool MFindDialog::GetWrap() const
{
	//return IsChecked(kWrapCheckboxID);
	return false;
}

bool MFindDialog::GetIgnoreCase() const
{
	//return IsChecked(kIgnoreCaseCheckboxID);
	return false;
}

bool MFindDialog::GetRegex() const
{
	//return IsChecked(kRegexCheckboxID);
	return false;
}

void MFindDialog::ValueChanged(
	uint32		inButonID)
{
	string s;
	
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
		
		case kBrowseStartDirButtonID:
			SelectSearchDir();
			break;
		
		case kMultiFileExpanderID:
			//SetVisible(kStatusPanelID, false);
			break;
		
		case kBatchCheckboxID:
		{
			//bool batch = IsChecked(kBatchCheckboxID);
		
			//SetEnabled(kReplaceAllButtonID, not batch);
			//SetEnabled(kReplaceButtonID, not batch);
			//SetEnabled(kReplaceAndFindButtonID, not batch);
			break;
		}
		
		case kMethodPopupID:
		{
			//switch (GetValue(kMethodPopupID))
			//{
			//	case kMethodDirectory:
			//		SetEnabled(kStartDirComboboxID, true);
			//		SetEnabled(kBrowseStartDirButtonID, true);
			//		SetEnabled(kRecursiveCheckboxID, true);
			//		SetEnabled(kEnableFilterCheckboxID, true);
			//		SetEnabled(kNameFilterEditboxID, IsChecked(kEnableFilterCheckboxID));
			//		break;
			//	
			//	case kMethodIncludeFiles:
			//		SetEnabled(kStartDirComboboxID, false);
			//		SetEnabled(kBrowseStartDirButtonID, false);
			//		SetEnabled(kRecursiveCheckboxID, true);
			//		SetEnabled(kEnableFilterCheckboxID, true);
			//		SetEnabled(kNameFilterEditboxID, IsChecked(kEnableFilterCheckboxID));
			//		break;
			//	
			//	case kMethodOpenWindows:
			//		SetEnabled(kStartDirComboboxID, false);
			//		SetEnabled(kBrowseStartDirButtonID, false);
			//		SetEnabled(kRecursiveCheckboxID, false);
			//		SetEnabled(kEnableFilterCheckboxID, false);
			//		SetEnabled(kNameFilterEditboxID, false);
			//		break;
			//}
			break;
		}
		
		case kFindComboboxID:
			if (not mUpdatingComboBox)
				mFindStringChanged = true;
			break;
		
		case kReplaceComboboxID:
			if (not mUpdatingComboBox)
				mReplaceStringChanged = true;
			break;
		
		case kStartDirComboboxID:
			if (not mUpdatingComboBox)
				mStartDirectoriesChanged = true;
			break;
		
		case kEnableFilterCheckboxID:
			//SetEnabled(kNameFilterEditboxID, IsChecked(kEnableFilterCheckboxID));
			break;
		
		default:
			//MDialog::ValueChanged(inButonID);
			break;
	}
}

bool MFindDialog::FindNext()
{
	if (mMultiFiles.size() == 0)
	{
		PlaySound("warning");
		return false;
	}
	
	bool found = false;
	
	while (not found and mMultiFiles.size() > 0)
	{
		try
		{
			MFile file(mMultiFiles.front());
			mMultiFiles.pop_front();
		
			MTextDocument* doc = dynamic_cast<MTextDocument*>(
				MDocument::GetDocumentForFile(file));
			
			if (doc != nil)
				found = doc->DoFindFirst();
			else
			{
				//unique_ptr<MTextDocument> newDoc(MDocument::Create<MTextDocument>(file));

				//if (newDoc->DoFindNext(kDirectionForward))
				//{
				//	doc = newDoc.release();
				//	found = true;
				//}
			}
			
			if (found and doc != nil)
			{
				MSelection s = doc->GetSelection();
				
				gApp->DisplayDocument(doc);
				
				// center the found and selected text
				doc->Select(s.GetAnchor(), s.GetCaret(),
					kScrollCenterSelection);
			}
		}
		catch (...)
		{
			found = false;
		}		
	}
	
	if (not found)
		PlaySound("warning");

	return found;
}

void MFindDialog::ReplaceAll(
	bool				inSaveToDisk)
{
	mStopFindAll = false;
	
	try
	{
		while (not mStopFindAll and mMultiFiles.size() > 0)
		{
			MFile file(mMultiFiles.front());
			mMultiFiles.pop_front();
		
			SetStatusString(file.GetPath().string());
			
			bool found = false;
			
			MTextDocument* doc = dynamic_cast<MTextDocument*>(
				MDocument::GetDocumentForFile(file));
			
			if (doc != nil)
				found = doc->DoFindFirst();
			else
			{
				//unique_ptr<MTextDocument> newDoc(MDocument::Create<MTextDocument>(file));

				//if (newDoc->DoFindFirst())
				//{
				//	doc = newDoc.release();
				//	found = true;
				//}
			}
			
			if (found and doc != nil)
			{
				doc->DoReplaceAll();
				if (inSaveToDisk and doc->DoSave())
				{
					MController* controller = doc->GetFirstController();
					if (controller != nil)
						controller->ProcessCommand(cmd_Close, nil, 0, 0);
					else
						delete doc;
				}
				else
					gApp->DisplayDocument(doc);
			}
		}
	}
	catch (exception& e)
	{
		DisplayError(e);
	}	
}

void MFindDialog::FindAll(
	const string&	inWhat,
	bool			inIgnoreCase,
	bool			inRegex,
	MMultiMethod	inMethod,
	fs::path			inDirectory,
	bool			inRecursive,
	const string&	inFileNameFilter)
{
	//mStopFindAll = false;
	//
	//try
	//{
	//	FileSet files;
	//	unique_ptr<MMessageList> list(new MMessageList);
	//	
	//	GetFilesForFindAll(inMethod, inDirectory,
	//		inRecursive, inFileNameFilter, files);
	//	
	//	for (FileSet::iterator file = files.begin(); file != files.end(); ++file)
	//	{
	//		if (mStopFindAll)
	//			break;
	//		
	//		SetStatusString(file->string());
	//		MFile url(*file);
	//		
	//		bool searched = false;
	//		
	//		//gdk_threads_enter();
	//		MTextDocument* doc = dynamic_cast<MTextDocument*>(MDocument::GetDocumentForFile(url));

	//		if (doc != nil)
	//		{
	//			doc->FindAll(inWhat, inIgnoreCase, inRegex, false, *list.get());
	//			searched = true;
	//		}
	//		//gdk_threads_leave();
	//		
	//		if (not searched)
	//			MTextDocument::FindAll(*file, inWhat, inIgnoreCase, inRegex, false, *list.get());
	//	}
	//	
	//	mFindAllResult = list.release();
	//}
	//catch (exception& e)
	//{
	//	mFindAllResult = new MMessageList;	// flag failure... sucks.. I know
	//	mFindAllResult->AddMessage(kMsgKindError, MFile(), 0, 0, 0, "Error in find all, sorry");
	//	mFindAllResult->AddMessage(kMsgKindError, MFile(), 0, 0, 0, e.what());
	//}	
	//catch (...)
	//{
	//	mFindAllResult = new MMessageList;	// flag failure... sucks.. I know
	//	mFindAllResult->AddMessage(kMsgKindError, MFile(), 0, 0, 0, "Error in find all, sorry");
	//}	
}

void MFindDialog::GetFilesForFindAll(
	MMultiMethod	inMethod,
	const fs::path&	inDirectory,
	bool			inRecursive,
	const string&	inFileNameFilter,
	FileSet&		outFiles)
{
	SetStatusString(inDirectory.string());
	
	switch (inMethod)
	{
		case eMMDirectory:
		{
			uint32 flags = 0;
			
			if (inRecursive)
				flags |= kFileIter_Deep;
			
			MFileIterator iter(inDirectory, flags);
		
			if (inFileNameFilter.c_str())
				iter.SetFilter(inFileNameFilter);
		
			fs::path file;
			while (iter.Next(file))
				outFiles.insert(file);
			break;
		}
		
		case eMMOpenWindows:
		{
			MDocument* doc = MDocument::GetFirstDocument();
			while (doc != nil)
			{
				fs::path file = doc->GetFile().GetPath();
				if (exists(file))
					outFiles.insert(file);
				doc = doc->GetNextDocument();
			}
			break;
		}
		
		case eMMIncludes:
		{
			//MProject* project = MProject::Instance();
			//if (project != nil)
			//{
			//	vector<fs::path> includePaths;

			//	project->GetIncludePaths(includePaths);

			//	sort(includePaths.begin(), includePaths.end());
			//	includePaths.erase(unique(includePaths.begin(), includePaths.end()), includePaths.end());

			//	for (vector<fs::path>::iterator p = includePaths.begin(); p != includePaths.end(); ++p)
			//		GetFilesForFindAll(eMMDirectory, *p, inRecursive, inFileNameFilter, outFiles);
			//}
			break;
		}
	}
}

void MFindDialog::SetStatusString(
	const string&		inMessage)
{
	if (mFindAllThread != nil)
	{
		boost::mutex::scoped_lock lock(mFindDialogMutex);
		mCurrentMultiFile = inMessage;
	}
	//else
	//	SetText(kStatusPanelID, mCurrentMultiFile);
}

void MFindDialog::SelectSearchDir()
{
	//fs::path dir;
	//if (ChooseDirectory(dir))
	//{
	//	mStartDirectories.erase(
	//		remove(mStartDirectories.begin(), mStartDirectories.end(), dir.string()),
	//		mStartDirectories.end());

	//	mStartDirectories.insert(mStartDirectories.begin(), dir.string());

	//	if (mStartDirectories.size() > kMaxComboListSize)
	//		mStartDirectories.erase(mStartDirectories.end() - 1);
	//	
	//	mUpdatingComboBox = true;
	//	SetValues(kStartDirComboboxID, mStartDirectories);
	//	mUpdatingComboBox = false;
	//	mStartDirectoriesChanged = false;
	//}
}

void MFindDialog::Idle(
	double	inSystemTime)
{
//	if (mFindAllThread != nil)
//	{
//		boost::mutex::scoped_lock lock(mFindDialogMutex);
//	
//		if (mFindAllResult != nil)
//		{
//			unique_ptr<MMessageList> list(mFindAllResult);
//			mFindAllResult = nil;
//
////			SetVisible(kChasingArrowsID, false);
//			SetVisible(kStatusPanelID, false);
//
//			mFindAllThread->join();
//			delete mFindAllThread;
//			
//			mFindAllThread = nil;
//			
//			if (list->GetCount() > 0)
//			{
//				MMessageWindow* w = new MMessageWindow("");
//				w->SetMessages(
//					FormatString("Found ^0 hits for ^1",
//						list->GetCount(), mFindStrings.front()),
//					*list.get());
//			}
//			else
//				PlaySound("warning");
//		}
//		else
//		{
////			SetVisible(kChasingArrowsID, true);
//			SetVisible(kStatusPanelID, true);
//
//			SetText(kStatusPanelID, mCurrentMultiFile);
//		}
//	}
}

bool MFindDialog::Stop()
{
	bool result = false;
	
	if (mFindAllThread != nil)
	{
		mStopFindAll = true;
		result = true;
	}
	
	return result;
}
