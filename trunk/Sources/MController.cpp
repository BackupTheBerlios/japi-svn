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

#include "MDocWindow.h"
#include "MDocument.h"

using namespace std;

MController::MController(
	MHandler*	inSuper)
	: MHandler(inSuper)
	, mDocument(nil)
{
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
	uint32			inItemIndex)
{
	bool result = true;
	
	if (mDocument == nil or
		not mDocument->ProcessCommand(inCommand, inMenu, inItemIndex))
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
				
			default:
				result = MHandler::ProcessCommand(inCommand, inMenu, inItemIndex);
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
				name = mDocument->GetURL().GetFileName();
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
		name = mDocument->GetURL().GetFileName();
	else
		name = mDocWindow->GetTitle();
	
	MSaverMixin::SaveDocumentAs(mDocWindow, name);
}

void MController::TryDiscardChanges()
{
	if (mDocument == nil)
		return;

	MSaverMixin::TryDiscardChanges(mDocument->GetURL().GetFileName(), mDocWindow);
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
	const MUrl&				inPath)
{
	return mDocument->DoSaveAs(inPath);
}

void MController::CloseAfterNavigationDialog()
{
	SetDocument(nil);
}
