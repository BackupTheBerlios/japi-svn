//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef MSAVERMIXIN_H
#define MSAVERMIXIN_H

#include "MCallbacks.h"

class MFile;
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
							const MFile&		inPath) = 0;
	virtual void		CloseAfterNavigationDialog() = 0;

	virtual bool		OnClose();

	//virtual bool		OnSaveResponse(
	//						gint				inArg);

	//virtual bool		OnDiscardResponse(
	//						gint				inArg);

	//MSlot<bool()>		slClose;
	//MSlot<bool(gint)>	slSaveResponse;
	//MSlot<bool(gint)>	slDiscardResponse;

	static MSaverMixin*	sFirst;
	MSaverMixin*		mNext;
	bool				mCloseOnNavTerminate;
	bool				mClosePending;
	bool				mCloseAllPending;
	bool				mQuitPending;
	//GtkWidget*			mDialog;
};

#endif
