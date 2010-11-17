//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "MJapi.h"

#include "MTextController.h"
#include "MEditWindow.h"
#include "MTextView.h"
#include "MSound.h"
#include "MTextDocument.h"
#include "MJapiApp.h"

using namespace std;

MTextController::MTextController(
	MWindow*	inSuper)
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
	uint32			inItemIndex,
	uint32			inModifiers)
{
	bool result = false;
	
	if (mDocument != nil)
	{
		result = true;
	
		switch (inCommand)
		{
			//case cmd_Find:
			//	MFindDialog::Instance().Select();
			//	break;
	
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

			//case cmd_QuotedRewrap:
			//	new MQuotedRewrapDialog(
			//		static_cast<MTextDocument*>(mDocument), GetWindow());
			//	break;

			//case cmd_MarkMatching:
			//	new MMarkMatchingDialog(
			//		static_cast<MTextDocument*>(mDocument), GetWindow());
			//	break;

			default:
				result = false;
				break;
		}
	}
			
	if (result == false)
		result = MController::ProcessCommand(inCommand, inMenu, inItemIndex, inModifiers);
	
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
			case cmd_MarkMatching:
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
	//MTextDocument* doc = dynamic_cast<MTextDocument*>(mDocument);
	//
	//if (doc == nil)
	//	return;
	//
	//new MGoToLineDialog(doc, mDocWindow);
}

bool MTextController::OpenInclude(
	string		inFileName)
{
	//MProject* project = MProject::Instance();
	MFile url;

	if (mDocument != nil)
		url = mDocument->GetFile().GetParent() / inFileName;
	
	bool result = false;
	
	if (url.IsValid())
	{
		if (url.IsLocal())
			result = url.Exists();
		else
			result = true;
	}
	
	//fs::path p;
	//if (not result and project != nil and project->LocateFile(inFileName, true, p))
	//{
	//	result = true;
	//	url = MFile(p);
	//}
	
	if (result)
		gApp->OpenOneDocument(url);
	
	return result;
}

void MTextController::DoOpenIncludeFile()
{
	//MTextDocument* doc = dynamic_cast<MTextDocument*>(mDocument);
	//
	//if (doc == nil)
	//	return;
	//
	//bool result = false;
	//
	//MSelection selection = doc->GetSelection();
	//
	//if (selection.IsEmpty())
	//{
	//	new MFindAndOpenDialog(this, mDocWindow);
	//	result = true;
	//}
	//else
	//{
	//	for (;;)
	//	{
	//		string s;
	//		doc->GetSelectedText(s);

	//		if (OpenInclude(s))
	//		{
	//			result = true;
	//			break;
	//		}
	//		
	//		MTextBuffer& textBuffer = doc->GetTextBuffer();
	//		MTextBuffer::iterator i =
	//			textBuffer.begin() + selection.GetMaxOffset();
	//		
	//		if (i.GetOffset() + 2 >= textBuffer.GetSize() or
	//			*i != '.' or
	//			not IsAlnum(*(i + 1)))
	//		{
	//			break;
	//		}
	//		
	//		i += 2;
	//		
	//		while (i != textBuffer.end() and IsAlnum(*i))
	//			++i;
	//		
	//		doc->Select(selection.GetMinOffset(), i.GetOffset());
	//	}
	//}
	//
	//if (not result)
	//	PlaySound("warning");
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
		string name = doc->GetFile().GetFileName();
		fs::path p;
	
		const char** ext = nil;
		
		if (FileNameMatches("*.h;*.hp;*.hpp", name))
			ext = kSourceExtensions;
		else if (FileNameMatches("*.c;*.cc;*.cp;*.cpp;*.c++;*.inl", name))
			ext = kHeaderExtensions;
	
		if (ext != nil)
		{
			name.erase(name.rfind('.') + 1);
			//MProject* project = MProject::Instance();
		
			//if (project != nil)
			//{
			//	for (const char** e = ext; result == false and *e != nil; ++e)
			//		result = project->LocateFile(name + *e, true, p);

			//	if (result)
			//		gApp->OpenOneDocument(MFile(p));
			//}

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
	//MTextDocument* doc = dynamic_cast<MTextDocument*>(mDocument);
	//
	//if (doc == nil)
	//	return;
	//
	//new MMarkMatchingDialog(doc, mDocWindow);
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

void MTextController::Print()
{
	MTextDocument* doc = dynamic_cast<MTextDocument*>(mDocument);

	if (doc == nil)
		THROW(("No document?"));
	
	if (mTextViews.empty())
		THROW(("No text view?"));
	
	uint32 wrapWidth = doc->GetWrapWidth();
	
	//MPrinter printer(mTextViews.front());
	//printer.DoPrint();
	
	doc->SetWrapWidth(wrapWidth);
}
