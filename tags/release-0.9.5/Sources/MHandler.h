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
