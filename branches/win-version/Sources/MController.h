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
#include "MP2PEvents.h"

class MDocument;
class MWindow;
class MFile;

class MController : public MHandler
{
  public:
						MController(
							MWindow*		inWindow);

						~MController();

	void				SetDocument(
							MDocument*		inDocument);

	MDocument*			GetDocument() const				{ return mDocument; }
	
	MWindow*			GetWindow() const				{ return mWindow; }
	
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

	virtual bool		HandleKeydown(
							uint32			inKeyCode,
							uint32			inModifiers,
							const std::string&
											inText);

	MEventOut<void(MDocument*)>		eDocumentChanged;

  protected:

	virtual void		Print();

						MController(const MController&);
	MController&		operator=(const MController&);

	MDocument*			mDocument;
	MWindow*			mWindow;
	bool				mCloseOnNavTerminate;
};

#endif
