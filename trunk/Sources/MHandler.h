#ifndef MHANDLER_H
#define MHANDLER_H

class MMenu;

class MHandler
{
  public:
						MHandler(
							MHandler*		inSuper);

	virtual				~MHandler();

	virtual bool		UpdateCommandStatus(
							uint32			inCommand,
							bool&			outEnabled,
							bool&			outChecked);

	virtual bool		ProcessCommand(
							uint32			inCommand,
							const MMenu*	inMenu,
							uint32			inItemIndex);

	void				SetSuper(
							MHandler*		inSuper);

	bool				IsFocus() const					{ return sFocus == this; }
	static MHandler*	GetFocus()						{ return sFocus; }
	
	void				TakeFocus();
	void				ReleaseFocus();

  protected:
	
	MHandler*			mSuper;
	static MHandler*	sFocus;
};

#endif
