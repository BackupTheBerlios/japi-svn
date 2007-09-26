#include "MJapieG.h"

#include "MDocument.h"
#include "MDocWindow.h"

#include <iostream>

using namespace std;

MJapieApp* gApp;

MJapieApp::MJapieApp(
	int				argc,
	char*			argv[])
	: MHandler(nil)
	, mQuit(false)
	, mQuitPending(false)
{
}

MJapieApp::~MJapieApp()
{
}

void MJapieApp::RecycleWindow(
	MWindow*		inWindow)
{
	mTrashCan.push_back(inWindow);
}

void MJapieApp::RunEventLoop()
{
	uint32 timer = g_timeout_add(250, &MJapieApp::Timeout, nil);
	
	gtk_main();
}

void MJapieApp::DoNew()
{
	MDocument* doc = new MDocument;
	MWindow* w = new MDocWindow(doc);
	w->Select();
}

void MJapieApp::DoQuit()
{
	gtk_main_quit();
}

void MJapieApp::DoOpen()
{
	GtkWidget* dialog = 
		gtk_file_chooser_dialog_new("Open", nil,
			GTK_FILE_CHOOSER_ACTION_OPEN,
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
			NULL);
	
	int32 result = gtk_dialog_run(GTK_DIALOG(dialog));
	
	MURL url;
	
	if (result == GTK_RESPONSE_ACCEPT)
		url = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
	
	gtk_widget_destroy(dialog);
	
	// open the file
	
	if (true /*fs::exists(url)*/)
	{
		MDocument* doc = MDocument::GetDocumentForURL(url, true);
		if (doc != nil)
			MDocWindow::DisplayDocument(doc);
	}
}

gboolean MJapieApp::Timeout(
	gpointer		inData)
{
	gApp->Pulse();
	return true;
}

void MJapieApp::Pulse()
{
	for (MWindowList::iterator w = mTrashCan.begin(); w != mTrashCan.end(); ++w)
		delete *w;
	
	mTrashCan.clear();
	
	if (MDocWindow::GetFirstDocWindow() == nil)
		DoQuit();
}

bool MJapieApp::UpdateCommandStatus(
	uint32			inCommand,
	bool&			outEnabled,
	bool&			outChecked)
{
	return false;
}

bool MJapieApp::ProcessCommand(
	uint32			inCommand)
{
	bool result = true;
	
	switch (inCommand)
	{
		case cmd_Quit:
			DoQuit();
			break;
		
		case cmd_Open:
			DoOpen();
			break;
		
		case cmd_New:
			DoNew();
			break;
		
		default:
			result = false;
			break;
	}
	
	return result;
}

int main(int argc, char* argv[])
{
	try
	{
		gtk_init(&argc, &argv);

		gApp = new MJapieApp(argc, argv);
		
		gApp->DoNew();
		gApp->RunEventLoop();
	}
	catch (exception& e)
	{
		cerr << e.what() << endl;
	}
	catch (...)
	{
		cerr << "Exception caught" << endl;
	}
	
	return 0;
}

