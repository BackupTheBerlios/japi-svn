#ifndef MDOCWINDOW_H
#define MDOCWINDOW_H

#include "MWindow.h"

class MDocument;
class MMenubar;

class MDocWindow : public MWindow
{
  public:
				MDocWindow(
					MDocument*		inDocument);

	static MDocWindow*
				DisplayDocument(
					MDocument*		inDocument);

	MDocument*	GetDocument()		{ return mDocument; }

  protected:

	virtual		~MDocWindow();

	MDocument*	mDocument;
	GtkWidget*	mVBox;
	MMenubar*	mMenubar;
};

#endif
