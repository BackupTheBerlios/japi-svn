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

#ifndef MSAVERMIXIN_H
#define MSAVERMIXIN_H

#include "MUrl.h"
#include "MCallbacks.h"

class MWindow;

enum MCloseReason {
	kSaveChangesClosingDocument,
	kSaveChangesClosingAllDocuments,
	kSaveChangesQuittingApplication
};

class MSaverMixin
{
  public:
						MSaverMixin();
	virtual				~MSaverMixin();
	
	static bool			IsNavDialogVisible();

	virtual void		TryCloseDocument(
							MCloseReason		inAction,
							const std::string&	inDocument,
							MWindow*			inParentWindow);
	
	virtual void		TryDiscardChanges(
							const std::string&	inDocument,
							MWindow*			inParentWindow);

	virtual void		SaveDocumentAs(
							MWindow*			inParentWindow,
							const std::string&	inSuggestedName);

  protected:

	virtual bool		SaveDocument() = 0;
	virtual void		RevertDocument() = 0;
	virtual bool		DoSaveAs(
							const MUrl&			inPath) = 0;
	virtual void		CloseAfterNavigationDialog() = 0;

	virtual bool		OnClose();

	virtual bool		OnSaveResponse(
							gint				inArg);

	virtual bool		OnDiscardResponse(
							gint				inArg);

	MSlot<bool()>		slClose;
	MSlot<bool(gint)>	slSaveResponse;
	MSlot<bool(gint)>	slDiscardResponse;

	static MSaverMixin*	sFirst;
	MSaverMixin*		mNext;
	bool				mCloseOnNavTerminate;
	bool				mClosePending;
	bool				mCloseAllPending;
	bool				mQuitPending;
	GtkWidget*			mDialog;
};

#endif
