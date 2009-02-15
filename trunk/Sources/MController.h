//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

/*
	Model = MDocument
	View = MTextView
	Controller = MController

*/


#ifndef MCONTROLLER_H
#define MCONTROLLER_H

#include "MHandler.h"
#include "MSaverMixin.h"
#include "MP2PEvents.h"

class MDocument;
class MWindow;
class MDocWindow;
class MFile;

class MController : public MHandler, public MSaverMixin
{
  public:
						MController(
							MHandler*		inSuper);

						~MController();

	void				SetWindow(
							MDocWindow*		inWindow);

	void				SetDocument(
							MDocument*		inDocument);

	MDocument*			GetDocument() const				{ return mDocument; }
	
	MDocWindow*			GetWindow() const				{ return mDocWindow; }
	
	bool				SaveDocument();

	void				RevertDocument();

	bool				DoSaveAs(
							const MFile&	inURL);

	void				CloseAfterNavigationDialog();

	void				SaveDocumentAs();

	virtual bool		TryCloseDocument(
							MCloseReason	inAction);

	virtual bool		TryCloseController(
							MCloseReason	inAction);

	void				TryDiscardChanges();
	
	virtual bool		UpdateCommandStatus(
							uint32			inCommand,
							MMenu*			inMenu,
							uint32			inItemIndex,
							bool&			outEnabled,
							bool&			outChecked);

	virtual bool		ProcessCommand(
							uint32			inCommand,
							const MMenu*	inMenu,
							uint32			inItemIndex,
							uint32			inModifiers);

	MEventOut<void(MDocument*)>		eDocumentChanged;

  protected:

	virtual void		Print();

						MController(const MController&);
	MController&		operator=(const MController&);

	MDocument*			mDocument;
	MDocWindow*			mDocWindow;
	bool				mCloseOnNavTerminate;
};

#endif
