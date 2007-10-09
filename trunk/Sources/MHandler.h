#ifndef MHANDLER_H
#define MHANDLER_H

class MHandler
{
  public:
					MHandler(
						MHandler*		inSuper);

	virtual			~MHandler();

	virtual bool	UpdateCommandStatus(
						uint32			inCommand,
						bool&			outEnabled,
						bool&			outChecked);

	virtual bool	ProcessCommand(
						uint32			inCommand);

	void			SetSuper(
						MHandler*		inSuper);

  protected:
	MHandler*		mSuper;
};

#endif
