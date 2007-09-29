#ifndef MDOCWINDOW_H
#define MDOCWINDOW_H

#include "MWindow.h"
#include "MMenu.h"

class MDocument;
class MMenubar;
class MView;
class MTextView;

class MDocWindow : public MWindow
{
  public:
					MDocWindow(
						MDocument*		inDocument);

	static MDocWindow*
					DisplayDocument(
						MDocument*		inDocument);

	static MDocWindow*
					GetFirstDocWindow()	{ return sFirst; }

	MDocument*		GetDocument()		{ return mDocument; }

	virtual bool	UpdateCommandStatus(
						uint32			inCommand,
						bool&			outEnabled,
						bool&			outChecked);

	virtual bool	ProcessCommand(
						uint32			inCommand);

	static std::string
					GetUntitledTitle();

  protected:

	virtual			~MDocWindow();

	MDocument*		mDocument;
	GtkWidget*		mVBox;
	GtkWidget*		mStatusbar;
	MMenubar		mMenubar;
	
	MView*			mTextView;
	
	static MDocWindow*	sFirst;
	MDocWindow*			mNext;
};

#endif
