//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "MJapi.h"

#include <boost/filesystem/fstream.hpp>
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
	const MUrl*			inURL)
	: mFileModDate(0)
	, mSpecified(false)
	, mReadOnly(false)
	, mWarnedReadOnly(false)
	, mDirty(false)
	, mNext(nil)
{
	if (inURL != nil)
	{
		mURL = *inURL;

		if (mURL.IsLocal() and fs::exists(mURL.GetPath()))
		{
			fs::path path = mURL.GetPath();
			
			mFileModDate = fs::last_write_time(path);
			mReadOnly = access(path.string().c_str(), W_OK) != 0;
		}
		else
			mFileModDate = GetLocalTime();

		mSpecified = true;
	}

	mNext = sFirst;
	sFirst = this;
}

MDocument::MDocument(
	const fs::path&		inPath)
	: mURL(inPath)
	, mFileModDate(0)
	, mSpecified(false)
	, mReadOnly(false)
	, mWarnedReadOnly(false)
	, mDirty(false)
	, mNext(nil)
{
	if (fs::exists(inPath))
	{
		mFileModDate = fs::last_write_time(inPath);
		mReadOnly = access(inPath.string().c_str(), W_OK) != 0;
	}
	else
		mFileModDate = GetLocalTime();

	mSpecified = true;

	mNext = sFirst;
	sFirst = this;
}

// ---------------------------------------------------------------------------
//	MDocument

MDocument::MDocument()
	: mFileModDate(0)
	, mSpecified(false)
	, mReadOnly(false)
	, mWarnedReadOnly(false)
	, mDirty(false)
	, mNext(nil)
{
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
	mSpecified = false;
	mURL.SetFileName(inNameHint);
	eFileSpecChanged(this, mURL);
}

// ---------------------------------------------------------------------------
//	DoSave

bool MDocument::DoSave()
{
	bool result = false;
	bool specified = mSpecified;
	
	assert(specified);
	assert(mURL.IsLocal());
	
	try
	{
		fs::ofstream file(mURL.GetPath(), ios::trunc | ios::binary);
		
		if (not file.is_open())
			THROW(("Failed to open file %s for writing", mURL.GetPath().string().c_str()));
		
		WriteFile(file);
		SetModified(false);

		gApp->AddToRecentMenu(mURL);
		
		result = true;
	}
	catch (exception& inErr)
	{
		DisplayError(inErr);
		
		mSpecified = specified;
		result = false;
	}
	
	return result;
}

// ---------------------------------------------------------------------------
//	DoSaveAs

bool MDocument::DoSaveAs(
	const MUrl&		inFile)
{
	bool result = false;
	
	mURL = inFile;

	if (DoSave())
	{
		mSpecified = true;
		mReadOnly = false;
	
		eFileSpecChanged(this, mURL);
		
		result = true;
	}
	
	return result;
}

// ---------------------------------------------------------------------------
//	RevertDocument

void MDocument::RevertDocument()
{
	try
	{
		fs::ifstream file(mURL.GetPath(), ios::binary);
		ReadFile(file);
		SetModified(false);
	}
	catch (exception& err)
	{
		DisplayError(err);
	}
}

// ---------------------------------------------------------------------------
//	UsesFile

bool MDocument::UsesFile(
	const MUrl&	inFileRef) const
{
	return mSpecified and mURL == inFileRef;
}

MDocument* MDocument::GetDocumentForURL(
	const MUrl&		inFile)
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

	if (mURL.IsLocal())
	{
		result = mURL.GetPath().string();
		
		// strip off HOME, if any
		
		const char* HOME = getenv("HOME");
		if (HOME != nil and ba::starts_with(result, HOME))
		{
			result.erase(0, strlen(HOME));
			result.insert(0, "~");
		}
	}
	else
		result = mURL.str();
	
	return result;
}
