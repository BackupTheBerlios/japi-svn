//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

/*	$Id$
	Copyright Drs M.L. Hekkelman
	Created 28-09-07 11:20:53
*/

#ifndef MWINDOW_H
#define MWINDOW_H

#include "MView.h"
#include "MHandler.h"

#include "MCallbacks.h"
#include "MP2PEvents.h"

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
								uint32			inItemIndex,
								uint32			inModifiers);

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

	void					Init();

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
