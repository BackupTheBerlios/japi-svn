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
#include "MScrollBar.h"
#include "MTextView.h"
#include "MUtils.h"
#include "MMenu.h"
#include "MFile.h"
#include "MResources.h"

using namespace std;

// ------------------------------------------------------------------
//

MEditWindow* MEditWindow::sHead = nil;

MEditWindow::MEditWindow()
{
	mNext = sHead;
	sHead = this;

	mMenubar.BuildFromResource("edit-window-menus");

	// content
	
	GtkWidget* hbox = gtk_hbox_new(false, 0);
	gtk_box_pack_start(GTK_BOX(mVBox), hbox, true, true, 0);

	MScrollBar* scrollBar = new MScrollBar(true);
    mTextView = new MTextView(scrollBar);
	
	gtk_box_pack_end(GTK_BOX(hbox), scrollBar->GetGtkWidget(), false, false, 0);
	gtk_box_pack_start(GTK_BOX(hbox), mTextView->GetGtkWidget(), true, true, 0);
	
	mController.AddTextView(mTextView);
	
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
	const MUrl&		inFile)
{
	if (not inFile.IsLocal() or fs::exists(inFile.GetPath()))
		SetTitle(inFile.str());
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
		
		MDocWindow::DocumentChanged(inDocument);
	}
	else
		Close();
}
