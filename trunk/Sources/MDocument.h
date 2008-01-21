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
	
	MDocument is the model in the Model-View-Controller triad
	
*/

#ifndef MDOCUMENT_H
#define MDOCUMENT_H

#include "MP2PEvents.h"
#include "MUrl.h"

class MDocClosedNotifier;

class MDocument
{
  public:
	explicit			MDocument(
							const MUrl*			inURL);

	virtual				~MDocument();

	virtual void		SetFileNameHint(
							const std::string&	inName);

	virtual bool		DoSave();

	virtual bool		DoSaveAs(
							const MUrl&			inFile);

	virtual void		RevertDocument();

	bool				IsSpecified() const					{ return mSpecified; }

	MUrl				GetURL() const						{ return mURL; }

	bool				UsesFile(
							const MUrl&			inFileRef) const;
	
	static MDocument*	GetDocumentForURL(
							const MUrl&			inURL);

	bool				IsReadOnly() const					{ return mReadOnly; }
	
	virtual void		AddNotifier(
							MDocClosedNotifier&	inNotifier,
							bool				inRead);
	
	// the MVC interface

	void				AddController(
							MController*		inController);

	void				RemoveController(
							MController*		inController);

	uint32				CountControllers() const			{ return mControllers.size(); }
	
	MDocWindow*			GetWindow() const;
	
	virtual MController*
						GetFirstController() const;

	static MDocument*	GetFirstDocument()					{ return sFirst; }

	MDocument*			GetNextDocument()					{ return mNext; }

	void				MakeFirstDocument();
	
	bool				IsModified() const					{ return mDirty; }

	virtual void		SetModified(
							bool				inModified);

	// note that MDocument is NOT a MHandler, but does 
	// implement the UpdateCommandStatus and ProcessCommand methods
	
	virtual bool		UpdateCommandStatus(
							uint32			inCommand,
							MMenu*			inMenu,
							uint32			inItemIndex,
							bool&			outEnabled,
							bool&			outChecked);

	virtual bool		ProcessCommand(
							uint32			inCommand,
							const MMenu*	inMenu,
							uint32			inItemIndex);

	MEventOut<void(bool)>				eModifiedChanged;
	MEventOut<void()>					eDocumentClosed;
	MEventOut<void(const MUrl&)>		eFileSpecChanged;
	MEventOut<void(const MUrl&)>		eBaseDirChanged;

  protected:

						MDocument();

	virtual void		CloseDocument();

	virtual void		ReadFile(
							std::istream&		inFile) = 0;

	virtual void		WriteFile(
							std::ostream&		inFile) = 0;

	virtual void		SaveState();
	
	typedef std::list<MController*>			MControllerList;
	typedef std::list<MDocClosedNotifier>	MDocClosedNotifierList;
	
	MControllerList		mControllers;
	MDocClosedNotifierList
						mNotifiers;
	MUrl				mURL;
	double				mFileModDate;
	bool				mSpecified;
	bool				mReadOnly;
	bool				mWarnedReadOnly;
	bool				mDirty;

  private:
	MDocument*			mNext;
	static MDocument*	sFirst;
};

#endif
