/*
	Copyright (c) 2007, Maarten L. Hekkelman
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

#ifndef MJG_H
#define MJG_H

#include <gtk/gtk.h>
#include <list>
#include <deque>

#include "MTypes.h"
#include "MHandler.h"
#include "MP2PEvents.h"
#include "MController.h"

#define nil NULL

extern const char kAppName[], kVersionString[];

class MWindow;
class MDocument;
class MUrl;

// ===========================================================================

class MJapieApp : public MHandler
{
  public:
						MJapieApp();
	
						~MJapieApp();
	
	void				RecycleWindow(
							MWindow*			inWindow);
	
	virtual bool		UpdateCommandStatus(
							uint32				inCommand,
							bool&				outEnabled,
							bool&				outChecked);

	virtual bool		ProcessCommand(
							uint32				inCommand,
							const MMenu*		inMenu,
							uint32				inItemIndex);

	void				UpdateWindowMenu(
							MMenu*				inMenu);

	bool				LocateSystemIncludeFile(
							const std::string&	inFileName,
							MPath&				outFile);

	MDocument*			OpenOneDocument(
							const MUrl&			inFileRef);

	MDocument*			AskOpenOneDocument();

	void				OpenProject(
							const MPath&		inPath);

	void				AddToRecentMenu(
							const MUrl&			inFileRef);

	const std::string&	GetCurrentFolder() const				{ return mCurrentFolder; }

	void				SetCurrentFolder(
							const char*			inFolder);

	GtkRecentManager*	GetRecentMgr() const					{ return mRecentMgr; }
	
	MEventOut<void(double)>					eIdle;

	void				RunEventLoop();

  private:
	typedef std::list<MWindow*>	MWindowList;

	void				DoQuit();

	void				DoNew();

	void				DoOpen();

	void				DoSaveAll();

	void				DoCloseAll(
							MCloseReason		inReason);

	void				DoOpenTemplate(
							uint32				inCommand);
	
	void				DoSelectWindowFromWindowMenu(
							uint32				inIndex);

	void				ShowWorksheet();
		
	void				Pulse();
	
	static gboolean		Timeout(
							gpointer			inData);

	static gint			Snooper(
							GtkWidget*			inGrabWidget,
							GdkEventKey*		inEvent,
							gpointer			inFuncData); 
	
	void				ProcessSocketMessages();
	
	MWindowList			mTrashCan;
	GtkRecentManager*	mRecentMgr;
	int					mSocketFD;
	bool				mReceivedFirstMsg;
	bool				mQuit;
	bool				mQuitPending;
	std::string			mCurrentFolder;
};

extern MJapieApp*	gApp;

#endif
