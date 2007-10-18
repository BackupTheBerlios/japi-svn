/*
	Copyright (c) 2006, Maarten L. Hekkelman
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

/*	$Id: MClipboard.h 158 2007-05-28 10:08:18Z maarten $
	Copyright Maarten L. Hekkelman
	Created Wednesday July 21 2004 18:18:19
*/

#ifndef MCLIPBOARD_H
#define MCLIPBOARD_H

#include "MTypes.h"

const uint32
	kClipboardRingSize = 7;

class MClipboard
{
  public:

	static MClipboard&	Instance();

	bool				HasData();

	bool				IsBlock();

	void				NextInRing();

	void				PreviousInRing();

	void				GetData(
							std::string&		outText,
							bool&				outIsBlock);

	void				SetData(
							const std::string&	inText,
							bool				inBlock);

	void				AddData(
							const std::string&	inText);

  private:

						MClipboard();
	virtual				~MClipboard();

//	static pascal OSStatus		
//						ScrapPromiseKeeperCallback(ScrapRef inScrap,
//							ScrapFlavorType inFlavor, void* inUserData);
//	static ScrapPromiseKeeperUPP
//						sScrapPromiseKeeperUPP;

	struct Data
	{
						Data(
							const std::string&	inText,
							bool				inBlock);

		void			SetData(
							const std::string&	inText,
							bool				inBlock);

		void			AddData(
							const std::string&	inText);
		
		std::string		mText;
		bool			mBlock;
	};

	void				LoadClipboardIfNeeded();

	void				OnOwnerChangedEvent(
							GdkEventOwnerChange*
												inEvent);
	
	MSlot<void(GdkEventOwnerChange*)>			mOwnerChangedEvent;
	
	static void			GtkClipboardGet(
							GtkClipboard*		inClipboard,
							GtkSelectionData*	inSelectionData,
							guint				inInfo,
							gpointer			inUserDataOrOwner);
	
	static void			GtkClipboardClear(
							GtkClipboard*		inClipboard,
							gpointer			inUserDataOrOwner);
	
	Data*				mRing[kClipboardRingSize];
	uint32				mCount;
	bool				mOwnerChanged;
	GtkClipboard*		mGtkClipboard;
};

#endif // MCLIPBOARD_H
