//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEPUBWINDOW_H
#define MEPUBWINDOW_H

#include "MDocWindow.h"

class MProjectItem;
class MePubFileTree;
class MTextDocument;

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

	void			SaveState();

	void			GetSelectedItems(
						std::vector<MProjectItem*>&
										outItems);
	
	virtual bool	OnKeyPressEvent(
						GdkEventKey*	inEvent);

	MSlot<bool(GdkEventKey*)>
					eKeyPressEvent;

	void			DeleteSelectedItems();

	void			InvokeFileRow(
						GtkTreePath*	inPath,
						GtkTreeViewColumn*
										inColumn);

	MSlot<void(GtkTreePath*path, GtkTreeViewColumn*)>
					eInvokeFileRow;

	void			InitializeTreeView(
						GtkTreeView*	inGtkTreeView);

	virtual void	ValueChanged(
						uint32			inID);

	MEventIn<void(MDocument*)>		eDocumentClosed;	// doc closed
	MEventIn<void(MDocument*, const MUrl&)>
									eFileSpecChanged;	// doc was saved as
	
	void			TextDocClosed(
						MDocument*		inDocument);
	void			TextDocFileSpecChanged(
						MDocument*		inDocument,
						const MUrl&		inURL);

	void			CreateNewGroup(
						const std::string&
										inGroupName);

	MEventIn<void(const std::string&)>	eCreateNewGroup;

	void			EditedItemName(
						gchar*			path,
						gchar*			new_text);

	MSlot<void(gchar*,gchar*)>
					mEditedItemName;

	void			EditedItemID(
						gchar*			path,
						gchar*			new_text);

	MSlot<void(gchar*,gchar*)>
					mEditedItemID;

	void			EditedItemMediaType(
						gchar*			path,
						gchar*			new_text);

	MSlot<void(gchar*,gchar*)>
					mEditedItemMediaType;

	MePubDocument*	mEPub;
	MePubFileTree*	mFilesTree;
	
	typedef std::map<fs::path,MTextDocument*>	MEPubTextDocMap;
	MEPubTextDocMap	mOpenFiles;
};

#endif
