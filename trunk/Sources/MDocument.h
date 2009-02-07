//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

/*
	
	MDocument is the model in the Model-View-Controller triad
	
*/

#ifndef MDOCUMENT_H
#define MDOCUMENT_H

#include "MP2PEvents.h"
#include "MUrl.h"

class MDocClosedNotifier;
class MController;
class MDocWindow;
class MMenu;

class MDocument
{
  public:
	explicit			MDocument(
							const MUrl*			inURL);

	explicit			MDocument(
							const fs::path&		inPath);

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

	virtual std::string	GetWindowTitle() const;

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
							uint32			inItemIndex,
							uint32			inModifiers);

	MEventOut<void(bool)>				eModifiedChanged;
	MEventOut<void(MDocument*)>			eDocumentClosed;
	MEventOut<void(MDocument*, const MUrl&)>
										eFileSpecChanged;
	MEventOut<void(const MUrl&)>		eBaseDirChanged;

  protected:

						MDocument();

	virtual void		CloseDocument();

	virtual void		ReadFile(
							std::istream&		inFile) = 0;

	virtual void		WriteFile(
							std::ostream&		inFile) = 0;

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
