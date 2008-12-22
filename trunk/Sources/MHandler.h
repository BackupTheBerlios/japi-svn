//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef MHANDLER_H
#define MHANDLER_H

#include <list>

class MMenu;

class MHandler
{
  public:
						MHandler(
							MHandler*		inSuper);

	virtual				~MHandler();

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

	void				SetSuper(
							MHandler*		inSuper);

	bool				IsFocus() const					{ return sFocus == this; }
	static MHandler*	GetFocus()						{ return sFocus; }
	
	bool				IsInCommandChain() const		{ return mOnCommandChain == eTriStateLatent; }
	
	void				TakeFocus();
	void				ReleaseFocus();

  protected:

	static void			SetFocus(
							MHandler*		inHandler);

	void				AddSubHandler(
							MHandler*		inHandler);
	
	void				RemoveSubHandler(
							MHandler*		inHandler);

	virtual void		BeFocus();
	
	virtual void		DontBeFocus();

	virtual void		PutOnDuty(
							MHandler*		inNewFocus);

	virtual void		TakeOffDuty();
	
  private:

	void				PutChainOnDuty(
							MHandler*		inNewFocus);

	void				TakeChainOffDuty(
							MHandler*		inUpToHndlr);
	
	typedef std::list<MHandler*>	MHandlerList;
	
	enum TriState {
		eTriStateOn,
		eTriStateOff,
		eTriStateLatent	
	};
	
	MHandler*			mSuper;
	MHandlerList		mSubHandlers;
	TriState			mOnCommandChain;
	
	static MHandler*	sFocus;
};

#endif
