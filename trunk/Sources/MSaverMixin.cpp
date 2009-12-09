//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "MJapi.h"

#include <iostream>

#include "MWindow.h"
#include "MSaverMixin.h"
#include "MStrings.h"
#include "MCommands.h"
#include "MAlerts.h"
#include "MError.h"
#include "MFile.h"
#include "MJapiApp.h"

using namespace std;

namespace {

const int32
	kAskSaveChanges_Save = 3,
	kAskSaveChanges_Cancel = 2,
	kAskSaveChanges_DontSave = 1,
	
	kDiscardChanges_Discard = 1,
	kDiscardChanges_Cancel = 2;

}

MSaverMixin* MSaverMixin::sFirst = nil;

MSaverMixin::MSaverMixin()
	: slClose(this, &MSaverMixin::OnClose)
	, slSaveResponse(this, &MSaverMixin::OnSaveResponse)
	, slDiscardResponse(this, &MSaverMixin::OnDiscardResponse)
	, mNext(nil)
	, mCloseOnNavTerminate(true)
	, mClosePending(false)
	, mCloseAllPending(false)
	, mQuitPending(false)
	, mDialog(nil)
{
	mNext = sFirst;
	sFirst = this;
}

MSaverMixin::~MSaverMixin()
{
	assert(mDialog == nil);
	assert(sFirst != nil);
	
	if (sFirst == this)
		sFirst = mNext;
	else
	{
		MSaverMixin* m = sFirst;
		while (m != nil and m->mNext != this)
			m = m->mNext;
		
		assert(m != nil);
		
		m->mNext = mNext;
	}
	
	if (mDialog != nil)
		gtk_widget_destroy(mDialog);

	mDialog = nil;
}
	
bool MSaverMixin::IsNavDialogVisible()
{
	MSaverMixin* m = sFirst;

	while (m != nil and m->mDialog == nil)
		m = m->mNext;
	
	return m != nil;
}

void MSaverMixin::TryCloseDocument(
	MCloseReason	inAction,
	const string&	inDocumentName,
	MWindow*		inParentWindow)
{
	inParentWindow->Select();

	if (mDialog != nil)
		return;
	
	mQuitPending = (inAction == kSaveChangesQuittingApplication);
	mCloseAllPending = (inAction == kSaveChangesClosingAllDocuments);
	
	mDialog = CreateAlert("save-changes-alert", inDocumentName);
	
	slClose.Connect(mDialog, "close");
	slSaveResponse.Connect(mDialog, "response");

	gtk_window_set_transient_for(
		GTK_WINDOW(mDialog),
		GTK_WINDOW(inParentWindow->GetGtkWidget()));
	
	gtk_widget_show_all(mDialog);
}

void MSaverMixin::TryDiscardChanges(
	const string&	inDocumentName,
	MWindow*		inParentWindow)
{
	inParentWindow->Select();

	if (mDialog != nil)
		return;

	mDialog = CreateAlert("discard-changes-alert", inDocumentName);
	
	slClose.Connect(mDialog, "close");
	slDiscardResponse.Connect(mDialog, "response");
	
	gtk_window_set_transient_for(
		GTK_WINDOW(mDialog),
		GTK_WINDOW(inParentWindow->GetGtkWidget()));

	gtk_widget_show_all(mDialog);
}

void MSaverMixin::SaveDocumentAs(
	MWindow*		inParentWindow,
	const string&	inSuggestedName)
{
	GtkWidget *dialog;
	
	dialog = gtk_file_chooser_dialog_new(_("Save File"),
					      GTK_WINDOW(inParentWindow->GetGtkWidget()),
					      GTK_FILE_CHOOSER_ACTION_SAVE,
					      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					      GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
					      NULL);

	gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog), true);
	
//	    gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dialog), default_folder_for_saving);
	gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (dialog), inSuggestedName.c_str());
	gtk_file_chooser_set_local_only(GTK_FILE_CHOOSER(dialog), false);

	if (gApp->GetCurrentFolder().length() > 0)
	{
		gtk_file_chooser_set_current_folder_uri(
			GTK_FILE_CHOOSER(dialog), gApp->GetCurrentFolder().c_str());
	}
	
	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
	{
		char* uri = gtk_file_chooser_get_uri(GTK_FILE_CHOOSER(dialog));
		
		THROW_IF_NIL((uri));
		
		MFile file(uri, true);
		DoSaveAs(file);
		g_free(uri);
		
		gApp->SetCurrentFolder(
			gtk_file_chooser_get_current_folder_uri(GTK_FILE_CHOOSER(dialog)));
	}
	else
	{
		mClosePending = false;
		mCloseAllPending = false;
	}
	
	gtk_widget_destroy(dialog);
	
	mDialog = nil;
	
	if (mClosePending)
		CloseAfterNavigationDialog();
}

bool MSaverMixin::OnClose()
{
	mDialog = nil;
	return false;
}
	
bool MSaverMixin::OnSaveResponse(
	gint		inArg)
{
	gtk_widget_destroy(mDialog);
	mDialog = nil;

	switch (inArg)
	{
		case kAskSaveChanges_Save:
			mClosePending = true;
			if (SaveDocument())
				CloseAfterNavigationDialog();
			break;
		
		case kAskSaveChanges_Cancel:
			mQuitPending = false;
			mClosePending = false;
			mCloseAllPending = false;
			break;
		
		case kAskSaveChanges_DontSave:
			CloseAfterNavigationDialog();
			if (mQuitPending)
				gApp->ProcessCommand(cmd_Quit, nil, 0, 0);
			else if (mCloseAllPending)
				gApp->ProcessCommand(cmd_CloseAll, nil, 0, 0);
			break;
	}
	
	return true;	
}

bool MSaverMixin::OnDiscardResponse(
	gint		inArg)
{
	gtk_widget_destroy(mDialog);
	mDialog = nil;

	if (inArg == kDiscardChanges_Discard)
		RevertDocument();
	
	return true;	
}
