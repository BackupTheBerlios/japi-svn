//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef MWINDOW_H
#define MWINDOW_H

#include "MView.h"
#include "MColor.h"
#include "MHandler.h"

#include "MP2PEvents.h"

class MWindowImpl;

enum MWindowFlags
{
	kMFixedSize				= (1 << 0),
	kMAcceptFileDrops		= (1 << 1),
	kMPostionDefault		= (1 << 2),
};

class MWindow : public MView, public MHandler
{
  public:
							MWindow(const std::string& inTitle,
								const MRect& inBounds, MWindowFlags inFlags,
								const std::string& inMenu);
	
	virtual					~MWindow();

	virtual MWindow*		GetWindow() const;

	MWindowFlags			GetFlags() const;

	virtual void			Show();
	virtual void			Select();
	
	virtual void			Activate();
	virtual void			Deactivate();

	void					Close();
	
	void					Beep();
	
	static MWindow*			GetFirstWindow()				{ return sFirst; }
	MWindow*				GetNextWindow() const			{ return mNext; }
	static void				RemoveWindowFromList(
								MWindow*		inWindow);
		
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
	
	// GtkBuilder support, views should have a name consisting of four characters
	// and so they are accessible by ID which is a uint32.
	
	//virtual void	GetSlotsForHandler(
	//					const char*			inHandler,
	//					MSignalHandlerArray&
	//										outSlots);
	
	//void			SetFocus(
	//					uint32				inID);
	//
	//void			GetText(
	//					uint32				inID,
	//					std::string&		outText) const	{ outText = GetText(inID); }

	//std::string		GetText(
	//					uint32				inID) const;

	//void			SetText(
	//					uint32				inID,
	//					const std::string&	inText);

	//void			SetPasswordField(
	//					uint32				inID,
	//					bool				isVisible);

	//int32			GetValue(
	//					uint32				inID) const;

	//void			SetValue(
	//					uint32				inID,
	//					int32				inValue);

	//// for comboboxes
	//void			GetValues(
	//					uint32				inID,
	//					std::vector<std::string>& 
	//										outValues) const;

	//void			SetValues(
	//					uint32				inID,
	//					const std::vector<std::string>&
	//										inValues);

	//void			SetColor(
	//					uint32				inID,
	//					MColor				inColor);

	//MColor			GetColor(
	//					uint32				inID) const;

	//bool			IsChecked(
	//					uint32				inID) const;

	//void			SetChecked(
	//					uint32				inID,
	//					bool				inOn);

	//bool			IsVisible(
	//					uint32				inID) const;

	//void			SetVisible(
	//					uint32				inID,
	//					bool				inVisible);

	//bool			IsEnabled(
	//					uint32				inID) const;
	//
	//void			SetEnabled(
	//					uint32				inID,
	//					bool				inEnabled);

	//bool			IsExpanded(
	//					uint32				inID) const;
	//
	//void			SetExpanded(
	//					uint32				inID,
	//					bool				inExpanded);

	//void			SetProgressFraction(
	//					uint32				inID,
	//					float				inFraction);

	//virtual void	ValueChanged(
	//					uint32				inID);

	// window recycling, called by application for now
	
	static void		RecycleWindows();

	MWindowImpl*	GetImpl() const			{ return mImpl; }

	// coordinate manipulations
	virtual void	ConvertToScreen(int32& ioX, int32& ioY) const;
	virtual void	ConvertFromScreen(int32& ioX, int32& ioY) const;

	virtual void	ScrollRect(
						MRect			inRect,
						int32			inX,
						int32			inY);

  protected:

	virtual bool			DoClose();

	//virtual bool			OnDestroy();

	//virtual bool			OnDelete(
	//							GdkEvent*		inEvent);

	//GtkWidget*				GetWidget(
	//							uint32			inID) const;

	static const char*		IDToName(
								uint32			inID,
								char			inName[5]);

	void					ConnectChildSignals();

	virtual void			PutOnDuty(
								MHandler*		inHandler);

  private:

	void					Init();

	void					TransitionTo(
								MRect			inPosition);

	//virtual void			DoForEach(
	//							GtkWidget*		inWidget);
	//
	//static void				DoForEachCallBack(
	//							GtkWidget*		inWidget,
	//							gpointer		inUserData);

	//bool					ChildFocus(
	//							GdkEventFocus*	inEvent);
	//
	//MSlot<bool(GdkEventFocus*)>					mChildFocus;

	virtual void			FocusChanged(
								uint32			inFocussedID);

	//MSlot<void()>			mChanged;
	//void					Changed();

	virtual void			ShowSelf();
	virtual void			HideSelf();

	MWindowImpl*			mImpl;
	std::string				mTitle;
	bool					mModified;

	static MWindow*			sFirst;
	MWindow*				mNext;
	
	static MWindow*			sRecycle;
};


#endif // MWINDOW_H
