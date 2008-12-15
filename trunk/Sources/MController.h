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
							const MUrl&		inURL);

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

						MController(const MController&);
	MController&		operator=(const MController&);

	MDocument*			mDocument;
	MDocWindow*			mDocWindow;
	bool				mCloseOnNavTerminate;
};

#endif
