/*	$Id$
	Copyright Drs M.L. Hekkelman
	Created 28-09-07 11:20:53
*/

#ifndef MWINDOW_H
#define MWINDOW_H

#include "MView.h"
#include "MCallbacks.h"

class MWindow : public MView, public MHandler
{
  public:
					MWindow();
	virtual			~MWindow();
	
	void			Show();
	void			Hide();
	void			Select();
	
	virtual void	Close();

	void			SetTitle(
						const std::string&	inTitle);
	std::string		GetTitle() const;

	virtual bool	UpdateCommandStatus(
						uint32			inCommand,
						bool&			outEnabled,
						bool&			outChecked);

	virtual bool	ProcessCommand(
						uint32			inCommand);

  protected:

	virtual void	OnDestroy();
	virtual bool	OnDelete(
						GdkEvent*	inEvent);

  private:
	MSlot<void()>			mOnDestroy;
	MSlot<bool(GdkEvent*)>	mOnDelete;
};


#endif // MWINDOW_H
