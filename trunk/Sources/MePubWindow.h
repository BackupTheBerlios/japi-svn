/* 
   Created by: Maarten L. Hekkelman
   Date: donderdag 05 februari, 2009
*/

#ifndef MEPUBWINDOW_H
#define MEPUBWINDOW_H

#include "MDocWindow.h"

class MProjectItem;
class MProjectTree;

class MePubWindow : public MDocWindow
{
  public:
					MePubWindow();
					
	virtual 		~MePubWindow();	
	
	virtual void	Initialize(
						MDocument*		inDocument);

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

	MePubDocument*	mEPub;
	MProjectTree*	mFilesTree;
};

#endif
