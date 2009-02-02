//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef MPRINTER_H
#define MPRINTER_H

#include "MCallbacks.h"

class MView;

class MPrinter
{
  public:
					MPrinter(
						MView*				inView);

					~MPrinter();

	void			DoPrint();

	static void		DoPageSetup();

  private:

	MRect			GetPrintBounds(
						GtkPrintContext*	inContext);

	void			OnBeginPrint(
						GtkPrintContext*	inContext);
	MSlot<void(GtkPrintContext*)>			mBeginPrint;

	void			OnDrawPage(
						GtkPrintContext*	inContext,
						int32				inPage);
	MSlot<void(GtkPrintContext*,int32)>		mDrawPage;

	GtkPrintOperation*			mPrint;
	MView*						mPrintedView;
	static GtkPrintSettings*	sSettings;
	static GtkPageSetup*		sPageSetup;
};

#endif // MPRINTER_H
