//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef MDOCWINDOW_H
#define MDOCWINDOW_H

#include "MWindow.h"
#include "MDocument.h"
#include "MController.h"
#include "MMenu.h"

class MDocWindow : public MWindow
{
  public:
						MDocWindow(
							const std::string& inTitle,
							const MRect& inBounds, MWindowFlags inFlags,
							const std::string& inMenu);

	static MDocWindow*	FindWindowForDocument(
							MDocument*		inDocument);
	
	MEventIn<void(bool)>					eModifiedChanged;
	MEventIn<void(MDocument*, const MFile&)>	eFileSpecChanged;
	MEventIn<void(MDocument*)>				eDocumentChanged;

	MDocument*			GetDocument();

						// document is about to be closed
	virtual void		SaveState();

	virtual void		AddRoutes(
							MDocument*		inDocument);
	
	virtual void		RemoveRoutes(
							MDocument*		inDocument);

	virtual void		BeFocus();

  protected:

	static std::string	GetUntitledTitle();

	virtual void		Initialize(
							MDocument*		inDocument);

	virtual void		DocumentChanged(
							MDocument*		inDocument);

	virtual bool		DoClose();

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

	virtual void		ModifiedChanged(
							bool			inModified);

	virtual void		FileSpecChanged(
							MDocument*		inDocument,
							const MFile&		inFile);
	
  protected:

	MController*		mController;
	//MMenubar			mMenubar;

	virtual				~MDocWindow();
};

#endif
