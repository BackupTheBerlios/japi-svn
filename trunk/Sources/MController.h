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

#include <list>

class MTextView;
class MTextViewContainer;
class MDocument;
class MWindow;
class MDocWindow;

class MController : public MHandler, public MSaverMixin
{
  public:
						MController();

						~MController();

	MTextViewContainer*	GetContainer();

	MTextView*			GetTextView();

	void				SetWindow(
							MWindow*		inWindow);

	void				SetDocument(
							MDocument*		inDocument);

	MDocument*			GetDocument() const;
	
	void				AddTextView(
							MTextView*		inTextView);

	void				RemoveTextView(
							MTextView*		inTextView);

	bool				SaveDocument();

	void				RevertDocument();

	bool				DoSaveAs(
							const MPath&		inPath);

	void				CloseAfterNavigationDialog();

	void				SaveDocumentAs();

	bool				TryCloseDocument(
							MCloseReason	inAction);

	bool				TryCloseController(
							MCloseReason	inAction);

	void				TryDiscardChanges();

	virtual bool		UpdateCommandStatus(
							uint32			inCommand,
							bool&			outEnabled,
							bool&			outChecked);

	virtual bool		ProcessCommand(
							uint32			inCommand);

	bool				OpenInclude(
							std::string		inFileName);

	void				OpenCounterpart();

	MEventOut<void(MDocument*)>		eDocumentChanged;

  private:

	void				DoGoToLine();
	void				DoOpenIncludeFile();
	void				DoOpenCounterpart();
	void				DoMarkMatching();

						MController(const MController&);
	MController&		operator=(const MController&);

//	OSStatus			DoTextInputUpdateActiveInputArea(EventRef inEvent);
//	OSStatus			DoTextInputUnicodeForKeyEvent(EventRef inEvent);
//	OSStatus			DoTextInputOffsetToPos(EventRef inEvent);
////	OSStatus			DoTextInputPosToOffset(EventRef inEvent);

	typedef std::list<MTextView*>	TextViewArray;

	MDocument*			mDocument;
	MDocWindow*			mWindow;
	TextViewArray		mTextViews;
	bool				mCloseOnNavTerminate;
};

#endif
