//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEPUBWINDOW_H
#define MEPUBWINDOW_H

#include "MDocWindow.h"

class MProjectItem;
class MTextDocument;
class MListBase;
class MListRowBase;
template<class R> class MList;
class MePubFileRow;
class MePubTOCRow;

class MePubWindow : public MDocWindow
{
  public:
					MePubWindow();
					
	virtual 		~MePubWindow();	
	
	virtual void	Initialize(
						MDocument*		inDocument);

	virtual bool	DoClose();

	virtual bool	UpdateCommandStatus(
						uint32			inCommand,
						MMenu*			inMenu,
						uint32			inItemIndex,
						bool&			outEnabled,
						bool&			outChecked);

	virtual bool	ProcessCommand(
						uint32			inCommand,
						const MMenu*	inMenu,
						uint32			inItemIndex,
						uint32			inModifiers);

  private:

	virtual void	SaveState();

	template<class R>
	void			AddItemsToList(
						MProjectGroup*		inGroup,
						MListRowBase*		inParent,
						MList<R>*			inList);

	virtual void	DocumentLoaded(
						MDocument*		inDocument);

	virtual bool	OnKeyPressEvent(
						GdkEventKey*	inEvent);

	MSlot<bool(GdkEventKey*)>
					eKeyPressEvent;

	void			SelectFileRow(
						MePubFileRow*	inRow);
	MEventIn<void(MePubFileRow*)>
					eSelectFileRow;

	void			InvokeFileRow(
						MePubFileRow*	inRow);
	MEventIn<void(MePubFileRow*)>
					eInvokeFileRow;

	void			DraggedFileRow(
						MePubFileRow*	inRow);
	MEventIn<void(MePubFileRow*)>
					eDraggedFileRow;

	void			SelectTOCRow(
						MePubTOCRow*	inRow);
	MEventIn<void(MePubTOCRow*)>
					eSelectTOCRow;

	void			InvokeTOCRow(
						MePubTOCRow*	inRow);
	MEventIn<void(MePubTOCRow*)>
					eInvokeTOCRow;

	void			DraggedTOCRow(
						MePubTOCRow*	inRow);
	MEventIn<void(MePubTOCRow*)>
					eDraggedTOCRow;

	virtual void	ValueChanged(
						uint32			inID);

	void			CreateNew(
						bool			inNewDirectory);

	void			SubjectChanged();
	MSlot<void()>	eSubjectChanged;

	void			DateChanged();
	MSlot<void()>	eDateChanged;

//	void			EditedItemName(
//						gchar*			path,
//						gchar*			new_text);
//
//	MSlot<void(gchar*,gchar*)>
//					mEditedItemName;
//
//	void			EditedItemID(
//						gchar*			path,
//						gchar*			new_text);
//
//	MSlot<void(gchar*,gchar*)>
//					mEditedItemID;
//
//	void			EditedItemLinear(
//						gchar*			path);
//
//	MSlot<void(gchar*)>
//					mEditedItemLinear;
//
//	void			EditedItemMediaType(
//						gchar*			path,
//						gchar*			new_text);
//
//	MSlot<void(gchar*,gchar*)>
//					mEditedItemMediaType;
//
//	void			EditedTOCTitle(
//						gchar*			path,
//						gchar*			new_text);
//
//	MSlot<void(gchar*,gchar*)>
//					mEditedTOCTitle;
//
//	void			EditedTOCSrc(
//						gchar*			path,
//						gchar*			new_text);
//
//	MSlot<void(gchar*,gchar*)>
//					mEditedTOCSrc;
//
//	void			EditedTOCClass(
//						gchar*			path,
//						gchar*			new_text);
//
//	MSlot<void(gchar*,gchar*)>
//					mEditedTOCClass;

	void			RenameItem();

	MProjectItem*	GetSelectedItem();

	void			DeleteSelectedItem();

	MePubDocument*	mEPub;
	MListBase*		mFileTree;
	MListBase*		mTOCTree;
};

#endif
