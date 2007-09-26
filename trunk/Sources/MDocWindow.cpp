#include "MJapieG.h"

#include "MDocWindow.h"
#include "MMenu.h"

using namespace std;

MDocWindow::MDocWindow(
	MDocument*		inDocument)
	: mDocument(inDocument)
	, mVBox(gtk_vbox_new(false, 0))
	, mMenubar(new MMenubar(mVBox))
{
    gtk_widget_set_size_request(GTK_WIDGET(GetGtkWidget()), 200, 100);

	gtk_container_add(GTK_CONTAINER(GetGtkWidget()), mVBox);
	
	MMenu* fileMenu = new MMenu("File");
	fileMenu->AddItem("Quit", cmd_Quit);
	
	mMenubar->AddMenu(fileMenu);
	
	gtk_widget_show_all(GetGtkWidget());
}

MDocWindow::~MDocWindow()
{
}

