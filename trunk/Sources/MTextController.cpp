/*
	Copyright (c) 2008, Maarten L. Hekkelman
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

#include "MJapi.h"

#include "MTextController.h"
#include "MEditWindow.h"
#include "MTextView.h"
#include "MFindDialog.h"
#include "MGoToLineDialog.h"
#include "MFindAndOpenDialog.h"
#include "MMarkMatchingDialog.h"
#include "MProject.h"
#include "MSound.h"
#include "MTextDocument.h"
#include "MJapiApp.h"

using namespace std;

MTextController::MTextController(
	MHandler*	inSuper)
	: MController(inSuper)
{
}

MTextController::~MTextController()
{
	assert(mDocument == nil);
}

void MTextController::AddTextView(
	MTextView*		inTextView)
{
	if (find(mTextViews.begin(), mTextViews.end(), inTextView) == mTextViews.end())
	{
		mTextViews.push_back(inTextView);
		
		AddRoute(eDocumentChanged, inTextView->eDocumentChanged);
		
		inTextView->SetController(this);
	}
}

void MTextController::RemoveTextView(MTextView* inTextView)
{
	RemoveRoute(eDocumentChanged, inTextView->eDocumentChanged);
	
	mTextViews.erase(
		remove(mTextViews.begin(), mTextViews.end(), inTextView),
		mTextViews.end());
}

bool MTextController::ProcessCommand(
	uint32			inCommand,
	const MMenu*	inMenu,
	uint32			inItemIndex)
{
	bool result = false;
	
	if (mDocument != nil)
	{
		result = true;
	
		switch (inCommand)
		{
			case cmd_Find:
				MFindDialog::Instance().Select();
				break;
	
			case cmd_OpenIncludeFile:
				DoOpenIncludeFile();
				break;
	
			case cmd_GoToLine:
				DoGoToLine();
				break;
	
			case cmd_SwitchHeaderSource:
				DoOpenCounterpart();
				break;
			
			case cmd_ShowDocInfoDialog:
//				new MDocInfoDialog(mDocument, mWindow);
				break;

			default:
				result = false;
				break;
		}
	}
			
	if (result == false)
		result = MController::ProcessCommand(inCommand, inMenu, inItemIndex);
	
	return result;
}

bool MTextController::UpdateCommandStatus(
	uint32			inCommand,
	MMenu*			inMenu,
	uint32			inItemIndex,
	bool&			outEnabled,
	bool&			outChecked)
{
	bool result = false;
	
	if (mDocument != nil)
	{
		result = true;
		
		switch (inCommand)
		{
			// always
			case cmd_Find:
			case cmd_SwitchHeaderSource:
			case cmd_GoToLine:
			case cmd_OpenIncludeFile:
			case cmd_ShowDocInfoDialog:
				outEnabled = true;
				break;
	
			default:
				result = false;
				break;
		}
	}
	
	if (result == false)
		result = MController::UpdateCommandStatus(inCommand, inMenu, inItemIndex, outEnabled, outChecked);

	return result;
}

void MTextController::DoGoToLine()
{
	MTextDocument* doc = dynamic_cast<MTextDocument*>(mDocument);
	
	if (doc == nil)
		return;
	
	new MGoToLineDialog(doc, mDocWindow);
}

bool MTextController::OpenInclude(
	string		inFileName)
{
	MProject* project = MProject::Instance();
	MUrl url;

	if (mDocument != nil)
	{
		url = mDocument->GetURL();
		url.SetPath(url.GetPath().branch_path() / inFileName);
	}
	
	bool result = false;
	
	if (url.IsValid())
	{
		if (url.IsLocal())
			result = fs::exists(url.GetPath());
		else
			result = true;
	}
	
	MPath p;
	if (not result and project != nil and project->LocateFile(inFileName, true, p))
	{
		result = true;
		url = MUrl(p);
	}
	
	if (result)
		gApp->OpenOneDocument(url);
	
	return result;
}

void MTextController::DoOpenIncludeFile()
{
	MTextDocument* doc = dynamic_cast<MTextDocument*>(mDocument);
	
	if (doc == nil)
		return;
	
	bool result = false;
	
	MSelection selection = doc->GetSelection();
	
	if (selection.IsEmpty())
	{
		new MFindAndOpenDialog(this, mDocWindow);
		result = true;
	}
	else
	{
		for (;;)
		{
			string s;
			doc->GetSelectedText(s);

			if (OpenInclude(s))
			{
				result = true;
				break;
			}
			
			MTextBuffer& textBuffer = doc->GetTextBuffer();
			MTextBuffer::iterator i =
				textBuffer.begin() + selection.GetMaxOffset(*doc);
			
			if (i.GetOffset() + 2 >= textBuffer.GetSize() or
				*i != '.' or
				not IsAlnum(*(i + 1)))
			{
				break;
			}
			
			i += 2;
			
			while (i != textBuffer.end() and IsAlnum(*i))
				++i;
			
			doc->Select(selection.GetMinOffset(*doc), i.GetOffset());
		}
	}
	
	if (not result)
		PlaySound("warning");
}

void MTextController::DoOpenCounterpart()
{
	MTextDocument* doc = dynamic_cast<MTextDocument*>(mDocument);
	
	if (doc == nil)
		return;
	
	bool result = false;
	
	const char* kSourceExtensions[] = {
		"c", "cc", "cp", "cpp", "c++", nil
	};

	const char* kHeaderExtensions[] = {
		"h", "hp", "hpp", nil
	};

	if (doc->IsSpecified())
	{
		string name = doc->GetURL().GetFileName();
		MPath p;
	
		const char** ext = nil;
		
		if (FileNameMatches("*.h;*.hp;*.hpp", name))
			ext = kSourceExtensions;
		else if (FileNameMatches("*.c;*.cc;*.cp;*.cpp;*.c++;*.inl", name))
			ext = kHeaderExtensions;
	
		if (ext != nil)
		{
			name.erase(name.rfind('.') + 1);
			MProject* project = MProject::Instance();
		
			if (project != nil)
			{
				for (const char** e = ext; result == false and *e != nil; ++e)
					result = project->LocateFile(name + *e, true, p);

				if (result)
					gApp->OpenOneDocument(MUrl(p));
			}

			if (not result)
			{
				for (const char** e = ext; result == false and *e != nil; ++e)
					result = OpenInclude(name + *e);
			}
		}
	}
	
	if (not result)
		PlaySound("warning");
}

void MTextController::DoMarkMatching()
{
	MTextDocument* doc = dynamic_cast<MTextDocument*>(mDocument);
	
	if (doc == nil)
		return;
	
	new MMarkMatchingDialog(doc, mDocWindow);
}

bool MTextController::TryCloseDocument(
	MCloseReason	inAction)
{
	if (mDocument != nil and
		mDocument->IsSpecified() and
		mDocument == MTextDocument::GetWorksheet() and
		mDocument->IsModified())
	{
		mDocument->DoSave();
	}

	return MController::TryCloseDocument(inAction);
}
