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

#include <iostream>

#include "MWindow.h"
#include "MSaverMixin.h"
#include "MStrings.h"
#include "MCommands.h"
#include "MAlerts.h"

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
	
	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
	{
		char* uri = gtk_file_chooser_get_uri(GTK_FILE_CHOOSER(dialog));
		
		THROW_IF_NIL((uri));
		
		MUrl url(uri);
		DoSaveAs(url);
		g_free(uri);
		
		gApp->SetCurrentFolder(gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER(dialog)));
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
				gApp->ProcessCommand(cmd_Quit, nil, 0);
			else if (mCloseAllPending)
				gApp->ProcessCommand(cmd_CloseAll, nil, 0);
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
