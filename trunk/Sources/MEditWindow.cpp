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
	fileMenu->AppendItem("New", cmd_New, GDK_N, GDK_CONTROL_MASK);
	fileMenu->AppendItem("Open…", cmd_Open, GDK_O, GDK_CONTROL_MASK);
	
	fileMenu->AppendRecentMenu("Open Recent…");
	
	fileMenu->AppendItem("Find and open…", cmd_OpenIncludeFile, GDK_D, GDK_CONTROL_MASK | GDK_MOD1_MASK);
	fileMenu->AppendItem("Open Source/Header", cmd_SwitchHeaderSource, GDK_1, GDK_CONTROL_MASK);
	
	fileMenu->AppendSeparator();
	fileMenu->AppendItem("Close", cmd_Close, GDK_W, GDK_CONTROL_MASK);
	fileMenu->AppendItem("Save", cmd_Save, GDK_S, GDK_CONTROL_MASK);
	fileMenu->AppendItem("Save As…", cmd_SaveAs);
	fileMenu->AppendItem("Revert", cmd_Revert);
	fileMenu->AppendSeparator();
	fileMenu->AppendItem("Quit", cmd_Quit, GDK_Q, GDK_CONTROL_MASK);
	
	mMenubar.AddMenu(fileMenu);
	
	MMenu* editMenu = new MMenu("Edit");
	editMenu->AppendItem("Undo", cmd_Undo, GDK_Z, GDK_CONTROL_MASK);
	editMenu->AppendItem("Redo", cmd_Redo, GDK_Z, GDK_CONTROL_MASK | GDK_MOD1_MASK);
	editMenu->AppendSeparator();
	editMenu->AppendItem("Cut", cmd_Cut, GDK_X, GDK_CONTROL_MASK);
	editMenu->AppendItem("Cut and append", cmd_CutAppend, GDK_X, GDK_CONTROL_MASK | GDK_MOD1_MASK);
	editMenu->AppendItem("Copy", cmd_Copy, GDK_C, GDK_CONTROL_MASK);
	editMenu->AppendItem("Copy and append", cmd_CopyAppend, GDK_C, GDK_CONTROL_MASK | GDK_MOD1_MASK);
	editMenu->AppendItem("Paste", cmd_Paste, GDK_V, GDK_CONTROL_MASK);
	editMenu->AppendItem("Clear", cmd_Clear);
	editMenu->AppendItem("Select all", cmd_SelectAll, GDK_A, GDK_CONTROL_MASK);
//	editMenu->AppendSeparator();
//	editMenu->AppendItem("Font…", cmd_
	
	mMenubar.AddMenu(editMenu);
	
	MMenu* textMenu = new MMenu("Text");
	textMenu->AppendItem("Balance", cmd_Balance, GDK_B, GDK_CONTROL_MASK);
	textMenu->AppendItem("Softwrap", cmd_Softwrap);
	textMenu->AppendSeparator();
	textMenu->AppendItem("Shift Left", cmd_ShiftLeft, GDK_bracketleft, GDK_CONTROL_MASK);
	textMenu->AppendItem("Shift Right", cmd_ShiftRight, GDK_bracketright, GDK_CONTROL_MASK);
	textMenu->AppendSeparator();
	textMenu->AppendItem("Comment", cmd_Comment, GDK_apostrophe, GDK_CONTROL_MASK);
	textMenu->AppendItem("Uncomment", cmd_Uncomment, GDK_apostrophe, GDK_CONTROL_MASK | GDK_MOD1_MASK);
	textMenu->AppendSeparator();
	textMenu->AppendItem("Entab", cmd_Entab);
	textMenu->AppendItem("Detab", cmd_Detab);
	textMenu->AppendSeparator();
	textMenu->AppendItem("File Info…", cmd_ShowDocInfoDialog);

	mMenubar.AddMenu(textMenu);

	MMenu* searchMenu = new MMenu("Search");

	searchMenu->AppendItem("Fast Find", cmd_FastFind, GDK_I, GDK_CONTROL_MASK);
	searchMenu->AppendItem("Fast Find Backward", cmd_FastFind, GDK_I, GDK_CONTROL_MASK | GDK_MOD1_MASK);
	searchMenu->AppendSeparator();
	searchMenu->AppendItem("Find", cmd_Find, GDK_F, GDK_CONTROL_MASK);
	searchMenu->AppendItem("Find Next", cmd_FindNext, GDK_G, GDK_CONTROL_MASK);
	searchMenu->AppendItem("Find Previous", cmd_FindPrev, GDK_G, GDK_CONTROL_MASK | GDK_MOD1_MASK);
	searchMenu->AppendItem("Find in Next File", cmd_FindInNextFile, GDK_J, GDK_CONTROL_MASK);
	searchMenu->AppendItem("Enter Search String", cmd_EnterSearchString, GDK_E, GDK_CONTROL_MASK);
	searchMenu->AppendItem("Enter Replace String", cmd_EnterReplaceString, GDK_E, GDK_CONTROL_MASK | GDK_MOD1_MASK);
	searchMenu->AppendSeparator();
	searchMenu->AppendItem("Replace", cmd_Replace, GDK_equal, GDK_CONTROL_MASK);
	searchMenu->AppendItem("Replace All", cmd_ReplaceAll, GDK_equal, GDK_CONTROL_MASK | GDK_MOD1_MASK);
	searchMenu->AppendItem("Replace and Find Next", cmd_ReplaceFindNext, GDK_T, GDK_CONTROL_MASK);
	searchMenu->AppendItem("Replace and Find Previous", cmd_ReplaceFindPrev, GDK_T, GDK_CONTROL_MASK | GDK_MOD1_MASK);
	searchMenu->AppendSeparator();
	searchMenu->AppendItem("Complete", cmd_CompleteLookingBack, GDK_P, GDK_CONTROL_MASK);
	searchMenu->AppendItem("Complete Looking Forward", cmd_CompleteLookingFwd, GDK_P, GDK_CONTROL_MASK | GDK_MOD1_MASK);
	searchMenu->AppendSeparator();
	searchMenu->AppendItem("Jump to Line", cmd_GoToLine, GDK_comma, GDK_CONTROL_MASK);
	
	mMenubar.AddMenu(searchMenu);

	MMenu* markMenu = new MMenu("Mark");
	
	markMenu->AppendItem("Jump to Next Marked Line", cmd_JumpToNextMark, GDK_F2);
	markMenu->AppendItem("Jump to Previous Marked Line", cmd_JumpToPrevMark, GDK_F2, GDK_MOD1_MASK);
	markMenu->AppendSeparator();
	markMenu->AppendItem("Clear all Marks", cmd_ClearMarks);
	markMenu->AppendItem("Mark Line", cmd_MarkLine, GDK_F1);
	markMenu->AppendItem("Find and Mark…", cmd_MarkMatching);
	markMenu->AppendSeparator();
	markMenu->AppendItem("Copy Marked Lines", cmd_CopyMarkedLines);
	markMenu->AppendItem("Cut Marked Lines", cmd_CutMarkedLines);
	markMenu->AppendItem("Clear Marked Lines", cmd_ClearMarkedLines);
	
	mMenubar.AddMenu(markMenu);

	// add status 
	
	mStatusbar = gtk_statusbar_new();
	gtk_box_pack_end(GTK_BOX(mVBox), mStatusbar, false, false, 0);

	// content
	
	GtkWidget* hbox = gtk_hbox_new(false, 0);
	gtk_box_pack_start(GTK_BOX(mVBox), hbox, true, true, 0);

	MScrollBar* scrollBar = new MScrollBar(true);
	
	MViewPort* viewPort = new MViewPort(nil, scrollBar);
	viewPort->SetShadowType(GTK_SHADOW_NONE);
	
	gtk_box_pack_end(GTK_BOX(hbox), scrollBar->GetGtkWidget(), false, false, 0);
	gtk_box_pack_start(GTK_BOX(hbox), viewPort->GetGtkWidget(), true, true, 0);
	
    mTextView = new MTextView(scrollBar);
	mController.AddTextView(mTextView);
	
	AddRoute(viewPort->eBoundsChanged, mTextView->eBoundsChanged);

	viewPort->Add(mTextView);

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
