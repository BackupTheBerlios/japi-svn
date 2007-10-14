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
	
	mDialog = gtk_message_dialog_new(GTK_WINDOW(inParentWindow->GetGtkWidget()),
		GTK_DIALOG_MODAL, GTK_MESSAGE_QUESTION, GTK_BUTTONS_NONE,
		"Do you want to save the changes made to document %s?",
		inParentWindow->GetTitle().c_str());
	
//	if (inAction == kSaveChangesClosingDocument)
//	{
//		
//	}

	gtk_dialog_add_button(GTK_DIALOG(mDialog), "Don't save", kAskSaveChanges_DontSave);
	gtk_dialog_add_button(GTK_DIALOG(mDialog), "Cancel", kAskSaveChanges_Cancel);
	gtk_dialog_add_button(GTK_DIALOG(mDialog), "Save", kAskSaveChanges_Save);
	
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
{//
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
	MWindow*		inParentWindow)
{
	GtkWidget *dialog;
	
	dialog = gtk_file_chooser_dialog_new("Save File",
					      GTK_WINDOW(inParentWindow->GetGtkWidget()),
					      GTK_FILE_CHOOSER_ACTION_SAVE,
					      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					      GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
					      NULL);
	gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog), true);
	
//	    gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dialog), default_folder_for_saving);
	gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (dialog), inParentWindow->GetTitle().c_str());
	
	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
	{
		char* filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
		MURL url(filename);
		DoSaveAs(url);
		g_free(filename);
	}
	
	gtk_widget_destroy (dialog);
//	NavDialogCreationOptions creationOptions;
//	THROW_IF_OSERROR(::NavGetDefaultDialogCreationOptions(&creationOptions));
//	
//	creationOptions.clientName	 = MApplication::Instance().GetAppName();
//	creationOptions.saveFileName = nil;
//	creationOptions.modality	 = kWindowModalityWindowModal;
//	creationOptions.parentWindow = inParentWindow->GetSysWindow();
//
//	THROW_IF_OSERROR(::NavCreatePutFileDialog(&creationOptions,
//		inOSType, inCreator, sNavEventUPP, this, &mDialog));
//	
//	THROW_IF_OSERROR(::NavDialogRun(mDialog));
}

//pascal void	MSaverMixin::NavEvent(
//	NavEventCallbackMessage	inMessage,
//	NavCBRecPtr				inParams,
//	void*					inUserData)
//{
//	MSaverMixin* obj = static_cast<MSaverMixin*>(inUserData);
//							
//	try
//	{
//		if (inMessage == kNavCBUserAction)
//			obj->DoNavUserAction(inParams);
//		else if (inMessage == kNavCBTerminate)
//		{
//			obj->DoNavTerminate(inParams);
//			::NavDialogDispose(inParams->context);
//		}
////		else
////			obj->DoNavEventCallback(inMessage, inParams);
//	}
//	catch (...) { }
//}
//
//void MSaverMixin::DoNavUserAction(
//	NavCBRecPtr				inParams)
//{
//	switch (inParams->userAction)
//	{
//		case kNavUserActionCancel:			// User cancelled a dialog
//			mClosePending = false;			//   which always aborts any
//			mCloseOnNavTerminate = false;
//			break;							//   pending close
//	
//		case kNavUserActionSaveAs:
//		{
//			NavReplyRecord navReply;
//			::NavDialogGetReply(inParams->context, &navReply);
//			
//			FSRef parentFSRef;
//			THROW_IF_OSERROR(::AEGetNthPtr(&navReply.selection,
//				1, typeFSRef, nil, nil, &parentFSRef, sizeof(parentFSRef), nil));
//			
//			string name;
//			MCFString(navReply.saveFileName, true).GetString(name);	
//		
//			::NavDisposeReply(&navReply);
//			
//			MPath parentPath;
//			THROW_IF_OSERROR(::FSRefMakePath(parentFSRef, parentPath));
//			
//			if (DoSaveAs(parentPath / name))
//				mCloseOnNavTerminate = mClosePending;
//			break;
//		}
//	
//		case kNavUserActionSaveChanges:				// Save before closing
//			mCloseOnNavTerminate = SaveDocument();	// Save was successful
//			mClosePending = true;			// Save operation is waiting
//											//   for user to specify the
//											//   file. We will close the
//											//   document if the save
//											//   completes.
//			break;
//			
//		case kNavUserActionDontSaveChanges:	// Close document without saving
//			mCloseOnNavTerminate = true;
//			break;
//			
//		case kNavUserActionDiscardChanges:	// User confirmed revert operation
//			RevertDocument();
//			mCloseOnNavTerminate = false;
//			mClosePending = false;
//			break;
//	}
//}
//
//void MSaverMixin::DoNavTerminate(
//	NavCBRecPtr				inParams)
//{
//	mDialog = nil;
//
//	if (mCloseOnNavTerminate)
//		CloseAfterNav();
//}

bool MSaverMixin::OnClose()
{
	cout << "Close Save Dialog" << endl;

	mDialog = nil;

	if (mCloseOnNavTerminate)
		CloseAfterNavigationDialog();
	
	return true;
}
	
bool MSaverMixin::OnResponse(
	gint		inArg)
{
	switch (inArg)
	{
		case kAskSaveChanges_Save:
			mCloseOnNavTerminate = SaveDocument();	// Save was successful
			mClosePending = true;			// Save operation is waiting
											//   for user to specify the
											//   file. We will close the
											//   document if the save
											//   completes.
			break;
		
		case kAskSaveChanges_Cancel:
			mClosePending = false;			//   which always aborts any
			mCloseOnNavTerminate = false;
			break;
		
		case kAskSaveChanges_DontSave:
			mCloseOnNavTerminate = true;
			break;
	}

	gtk_widget_destroy(mDialog);
	mDialog = nil;
	
	return true;	
}
