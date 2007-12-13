/*	$Id$
	Copyright Drs M.L. Hekkelman
	Created 28-09-07 11:20:53
*/

#ifndef MWINDOW_H
#define MWINDOW_H

#include "MView.h"
#include "MCallbacks.h"

class MWindow : public MView, public MHandler
{
  public:
							MWindow();
	virtual					~MWindow();
	
	void					Show();
	void					Hide();
	virtual void			Select();
	
	void					Close();
	
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
								const MRect&	outPosition);

  protected:

							MWindow(
								GtkWidget*		inWindow);

	virtual bool			DoClose();

	virtual bool			OnDestroy();

	virtual bool			OnDelete(
								GdkEvent*		inEvent);

  private:
	MSlot<bool()>			mOnDestroy;
	MSlot<bool(GdkEvent*)>	mOnDelete;

	std::string				mTitle;
	bool					mModified;
	
	static MWindow*			sFirst;
	MWindow*				mNext;
};


#endif // MWINDOW_H
