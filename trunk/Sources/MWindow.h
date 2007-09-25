#ifndef MWINDOW_H
#define MWINDOW_H

#include "MCallbacks.h"

class MWindow
{
  public:
					MWindow();
	virtual			~MWindow();
	
	void			Show();
	void			Hide();
	void			Select();

  protected:

	virtual void	OnDestroy();
	virtual bool	OnDelete(
						GdkEvent*	inEvent);

  private:
	MSlot<void()>			mOnDestroy;
	MSlot<bool(GdkEvent*)>	mOnDelete;

	GtkWidget*		mWindow;
};

#endif
