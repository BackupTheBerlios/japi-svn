//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "MJapi.h"

#include <cstring>
#include <boost/algorithm/string.hpp>

#include "MDocument.h"
#include "MUtils.h"
#include "MError.h"
#include "MPreferences.h"
#include "MDocClosedNotifier.h"
#include "MJapiApp.h"

using namespace std;
namespace ba = boost::algorithm;

MDocument* MDocument::sFirst;

// ---------------------------------------------------------------------------
//	MDocument

MDocument::MDocument(
	const MFile&	inFile)
	: mFile(inFile)
	, mWarnedReadOnly(false)
	, mDirty(false)
	, mFileLoader(nil)
	, mFileSaver(nil)
	, mNext(nil)
{
	mNext = sFirst;
	sFirst = this;
}

// ---------------------------------------------------------------------------
//	~MDocument

MDocument::~MDocument()
{
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
	
	eDocumentClosed(this);
}

// ---------------------------------------------------------------------------
//	SetFileNameHint

void MDocument::SetFileNameHint(
	const string&	inNameHint)
{
	mFile = MFile();
	
	mFileNameHint = inNameHint;
	eFileSpecChanged(this, mFile);
}

// ---------------------------------------------------------------------------
//	MDocument::DoLoad

void MDocument::DoLoad()
{
	if (not mFile.IsValid())
		THROW(("File is not specified"));
	
	if (mFileLoader != nil)
		THROW(("File is already being loaded"));
	
	mFileLoader = mFile.Load();
	
	SetCallback(mFileLoader->eProgress, this, &MDocument::IOProgress);
	SetCallback(mFileLoader->eError, this, &MDocument::IOError);
	SetCallback(mFileLoader->eReadFile, this, &MDocument::ReadFile);
	
	mFileLoader->DoLoad();
}

// ---------------------------------------------------------------------------
//	DoSave

bool MDocument::DoSave()
{
	assert(IsSpecified());

	if (not mFile.IsValid())
		THROW(("File is not specified"));
	
	if (mFileSaver != nil)
		THROW(("File is already being loaded"));
	
	mFileSaver = mFile.Save();
	
	SetCallback(mFileSaver->eProgress, this, &MDocument::IOProgress);
	SetCallback(mFileSaver->eError, this, &MDocument::IOError);
	SetCallback(mFileSaver->eWriteFile, this, &MDocument::WriteFile);

	mFileSaver->DoSave();

	gApp->AddToRecentMenu(mFile);
		
	return true;
}

// ---------------------------------------------------------------------------
//	DoSaveAs

bool MDocument::DoSaveAs(
	const MFile&		inFile)
{
	bool result = false;
	
	MFile savedFile = mFile;
	mFile = inFile;

	if (DoSave())
	{
		eFileSpecChanged(this, mFile);
		result = true;
	}
	else
		mFile = savedFile;
	
	return result;
}

// ---------------------------------------------------------------------------
//	RevertDocument

void MDocument::RevertDocument()
{
	DoLoad();
}

// ---------------------------------------------------------------------------
//	UsesFile

bool MDocument::UsesFile(
	const MFile&	inFile) const
{
	return mFile.IsValid() and mFile == inFile;
}

MDocument* MDocument::GetDocumentForFile(
	const MFile&	inFile)
{
	MDocument* doc = sFirst;

	while (doc != nil and not doc->UsesFile(inFile))
		doc = doc->mNext;
	
	return doc;
}

// ---------------------------------------------------------------------------
//	AddNotifier

void MDocument::AddNotifier(
	MDocClosedNotifier&		inNotifier,
	bool					inRead)
{
	mNotifiers.push_back(inNotifier);
}

// ---------------------------------------------------------------------------
//	AddController

void MDocument::AddController(MController* inController)
{
	if (find(mControllers.begin(), mControllers.end(), inController) == mControllers.end())
		mControllers.push_back(inController);
}

// ---------------------------------------------------------------------------
//	RemoveController

void MDocument::RemoveController(MController* inController)
{
	assert(find(mControllers.begin(), mControllers.end(), inController) != mControllers.end());
	
	mControllers.erase(
		remove(mControllers.begin(), mControllers.end(), inController),
		mControllers.end());

	if (mControllers.size() == 0)
		CloseDocument();
}

// ---------------------------------------------------------------------------
//	GetFirstController

MController* MDocument::GetFirstController() const
{
	MController* controller = nil;
	if (mControllers.size() > 0)
		controller = mControllers.front();
	return controller;
}

// ---------------------------------------------------------------------------
//	GetWindow

MDocWindow* MDocument::GetWindow() const
{
	MDocWindow* result = nil;
	MController* controller = GetFirstController();
	
	if (controller != nil)
		result = controller->GetWindow();
	
	return result;
}

// ---------------------------------------------------------------------------
//	MakeFirstDocument

void MDocument::MakeFirstDocument()
{
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
//	SetModified

void MDocument::SetModified(bool inModified)
{
	if (inModified != mDirty)
	{
		mDirty = inModified;
		eModifiedChanged(mDirty);
	}
}

// ---------------------------------------------------------------------------
//	CloseDocument

void MDocument::CloseDocument()
{
	try
	{
		eDocumentClosed(this);
	}
	catch (...) {}
	
	delete this;
}

// ---------------------------------------------------------------------------
//	UpdateCommandStatus

bool MDocument::UpdateCommandStatus(
	uint32			inCommand,
	MMenu*			inMenu,
	uint32			inItemIndex,
	bool&			outEnabled,
	bool&			outChecked)
{
	return false;
}

// ---------------------------------------------------------------------------
//	ProcessCommand

bool MDocument::ProcessCommand(
	uint32			inCommand,
	const MMenu*	inMenu,
	uint32			inItemIndex,
	uint32			inModifiers)
{
	return false;
}

// ---------------------------------------------------------------------------
//	GetWindowTitle

string MDocument::GetWindowTitle() const
{
	string result;

	if (mFile.IsLocal())
	{
		result = mFile.GetPath().string();
		
		// strip off HOME, if any
		
		const char* HOME = getenv("HOME");
		if (HOME != nil and ba::starts_with(result, HOME))
		{
			result.erase(0, strlen(HOME));
			result.insert(0, "~");
		}
	}
	else
		result = mFile.GetURI();
	
	return result;
}

// ---------------------------------------------------------------------------
//	IOProgress

void MDocument::IOProgress(float inProgress, const string& inMessage)
{
	if (inProgress == -1)	// we're done
	{
		mFileLoader = nil;
		mFileSaver = nil;
	}
}

// ---------------------------------------------------------------------------
//	IOError

void MDocument::IOError(const std::string& inError)
{
	DisplayError(inError);

	mFileLoader = nil;
	mFileSaver = nil;
}
