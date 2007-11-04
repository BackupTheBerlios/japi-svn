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

#include "MJapieG.h"

#include "MListView.h"
#include "MGlobals.h"
#include "MDevice.h"
#include "MUtils.h"
#include "MViewPort.h"
#include "MScrollBar.h"
#include "MDevice.h"

using namespace std;

const uint32
	kMyListItemDragKind	= 'lstI';

class MListItem
{
  public:
					MListItem();

					MListItem(
						const void*			inData,
						uint32				inDataLength);

					MListItem(
						const MListItem&	inOther);

	virtual			~MListItem();

	MListItem&		operator=(
						const MListItem&	inOther);
	
	void			SetData(
						const void*			inData,
						uint32				inDataLength);

	const void*		Peek() const							{ return mData->mData; }
	
	uint32			Size() const							{ return mData->mSize; }
	
	void			SetHeight(
						uint32				inHeight)		{ mData->mHeight = inHeight; }

	uint32			GetHeight() const						{ return mData->mHeight; }
	
	void			SetSelected(
						bool				inSelected)		{ mData->mSelected = inSelected; }

	bool			GetSelected() const						{ return mData->mSelected; }
	
  private:
	
	struct MListItemData
	{
		int			mRefCount;
		uint32		mSize;
		uint32		mHeight;
		bool		mSelected;
		bool		mContainer;
		bool		mExpanded;
		char		mData[1];

		void*		operator new(size_t, size_t inDataSize);
		void*		operator new(size_t);	// defined but not implented !!!
		void		operator delete(void*);

	};
	
	MListItemData*	mData;
};

void* MListItem::MListItemData::operator new(
	size_t		inItemSize,
	size_t		inDataSize)
{
	void* r = malloc(inItemSize + inDataSize);
	return r;
}

void MListItem::MListItemData::operator delete(
	void*		inPtr)
{
	free(inPtr);
}

MListItem::MListItem()
{
	mData = new(0) MListItemData;

	mData->mRefCount = 1;
	mData->mSize = 0;
	mData->mHeight = 0;
//	mData->mNestingLevel = 0;
	mData->mSelected = false;
//	mData->mExpanded = false;
}

MListItem::MListItem(
	const void*			inData,
	uint32				inDataLength)
{
	mData = new(inDataLength) MListItemData;
	
	mData->mRefCount = 1;
	mData->mSize = inDataLength;
	memcpy(mData->mData, inData, inDataLength);
	mData->mHeight = 0;
//	mData->mNestingLevel = 0;
	mData->mSelected = false;
//	mData->mExpanded = false;
}

MListItem::MListItem(
	const MListItem&	inItem)
{
	mData = inItem.mData;
	++mData->mRefCount;
}

MListItem::~MListItem()
{
	if (--mData->mRefCount == 0)
		delete mData;
}

MListItem& MListItem::operator=(
	const MListItem&	inItem)
{
	if (&inItem != this and inItem.mData != mData)
	{
		if (--mData->mRefCount == 0)
			delete mData;
		mData = inItem.mData;
		++mData->mRefCount;
	}
	
	return *this;
}

void MListItem::SetData(
	const void*			inData,
	uint32				inDataLength)
{
	auto_ptr<MListItemData> data(new(inDataLength) MListItemData);
	
	data->mRefCount = 1;
	data->mSize = inDataLength;
	memcpy(data->mData, inData, inDataLength);
	data->mHeight = mData->mHeight;
//	data->mNestingLevel = mData->mNestingLevel;
	data->mSelected = mData->mSelected;
//	data->mExpanded = mData->mExpanded;
	
	if (--mData->mRefCount == 0)
		delete mData;

	mData = data.release();
}

typedef std::vector<MListItem>	MItemList;

// the impl

struct MListImp : public MDrawingArea, public MHandler
{
						MListImp(
							MListView*		inListView,
							MScrollBar*		inVScrollBar);	

//	OSStatus			HandleDraw(EventRef ioEvent);
//	OSStatus			HandleHitTest(EventRef ioEvent);
//	OSStatus			HandleTrack(EventRef ioEvent);
//	OSStatus			HandleScrollableGetInfo(EventRef inEvent);
//	OSStatus			HandleScrollableScrollTo(EventRef inEvent);
//	OSStatus			HandleUnicodeForKeyEvent(EventRef inEvent);
//	OSStatus			HandleSetFocusPart(EventRef inEvent);

	virtual bool		OnExposeEvent(
							GdkEventExpose*	inEvent);

	virtual bool		OnFocusInEvent(
							GdkEventFocus*	inEvent);

	virtual bool		OnFocusOutEvent(
							GdkEventFocus*	inEvent);	

	virtual bool		OnButtonPressEvent(
							GdkEventButton*	inEvent);

	virtual bool		OnMotionNotifyEvent(
							GdkEventMotion*	inEvent);
	
	virtual bool		OnButtonReleaseEvent(
							GdkEventButton*	inEvent);

	virtual bool		OnScrollEvent(
							GdkEventScroll*	inEvent);
	
	void				DoScrollTo(
							int32			inX,
							int32			inY);
	
	void				GetScrollPosition(
							int32&			outX,
							int32&			outY);

	MRect				GetItemsBounds() const;

	MRect				GetItemRect(
							uint32			inItemNr) const;

	uint32				PointToItem(
							int32			inX,
							int32			inY) const;

	void				InsertItem(
							uint32			inRowNr,
							const void*		inData,
							uint32			inDataLength);

	void				ReplaceItem(
							uint32			inRowNr,
							const void*		inData,
							uint32 inDataLength);

	void				RemoveItem(
							uint32			inRowNr);

	void				RemoveAll();

	uint32				GetItem(
							uint32			inRowNr,
							void*			outData,
							uint32			inDataLength);

	void				CountChanged();
	
	void				SelectItem(
							uint32			inItemNr);

	int32				GetSelected() const;

	void				SetAcceptDragAndDrop();

//	bool				InitiateDrag(
//							int32			inX,
//							int32			inY,
//							uint32			inItemNr,
//							GdkEventButton*	inEvent);

//	OSStatus			DoDragEnter(EventRef inEvent);
//	OSStatus			DoDragWithin(EventRef inEvent);
//	OSStatus			DoDragLeave(EventRef inEvent);
//	OSStatus			DoDragReceive(EventRef inEvent);
	
	MItemList			mItems;
	MListView*			mList;
	static MListImp*	sDraggingView;
	vector<uint32>		mColumns;
	
	MScrollBar*			mVScrollBar;
	
	bool				mSupportsDragAndDrop;
	bool				mDrawForDragImage;
	bool				mDrawListBox;
	bool				mHasFocus;
	int32				mDraggedItemNr;
	int32				mDropItemNr;
	bool				mDropUnder;
	bool				mDrawRoundEdges;
	
	uint32				mItemHeight;
	uint32				mLastClickedItemNr;
	double				mLastClickTime;
};

MListImp* MListImp::sDraggingView = nil;

MListImp::MListImp(
	MListView*		inListView,
	MScrollBar*		inVScrollBar)
	: MDrawingArea(100, 100)
	, MHandler(inListView)
	, mList(inListView)
	, mVScrollBar(inVScrollBar)
	, mSupportsDragAndDrop(false)
	, mDrawForDragImage(false)
	, mDrawListBox(false)
	, mHasFocus(false)
	, mDraggedItemNr(-1)
	, mDropItemNr(-1)
	, mDropUnder(false)
	, mDrawRoundEdges(false)
{
	MDevice dev;
	mItemHeight = dev.GetLineHeight() + 2;

//	Install(kEventClassControl, kEventControlDraw,
//		this, &MListImp::HandleDraw);
//	Install(kEventClassControl, kEventControlTrack,
//		this, &MListImp::HandleTrack);
//	Install(kEventClassControl, kEventControlHitTest,
//		this, &MListImp::HandleHitTest);
//	Install(kEventClassScrollable, kEventScrollableGetInfo,
//		this, &MListImp::HandleScrollableGetInfo);
//	Install(kEventClassScrollable, kEventScrollableScrollTo,
//		this, &MListImp::HandleScrollableScrollTo);
//	
//	Install(kEventClassControl, kEventControlSetFocusPart,
//		this, &MListImp::HandleSetFocusPart);
//
//	Install(kEventClassTextInput, kEventTextInputUnicodeForKeyEvent,
//		this, &MListImp::HandleUnicodeForKeyEvent);
	
	mLastClickTime = 0;
}

void MListImp::SetAcceptDragAndDrop()
{
//	mSupportsDragAndDrop = true;
//
//	THROW_IF_OSERROR(::SetControlDragTrackingEnabled(GetSysView(), true));
//	
//	Install(kEventClassControl, kEventControlDragEnter,		this, &MListImp::DoDragEnter);
//	Install(kEventClassControl, kEventControlDragWithin,	this, &MListImp::DoDragWithin);
//	Install(kEventClassControl, kEventControlDragLeave,		this, &MListImp::DoDragLeave);
//	Install(kEventClassControl, kEventControlDragReceive,	this, &MListImp::DoDragReceive);
}

//bool MListImp::InitiateDrag(
//	int32			inX,
//	int32			inY,
//	uint32			inItemNr,
//	GdkEventButton*	inEvent)
//{
//	bool result = false;
//	
////	assert(inItemNr < mItems.size());
////	
////	ConvertToGlobal(inWhere);
////
////	Point pt;
////	pt.h = static_cast<int16>(inWhere.x);
////	pt.v = static_cast<int16>(inWhere.y);
////
////	if (mSupportsDragAndDrop and ::WaitMouseMoved(pt))
////	{
////		CGImageRef imageRef;
////		CGRect bounds;
////		
////		mDrawForDragImage = true;
////		::HIViewCreateOffscreenImage(GetSysView(), 0, &bounds, &imageRef);
////		mDrawForDragImage = false;
////
////		MCFRef<CGImageRef> image(imageRef, false);
////		
////		HIRect itemBounds = GetItemRect(inItemNr);
////		MCFRef<CGImageRef> subImage(::CGImageCreateWithImageInRect(image, itemBounds), false);
////		
////		HIPoint offset;
////		offset.x = -inWhere.x;
////		offset.y = -inWhere.y + itemBounds.origin.y;
////		ConvertToGlobal(offset);
////
////		try
////		{
////			DragRef dragRef;
////			::NewDrag(&dragRef);
////			
////			EventRecord er;
////			::ConvertEventRefToEventRecord(inEvent, &er);
////			
////			THROW_IF_OSERROR(::SetDragImageWithCGImage(dragRef,
////				subImage, &offset, 0));
////			
////			THROW_IF_OSERROR(::AddDragItemFlavor(
////				dragRef, 1, kMyListItemDragKind, &inItemNr, sizeof(uint32), 0));
////			
////			sDraggingView = this;
////			mDraggedItemNr = inItemNr;
////			mDropItemNr = -1;
////			mDropUnder = false;
////			
////			result = true;
////			
////			RgnHandle rgn = ::NewRgn();
////			(void)::TrackDrag(dragRef, &er, rgn);
////			::DisposeRgn(rgn);
////			
////			::SetThemeCursor(kThemeArrowCursor);
////		}
////		catch (std::exception& e)
////		{
////			MError::DisplayError(e);
////		}
////		
////		sDraggingView = nil;
////		mDraggedItemNr = -1;
////	}
//	
//	return result;
//}

void MListImp::InsertItem(
	uint32			inRowNr,
	const void*		inData,
	uint32			inDataLength)
{
	MListItem item(inData, inDataLength);
	item.SetHeight(mItemHeight);
	item.SetSelected(false);
	
	if (inRowNr < mItems.size())
		mItems.insert(mItems.begin() + inRowNr, item);
	else
		mItems.insert(mItems.end(), item);

	Invalidate();
	
	CountChanged();
}

void MListImp::ReplaceItem(
	uint32			inRowNr,
	const void*		inData,
	uint32			inDataLength)
{
	mItems[inRowNr].SetData(inData, inDataLength);
	Invalidate();
}

void MListImp::RemoveItem(
	uint32			inRowNr)
{
	mItems.erase(mItems.begin() + inRowNr);

	CountChanged();
	Invalidate();
}

uint32 MListImp::GetItem(
	uint32			inRowNr,
	void*			outData,
	uint32			inDataLength)
{
	if (inRowNr >= mItems.size())
		THROW(("item is out of range"));
	
	uint32 result = mItems[inRowNr].Size();
	
	if (outData != nil)
	{
		if (inDataLength < result)
			result = inDataLength;
		
		memcpy(outData, mItems[inRowNr].Peek(), result);
	}
	
	return result;
}

void MListImp::RemoveAll()
{
	mItems.clear();
	Invalidate();

	CountChanged();
}

void MListImp::CountChanged()
{
	uint32 count = mItems.size();
	uint32 height = count * mItemHeight;
	ResizeTo(100, height);
}

MRect MListImp::GetItemsBounds() const
{
	MRect result;
	GetBounds(result);
//	if (mDrawListBox)
//		result = ::CGRectInset(result, 3, 3);
	return result;
}

MRect MListImp::GetItemRect(
	uint32			inItemNr) const
{
	assert(inItemNr <= mItems.size());
	
	MRect result = GetItemsBounds();
	
	result.y = 0;
	
	for (uint32 ix = 0; ix < inItemNr; ++ix)
		result.y += mItems[ix].GetHeight();
	
	if (inItemNr < mItems.size())
		result.height = mItems[inItemNr].GetHeight();
	else
		result.height = mItemHeight;
	
	return result;
}

uint32 MListImp::PointToItem(
	int32			inX,
	int32			inY) const
{
	uint32 itemNr = 0;
	
	MRect r = GetItemsBounds();
	r.y = 0;
	r.height = 0;
	
	while (itemNr < mItems.size())
	{
		r.y += r.height;
		r.height = mItems[itemNr].GetHeight();
		
		if (r.ContainsPoint(inX, inY))
			break;
		
		++itemNr;
	}
	
	return itemNr;
}

void MListImp::SelectItem(
	uint32			inItemNr)
{
	for (uint32 ix = 0; ix < mItems.size(); ++ix)
		mItems[ix].SetSelected(ix == inItemNr);

	Invalidate();
}

int32 MListImp::GetSelected() const
{
	int32 result = -1;
	
	for (uint32 ix = 0; ix < mItems.size(); ++ix)
	{
		if (mItems[ix].GetSelected())
		{
			result = ix;
			break;
		}
	}
	
	return result;
}

bool MListImp::OnExposeEvent(
	GdkEventExpose*		inEvent)
{
	MRect bounds;
	GetBounds(bounds);

	MDevice dev(this, bounds);

//	if (mDrawListBox)
//	{
//		bounds = ::CGRectInset(bounds, 3, 3);
//		dev.DrawListBoxFrame(bounds, mHasFocus);
//	}

	dev.SetForeColor(kBlack);
	dev.SetBackColor(kWhite);
	dev.EraseRect(bounds);
	
	for (uint32 ix = 0; ix < mItems.size(); ++ix)
	{
		MRect r = GetItemRect(ix);
		
		if (r.y + r.height < bounds.y)
			continue;
		
		// keep on drawing...
		if (r.y - r.height > bounds.y + bounds.height)
			break;
		
		MListItem& item = mItems[ix];
		
		bool selected = item.GetSelected() or (mDropUnder and static_cast<int32>(ix) == mDropItemNr);
		
		dev.DrawListItemBackground(r, selected, IsActive(), ix & 1, mDrawRoundEdges);

		mList->cbDrawItem(dev, r, ix, selected, item.Peek(), item.Size());
		
		if (static_cast<int32>(ix) == mDropItemNr and mDropItemNr != mDraggedItemNr and not mDropUnder)
		{
			dev.SetForeColor(kBlack);
			
			r.height = 1;
			dev.FillRect(r);
		}		
	}

	if (static_cast<uint32>(mDropItemNr) == mItems.size())
	{
		dev.SetForeColor(kBlack);
		
		MRect r = GetItemRect(mDropItemNr);
		
		r.height = 1;
		dev.FillRect(r);
	}
	
	return true;
}

bool MListImp::OnFocusInEvent(
	GdkEventFocus*	inEvent)
{
	TakeFocus();
	Invalidate();
	return true;
}

bool MListImp::OnFocusOutEvent(
	GdkEventFocus*	inEvent)
{
	ReleaseFocus();
	Invalidate();
	return true;
}
	
bool MListImp::OnButtonPressEvent(
	GdkEventButton*	inEvent)
{
	gtk_widget_grab_focus(GetGtkWidget());
	
	int32 itemNr = PointToItem(inEvent->x, inEvent->y);
	
	if (itemNr >= 0 and itemNr < mItems.size())
	{
		SelectItem(itemNr);
		
		if (inEvent->type == GDK_2BUTTON_PRESS)
			mList->cbRowInvoked(itemNr);
		else
			mList->cbRowSelected(itemNr);
	
		MRect r = GetItemRect(itemNr);
		mList->cbClickItem(r, itemNr, inEvent->x, inEvent->y);
	}
	
	return true;
}

bool MListImp::OnMotionNotifyEvent(
	GdkEventMotion*	inEvent)
{
	return true;
}

bool MListImp::OnButtonReleaseEvent(
	GdkEventButton*	inEvent)
{
	return true;
}

//OSStatus MListImp::HandleTrack(EventRef ioEvent)
//{
//	HIPoint where;
//	::GetEventParameter(ioEvent, kEventParamMouseLocation,
//		typeHIPoint, nil, sizeof(HIPoint), nil, &where);
//	
//	uint32 itemNr = PointToItem(where);
//	if (itemNr < mItems.size())
//	{
//		uint32 clickCount = 1;
//		
//		if (itemNr == mLastClickedItemNr and mLastClickTime + GetDoubleClickTime() > GetLocalTime())
//			++clickCount;
//		
//		SelectItem(itemNr);
//
//		if (mSupportsDragAndDrop and InitiateDrag(where, itemNr, ioEvent))
//			return noErr;
//
//		if (clickCount == 2)
//			mList->cbRowInvoked(itemNr);
//		else
//			mList->cbRowSelected(itemNr);
//
//		HIRect r = GetItemRect(itemNr);
//		
//		mList->cbClickItem(r, itemNr, where);
//	}
//
//	mLastClickTime = GetLocalTime();
//	mLastClickedItemNr = itemNr;
//	
//	return noErr;
//}
//
//OSStatus MListImp::HandleScrollableGetInfo(EventRef ioEvent)
//{
//	HIRect bounds = GetItemsBounds();
//	
//	HIRect r = bounds;
//	if (mItems.size() > 0)
//	{
//		r = GetItemRect(mItems.size() - 1);
//		r.size.height += r.origin.y + mOrigin.y;
////		r.origin.y = 0;
//	}
//
//	HISize		imageSize = { 0 };
//	HISize		viewSize = { 0 };
//	HISize		lineSize = { 0 };
//	HIPoint		origin = { 0 };
//
//	lineSize.height = mItemHeight;
//	imageSize.height = r.size.height;
//	viewSize.height = bounds.size.height;
//
//	origin = mOrigin;
//	
//	::SetEventParameter(ioEvent, kEventParamImageSize, typeHISize, sizeof(HISize), &imageSize);
//	::SetEventParameter(ioEvent, kEventParamViewSize, typeHISize, sizeof(HISize), &viewSize);
//	::SetEventParameter(ioEvent, kEventParamLineSize, typeHISize, sizeof(HISize), &lineSize);
//	::SetEventParameter(ioEvent, kEventParamOrigin, typeHIPoint, sizeof(HIPoint), &origin);
//	
//	return noErr;
//}
//
//OSStatus MListImp::HandleScrollableScrollTo(EventRef ioEvent)
//{
//	HIPoint location;
//	::GetEventParameter(ioEvent, kEventParamOrigin, typeHIPoint, nil,
//		sizeof(location), nil, &location);
//	
//	DoScrollTo(location);
//	
//	return noErr;
//}

bool MListImp::OnScrollEvent(
	GdkEventScroll*		inEvent)
{
	if (mItems.size() == 0)
		return true;
	
	int32 x, y;
	GetScrollPosition(x, y);
	
	MRect bounds;
	GetBounds(bounds);
	
	MRect lastItemBounds = GetItemRect(mItems.size() - 1);
	
	switch (inEvent->direction)
	{
		case GDK_SCROLL_UP:
			if (y > mItemHeight)
				DoScrollTo(x, y - mItemHeight);
			else if (y > 0)
				DoScrollTo(x, 0);
			break; 

		case GDK_SCROLL_DOWN:
			if (y + bounds.height + mItemHeight < lastItemBounds.y + lastItemBounds.height)
				DoScrollTo(x, y + mItemHeight);
			break; 

		case GDK_SCROLL_LEFT:
			break; 

		case GDK_SCROLL_RIGHT:
			break; 

	}	
	
	return true;
}

void MListImp::DoScrollTo(
	int32		inX,
	int32		inY)
{
	mVScrollBar->SetValue(inY);
}

void MListImp::GetScrollPosition(
	int32&		outX,
	int32&		outY)
{
	MRect bounds;
	GetBounds(bounds);	
	
	outX = bounds.x;
	outY = bounds.y;
}

//OSStatus MListImp::HandleUnicodeForKeyEvent(EventRef inEvent)
//{
//	UInt32 dataSize; 
//	auto_array<UniChar> buffer;
//	
//	OSStatus result = ::GetEventParameter(inEvent, kEventParamTextInputSendText,
//		typeUnicodeText, nil, 0, &dataSize, nil);
//	
//	if (result == noErr and dataSize > 0) 
//	{ 
//		buffer.reset(new UniChar[dataSize / sizeof(UniChar)]);
//		
//		result = ::GetEventParameter(inEvent, kEventParamTextInputSendText,
//			typeUnicodeText, nil, dataSize, nil, buffer.get());
//	}
//	
////	UniChar* chars = buffer.get();
////	uint32 charCount = dataSize / sizeof(UniChar);
//	
//	THROW_IF_OSERROR(result); 
//	
//	EventRef origEvent;
//	if (::GetEventParameter(inEvent, kEventParamTextInputSendKeyboardEvent,
//	    typeEventRef, nil, sizeof(EventRef), nil, &origEvent) == noErr)
//	{
//	    uint32 modifiers, keyCode;
//
//	    ::GetEventParameter(origEvent, kEventParamKeyModifiers,
//	        typeUInt32, nil, sizeof(uint32), nil, &modifiers);
//	    ::GetEventParameter(origEvent, kEventParamKeyCode,
//			typeUInt32, nil, sizeof(uint32), nil, &keyCode);
//
//		UInt32 state = 0;    // we don't want to save the state
//		void* transData = (void*)::GetScriptManagerVariable(smKCHRCache);
//		uint32 charCode = static_cast<uint32>(
//			toupper(static_cast<int>(::KeyTranslate(transData,
//			static_cast<uint32>(keyCode & 0x000000FF), &state))));
//
//		uint32 selected = GetSelected();
//
//		switch (charCode)
//		{
//			case kHomeCharCode:
//			{
//				HIPoint origin = { 0, 0 };
//				DoScrollTo(origin);
//				break;
//			}
//
//			case kEndCharCode:
//				break;
//				
//			case kPageUpCharCode:
//				break;
//				
//			case kPageDownCharCode:
//				break;
//				
//			case kUpArrowCharCode:
//				if (selected > 0)
//				{
//					SelectItem(selected - 1);
//					mList->cbRowSelected(selected - 1);
//				}
//				break;
//
//			case kDownArrowCharCode:
//				if (selected + 1 < mItems.size())
//				{
//					SelectItem(selected + 1);
//					mList->cbRowSelected(selected + 1);
//				}
//				break;
//			
//			case kReturnCharCode:
//			case kEnterCharCode:
//				mList->cbRowInvoked(selected);
//				break;
//			
//			case kBackspaceCharCode:
//			case kDeleteCharCode:
//				mList->cbRowDeleted(selected);
//				break;
//			
//			default:
//				result = eventNotHandledErr;
//				break;
//		}
//	}
//	
//	return result;
//}
//
//OSStatus MListImp::DoDragEnter(EventRef inEvent)
//{
//	OSStatus err = eventNotHandledErr;
//	
//	try
//	{
//		DragRef dragRef;
//		THROW_IF_OSERROR(::GetEventParameter(inEvent, kEventParamDragRef, typeDragRef, nil,
//			sizeof(dragRef), nil, &dragRef));
//		
//		UInt16 itemCount;
//		THROW_IF_OSERROR(::CountDragItems(dragRef, &itemCount));
//		
//		Boolean accept = false;
//		
//		for (UInt16 ix = 1; ix <= itemCount and accept == false; ++ix)
//		{
//			ItemReference itemRef;
//			THROW_IF_OSERROR(::GetDragItemReferenceNumber(dragRef, ix, &itemRef));
//			
//			UInt16 flavorCount;
//			if (::CountDragItemFlavors(dragRef, itemRef, &flavorCount) != noErr)
//				return false;
//		
//			for (UInt16 index = 1; index <= flavorCount and accept == false; ++index)
//			{
//				FlavorType type;
//				if (::GetFlavorType(dragRef, itemRef, index, &type) != noErr)
//					continue;
//		
//				accept = type == kMyListItemDragKind or type == flavorTypeHFS;
//			}
//		}
//		
//		if (accept)
//		{
//			if (sDraggingView == this)
//				::SetThemeCursor(kThemeClosedHandCursor);
//
//			err = noErr;
//
//			THROW_IF_OSERROR(::SetEventParameter(inEvent, kEventParamControlWouldAcceptDrop,
//				typeBoolean, sizeof(accept), &accept));
//		}
//	}
//	catch (...)
//	{
//		err = eventNotHandledErr;
//	}
//	
//	return err;
//}
//
//OSStatus MListImp::DoDragWithin(EventRef inEvent)
//{
//	try
//	{
//		DragRef dragRef;
//		THROW_IF_OSERROR(::GetEventParameter(inEvent, kEventParamDragRef, typeDragRef, nil,
//			sizeof(dragRef), nil, &dragRef));
//		
//		Point where, dummy;
//		THROW_IF_OSERROR(::GetDragMouse(dragRef, &where, &dummy));
//		
//		HIPoint pt = { where.h, where.v };
//		ConvertFromGlobal(pt);
//		bool isContainer = false;
//		
//		uint32 itemNr = PointToItem(pt);
//		
//		if (itemNr != static_cast<uint32>(mDraggedItemNr) and
//			itemNr < mItems.size())
//		{
//			mList->cbIsContainerItem(itemNr, isContainer);
//			
//			if (not isContainer)
//			{
//				HIRect r = GetItemRect(itemNr);
//				if ((where.v - r.origin.y) > (r.size.height / 2))
//					++itemNr;
//			}
//		}
//
//		if (static_cast<int32>(itemNr) != mDropItemNr or
//			isContainer != mDropUnder)
//		{
//			Invalidate();
//			mDropItemNr = itemNr;
//			mDropUnder = isContainer;
//
////			double timeOut = 0.01;
////			
////			while (ScrollToSelection())
////			{
////				MouseTrackingResult flags;
////				OSStatus err = ::TrackMouseLocationWithOptions(
////					::GetWindowPort(GetSysWindow()), 0, timeOut, &where, nil, &flags);
////				
////				if (err != noErr and err != kMouseTrackingTimedOut)
////					THROW_IF_OSERROR(err);
////				
////				pt.x = where.h;
////				pt.y = where.v;
////				ConvertFromRoot(pt);
////				
////				if (not ::CGRectContainsPoint(bounds, pt))
////					break;
////				
////				pt.x += mImageOrigin.x - kLeftMargin;
////				pt.y += mImageOrigin.y;
////	
////				mDocument->PositionToOffset(pt, caret);
////
////				if (mDragCaret != caret)
////				{
////					InvalidateLine(mDocument->OffsetToLine(mDragCaret));
////					mDragCaret = caret;
////					InvalidateLine(mDocument->OffsetToLine(mDragCaret));
////				}
////		
////				if (flags == kMouseTrackingMouseUp)
////					break;
////			}
//		}
//	}
//	catch (...) {}
//	return noErr;
//}
//
//OSStatus MListImp::DoDragLeave(EventRef inEvent)
//{
//	mDropItemNr = -1;
//	mDropUnder = false;
//	
//	Invalidate();
//	
//	return noErr;
//}
//
//OSStatus MListImp::DoDragReceive(EventRef inEvent)
//{
//	if (mDropItemNr != -1)
//	{
//		if (mDropItemNr != mDraggedItemNr and sDraggingView == this)
//			mList->cbItemDragged(mDraggedItemNr, mDropItemNr, mDropUnder);
//		else
//		{
//			DragRef dragRef;
//			THROW_IF_OSERROR(::GetEventParameter(inEvent, kEventParamDragRef, typeDragRef, nil,
//				sizeof(dragRef), nil, &dragRef));
//			
//			UInt16 itemCount;
//			THROW_IF_OSERROR(::CountDragItems(dragRef, &itemCount));
//			
//			vector<MPath> files;
//			
//			for (UInt16 ix = 1; ix <= itemCount; ++ix)
//			{
//				ItemReference itemRef;
//				if (::GetDragItemReferenceNumber(dragRef, ix, &itemRef) != noErr)
//					continue;
//				
//				UInt16 flavorCount;
//				if (::CountDragItemFlavors(dragRef, itemRef, &flavorCount) != noErr)
//					continue;
//				
//				for (UInt16 index = 1; index <= flavorCount; ++index)
//				{
//					FlavorType type;
//					if (::GetFlavorType(dragRef, itemRef, index, &type) != noErr)
//						continue;
//			
//					if (type == flavorTypeHFS)
//					{
//						HFSFlavor hfs;
//						long size = sizeof(hfs);
//						
//						THROW_IF_OSERROR(::GetFlavorData(dragRef, itemRef, flavorTypeHFS, &hfs, &size, 0));
//						
//						MPath file;
//						THROW_IF_OSERROR(::FSSpecMakePath(hfs.fileSpec, file));
//						files.push_back(file);
//					}
//				}
//			}
//			
//			mList->cbFilesDropped(mDropItemNr, files);
//		}
//	}
//	
//	::SetThemeCursor(kThemeArrowCursor);
//	
//	return noErr;
//}

//// the list header
//
//class MListHeader : public MView
//{
//  public:
//
//	static CFStringRef	GetClassID()			{ return CFSTR("MListViewHeader"); }
//
//						MListHeader(HIObjectRef inObjectRef);
//
//	OSStatus			HandleDraw(EventRef inEvent);
//
//  private:
//	struct ColumnInfo
//	{
//		string			mLabel;
//		uint32			mWidth;
//	};
//	
//	typedef vector<ColumnInfo>	ColumnInfoArray;
//	ColumnInfoArray		mColumns;
//};
//
//MListHeader::MListHeader(HIObjectRef inObjectRef)
//	: MView(inObjectRef, 0)
//{
//	Install(kEventClassControl, kEventControlDraw, this, &MListHeader::HandleDraw);
//}
//
//OSStatus MListHeader::HandleDraw(EventRef inEvent)
//{
//	CGContextRef context = nil;
//	::GetEventParameter(inEvent, kEventParamCGContextRef,
//		typeCGContextRef, nil, sizeof(CGContextRef), nil, &context);
//	
//	HIRect bounds;
//	GetBounds(bounds);
//
//	MDevice dev(context, bounds);
//	
//	return noErr;
//}

// the list interface implementation

MListView::MListView(
	MHandler*		inSuperHandler)
	: MHandler(inSuperHandler)
	, mImpl(nil)
{
	SetWidget(gtk_hbox_new(false, 0), false);
	
	MScrollBar* scrollBar = new MScrollBar(true);
	
	MViewPort* viewPort = new MViewPort(nil, scrollBar);
	viewPort->SetShadowType(GTK_SHADOW_NONE);
	
	gtk_box_pack_end(GTK_BOX(GetGtkWidget()), scrollBar->GetGtkWidget(), false, false, 0);
	gtk_box_pack_start(GTK_BOX(GetGtkWidget()), viewPort->GetGtkWidget(), true, true, 0);
	
	mImpl = new MListImp(this, scrollBar);
	
	viewPort->Add(mImpl);
	
	gtk_widget_show_all(GetGtkWidget());
}

MListView::~MListView()
{
}

void MListView::CreateColumns(
	const ColumnInfo&	inColumns)
{
//	mHeader = MView::Create<MListHeader>(GetSysView(), r);
//	mHeader->CreateColumns(inColumns);
}

uint32 MListView::GetCount() const
{
	return mImpl->mItems.size();
}

uint32 MListView::InsertItem(
	uint32			inRowNr,
	const void*		inData,
	uint32			inDataLength)
{
	mImpl->InsertItem(inRowNr, inData, inDataLength);
	return mImpl->mItems.size();
}

void MListView::ReplaceItem(
	uint32			inRowNr,
	const void*		inData,
	uint32			inDataLength)
{
	mImpl->ReplaceItem(inRowNr, inData, inDataLength);
}

void MListView::RemoveItem(
	uint32			inRowNr)
{
	mImpl->RemoveItem(inRowNr);
}

uint32 MListView::GetItem(
	uint32			inRowNr,
	void*			outData,
	uint32			inDataLength)
{
	return mImpl->GetItem(inRowNr, outData, inDataLength);
}

void MListView::RemoveAll()
{
	mImpl->RemoveAll();
}

void MListView::SelectItem(
	int32			inItemNr)
{
	mImpl->SelectItem(inItemNr);
}

int32 MListView::GetSelected() const
{
	return mImpl->GetSelected();
}

void MListView::ScrollToPosition(
	int32			inX,
	int32			inY)
{
	mImpl->DoScrollTo(inX, inY);
}

void MListView::GetScrollPosition(
	int32&			outX,
	int32&			outY)
{
	mImpl->GetScrollPosition(outX, outY);
}
//
//OSStatus MListView::DoControlActivate(EventRef inEvent)
//{
//	Invalidate();
//	return noErr;
//}
//
//OSStatus MListView::DoControlDeactivate(EventRef inEvent)
//{
//	Invalidate();
//	return noErr;
//}
//
//OSStatus MListView::DoControlDraw(EventRef inEvent)
//{
//	CGContextRef context = nil;
//	::GetEventParameter(inEvent, kEventParamCGContextRef,
//		typeCGContextRef, nil, sizeof(CGContextRef), nil, &context);
//	
//	HIRect bounds;
//	GetBounds(bounds);
//
//	MDevice dev(context, bounds);
//	
//	dev.DrawListBoxFrame(bounds, mImpl->mHasFocus);
//	
//	return noErr;
//}

void MListView::SetAcceptDragAndDrop()
{
	if (not mImpl->mSupportsDragAndDrop)
		mImpl->SetAcceptDragAndDrop();
}

void MListView::SetDrawBox(
	bool			inDrawBox)
{
	mImpl->mDrawListBox = inDrawBox;
//	Install(kEventClassControl, kEventControlDraw, this, &MListView::DoControlDraw);
//	
//	HIRect f;
//	mImpl->GetFrame(f);
//	f = ::CGRectInset(f, 3, 3);
//	mImpl->SetFrame(f);
}

void MListView::SetRoundedSelectionEdges(
	bool			inRoundEdges)
{
	mImpl->mDrawRoundEdges = inRoundEdges;
}

