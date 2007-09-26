#include "MJapieG.h"

#include <sstream>
#include <cassert>

#include "MDocument.h"
#include "MDocWindow.h"
#include "MCommands.h"
#include "MMenu.h"

using namespace std;

MDocWindow* MDocWindow::sFirst;

MDocWindow::MDocWindow(
	MDocument*		inDocument)
	: mDocument(inDocument)
	, mVBox(gtk_vbox_new(false, 0))
	, mMenubar(this, mVBox)
{
    gtk_widget_set_size_request(GTK_WIDGET(GetGtkWidget()), 200, 100);

	gtk_container_add(GTK_CONTAINER(GetGtkWidget()), mVBox);
	
	// the menubar
	
	MMenu* fileMenu = new MMenu("File");
	fileMenu->AddItem("New", cmd_New);
	fileMenu->AddItem("Open…", cmd_Open);
	fileMenu->AddSeparator();
	fileMenu->AddItem("Close", cmd_Close);
	fileMenu->AddItem("Save", cmd_Save);
	fileMenu->AddItem("Save As…", cmd_SaveAs);
	fileMenu->AddSeparator();
	fileMenu->AddItem("Quit", cmd_Quit);
	
	mMenubar.AddMenu(fileMenu);
	
	// set title
	
	if (inDocument->IsSpecified())
		SetTitle(inDocument->GetURL());
	else
		SetTitle(GetUntitledTitle());
	
	// add status 
	
	mStatusbar = gtk_statusbar_new();
	gtk_box_pack_end(GTK_BOX(mVBox), mStatusbar, false, false, 0);
	
	// content
	
	GtkWidget* scroller = gtk_scrolled_window_new(nil, nil);
	gtk_box_pack_start(GTK_BOX(mVBox), scroller, true, true, 0);
	
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroller),
		GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	
	// image
	GtkWidget* image = gtk_image_new_from_file("wallpaper-de-ring-3.jpg");
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scroller), image);
	
	
	gtk_widget_show_all(GetGtkWidget());
	
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
		case cmd_Save:
			outEnabled = mDocument->IsDirty();
			break;
	
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

MDocWindow* MDocWindow::DisplayDocument(
	MDocument*		inDocument)
{
	MDocWindow* result = sFirst;
	while (result != nil and result->GetDocument() != inDocument)
		result = result->mNext;
	
	if (result == nil)
		result = new MDocWindow(inDocument);
	
	return result;
}

