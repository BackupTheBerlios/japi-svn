//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "MLib.h"

#include "MWindow.h"
#include "MCommands.h"
#include "MDocument.h"
#include "MPreferences.h"
#include "MApplication.h"

using namespace std;

namespace {

const int32
	kAskSaveChanges_Save = 3,
	kAskSaveChanges_Cancel = 2,
	kAskSaveChanges_DontSave = 1,
	
	kDiscardChanges_Discard = 1,
	kDiscardChanges_Cancel = 2;

}

MController::MController(
	MWindow*	inWindow)
	: MHandler(inWindow)
	, mDocument(nil)
	, mWindow(inWindow)
{
}

MController::~MController()
{
	assert(mDocument == nil);
}

void MController::SetDocument(
	MDocument*		inDocument)
{
	if (inDocument != mDocument)
	{
		if (mDocument != nil)
		{
			eAboutToCloseDocument(mDocument);
			mDocument->RemoveController(this);
		}
		
		mDocument = inDocument;
		
		if (mDocument != nil)
			mDocument->AddController(this);
		
		eDocumentChanged(mDocument);
	}
}

bool MController::ProcessCommand(
	uint32			inCommand,
	const MMenu*	inMenu,
	uint32			inItemIndex,
	uint32			inModifiers)
{
	bool handled = false;
	
	if (mDocument != nil)
		handled = mDocument->ProcessCommand(inCommand, inMenu, inItemIndex, inModifiers);

	if (not handled)
	{
		handled = true;
		switch (inCommand)
		{
			//case cmd_Close:
			//	TryCloseController(kSaveChangesClosingDocument);
			//	break;
	
			case cmd_Save:
				SaveDocument();
				break;
	
			case cmd_SaveAs:
				SaveDocumentAs();
				break;
	
			case cmd_Revert:
				TryDiscardChanges();
				break;
			
			case cmd_Print:
				Print();
				break;
				
			default:
				handled = MHandler::ProcessCommand(inCommand, inMenu, inItemIndex, inModifiers);
				break;
		}
	}
	
	return handled;
}

bool MController::UpdateCommandStatus(
	uint32			inCommand,
	MMenu*			inMenu,
	uint32			inItemIndex,
	bool&			outEnabled,
	bool&			outChecked)
{
	bool handled = false;

	if (mDocument != nil)
		handled = mDocument->UpdateCommandStatus(inCommand, inMenu, inItemIndex,
					outEnabled, outChecked);

	if (not handled)
	{
		handled = true;
		switch (inCommand)
		{
			// always
			//case cmd_Close:
			case cmd_SaveAs:
			case cmd_Find:
			case cmd_Print:
				outEnabled = true;
				break;
	
			// dirty
			case cmd_Save:
				outEnabled =
					mDocument != nil and
					mDocument->IsModified() and
					(not mDocument->IsSpecified() or not mDocument->IsReadOnly());
				break;
	
			case cmd_Revert:
				outEnabled = mDocument != nil and mDocument->IsSpecified() and mDocument->IsModified();
				break;
	
			default:
				handled = MHandler::UpdateCommandStatus(
					inCommand, inMenu, inItemIndex, outEnabled, outChecked);
				break;
		}
	}
	
	return handled;
}

bool MController::HandleKeydown(
	uint32			inKeyCode,
	uint32			inModifiers,
	const string&	inText)
{
	bool handled = false;
	if (mDocument != nil)
		handled = mDocument->HandleKeydown(inKeyCode, inModifiers, inText);
	if (not handled)
		handled = MHandler::HandleKeydown(inKeyCode, inModifiers, inText);
	return handled;
}

bool MController::TryCloseDocument(
	MCloseReason		inAction)
{
	bool result = true;

	if (mDocument != nil)
	{
		if (not mDocument->IsModified())
			SetDocument(nil);
		else
		{
			result = false;
			string name;	
			
			if (mDocument->IsSpecified())
				name = mDocument->GetFile().GetFileName();
			else
				name = mWindow->GetTitle();
			
			switch (DisplayAlert(mWindow, "save-changes-alert", name))
			{
				case kAskSaveChanges_Save:
					if (SaveDocument())
					{
						CloseAfterNavigationDialog();
						result = true;
					}
					break;

				case kAskSaveChanges_Cancel:
					break;

				case kAskSaveChanges_DontSave:
					SetDocument(nil);
					result = true;
					break;
			}
		}
	}
	
	return result;
}

bool MController::TryCloseController(
	MCloseReason		inAction)
{
	bool result = true;

	if (mDocument != nil)
	{
		if (mDocument->CountControllers() > 1 or not mDocument->IsModified())
			SetDocument(nil);
		else
			result = TryCloseDocument(inAction);
	}
	
	return result;
}

void MController::SaveDocumentAs()
{
	fs::path file;
	
	if (mDocument->IsSpecified())
		file = mDocument->GetFile().GetPath();
	else
		file = fs::path(mDocument->GetDocumentName());
	
	if (MFileDialogs::SaveFileAs(mWindow, file))
		mDocument->DoSaveAs(MFile(file));
}

void MController::TryDiscardChanges()
{
	if (mDocument == nil)
		return;

	if (DisplayAlert(mWindow, "discard-changes-alert", mDocument->GetFile().GetFileName()) == kDiscardChanges_Discard)
		RevertDocument();
}

bool MController::SaveDocument()
{
	bool result = true;
	
	try
	{
		if (mDocument != nil)
		{
			if (mDocument->IsSpecified() and not mDocument->IsReadOnly())
				result = mDocument->DoSave();
			else
			{
				result = false;
				SaveDocumentAs();
			}
		}
	}
	catch (std::exception& inErr)
	{
		DisplayError(inErr);
		result = false;
	}
	
	return result;
}

void MController::RevertDocument()
{
	mDocument->RevertDocument();
}

bool MController::DoSaveAs(
	const MFile&				inPath)
{
	return mDocument->DoSaveAs(inPath);
}

void MController::CloseAfterNavigationDialog()
{
	SetDocument(nil);
}

void MController::Print()
{
}
