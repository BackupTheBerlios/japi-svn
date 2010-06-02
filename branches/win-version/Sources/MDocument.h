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
#include "MFile.h"
#include "MHandler.h"

class MDocClosedNotifier;
class MController;
class MDocWindow;
class MMenu;

class MDocument : public MHandler
{
  public:
	virtual				~MDocument();

	template<class D>
	static D*			Create(
							MHandler*			inSuper,
							const MFile&		inFile)
						{
							std::unique_ptr<D> doc(new D(inSuper, inFile));
							if (inFile.IsValid())
								doc->DoLoad();
							return doc.release();
						}

	virtual bool		DoSave();

	virtual bool		DoSaveAs(
							const MFile&		inFile);

	virtual void		RevertDocument();
	
	virtual bool		IsSpecified() const					{ return mFile.IsValid(); }

	virtual void		SetFile(
							const MFile&		inNewFile);

	virtual const MFile&
						GetFile() const						{ return mFile; }

	virtual bool		UsesFile(
							const MFile&		inFile) const;
	
	static MDocument*	GetDocumentForFile(
							const MFile&		inFile);

	virtual bool		IsReadOnly() const					{ return mFile.ReadOnly(); }
	
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

	virtual bool		HandleKeydown(
							uint32			inKeyCode,
							uint32			inModifiers,
							const std::string&
											inText);

	MEventOut<void(bool)>					eModifiedChanged;
	MEventOut<void(MDocument*)>				eDocumentClosed;
	MEventOut<void(MDocument*, const MFile&)>
											eFileSpecChanged;
	MEventOut<void(const fs::path&)>		eBaseDirChanged;

  protected:

	explicit			MDocument(
							MHandler*			inSuper,
							const MFile&		inFile);

	virtual void		DoLoad();

	virtual void		CloseDocument();

	// Asynchronous IO support
	virtual void		ReadFile(
							std::istream&		inFile) = 0;

	virtual void		WriteFile(
							std::ostream&		inFile) = 0;

	virtual void		IOProgress(float inProgress, const std::string&);
	virtual void		IOError(const std::string& inError);
	virtual bool		IOAskOverwriteNewer();
	
	virtual void		IOFileLoaded();
	virtual void		IOFileWritten();

	typedef std::list<MController*>			MControllerList;
	typedef std::list<MDocClosedNotifier>	MDocClosedNotifierList;
	
	MControllerList		mControllers;
	MDocClosedNotifierList
						mNotifiers;
	MFile				mFile;
	bool				mWarnedReadOnly;
	bool				mDirty;

  private:
	MFileLoader*		mFileLoader;
	MFileSaver*			mFileSaver;

	MDocument*			mNext;
	static MDocument*	sFirst;
};

#endif
