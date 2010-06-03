//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "MJapi.h"

#include <sstream>

#include "MEditWindow.h"
#include "MTextDocument.h"
#include "MTextController.h"
#include "MStrings.h"
#include "MTextView.h"
#include "MUtils.h"
#include "MMenu.h"
#include "MFile.h"
#include "MResources.h"
#include "MControls.h"
#include "MPreferences.h"

using namespace std;

// ------------------------------------------------------------------
//

//class MParsePopup : public MView
//{
//  public:
//						MParsePopup(
//							int32			inWidth);
//
//	void				SetText(
//							const string&	inText);
//
//	void				SetController(
//							MController*	inController,
//							bool			inIsFunctionParser);
//
//  private:
//
//	//bool				OnButtonPressEvent(
//	//						GdkEventButton*	inEvent);
//
//	MController*		mController;
//	bool				mIsFunctionParser;
//};
//
//MParsePopup::MParsePopup(
//	int32			inWidth)
//	: MView(gtk_label_new(nil), true)
//	, mIsFunctionParser(false)
//{
//	MRect b;
//	GetBounds(b);
//	b.width = inWidth;
//	SetBounds(b);
//	
//	gtk_label_set_selectable(GTK_LABEL(GetGtkWidget()), true);
//	gtk_misc_set_alignment(GTK_MISC(GetGtkWidget()), 0, 0.5);
//}
//
//void MParsePopup::SetController(
//	MController*	inController,
//	bool			inIsFunctionParser)
//{
//	mController = inController;
//	mIsFunctionParser = inIsFunctionParser;
//	
//	if (not mIsFunctionParser)
//		SetText("#inc<>");
//}
//
//void MParsePopup::SetText(
//	const string&	inText)
//{
//	if (GTK_IS_LABEL(GetGtkWidget()))
//		gtk_label_set_text(GTK_LABEL(GetGtkWidget()), inText.c_str());
//}
//
//bool MParsePopup::OnButtonPressEvent(
//	GdkEventButton*		inEvent)
//{
//	assert(mController != nil);
//	
//	MTextDocument* doc = dynamic_cast<MTextDocument*>(mController->GetDocument());
//	if (doc != nil)
//	{
//		MMenu* popup = new MMenu("popup");
//		
//		int32 x = 0, y = 0;
//		ConvertToGlobal(x, y);
//		
//		if (mIsFunctionParser)
//		{
//			if (doc->GetParsePopupItems(*popup))
//				popup->Popup(mController, inEvent, x, y, true);
//		}
//		else
//		{
//			if (doc->GetIncludePopupItems(*popup))
//				popup->Popup(mController, inEvent, x, y, true);
//		}
//	}
//	
//	return true;
//}

// ------------------------------------------------------------------
//

//class MSSHProgress
//{
//  public:
//					MSSHProgress(
//						GtkWidget*		inWindowVBox);
//
//	virtual			~MSSHProgress();
//
//	void			Progress(
//						float			inFraction,
//						const string&	inMessage);
//
//  private:
//	GtkWidget*		mProgressBin;
//	GtkWidget*		mProgressBar;
//	GtkWidget*		mProgressLabel;
//};
//
//MSSHProgress::MSSHProgress(
//	GtkWidget*		inWindowVBox)
//{
//	mProgressBin = gtk_vbox_new(false, 4);
//	gtk_container_set_border_width(GTK_CONTAINER(mProgressBin), 10);
//	gtk_box_pack_start(GTK_BOX(inWindowVBox), mProgressBin, false, false, 0);
//	gtk_box_reorder_child(GTK_BOX(inWindowVBox), mProgressBin, 1);
//	
//	mProgressBar = gtk_progress_bar_new();
//	gtk_box_pack_start(GTK_BOX(mProgressBin), mProgressBar, false, false, 0);
//	
//	mProgressLabel = gtk_label_new("");
//	gtk_misc_set_alignment(GTK_MISC(mProgressLabel), 0, 0.5);
//	gtk_box_pack_start(GTK_BOX(mProgressBin), mProgressLabel, false, false, 0);
//	
//	gtk_widget_show_all(mProgressBin);
//}
//
//MSSHProgress::~MSSHProgress()
//{
//	gtk_widget_hide(mProgressBin);
//}
//
//void MSSHProgress::Progress(
//	float			inFraction,
//	const string&	inMessage)
//{
//	gtk_label_set_text(GTK_LABEL(mProgressLabel), inMessage.c_str());
//	gtk_label_set_ellipsize(GTK_LABEL(mProgressLabel), PANGO_ELLIPSIZE_MIDDLE);
//
//	gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(mProgressBar), inFraction);
//}

// ------------------------------------------------------------------
//

MEditWindow::MEditWindow()
	: MDocWindow("Untitled", MRect(0, 0, 600, 600), kMPostionDefault, "edit-window-menu")
	, eSelectionChanged(this, &MEditWindow::SelectionChanged)
	, eShellStatus(this, &MEditWindow::ShellStatus)
	, eSSHProgress(this, &MEditWindow::SSHProgress)
	, eSaveState(this, &MEditWindow::SaveState)
	, mTextView(nil)
	//, mSelectionPanel(nil)
	, mParsePopup(nil)
	, mIncludePopup(nil)
	, mSSHProgress(nil)
	, mStatusbar(nil)
{
	//MTextController* textController = new MTextController(this);
	//mController = textController;
	
	//mMenubar.SetTarget(mController);

//	// add status 
//	
//	GtkWidget* statusBar = GetWidget('stat');
//	
//	GtkShadowType shadow_type;
//	gtk_widget_style_get(statusBar, "shadow_type", &shadow_type, nil);
//	
//	// selection status
//
//	GtkWidget* frame = gtk_frame_new(nil);
//	gtk_frame_set_shadow_type(GTK_FRAME(frame), shadow_type);
//	
//	mSelectionPanel = gtk_label_new("1, 1");
//	gtk_label_set_single_line_mode(GTK_LABEL(mSelectionPanel), true);
//	gtk_container_add(GTK_CONTAINER(frame), mSelectionPanel);	
//	
//	gtk_box_pack_start(GTK_BOX(statusBar), frame, false, false, 0);
//	gtk_box_reorder_child(GTK_BOX(statusBar), frame, 0);
//	gtk_widget_set_size_request(mSelectionPanel, 100, -1);
//	
//	// parse popups
//	
//	mIncludePopup = new MParsePopup(50);
//	frame = gtk_frame_new(nil);
//	gtk_frame_set_shadow_type(GTK_FRAME(frame), shadow_type);
//	gtk_container_add(GTK_CONTAINER(frame), mIncludePopup->GetGtkWidget());
//	gtk_box_pack_start(GTK_BOX(statusBar), frame, false, false, 0);
//	gtk_box_reorder_child(GTK_BOX(statusBar), frame, 1);
//	mIncludePopup->SetController(mController, false);
//	
//	mParsePopup = new MParsePopup(200);
//	frame = gtk_frame_new(nil);
//	gtk_frame_set_shadow_type(GTK_FRAME(frame), shadow_type);
//	gtk_container_add(GTK_CONTAINER(frame), mParsePopup->GetGtkWidget());
//	gtk_box_pack_start(GTK_BOX(statusBar), frame, true, true, 0);
//	gtk_box_reorder_child(GTK_BOX(statusBar), frame, 2);	
//	mParsePopup->SetController(mController, true);
//
//	// hscrollbar
//	GtkWidget* hScrollBar = gtk_hscrollbar_new(nil);
//	frame = gtk_frame_new(nil);
//	gtk_frame_set_shadow_type(GTK_FRAME(frame), shadow_type);
//	gtk_container_add(GTK_CONTAINER(frame), hScrollBar);
//	gtk_box_pack_end(GTK_BOX(statusBar), frame, false, false, 0);
////	gtk_box_reorder_child(GTK_BOX(statusBar), frame, 3);
//	gtk_widget_set_size_request(hScrollBar, 150, -1);
//	
//	gtk_widget_show_all(statusBar);

	MRect bounds;

	// status bar
	GetBounds(bounds);
	bounds.y += bounds.height - kScrollbarWidth;
	bounds.height = kScrollbarWidth;

	int32 partWidths[4] = { 100, -1 };
	mStatusbar = new MStatusbar('stat', bounds, 2, partWidths);
	AddChild(mStatusbar);
	mStatusbar->GetFrame(bounds);

	int32 statusbarHeight = bounds.height;

	// text view
	GetBounds(bounds);
	
	bounds.height -= statusbarHeight;

    mTextView = new MTextView('text', bounds);
	MViewScroller* scroller = new MViewScroller('scrl', mTextView, false, true);
	scroller->SetBindings(true, true, true, true);
	AddChild(scroller);
	//textController->AddTextView(mTextView);

	mTextView->SetController(&mController);
	SetFocus(&mController);
	
	AddRoute(mController.eAboutToCloseDocument, eSaveState);

	ShellStatus(false);
}

MEditWindow::~MEditWindow()
{
	//delete mParsePopup;
	//delete mIncludePopup;
	//delete mSSHProgress;
}

void MEditWindow::SetDocument(
	MDocument*		inDocument)
{
	RemoveRoutes(inDocument);

	MDocWindow::SetDocument(inDocument);

	AddRoutes(inDocument);
	
	MTextDocument* doc = dynamic_cast<MTextDocument*>(inDocument);
	
	if (doc != nil)
	{
		try
		{
			MDocState state = {};
		
			if (inDocument->IsSpecified() and doc->ReadDocState(state))
			{
				mTextView->ScrollTo(state.mScrollPosition[0], state.mScrollPosition[1]);
				
				if (state.mWindowSize[0] > 100 and state.mWindowSize[1] > 100)
				{
					SetWindowPosition(MRect(
						state.mWindowPosition[0], state.mWindowPosition[1],
						state.mWindowSize[0], state.mWindowSize[1]));
				}
			}
		}
		catch (...) {
		}
	}
}

void MEditWindow::SaveState(
	MDocument*		inDocument)
{
	try
	{
		if (inDocument->IsSpecified() and Preferences::GetInteger("save state", 1))
		{
			MTextDocument* doc = static_cast<MTextDocument*>(inDocument);
			doc->SaveState(this);
		}
	}
	catch (...) {}
}

MEditWindow* MEditWindow::FindWindowForDocument(MDocument* inDocument)
{
	MWindow* w = MWindow::GetFirstWindow();

	while (w != nil)
	{
		MEditWindow* d = dynamic_cast<MEditWindow*>(w);

		if (d != nil and d->GetDocument() == inDocument)
			break;

		w = w->GetNextWindow();
	}

	return static_cast<MEditWindow*>(w);
}

MEditWindow* MEditWindow::DisplayDocument(MDocument* inDocument)
{
	// If document is already open in this program, bring its
	// window to the front
		
	MEditWindow* w = FindWindowForDocument(inDocument);
	
	if (w == nil)
	{
		w = new MEditWindow;
		w->SetDocument(inDocument);
		w->Show();
	}
	
	w->Select();
	
	return w;
}

void MEditWindow::SelectionChanged(
	MSelection		inNewSelection,
	string			inRangeName)
{
	stringstream str;

	try
	{
		uint32 line, column;
		MTextDocument* doc = dynamic_cast<MTextDocument*>(GetDocument());
		THROW_IF_NIL(doc);
		
		inNewSelection.GetCaretLineAndColumn(line, column);
	
		str << line + 1 << ',' << column + 1;
	}
	catch (...) {}

	mStatusbar->SetStatusText(0, str.str(), false);
	mStatusbar->SetStatusText(1, inRangeName, false);
	//mStatusbar->SetStatusText(3, "", true);

	//mParsePopup->SetText(inRangeName);
}

void MEditWindow::ShellStatus(
	bool			inActive)
{
//	HIViewID id = { kJapieSignature, kChasingArrowsViewID };
//	HIViewRef viewRef;
//	THROW_IF_OSERROR(::HIViewFindByID(GetContentViewRef(), id, &viewRef));
//	::HIViewSetVisible(viewRef, inActive);
}

void MEditWindow::SSHProgress(
	float			inFraction,
	std::string		inMessage)
{
	if (inFraction < 0)
	{
		//delete mSSHProgress;
		//mSSHProgress = nil;
	}
	else
	{
		//if (mSSHProgress == nil)
		//	mSSHProgress = new MSSHProgress(GetWidget('vbox'));
		//
		//mSSHProgress->Progress(inFraction, inMessage);
	}
}

void MEditWindow::AddRoutes(
	MDocument*		inDocument)
{
	MDocWindow::AddRoutes(inDocument);
	
	MTextDocument* doc = dynamic_cast<MTextDocument*>(inDocument);

	if (doc != nil)
	{
		AddRoute(doc->eSelectionChanged, eSelectionChanged);
		AddRoute(doc->eShellStatus, eShellStatus);
		AddRoute(doc->eSSHProgress, eSSHProgress);
	}
}

void MEditWindow::RemoveRoutes(
	MDocument*		inDocument)
{
	MDocWindow::RemoveRoutes(inDocument);
	
	MTextDocument* doc = dynamic_cast<MTextDocument*>(inDocument);

	if (doc != nil)
	{
		RemoveRoute(doc->eSelectionChanged, eSelectionChanged);
		RemoveRoute(doc->eShellStatus, eShellStatus);
		RemoveRoute(doc->eSSHProgress, eSSHProgress);
	}
}
