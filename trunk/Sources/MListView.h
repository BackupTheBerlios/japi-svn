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

#ifndef MLISTVIEW_H
#define MLISTVIEW_H

#include <vector>
#include <string>

#include "MView.h"
#include "MP2PEvents.h"
#include "MCallbacks.h"
#include "MFile.h"

class MDevice;

const uint32
	kListItemLast = static_cast<uint32>(~0UL);

class MListView : public MView, public MHandler
{
	friend struct MListImp;
	
  public:
						MListView(
							MHandler*		inSuperHandler);

	virtual				~MListView();
	
	void				SetDrawBox(
							bool			inDrawBox);
	
	void				SetRoundedSelectionEdges(
							bool			inRoundEdges);

	typedef std::vector<std::pair<uint32,std::string> >	ColumnInfo;
	void				CreateColumns(const ColumnInfo& inColumns);
	
	MCallBack<
		void(
			MDevice&		inDevice,
			MRect			inFrame,
			uint32			inRow,
			bool			inSelected,
			const void*		inData,
			uint32			inDataSize)>		cbDrawItem;

	MCallBack<
		void(
			MRect			inFrame,
			uint32			inRow,
			int32			inX,
			int32			inY)>				cbClickItem;

	MCallBack<
		void(
			uint32			inTargetRow,
			uint32			inNewRow,
			bool			inUnder)>			cbItemDragged;

	MCallBack<
		void(
			uint32			inTargetRow,
			std::vector<MPath>
							inFiles)>			cbFilesDropped;

	MCallBack<
		void(
			uint32			inRow,
			bool&			outIsContainer)>	cbIsContainerItem;

	MCallBack<
		void(
			uint32			inRow)>				cbRowSelected;			

	MCallBack<
		void(
			uint32			inRow)>				cbRowInvoked;

	MCallBack<
		void(
			uint32			inRow)>				cbRowDeleted;

	uint32				GetCount() const;

	uint32				InsertItem(
							uint32			inRowNr,
							const void*		inData,
							uint32			inDataLength);

	void				ReplaceItem(
							uint32			inRowNr,
							const void*		inData,
							uint32			inDataLength);

	uint32				GetItem(
							uint32			inRowNr,
							void*			outData,
							uint32			inDataLength);

	void				RemoveItem(
							uint32			inRowNr);

	void				RemoveAll();
	
	void				SelectItem(
							int32			inItemNr);

	int32				GetSelected() const;

	void				ScrollToPosition(
							int32			inX,
							int32			inY);

	void				GetScrollPosition(
							int32&			outX,
							int32&			outY);
	
  private:

	void				SetAcceptDragAndDrop();

	struct MListImp*	mImpl;
};

#endif
