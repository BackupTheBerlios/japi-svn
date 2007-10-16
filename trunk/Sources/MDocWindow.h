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

#ifndef MDOCWINDOW_H
#define MDOCWINDOW_H

#include "MWindow.h"
#include "MSelection.h"
#include "MDocument.h"
#include "MController.h"
#include "MMenu.h"

class MParsePopup;

class MDocWindow : public MWindow
{
  public:
						MDocWindow();

	static MDocWindow*	DisplayDocument(
							MDocument*		inDocument);

	static MDocWindow*	FindWindowForDocument(
							MDocument*		inDocument);
	
	static MDocWindow*	GetFirstDocWindow()	{ return sFirst; }

	MEventIn<void(bool)>					eModifiedChanged;
	MEventIn<void(const MPath&)>				eFileSpecChanged;
	MEventIn<void(MSelection,std::string)>	eSelectionChanged;
	MEventIn<void(bool)>					eShellStatus;
	MEventIn<void(MDocument*)>				eDocumentChanged;

	MDocument*		GetDocument();

  protected:

	static std::string	GetUntitledTitle();

	virtual void		Initialize(
							MDocument*		inDocument);

	virtual bool		UpdateCommandStatus(
							uint32			inCommand,
							bool&			outEnabled,
							bool&			outChecked);

	virtual void		DocumentChanged(
							MDocument*		inDocument);

	virtual bool		DoClose();

	virtual bool		ProcessCommand(
							uint32			inCommand);

	virtual void		ModifiedChanged(
							bool			inModified);

	virtual void		FileSpecChanged(
							const MPath&		inFile);
	
	void				SelectionChanged(
							MSelection		inNewSelection,
							std::string		inRangeName);

	void				ShellStatus(
							bool			inActive);
	
//	OSStatus			DoParsePaneClick(
//							EventRef		ioEvent);

  protected:

	MController			mController;
	MTextView*			mTextView;
	GtkWidget*			mVBox;
	GtkWidget*			mStatusbar;
	MMenubar			mMenubar;
	MParsePopup*		mParsePopup;
	MParsePopup*		mIncludePopup;

	virtual				~MDocWindow();

  private:

	static MDocWindow*	sFirst;
	MDocWindow*			mNext;
};

#endif
