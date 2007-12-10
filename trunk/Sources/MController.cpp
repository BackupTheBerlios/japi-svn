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

#include "MDocument.h"
#include "MDocWindow.h"
#include "MController.h"
#include "MTextView.h"
//#include "MTextViewContainer.h"
#include "MCommands.h"
#include "MFindDialog.h"
#include "MClipboard.h"
//#include "MApplication.h"
#include "MGoToLineDialog.h"
#include "MFindAndOpenDialog.h"
#include "MMarkMatchingDialog.h"
#include "MUnicode.h"
#include "MGlobals.h"
#include "MProject.h"
#include "MLanguage.h"
//#include "MDocInfoDialog.h"
#include "MUtils.h"

using namespace std;

MController::MController()
	: MHandler(nil)
	, mDocument(nil)
{
}

MController::~MController()
{
	assert(mDocument == nil);
}

void MController::SetWindow(MWindow* inWindow)
{
	SetSuper(inWindow);
	
	mWindow = dynamic_cast<MDocWindow*>(inWindow);
	THROW_IF_NIL(mWindow);
	
//	mWindow->Install(kEventClassTextInput, kEventTextInputUpdateActiveInputArea,
//		this, &MController::DoTextInputUpdateActiveInputArea);
//	mWindow->Install(kEventClassTextInput, kEventTextInputUnicodeForKeyEvent,
//		this, &MController::DoTextInputUnicodeForKeyEvent);
//	mWindow->Install(kEventClassTextInput, kEventTextInputOffsetToPos,
//		this, &MController::DoTextInputOffsetToPos);
////	mWindow->Install(kEventClassTextInput, kEventTextInputPosToOffset,
////		this, &MController::DoTextInputPosToOffset);

	AddRoute(eDocumentChanged, mWindow->eDocumentChanged);
}

void MController::SetDocument(MDocument* inDocument)
{
	assert(mWindow);
	
	if (inDocument != mDocument)
	{
		if (mDocument != nil)
		{
			RemoveRoute(mDocument->eModifiedChanged, mWindow->eModifiedChanged);
			RemoveRoute(mDocument->eFileSpecChanged, mWindow->eFileSpecChanged);
			RemoveRoute(mDocument->eSelectionChanged, mWindow->eSelectionChanged);
			RemoveRoute(mDocument->eShellStatus, mWindow->eShellStatus);

			mDocument->RemoveController(this);
		}
		
		mDocument = inDocument;
		
		if (mDocument != nil)
		{
			mDocument->AddController(this);

			AddRoute(mDocument->eModifiedChanged, mWindow->eModifiedChanged);
			AddRoute(mDocument->eFileSpecChanged, mWindow->eFileSpecChanged);
			AddRoute(mDocument->eSelectionChanged, mWindow->eSelectionChanged);
			AddRoute(mDocument->eShellStatus, mWindow->eShellStatus);
		}
		
		eDocumentChanged(mDocument);
	}
}

MDocument* MController::GetDocument() const
{
	return mDocument;
}

void MController::AddTextView(MTextView* inTextView)
{
	if (find(mTextViews.begin(), mTextViews.end(), inTextView) == mTextViews.end())
	{
		mTextViews.push_back(inTextView);
		
		AddRoute(eDocumentChanged, inTextView->eDocumentChanged);
		
		inTextView->SetController(this);
	}
}

void MController::RemoveTextView(MTextView* inTextView)
{
	RemoveRoute(eDocumentChanged, inTextView->eDocumentChanged);
	
	mTextViews.erase(remove(mTextViews.begin(), mTextViews.end(), inTextView),
		mTextViews.end());
}

bool MController::ProcessCommand(
	uint32			inCommand,
	const MMenu*	inMenu,
	uint32			inItemIndex)
{
	bool result = false;
	
//	// short cut
//	if (HIWindowIsDocumentModalTarget(mWindow->GetSysWindow(), nil))
//		return noErr;

	if (mDocument != nil)
	{
		result = true;
		MProject* project = MProject::Instance();

		string s;
		
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
			
			case cmd_Undo:
				mDocument->DoUndo();
				break;
	
			case cmd_Redo:
				mDocument->DoRedo();
				break;
	
			case cmd_Cut:
				mDocument->DoCut(false);
				break;
	
			case cmd_Copy:
				mDocument->DoCopy(false);
				break;
	
			case cmd_Paste:
				mDocument->DoPaste();
				break;
	
			case cmd_Clear:
				mDocument->DoClear();
				break;
	
			case cmd_SelectAll:
				mDocument->SelectAll();
				break;
	
			case cmd_Balance:
				mDocument->DoBalance();
				break;
	
			case cmd_ShiftLeft:
				mDocument->DoShiftLeft();
				break;
	
			case cmd_ShiftRight:
				mDocument->DoShiftRight();
				break;
	
			case cmd_Entab:
				mDocument->DoEntab();
				break;
	
			case cmd_Detab:
				mDocument->DoDetab();
				break;
	
			case cmd_Comment:
				mDocument->DoComment();
				break;
	
			case cmd_Uncomment:
				mDocument->DoUncomment();
				break;
	
			case cmd_PasteNext:
				mDocument->DoPasteNext();
				break;
	
			case cmd_CopyAppend:
				mDocument->DoCopy(true);
				break;
	
			case cmd_CutAppend:
				mDocument->DoCut(true);
				break;
	
			case cmd_FastFind:
				mDocument->DoFastFind(kDirectionForward);
				break;
	
			case cmd_FastFindBW:
				mDocument->DoFastFind(kDirectionBackward);
				break;
	
			case cmd_Find:
				MFindDialog::Instance().Select();
				break;
	
			case cmd_FindNext:
				if (not mDocument->DoFindNext(kDirectionForward))
					Beep();
				break;
	
			case cmd_FindPrev:
				if (not mDocument->DoFindNext(kDirectionBackward))
					Beep();
				break;
			
			case cmd_EnterSearchString:
				mDocument->GetSelectedText(s);
				MFindDialog::Instance().SetFindString(s, false);
				break;
	
			case cmd_EnterReplaceString:
				mDocument->GetSelectedText(s);
				MFindDialog::Instance().SetReplaceString(s);
				break;
	
			case cmd_Replace:
				mDocument->DoReplace(false, kDirectionForward);
				break;
	
			case cmd_ReplaceAll:
				mDocument->DoReplaceAll();
				break;
	
			case cmd_ReplaceFindNext:
				mDocument->DoReplace(true, kDirectionForward);
				break;
	
			case cmd_ReplaceFindPrev:
				mDocument->DoReplace(true, kDirectionBackward);
				break;
	
			case cmd_CompleteLookingBack:
				mDocument->DoComplete(kDirectionBackward);
				break;
	
			case cmd_CompleteLookingFwd:
				mDocument->DoComplete(kDirectionForward);
				break;
	
			case cmd_ClearMarks:
				mDocument->ClearAllMarkers();
				break;
	
			case cmd_MarkLine:
				mDocument->DoMarkLine();
				break;
	
			case cmd_JumpToNextMark:
				mDocument->DoJumpToNextMark(kDirectionForward);
				break;
	
			case cmd_JumpToPrevMark:
				mDocument->DoJumpToNextMark(kDirectionBackward);
				break;
			
			case cmd_MarkMatching:
				DoMarkMatching();
				break;
			
			case cmd_CutMarkedLines:
				mDocument->CCCMarkedLines(true, true);
				break;
	
			case cmd_CopyMarkedLines:
				mDocument->CCCMarkedLines(true, false);
				break;
	
			case cmd_ClearMarkedLines:
				mDocument->CCCMarkedLines(false, true);
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
			
			case cmd_Softwrap:
				mDocument->DoSoftwrap();
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
				mDocument->SelectParsePopupItem(inItemIndex);
				break;
			
			case cmd_OpenIncludeFromMenu:
				mDocument->SelectIncludePopupItem(inItemIndex);
				break;
		
	//#ifndef NDEBUG
	//		case cmd_Test:
	//		{
	//			UInt32 offset = 0;
	//			mText.FindExpression(offset, MFindDialog::Instance().GetFindString(0), kDirectionForward, false);
	//			break;
	//		}
	//#endif

			case cmd_Preprocess:
				if (project != nil)
					project->Preprocess(mDocument->GetURL().GetPath());
				break;
				
			case cmd_CheckSyntax:
				if (project != nil)
					project->CheckSyntax(mDocument->GetURL().GetPath());
				break;
				
			case cmd_Compile:
				if (project != nil)
					project->Compile(mDocument->GetURL().GetPath());
				break;

			case cmd_Disassemble:
				if (project != nil)
					project->Disassemble(mDocument->GetURL().GetPath());
				break;
	
			case cmd_2CharsPerTab:
				if (mDocument != nil)
					mDocument->SetCharsPerTab(2);
				break;
	
			case cmd_4CharsPerTab:
				if (mDocument != nil)
					mDocument->SetCharsPerTab(4);
				break;
	
			case cmd_8CharsPerTab:
				if (mDocument != nil)
					mDocument->SetCharsPerTab(8);
				break;
	
			case cmd_16CharsPerTab:
				if (mDocument != nil)
					mDocument->SetCharsPerTab(16);
				break;
			
			case cmd_SyntaxNone:
				mDocument->SetLanguage("");
				break;
			
			case cmd_SyntaxLanguage:
				mDocument->SetLanguage(inMenu->GetItemLabel(inItemIndex));
				break;
	
			default:
				result = false;
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
//	if (HIWindowIsDocumentModalTarget(mWindow->GetSysWindow(), nil) or mDocument == nil)
//		return noErr;

	MProject* project = MProject::Instance();
	MLanguage* lang = mDocument->GetLanguage();

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
		case cmd_SwitchHeaderSource:
		case cmd_GoToLine:
		case cmd_ShiftLeft:
		case cmd_ShiftRight:
		case cmd_OpenIncludeFile:
		case cmd_ShowDocInfoDialog:
			outEnabled = true;
			break;

		// dirty
		case cmd_Save:
			outEnabled = mDocument->IsModified();
			break;

		// has selection
		case cmd_Cut:
		case cmd_Copy:
		case cmd_Clear:
		case cmd_CopyAppend:
		case cmd_CutAppend:
		case cmd_EnterSearchString:
		case cmd_EnterReplaceString:
			outEnabled = not mDocument->GetSelection().IsEmpty();
			break;

		// special
		case cmd_Undo:
			outEnabled = mDocument->CanUndo(title);
			break;

		case cmd_Redo:
			outEnabled = mDocument->CanRedo(title);
			break;

		case cmd_Revert:
			outEnabled = mDocument->IsSpecified() and mDocument->IsModified();
			break;

		case cmd_Paste:
		case cmd_PasteNext:
			outEnabled = MClipboard::Instance().HasData();
			break;

		case cmd_Balance:
		case cmd_Comment:
		case cmd_Uncomment:
			outEnabled = mDocument->GetLanguage() != nil and
						not mDocument->GetSelection().IsBlock();
			break;

		case cmd_Replace:
		case cmd_ReplaceFindNext:
		case cmd_ReplaceFindPrev:
			outEnabled = mDocument->CanReplace();
			break;
		
		case cmd_Softwrap:
			outEnabled = true;
			outChecked = mDocument->GetSoftwrap();
			break;

		case cmd_Preprocess:
		case cmd_Compile:
		case cmd_CheckSyntax:
		case cmd_Disassemble:
			outEnabled =
				project != nil and
				mDocument->GetURL().IsLocal() and
				project->IsFileInProject(mDocument->GetURL().GetPath());
			break;
		
		case cmd_2CharsPerTab:
			outEnabled = true;
			outChecked = mDocument->GetCharsPerTab() == 2;
			break;
		
		case cmd_4CharsPerTab:
			outEnabled = true;
			outChecked = mDocument->GetCharsPerTab() == 4;
			break;
		
		case cmd_8CharsPerTab:
			outEnabled = true;
			outChecked = mDocument->GetCharsPerTab() == 8;
			break;
		
		case cmd_16CharsPerTab:
			outEnabled = true;
			outChecked = mDocument->GetCharsPerTab() == 16;
			break;
		
		case cmd_SyntaxNone:
			outEnabled = true;
			outChecked = (lang == nil);
			break;
		
		case cmd_SyntaxLanguage:
			outEnabled = true;
			outChecked = (lang != nil and lang->GetName() == inMenu->GetItemLabel(inItemIndex));
			break;
		
		default:
			result = false;
			break;
	}
	
	return result;
}

bool MController::TryCloseDocument(
	MCloseReason		inAction)
{
	bool result = true;

	if (mDocument != nil)
	{
		if (not mDocument->IsModified() or
			(mDocument->IsSpecified() and mDocument->IsWorksheet() and SaveDocument()))
		{
			SetDocument(nil);
		}
		else
		{
			result = false;
			MSaverMixin::TryCloseDocument(inAction, mWindow);
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
		name = mWindow->GetTitle();
	
	MSaverMixin::SaveDocumentAs(mWindow, name);
}

void MController::TryDiscardChanges()
{
	if (mDocument == nil)
		return;

	MSaverMixin::TryDiscardChanges(mDocument->GetURL().GetFileName(), mWindow);
}

bool MController::SaveDocument()
{
	bool result = true;
	
	try
	{
		if (mDocument != nil)
		{
			if (mDocument->IsSpecified())
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
		MError::DisplayError(inErr);
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

//OSStatus MController::DoTextInputUpdateActiveInputArea(EventRef inEvent)
//{
//	OSStatus err = eventNotHandledErr;
//	if (mDocument)
//		err = mDocument->DoTextInputUpdateActiveInputArea(inEvent);
//
//	mWindow->FlushIfNeeded();
//
//	return err;
//}
//
//OSStatus MController::DoTextInputUnicodeForKeyEvent(EventRef inEvent)
//{
//	OSStatus err = eventNotHandledErr;
//	if (mDocument)
//		err = mDocument->DoTextInputUnicodeForKeyEvent(inEvent);
//
//	mWindow->FlushIfNeeded();
//
//	return err;
//}
//
//OSStatus MController::DoTextInputOffsetToPos(EventRef inEvent)
//{
//	OSStatus err = eventNotHandledErr;
//	if (mDocument)
//		err = mDocument->DoTextInputOffsetToPos(inEvent);
//
//	mWindow->FlushIfNeeded();
//
//	return err;
//}
//
////OSStatus MController::DoTextInputPosToOffset(EventRef inEvent)
////{
////	OSStatus err = eventNotHandledErr;
////	if (mDocument)
////		err = mDocument->DoTextInputPosToOffset(inEvent);
////
////	if (::HIViewGetNeedsDisplay(mWindow->GetContentViewRef()))
////		::HIWindowFlush(mWindow->GetSysWindow());
////
////	return err;
////}

void MController::DoGoToLine()
{
	if (mDocument == nil)
		return;
	
	std::auto_ptr<MGoToLineDialog> dlog(MDialog::Create<MGoToLineDialog>());
	dlog->Initialize(mDocument, mWindow);
	dlog.release();
}

bool MController::OpenInclude(std::string inFileName)
{
	MProject* project = MProject::Instance();
	MPath p;
	
	bool result = false;
		
	if (project != nil and project->LocateFile(inFileName, true, p))
		result = true;
	else if (mDocument != nil and mDocument->GetURL().IsLocal())
	{
		p = mDocument->GetURL().GetPath();
		p = p.branch_path() / inFileName;
		result = exists(p);
	}
	
	if (result)
		gApp->OpenOneDocument(MUrl(p));
	
	return result;
}

void MController::DoOpenIncludeFile()
{
	if (mDocument == nil)
		return;
	
	bool result = false;
	
	MSelection selection = mDocument->GetSelection();
	
	if (selection.IsEmpty())
	{
		std::auto_ptr<MFindAndOpenDialog> dlog(
			MDialog::Create<MFindAndOpenDialog>());
		dlog->Initialize(this, mWindow);
		dlog.release();
		result = true;
	}
	else
	{
		for (;;)
		{
			string s;
			mDocument->GetSelectedText(s);

			if (OpenInclude(s))
			{
				result = true;
				break;
			}
			
			MTextBuffer& textBuffer = mDocument->GetTextBuffer();
			MTextBuffer::iterator i =
				textBuffer.begin() + selection.GetMaxOffset(*mDocument);
			
			if (i.GetOffset() + 2 >= textBuffer.GetSize() or
				*i != '.' or
				not IsAlnum(*(i + 1)))
			{
				break;
			}
			
			i += 2;
			
			while (i != textBuffer.end() and IsAlnum(*i))
				++i;
			
			mDocument->Select(selection.GetMinOffset(*mDocument), i.GetOffset());
		}
	}
	
	if (not result)
		Beep();
}

void MController::DoOpenCounterpart()
{
	if (mDocument == nil)
		return;
	
	bool result = false;
	
	const char* kSourceExtensions[] = {
		"c", "cc", "cp", "cpp", "c++", nil
	};

	const char* kHeaderExtensions[] = {
		"h", "hp", "hpp", nil
	};

	if (mDocument->IsSpecified())
	{
		string name = mDocument->GetURL().GetFileName();
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
		Beep();
}

//// ---------------------------------------------------------------------------
////	DoSaveAll
//
//void MController::DoSaveAll()
//{
//	MDocument* doc = MDocument::GetFirstDocument();
//	while (doc != nil)
//	{
//		if (doc->IsModified())
//		{
////			if (not doc->IsSpecified())
////				doc->BringToFront();
//
//			doc->SaveDocument();
//
//			if (not doc->IsSpecified())
//				break;
//		}
//
//		doc = doc->GetNextDocument();
//	}
//}
//
//void MController::DoCloseAll(NavAskSaveChangesAction inAction)
//{
//	MDocument* doc = MDocument::GetFirstDocument();
//	while (doc != nil)
//	{
//		if (doc->IsModified() and not doc->mIsWorksheet)
//		{
////			doc->BringToFront();
//			if (not doc->mNavDialogVisible)
//				doc->TryCloseController(inAction);
//			break;
//		}
//		
//		MDocument* next = doc->GetNextDocument();
//		
//		if (inAction != kNavSaveChangesClosingDocument or not doc->IsWorksheet())
//			doc->TryCloseController(inAction);
//
//		doc = next;
//	}
//}

MTextView* MController::GetTextView()
{
//	return mWindow->FindViewByID<MTextView>(128);
	return nil;
}

MTextViewContainer* MController::GetContainer()
{
//	return mWindow->FindViewByID<MTextViewContainer>(127);
	return nil;
}

void MController::DoMarkMatching()
{
	if (mDocument == nil)
		return;
	
	std::auto_ptr<MMarkMatchingDialog> dlog(MDialog::Create<MMarkMatchingDialog>());
	dlog->Initialize(mDocument, mWindow);
	dlog.release();
}

