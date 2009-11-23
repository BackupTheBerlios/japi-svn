//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "MJapi.h"

#include "MPrinter.h"
#include "MDocWindow.h"
#include "MDocument.h"
#include "MDevice.h"
#include "MError.h"

using namespace std;

GtkPrintSettings*	MPrinter::sSettings;
GtkPageSetup*		MPrinter::sPageSetup;

MPrinter::MPrinter(
	MView*		inView)
	: mBeginPrint(this, &MPrinter::OnBeginPrint)
	, mDrawPage(this, &MPrinter::OnDrawPage)
	, mPrint(gtk_print_operation_new())
	, mPrintedView(inView)
{
	if (sSettings != nil)
		gtk_print_operation_set_print_settings(mPrint, sSettings);
	
//	if (sPageSetup != nil)
//		gtk_print_operation_set_default_page_setup(mPrint, sPageSetup);
	
	mBeginPrint.Connect(G_OBJECT(mPrint), "begin-print");
	mDrawPage.Connect(G_OBJECT(mPrint), "draw-page");
}

MPrinter::~MPrinter()
{
	g_object_unref(mPrint);
}

void MPrinter::DoPageSetup()
{
	if (sSettings == nil)
		sSettings = gtk_print_settings_new();

	GtkPageSetup* newPageSetup = gtk_print_run_page_setup_dialog(nil, sPageSetup, sSettings);

	if (sPageSetup != nil)
		g_object_unref(sPageSetup);

	sPageSetup = newPageSetup;
}

void MPrinter::DoPrint()
{
	MDocWindow* w = dynamic_cast<MDocWindow*>(mPrintedView->GetWindow());
	
	if (w == nil)
		THROW(("No window!"));

	if (sPageSetup)
		gtk_print_operation_set_default_page_setup(mPrint, sPageSetup);

	GtkPrintOperationResult res = gtk_print_operation_run(mPrint,
		GTK_PRINT_OPERATION_ACTION_PRINT_DIALOG,
		GTK_WINDOW(w->GetGtkWidget()), nil);

	MDocument* doc = w->GetDocument();
	if (doc->IsSpecified())
	{
		string jobname = doc->GetFile().GetPath().leaf();
		gtk_print_operation_set_job_name(mPrint, jobname.c_str());
		gtk_print_operation_set_export_filename(mPrint, jobname.c_str());
	}
	
	if (res == GTK_PRINT_OPERATION_RESULT_APPLY)
	{
		if (sSettings != nil)
			g_object_unref(sSettings);
		sSettings = static_cast<GtkPrintSettings*>(
			g_object_ref(gtk_print_operation_get_print_settings(mPrint)));
	}
}

void MPrinter::OnBeginPrint(
	GtkPrintContext*	inContext)
{
	MRect update = GetPrintBounds(inContext);
	MDevice dev(inContext, update, 0);
	gtk_print_operation_set_n_pages(mPrint, mPrintedView->CountPages(dev));
}

void MPrinter::OnDrawPage(
	GtkPrintContext*	inContext,
	int32				inPage)
{
	MRect update = GetPrintBounds(inContext);
	MDevice dev(inContext, update, inPage);
	mPrintedView->Draw(dev, update);
}

MRect MPrinter::GetPrintBounds(
	GtkPrintContext*	inContext)
{
	GtkPageSetup* setup = gtk_print_operation_get_default_page_setup(mPrint);
	
	uint32 margin[4];
	margin[0] = static_cast<uint32>(gtk_page_setup_get_left_margin(setup, GTK_UNIT_POINTS));
	if (margin[0] == 0)
		margin[0] = 15;
	
	margin[1] = static_cast<uint32>(gtk_page_setup_get_top_margin(setup, GTK_UNIT_POINTS));
	if (margin[1] == 0)
		margin[1] = 30;
	
	margin[2] = static_cast<uint32>(gtk_page_setup_get_right_margin(setup, GTK_UNIT_POINTS));
	if (margin[2] == 0)
		margin[2] = 15;
	
	margin[3] = static_cast<uint32>(gtk_page_setup_get_bottom_margin(setup, GTK_UNIT_POINTS));
	if (margin[3] == 0)
		margin[3] = 30;
	
	uint32 width = static_cast<uint32>(gtk_print_context_get_width(inContext));
	uint32 height = static_cast<uint32>(gtk_print_context_get_height(inContext));
	
	return MRect(margin[0], margin[1], width - margin[0] - margin[2], height - margin[1] - margin[3]);
}
