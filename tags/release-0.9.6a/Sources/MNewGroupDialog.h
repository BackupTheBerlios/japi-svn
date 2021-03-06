//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef MNEWGROUPDIALOG_H
#define MNEWGROUPDIALOG_H

#include "MDialog.h"

class MProjectWindow;

class MNewGroupDialog : public MDialog
{
  public:
					MNewGroupDialog(
						MProjectWindow*		inParentWindow);

	virtual bool	OKClicked();
	
  private:
	MProjectWindow*	mProject;
};

#endif // MNEWGROUPDIALOG_H
