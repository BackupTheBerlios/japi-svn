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

#include <sstream>

#include "MEditWindow.h"
#include "MDocument.h"
#include "MGlobals.h"
//#include "MToolbar.h"
//#include "MTextViewContainer.h"
#include "MViewPort.h"
#include "MScrollBar.h"
#include "MTextView.h"
#include "MUtils.h"
#include "MMenu.h"
#include "MFile.h"

using namespace std;

// ------------------------------------------------------------------
//

MEditWindow* MEditWindow::sHead = nil;

MEditWindow::MEditWindow()
{
	mNext = sHead;
	sHead = this;

	// the menubar
	
	MMenu* fileMenu = new MMenu("File");
	fileMenu->AppendItem("New", cmd_New);
	fileMenu->AppendItem("Open…", cmd_Open);
	
	fileMenu->AppendRecentMenu("Open Recent…");
	
	fileMenu->AppendItem("Find and open…", cmd_OpenIncludeFile);
	fileMenu->AppendItem("Open Source/Header", cmd_SwitchHeaderSource);
	
	fileMenu->AppendSeparator();
	fileMenu->AppendItem("Close", cmd_Close);
	fileMenu->AppendItem("Save", cmd_Save);
	fileMenu->AppendItem("Save As…", cmd_SaveAs);
	fileMenu->AppendItem("Revert", cmd_Revert);
	fileMenu->AppendSeparator();
	fileMenu->AppendItem("Quit", cmd_Quit);
	
	mMenubar.AddMenu(fileMenu);
	
	MMenu* editMenu = new MMenu("Edit");
	editMenu->AppendItem("Undo", cmd_Undo);
	editMenu->AppendItem("Redo", cmd_Redo);
	editMenu->AppendSeparator();
	editMenu->AppendItem("Cut", cmd_Cut);
	editMenu->AppendItem("Cut and append", cmd_CutAppend);
	editMenu->AppendItem("Copy", cmd_Copy);
	editMenu->AppendItem("Copy and append", cmd_CopyAppend);
	editMenu->AppendItem("Paste", cmd_Paste);
	editMenu->AppendItem("Paste Next", cmd_PasteNext);
	editMenu->AppendItem("Clear", cmd_Clear);
	editMenu->AppendItem("Select all", cmd_SelectAll);
//	editMenu->AppendSeparator();
//	editMenu->AppendItem("Font…", cmd_
	
	mMenubar.AddMenu(editMenu);
	
	MMenu* textMenu = new MMenu("Text");
	textMenu->AppendItem("Balance", cmd_Balance);
	textMenu->AppendItem("Softwrap", cmd_Softwrap);
	textMenu->AppendSeparator();
	textMenu->AppendItem("Shift Left", cmd_ShiftLeft);
	textMenu->AppendItem("Shift Right", cmd_ShiftRight);
	textMenu->AppendSeparator();
	textMenu->AppendItem("Comment", cmd_Comment);
	textMenu->AppendItem("Uncomment", cmd_Uncomment);
	textMenu->AppendSeparator();
	textMenu->AppendItem("Entab", cmd_Entab);
	textMenu->AppendItem("Detab", cmd_Detab);
	textMenu->AppendSeparator();
	textMenu->AppendItem("File Info…", cmd_ShowDocInfoDialog);

	mMenubar.AddMenu(textMenu);

	MMenu* searchMenu = new MMenu("Search");

	searchMenu->AppendItem("Fast Find", cmd_FastFind);
	searchMenu->AppendItem("Fast Find Backward", cmd_FastFind);
	searchMenu->AppendSeparator();
	searchMenu->AppendItem("Find", cmd_Find);
	searchMenu->AppendItem("Find Next", cmd_FindNext);
	searchMenu->AppendItem("Find Previous", cmd_FindPrev);
	searchMenu->AppendItem("Find in Next File", cmd_FindInNextFile);
	searchMenu->AppendItem("Enter Search String", cmd_EnterSearchString);
	searchMenu->AppendItem("Enter Replace String", cmd_EnterReplaceString);
	searchMenu->AppendSeparator();
	searchMenu->AppendItem("Replace", cmd_Replace);
	searchMenu->AppendItem("Replace All", cmd_ReplaceAll);
	searchMenu->AppendItem("Replace and Find Next", cmd_ReplaceFindNext);
	searchMenu->AppendItem("Replace and Find Previous", cmd_ReplaceFindPrev);
	searchMenu->AppendSeparator();
	searchMenu->AppendItem("Complete", cmd_CompleteLookingBack);
	searchMenu->AppendItem("Complete Looking Forward", cmd_CompleteLookingFwd);
	searchMenu->AppendSeparator();
	searchMenu->AppendItem("Jump to Line", cmd_GoToLine);
	
	mMenubar.AddMenu(searchMenu);

	MMenu* markMenu = new MMenu("Mark");
	
	markMenu->AppendItem("Jump to Next Marked Line", cmd_JumpToNextMark);
	markMenu->AppendItem("Jump to Previous Marked Line", cmd_JumpToPrevMark);
	markMenu->AppendSeparator();
	markMenu->AppendItem("Clear all Marks", cmd_ClearMarks);
	markMenu->AppendItem("Mark Line", cmd_MarkLine);
	markMenu->AppendItem("Find and Mark…", cmd_MarkMatching);
	markMenu->AppendSeparator();
	markMenu->AppendItem("Copy Marked Lines", cmd_CopyMarkedLines);
	markMenu->AppendItem("Cut Marked Lines", cmd_CutMarkedLines);
	markMenu->AppendItem("Clear Marked Lines", cmd_ClearMarkedLines);
	
	mMenubar.AddMenu(markMenu);

	MMenu* windowMenu = new MMenu("Window");
	
	windowMenu->AppendItem("Worksheet", cmd_Worksheet);	
	windowMenu->AppendSeparator();
	
	mMenubar.AddMenu(windowMenu, true);

	// content
	
	GtkWidget* hbox = gtk_hbox_new(false, 0);
	gtk_box_pack_start(GTK_BOX(mVBox), hbox, true, true, 0);

	MScrollBar* scrollBar = new MScrollBar(true);
    mTextView = new MTextView(scrollBar);
	
	gtk_box_pack_end(GTK_BOX(hbox), scrollBar->GetGtkWidget(), false, false, 0);
	gtk_box_pack_start(GTK_BOX(hbox), mTextView->GetGtkWidget(), true, true, 0);
	
	mController.AddTextView(mTextView);
	
//	AddRoute(eBoundsChanged, mTextView->eBoundsChanged);

	mTextView->SetSuper(this);
	
	gtk_widget_show_all(GetGtkWidget());

	ShellStatus(false);
}

MEditWindow::~MEditWindow()
{
	if (this == sHead)
		sHead = mNext;
	else if (sHead != nil)
	{
		MEditWindow* w = sHead;
		while (w != nil)
		{
			MEditWindow* next = w->mNext;
			if (next == this)
			{
				w->mNext = mNext;
				break;
			}
			w = next;
		}
	}
}

MEditWindow* MEditWindow::FindWindowForDocument(MDocument* inDocument)
{
	MEditWindow* w = sHead;

	while (w != nil and w->GetDocument() != inDocument)
		w = w->mNext;

	return w;
}

MEditWindow* MEditWindow::DisplayDocument(MDocument* inDocument)
{
	// If document is already open in this program, bring its
	// window to the front
		
	MEditWindow* w = FindWindowForDocument(inDocument);
	
	if (w == nil)
	{
		w = new MEditWindow;
		w->Initialize(inDocument);
		w->Show();
	}
	
	w->Select();
	
	return w;
}

void MEditWindow::Initialize(
	MDocument*		inDocument)
{
	MDocWindow::Initialize(inDocument);

	if (inDocument != nil)
	{
		try
		{
			MDocState state = {};
		
			if (inDocument->IsSpecified() and inDocument->ReadDocState(state))
			{
				mTextView->ScrollToPosition(state.mScrollPosition[0], state.mScrollPosition[1]);
				
//				::MoveWindow(GetSysWindow(),
//					state.mWindowPosition[0], state.mWindowPosition[1], true);
//				::SizeWindow(GetSysWindow(),
//					state.mWindowSize[0], state.mWindowSize[1], true);
//				::ConstrainWindowToScreen(GetSysWindow(),
//					kWindowStructureRgn, kWindowConstrainStandardOptions,
//					NULL, NULL);
			}
//			else
//				::RepositionWindow(GetSysWindow(), nil, kWindowCascadeOnMainScreen);
		}
		catch (...) {
//			::RepositionWindow(GetSysWindow(), nil, kWindowCascadeOnMainScreen);
		}
	}
}

bool MEditWindow::DoClose()
{
	return mController.TryCloseController(kSaveChangesClosingDocument);
}

void MEditWindow::ModifiedChanged(bool inModified)
{
	SetModifiedMarkInTitle(inModified);
}

void MEditWindow::FileSpecChanged(
	const MPath&		inFile)
{
	if (fs::exists(inFile))
		SetTitle(inFile.string());
	else
		SetTitle(GetUntitledTitle());
}

void MEditWindow::DocumentChanged(
	MDocument*		inDocument)
{
	if (inDocument != nil)
	{
		FileSpecChanged(inDocument->GetURL());
		ModifiedChanged(inDocument->IsModified());
	}
	else
		Close();
}
