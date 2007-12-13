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

using namespace std;

namespace {

const int32
	kAskSaveChanges_Save = 'save',
	kAskSaveChanges_Cancel = 'canc',
	kAskSaveChanges_DontSave = 'dont';

}

MSaverMixin* MSaverMixin::sFirst = nil;

MSaverMixin::MSaverMixin()
	: slClose(this, &MSaverMixin::OnClose)
	, slResponse(this, &MSaverMixin::OnResponse)
	, mNext(nil)
	, mCloseOnNavTerminate(true)
	, mClosePending(false)
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
	MWindow*		inParentWindow)
{
	inParentWindow->Select();

	if (mDialog != nil)
		return;
	
	mQuitPending = (inAction == kSaveChangesQuittingApplication);
	
	mDialog = gtk_message_dialog_new(GTK_WINDOW(inParentWindow->GetGtkWidget()),
		GTK_DIALOG_MODAL, GTK_MESSAGE_QUESTION, GTK_BUTTONS_NONE,
		_("Do you want to save the changes made to document %s?"),
		inParentWindow->GetTitle().c_str());
	
//	if (inAction == kSaveChangesClosingDocument)
//	{
//		
//	}

	gtk_dialog_add_button(GTK_DIALOG(mDialog), _("Don't save"), kAskSaveChanges_DontSave);
	gtk_dialog_add_button(GTK_DIALOG(mDialog), _("Cancel"), kAskSaveChanges_Cancel);
	gtk_dialog_add_button(GTK_DIALOG(mDialog), _("Save"), kAskSaveChanges_Save);
	gtk_dialog_set_default_response(GTK_DIALOG(mDialog), kAskSaveChanges_Save);
	
	slClose.Connect(mDialog, "close");
	slResponse.Connect(mDialog, "response");
	
	gtk_widget_show_all(mDialog);
	
//	NavDialogCreationOptions creationOptions;
//	::NavGetDefaultDialogCreationOptions(&creationOptions);
//	
//	creationOptions.clientName = MApplication::Instance().GetAppName();
//	creationOptions.modality = kWindowModalityWindowModal;
//	creationOptions.parentWindow = inParentWindow->GetSysWindow();
//	
//	THROW_IF_OSERROR(::NavCreateAskSaveChangesDialog(&creationOptions,
//		inAction, sNavEventUPP, this, &mDialog));
//	
//	THROW_IF_OSERROR(::NavDialogRun(mDialog));
}

void MSaverMixin::TryDiscardChanges(
	const string&	inDocumentName,
	MWindow*		inParentWindow)
{
#warning("unimplemented")


//	inParentWindow->Select();
//
//	if (mDialog != nil)
//		return;
//	
//	NavDialogCreationOptions creationOptions;
//	::NavGetDefaultDialogCreationOptions(&creationOptions);
//	
//	creationOptions.clientName = MApplication::Instance().GetAppName();
//	
//	creationOptions.modality	 = kWindowModalityWindowModal;
//	creationOptions.parentWindow = inParentWindow->GetSysWindow();
//	
//	MCFString documentName(inDocumentName);
//	creationOptions.saveFileName = documentName.UseRef();
//	
//	THROW_IF_OSERROR(::NavCreateAskDiscardChangesDialog(
//		&creationOptions, sNavEventUPP, this, &mDialog));
//	
//	THROW_IF_OSERROR(::NavDialogRun(mDialog));
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
	
bool MSaverMixin::OnResponse(
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
			break;
		
		case kAskSaveChanges_DontSave:
			CloseAfterNavigationDialog();
			if (mQuitPending)
				gApp->ProcessCommand(cmd_Quit, nil, 0);
			break;
	}
	
	return true;	
}
