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
#include <cassert>

#include "MDocWindow.h"
#include "MDocument.h"
#include "MGlobals.h"
#include "MTextView.h"
//#include "MToolbar.h"
//#include "MTextViewContainer.h"
#include "MUtils.h"
#include "MMenu.h"
#include "MFile.h"
#include "MEditWindow.h"
#include "MView.h"
#include "MViewPort.h"
#include "MScrollBar.h"
#include "MDrawingArea.h"
#include "MDevice.h"

using namespace std;

// ------------------------------------------------------------------
//

class MParsePopup : public MDrawingArea
{
  public:
						MParsePopup(
							int32			inWidth);

	void				SetText(
							const string&	inText);

	void				SetController(
							MController*	inController,
							bool			inIsFunctionParser);

  private:

	bool				OnExposeEvent(
							GdkEventExpose*	inEvent);
	
	bool				OnButtonPressEvent(
							GdkEventButton*	inEvent);

	MController*		mController;
	string				mName;
	bool				mIsFunctionParser;
};

MParsePopup::MParsePopup(
	int32			inWidth)
	: MDrawingArea(inWidth, -1)
	, mIsFunctionParser(false)
{
}

void MParsePopup::SetController(
	MController*	inController,
	bool			inIsFunctionParser)
{
	mController = inController;
	mIsFunctionParser = inIsFunctionParser;
	
	if (not mIsFunctionParser)
		SetText("#inc<>");
}

void MParsePopup::SetText(
	const string&	inName)
{
	mName = inName;
	Invalidate();
}

bool MParsePopup::OnExposeEvent(
	GdkEventExpose*		inEvent)
{
	if (mName.length())
	{
		MRect bounds;
		GetBounds(bounds);
		
		MDevice dev(this, bounds);

		dev.DrawString(mName, bounds.x, bounds.y, true);
	}
	
	return true;
}

bool MParsePopup::OnButtonPressEvent(
	GdkEventButton*		inEvent)
{
	assert(mController != nil);
	
	MDocument* doc = mController->GetDocument();
	if (doc != nil)
	{
		MMenu* popup = new MMenu("popup");
		
		int32 x = 0, y = 0;
		ConvertToGlobal(x, y);
		
		if (mIsFunctionParser)
		{
			if (doc->GetParsePopupItems(*popup))
				popup->Popup(mController, inEvent, x, y, true);
		}
		else
		{
			if (doc->GetIncludePopupItems(*popup))
				popup->Popup(mController, inEvent, x, y, true);
		}
	}
	
	return true;
}

// ------------------------------------------------------------------
//

MDocWindow* MDocWindow::sFirst;

MDocWindow::MDocWindow()
	: eModifiedChanged(this, &MDocWindow::ModifiedChanged)
	, eFileSpecChanged(this, &MDocWindow::FileSpecChanged)
	, eSelectionChanged(this, &MDocWindow::SelectionChanged)
	, eShellStatus(this, &MDocWindow::ShellStatus)
	, eDocumentChanged(this, &MDocWindow::DocumentChanged)
	, mTextView(nil)
	, mVBox(gtk_vbox_new(false, 0))
	, mStatusbar(nil)
	, mMenubar(this, mVBox, GetGtkWidget())
	, mParsePopup(nil)
	, mIncludePopup(nil)
{
	GdkGeometry geom = {};
	geom.min_width = 300;
	geom.min_height = 100;
	gtk_window_set_geometry_hints(GTK_WINDOW(GetGtkWidget()), nil, &geom, GDK_HINT_MIN_SIZE);	
	gtk_window_set_default_size(GTK_WINDOW(GetGtkWidget()), 600, 700);

	gtk_container_add(GTK_CONTAINER(GetGtkWidget()), mVBox);

	// add status 
	
	mStatusbar = gtk_statusbar_new();
	gtk_box_pack_end(GTK_BOX(mVBox), mStatusbar, false, false, 0);
	
	// selection status
	
	mSelectionPanel = gtk_label_new("aap noot mies");
	gtk_box_pack_start(GTK_BOX(mStatusbar), mSelectionPanel, false, false, 0);
	gtk_box_reorder_child(GTK_BOX(mStatusbar), mSelectionPanel, 0);
	gtk_widget_set_size_request(mSelectionPanel, 100, -1);
	
	// parse popups
	
	mIncludePopup = new MParsePopup(50);
	gtk_box_pack_start(GTK_BOX(mStatusbar), mIncludePopup->GetGtkWidget(), false, false, 0);
	gtk_box_reorder_child(GTK_BOX(mStatusbar), mIncludePopup->GetGtkWidget(), 1);
	mIncludePopup->SetController(&mController, false);
	
	mParsePopup = new MParsePopup(200);
	gtk_box_pack_start(GTK_BOX(mStatusbar), mParsePopup->GetGtkWidget(), true, true, 0);
	gtk_box_reorder_child(GTK_BOX(mStatusbar), mParsePopup->GetGtkWidget(), 2);	
	mParsePopup->SetController(&mController, true);
	
	mNext = sFirst;
	sFirst = this;
}

MDocWindow::~MDocWindow()
{
	if (sFirst == this)
		sFirst = mNext;
	else
	{
		MDocWindow* w = sFirst;
		while (w != nil and w->mNext != this)
			w = w->mNext;
		assert(w != nil);
		if (w != nil)
			w->mNext = mNext;
	}
}

void MDocWindow::Initialize(
	MDocument*		inDocument)
{
	mController.SetWindow(this);
	mController.SetDocument(inDocument);
}

bool MDocWindow::DoClose()
{
	return mController.TryCloseController(kSaveChangesClosingDocument);
}

MDocWindow* MDocWindow::FindWindowForDocument(MDocument* inDocument)
{
	MDocWindow* w = sFirst;

	while (w != nil and w->GetDocument() != inDocument)
		w = w->mNext;

	return w;
}

MDocWindow* MDocWindow::DisplayDocument(
	MDocument*		inDocument)
{
	MDocWindow* result = FindWindowForDocument(inDocument);
	
	if (result == nil)
	{
		MEditWindow* e = new MEditWindow;
		e->Initialize(inDocument);
		e->Show();

		result = e;
	}
	
	result->Select();
	
	return result;
}

string MDocWindow::GetUntitledTitle()
{
	static int sDocNr = 0;
	stringstream result;
	
	result << "Naamloos";
	
	if (++sDocNr > 1)
		result << ' ' << sDocNr;
	
	return result.str();
}

bool MDocWindow::UpdateCommandStatus(
	uint32			inCommand,
	bool&			outEnabled,
	bool&			outChecked)
{
	bool result = mController.UpdateCommandStatus(inCommand, outEnabled, outChecked);

	if (result == false)
	{
		result = true;
		
		switch (inCommand)
		{
			default:
				result = MWindow::UpdateCommandStatus(inCommand, outEnabled, outChecked);
				break;
		}
	}
	
	return result;
}

bool MDocWindow::ProcessCommand(
	uint32			inCommand,
	const MMenu*	inMenu,
	uint32			inItemIndex)
{
	bool result = mController.ProcessCommand(inCommand, inMenu, inItemIndex);
	
	if (result == false)
		result = MWindow::ProcessCommand(inCommand, inMenu, inItemIndex);
	
	return result;
}

MDocument* MDocWindow::GetDocument()
{
	return mController.GetDocument();
}

void MDocWindow::ModifiedChanged(
	bool			inModified)
{
}

void MDocWindow::FileSpecChanged(
	const MPath&		inFile)
{
}

void MDocWindow::SelectionChanged(
	MSelection		inNewSelection,
	string			inRangeName)
{
	stringstream str;

	try
	{
		uint32 line, column;
		inNewSelection.GetCaretLineAndColumn(*GetDocument(), line, column);
	
		str << line + 1 << ',' << column + 1;
	}
	catch (...) {}

	gtk_label_set_text(GTK_LABEL(mSelectionPanel), str.str().c_str());
	
	mParsePopup->SetText(inRangeName);
}

void MDocWindow::DocumentChanged(
	MDocument*		inDocument)
{
	// set title
	
	if (inDocument->IsSpecified())
		SetTitle(inDocument->GetURL().string());
	else
		SetTitle(GetUntitledTitle());
}

void MDocWindow::ShellStatus(
	bool			inActive)
{
//	HIViewID id = { kJapieSignature, kChasingArrowsViewID };
//	HIViewRef viewRef;
//	THROW_IF_OSERROR(::HIViewFindByID(GetContentViewRef(), id, &viewRef));
//	::HIViewSetVisible(viewRef, inActive);
}

