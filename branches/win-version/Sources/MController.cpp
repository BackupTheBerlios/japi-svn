//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "MLib.h"

#include "MDocWindow.h"
#include "MDocument.h"
#include "MPreferences.h"

using namespace std;

MController::MController(
	MHandler*	inSuper)
	: MHandler(inSuper)
	, mDocument(nil)
{
	if (dynamic_cast<MDocWindow*>(inSuper) != nil)
		SetWindow(static_cast<MDocWindow*>(inSuper));
}

MController::~MController()
{
	assert(mDocument == nil);
}

void MController::SetWindow(
	MDocWindow*		inWindow)
{
	SetSuper(inWindow);
	
	mDocWindow = inWindow;
	
	AddRoute(eDocumentChanged, mDocWindow->eDocumentChanged);
}

void MController::SetDocument(
	MDocument*		inDocument)
{
	assert(mDocWindow);
	
	if (inDocument != mDocument)
	{
		if (mDocument != nil)
		{
			try
			{
				if (mDocument->IsSpecified() and Preferences::GetInteger("save state", 1))
					mDocWindow->SaveState();
			}
			catch (...) {}
			
			mDocWindow->RemoveRoutes(mDocument);
			mDocument->RemoveController(this);
		}
		
		mDocument = inDocument;
		
		if (mDocument != nil)
		{
			mDocument->AddController(this);
			mDocWindow->AddRoutes(mDocument);
		}
		
		eDocumentChanged(mDocument);
	}
}

bool MController::ProcessCommand(
	uint32			inCommand,
	const MMenu*	inMenu,
	uint32			inItemIndex,
	uint32			inModifiers)
{
	bool result = true;
	
	if (mDocument == nil or
		not mDocument->ProcessCommand(inCommand, inMenu, inItemIndex, inModifiers))
	{
		switch (inCommand)
		{
			case cmd_Close:
				TryCloseController(kSaveChangesClosingDocument);
				break;
	
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
				result = MHandler::ProcessCommand(inCommand, inMenu, inItemIndex, inModifiers);
				break;
		}
	}
	
	return result;
}

bool MController::UpdateCommandStatus(
	uint32			inCommand,
	MMenu*			inMenu,
	uint32			inItemIndex,
	bool&			outEnabled,
	bool&			outChecked)
{
	bool result = true;

	if (mDocument == nil or
		not mDocument->UpdateCommandStatus(inCommand, inMenu, inItemIndex,
				outEnabled, outChecked))
	{
		switch (inCommand)
		{
			// always
			case cmd_Close:
			case cmd_SaveAs:
			case cmd_Find:
			case cmd_Print:
				outEnabled = true;
				break;
	
			// dirty
			case cmd_Save:
				outEnabled =
					mDocument->IsModified() and
					(not mDocument->IsSpecified() or not mDocument->IsReadOnly());
				break;
	
			case cmd_Revert:
				outEnabled = mDocument->IsSpecified() and mDocument->IsModified();
				break;
	
			default:
				result = MHandler::UpdateCommandStatus(
					inCommand, inMenu, inItemIndex, outEnabled, outChecked);
				break;
		}
	}
	
	return result;
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
				name = mDocWindow->GetTitle();
			
			MSaverMixin::TryCloseDocument(inAction, name, mDocWindow);
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
		{
			TryCloseDocument(inAction);
			mCloseOnNavTerminate = true;
			result = false;
		}
	}
	
	return result;
}

void MController::SaveDocumentAs()
{
	string name;	
	
	if (mDocument->IsSpecified())
		name = mDocument->GetFile().GetFileName();
	else
		name = mDocWindow->GetTitle();
	
	MSaverMixin::SaveDocumentAs(mDocWindow, name);
}

void MController::TryDiscardChanges()
{
	if (mDocument == nil)
		return;

	MSaverMixin::TryDiscardChanges(mDocument->GetFile().GetFileName(), mDocWindow);
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
