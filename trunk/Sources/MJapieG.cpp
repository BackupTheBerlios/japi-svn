#include "MJapieG.h"
#include "MWindow.h"

#include <iostream>

using namespace std;

MJapieApp* gApp;

MJapieApp::MJapieApp(
	int				argc,
	char*			argv[])
	: mQuit(false)
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
	gtk_main();
}

void MJapieApp::DoNew()
{
	MWindow* w = new MWindow();
	w->Select();
}

void MJapieApp::DoQuit()
{
	gtk_main_quit();
}

void MJapieApp::Pulse()
{
	
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

