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

/*	$Id$
	Copyright Drs M.L. Hekkelman
	Created 28-09-07 11:20:53
*/

#ifndef MWINDOW_H
#define MWINDOW_H

#include "MView.h"
#include "MCallbacks.h"
#include <glade/glade-xml.h>

namespace boost {
class thread;
}

class MWindow : public MView, public MHandler
{
  public:
							MWindow();

	virtual					~MWindow();
	
	void					Show();
	void					Hide();
	virtual void			Select();
	
	void					Close();
	
	void					Beep();
	
	static MWindow*			GetFirstWindow()				{ return sFirst; }
	MWindow*				GetNextWindow() const			{ return mNext; }
		
	void					SetTitle(
								const std::string&	inTitle);
	std::string				GetTitle() const;
	
	void					SetModifiedMarkInTitle(
								bool			inModified);

	virtual bool			UpdateCommandStatus(
								uint32			inCommand,
								MMenu*			inMenu,
								uint32			inItemIndex,
								bool&			outEnabled,
								bool&			outChecked);

	virtual bool			ProcessCommand(
								uint32			inCommand,
								const MMenu*	inMenu,
								uint32			inItemIndex);

	MEventOut<void(MWindow*)>		eWindowClosed;

	void					GetWindowPosition(
								MRect&			outPosition);

	void					SetWindowPosition(
								const MRect&	outPosition,
								bool			inTransition = false);

	void					GetMaxPosition(
								MRect&			outRect) const;

  protected:

							MWindow(
								GtkWidget*		inWindow);
	
							MWindow(
								const char*		inWindowResourceName,
								const char*		inRootWidgetName = "window");

	virtual bool			DoClose();

	virtual bool			OnDestroy();

	virtual bool			OnDelete(
								GdkEvent*		inEvent);

	GladeXML*				GetGladeXML() const				{ return mGladeXML; }

	GtkWidget*				GetWidget(
								uint32			inID) const;

	static const char*		IDToName(
								uint32			inID,
								char			inName[5]);

	void					ConnectChildSignals();

	virtual void			PutOnDuty(
								MHandler*		inHandler);

  private:
	MSlot<bool()>			mOnDestroy;
	MSlot<bool(GdkEvent*)>	mOnDelete;

	std::string				mTitle;
	bool					mModified;
	boost::thread*			mTransitionThread;
	GladeXML*				mGladeXML;

	void					TransitionTo(
								MRect			inPosition);

	static void				RemoveWindowFromList(
								MWindow*		inWindow);
	
	void					DoForEach(
								GtkWidget*		inWidget);

	static void				DoForEachCallBack(
								GtkWidget*		inWidget,
								gpointer		inUserData);

	bool					ChildFocus(
								GdkEventFocus*	inEvent);
	
	MSlot<bool(GdkEventFocus*)>					mChildFocus;

	virtual void			FocusChanged(
								uint32			inFocussedID);

	static MWindow*			sFirst;
	MWindow*				mNext;
};


#endif // MWINDOW_H
