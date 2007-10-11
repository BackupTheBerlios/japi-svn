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
#include "MVScrollbar.h"
#include "MDrawingArea.h"

using namespace std;

//namespace
//{
//enum {
//	kTextViewPlaceHolderID = 128,
//	kSelectionPanelViewID = 129,
//	kChasingArrowsViewID = 130,
//	kFunctionPopupViewID = 131,
//	kIncludePopupViewID = 132
//};
//}
//
//// ------------------------------------------------------------------
////
//
//class MParsePopup : public MView
//{
//  public:
//	static CFStringRef	GetClassID()					{ return CFSTR("com.hekkelman.japie.ParsePopup"); }
////	static CFStringRef	GetBaseClassID()				{ return kHIScrollViewClassID; }
//
//						MParsePopup(HIObjectRef inObjectRef);
//
////	virtual OSStatus	Initialize(EventRef ioEvent);
//	void				SetText(const string& inText);
//	void				SetController(MController* inController, bool inIsFunctionParser);
//
//  private:
//
//	OSStatus			DoControlDraw(EventRef ioEvent);
//	OSStatus			DoControlClick(EventRef inEvent);
//
//	MController*		mController;
//	string				mName;
//	bool				mIsFunctionParser;
//};
//
//MParsePopup::MParsePopup(HIObjectRef inObjectRef)
//	: MView(inObjectRef, kControlSupportsEmbedding)
//	, mIsFunctionParser(false)
//{
//	Install(kEventClassControl, kEventControlDraw,	this, &MParsePopup::DoControlDraw);
//	Install(kEventClassControl, kEventControlClick,	this, &MParsePopup::DoControlClick);
//}
//
////OSStatus MParsePopup::Initialize(EventRef ioEvent)
////{
////	OSStatus err = MView::Initialize(ioEvent);
////	
////	return err;
////}
//
//void MParsePopup::SetController(MController* inController, bool inIsFunctionParser)
//{
//	mController = inController;
//	mIsFunctionParser = inIsFunctionParser;
//	
//	if (not mIsFunctionParser)
//		SetText("<>");
//}
//
//void MParsePopup::SetText(const string& inName)
//{
//	mName = inName;
//	SetNeedsDisplay(true);
//}
//
//OSStatus MParsePopup::DoControlDraw(EventRef ioEvent)
//{
//	if (mName.length())
//	{
//		CGContextRef context = nil;
//		::GetEventParameter(ioEvent, kEventParamCGContextRef,
//			typeCGContextRef, nil, sizeof(CGContextRef), nil, &context);
//	
//		HIRect bounds;
//		GetBounds(bounds);
//		
//		ThemeDrawState state = kThemeStateActive;
//		if (not IsActive())
//			state = kThemeStateInactive;
//		
//		Rect r;
//		r.left =	static_cast<short>(bounds.origin.x);
//		r.top = 	static_cast<short>(bounds.origin.y + bounds.size.height / 2 - 1);
//		r.right =	static_cast<short>(r.left + 8);
//		r.bottom =	static_cast<short>(r.top + 3);
//	
//		::DrawThemePopupArrow(&r, kThemeArrowDown, kThemeArrow5pt, state, nil, 0);
//	
//		r.left =	static_cast<short>(bounds.origin.x + 9);
//		r.top =		static_cast<short>(bounds.origin.y);
//		r.right =	static_cast<short>(r.left + bounds.size.width - 7);
//		r.bottom =	static_cast<short>(bounds.origin.y + bounds.size.height);
//		
//		MCFString s(mName);
//		::DrawThemeTextBox(s, kThemeSmallSystemFont, state, false, &r, teJustLeft, context);
//	}
//	
//	return noErr;
//}
//
//OSStatus MParsePopup::DoControlClick(EventRef ioEvent)
//{
//	assert(mController != nil);
//	
//	MDocument* doc = mController->GetDocument();
//	if (doc != nil)
//	{
//		MMenu popup;
//
//		HIPoint pt = {};
//		ConvertToGlobal(pt);
//		
//		if (mIsFunctionParser)
//		{
//			if (doc->GetParsePopupItems(popup))
//			{
//				uint32 select = popup.Popup(pt, true);
//				if (select)
//					doc->SelectParsePopupItem(select);
//			}
//		}
//		else
//		{
//			if (doc->GetIncludePopupItems(popup))
//			{
//				uint32 select = popup.Popup(pt, true);
//				if (select)
//					doc->SelectIncludePopupItem(select);
//			}
//		}
//	}
//	
//	return noErr;
//}

// ------------------------------------------------------------------
//

MDocWindow* MDocWindow::sFirst;

MDocWindow::MDocWindow()
	: eModifiedChanged(this, &MDocWindow::ModifiedChanged)
	, eFileSpecChanged(this, &MDocWindow::FileSpecChanged)
	, eSelectionChanged(this, &MDocWindow::SelectionChanged)
	, eShellStatus(this, &MDocWindow::ShellStatus)
	, eDocumentChanged(this, &MDocWindow::DocumentChanged)
	, mVBox(gtk_vbox_new(false, 0))
	, mMenubar(this, mVBox)
{
    gtk_widget_set_size_request(GTK_WIDGET(GetGtkWidget()), 400, 200);

	gtk_container_add(GTK_CONTAINER(GetGtkWidget()), mVBox);
	
	// the menubar
	
	MMenu* fileMenu = new MMenu("File");
	fileMenu->AppendItem("New", cmd_New);
	fileMenu->AppendItem("Open…", cmd_Open);
	fileMenu->AppendSeparator();
	fileMenu->AppendItem("Close", cmd_Close);
	fileMenu->AppendItem("Save", cmd_Save);
	fileMenu->AppendItem("Save As…", cmd_SaveAs);
	fileMenu->AppendSeparator();
	fileMenu->AppendItem("Quit", cmd_Quit);
	
	mMenubar.AddMenu(fileMenu);
	
	// add status 
	
	mStatusbar = gtk_statusbar_new();
	gtk_box_pack_end(GTK_BOX(mVBox), mStatusbar, false, false, 0);

	// content
	
	GtkWidget* hbox = gtk_hbox_new(false, 0);
	gtk_box_pack_start(GTK_BOX(mVBox), hbox, true, true, 0);

	GtkObject* adj = gtk_adjustment_new(0, 0, 0, 0, 0, 0);
	
	MViewPort* viewPort = new MViewPort(nil, adj);
	viewPort->SetShadowType(GTK_SHADOW_NONE);
	
	MVScrollbar* scrollBar = new MVScrollbar(adj);
	
	gtk_box_pack_end(GTK_BOX(hbox), scrollBar->GetGtkWidget(), false, false, 0);
	gtk_box_pack_start(GTK_BOX(hbox), viewPort->GetGtkWidget(), true, true, 0);
	
//	GtkWidget* scroller = gtk_scrolled_window_new(nil, nil);
//	gtk_box_pack_start(GTK_BOX(mVBox), scroller, true, true, 0);
//	
//	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroller),
//		GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	
    mTextView = new MTextView();
	mController.AddTextView(mTextView);

//	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scroller), image);
	viewPort->Add(mTextView);
	
	gtk_widget_show_all(GetGtkWidget());

	ShellStatus(false);
	
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
	bool result = true;
	
	switch (inCommand)
	{
		default:
			result = MWindow::UpdateCommandStatus(inCommand, outEnabled, outChecked);
			break;
	}
	
	return result;
}

bool MDocWindow::ProcessCommand(
	uint32			inCommand)
{
	return MWindow::ProcessCommand(inCommand);
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
	const MURL&		inFile)
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

//	MCFString s(str.str());
//	::HIViewSetText(mSelectionPanel, s);
	
//	mParsePopup->SetText(inRangeName);
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

