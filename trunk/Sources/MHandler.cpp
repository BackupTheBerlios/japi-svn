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

#include "MJapieG.h"

#include "MHandler.h"

MHandler* MHandler::sFocus = nil;

MHandler::MHandler(
	MHandler*		inSuper)
	: mSuper(inSuper)
{
}

MHandler::~MHandler()
{
	SetSuper(nil);
	ReleaseFocus();
}

bool MHandler::UpdateCommandStatus(
	uint32			inCommand,
	MMenu*			inMenu,
	uint32			inItemIndex,
	bool&			outEnabled,
	bool&			outChecked)
{
	bool result = false;
	
	if (mSuper != nil)
	{
		result = mSuper->UpdateCommandStatus(
			inCommand, inMenu, inItemIndex, outEnabled, outChecked);
	}
	else
	{
		outEnabled = false;
		outChecked = false;
	}
	
	return result;
}

bool MHandler::ProcessCommand(
	uint32			inCommand,
	const MMenu*	inMenu,
	uint32			inItemIndex)
{
	bool result = false;
	
	if (mSuper != nil)
		result = mSuper->ProcessCommand(inCommand, inMenu, inItemIndex);
	
	return result;
}

void MHandler::SetSuper(
	MHandler*		inSuper)
{
	if (mSuper != nil)
		mSuper->RemoveSubHandler(this);
	
	mSuper = inSuper;
	
	if (mSuper != nil)
		mSuper->AddSubHandler(this);
}

void MHandler::TakeFocus()
{
	SetFocus(this);
}

void MHandler::ReleaseFocus()
{
	if (sFocus == this)
		SetFocus(nil);
}

void MHandler::SetFocus(
	MHandler*		inNewFocus)
{
			// Exit if current and new target are the same
	if (inNewFocus == sFocus) {
		return;
	}

	if (inNewFocus == nil) {
			// New chain of command is empty
			// Take off duty all commanders in current chain of command
		if (sFocus != nil) {
			sFocus->DontBeFocus();
			sFocus->TakeChainOffDuty(nil);
		}

	} else if (sFocus == nil) {
			// There is no current chain of command
			// Put on duty all commanders in new chain
		inNewFocus->PutChainOnDuty(inNewFocus);

	} else {

			// Current and new targets both exist. We must determine
			// where the current and new chains of command overlap.
			// The "junction" is the deepest commander that is common to
			// both the current and new chains of command--it is a
			// Superior of both the current and new targets. Searching up
			// from the new target, the first on duty commander is the
			// junction.

		MHandler*		junction = inNewFocus;
		while (junction != nil and not junction->IsInCommandChain())
			junction = junction->mSuper;

			// Tell current target that it will no longer be the target.
			// If that results in a target change, tell the new target that
			// it will no longer be the target either.

        MHandler*		oldFocus;
        do {
            oldFocus = sFocus;
            oldFocus->DontBeFocus();
        } while ( (sFocus != nil) and
        		  (sFocus != inNewFocus) and
        		  (sFocus != oldFocus) );

			// Determine relative positions of current and new targets in
			// the command chain. There are 5 possibilities.

        if (sFocus == nil) {
                // 1. There is no current chain of command
                							// Put on duty all commanders
                                            // in new chain
            inNewFocus->PutChainOnDuty(inNewFocus);

        } else if (sFocus == inNewFocus) {
                // 2. Focus switching during some "DontBeFocus()" call
                //    brought us to the desired target
            return;

		} else if (junction == sFocus) {
				// 3. New target is a sub commander of the current target
											// Extend chain of command down to
											// the new target
			inNewFocus->PutChainOnDuty(inNewFocus);

		} else if (junction == inNewFocus) {
				// 4. New target is a super commander of the current target
											// Shorten chain of command up to
											// the new target
			sFocus->TakeChainOffDuty(inNewFocus);

		} else {
				// 5. New and current targets are on different branches
											// Take off duty current chain up
											// to, but not including, the
											// junction
			sFocus->TakeChainOffDuty(junction);
											// Put on duty new chain
			inNewFocus->PutChainOnDuty(inNewFocus);
		}
	}

	sFocus = inNewFocus;			// Finally, change to new target
}

void MHandler::AddSubHandler(
	MHandler*		inHandler)
{
}

void MHandler::RemoveSubHandler(
	MHandler*		inHandler)
{
}

void MHandler::BeFocus()
{
}

void MHandler::DontBeFocus()
{
}

void MHandler::PutOnDuty(
	MHandler*		inNewFocus)
{
}

void MHandler::TakeOffDuty()
{
}

void MHandler::PutChainOnDuty(
	MHandler*		inNewFocus)
{
	if (mOnCommandChain != eTriStateOn)
	{
		if (mSuper != nil)
			mSuper->PutChainOnDuty(inNewFocus);

		MHandlerList::iterator i;
		for (i = mSubHandlers.begin(); i != mSubHandlers.end(); ++i)
			(*i)->mOnCommandChain = eTriStateOff;

		mOnCommandChain = eTriStateOn;
		PutOnDuty(inNewFocus);
	}
}

void MHandler::TakeChainOffDuty(
	MHandler*		inUpToHndlr)
{
//	assert(mOnCommandChain == eTriStateOn);

	MHandler* handler = this;

	do
	{
		handler->mOnCommandChain = eTriStateLatent;
		handler->TakeOffDuty();

		if (handler->mSuper == inUpToHndlr)
		{
			handler->mOnCommandChain = eTriStateOff;
			break;
		}

		handler = handler->mSuper;

	}
	while (handler != nil);
}

