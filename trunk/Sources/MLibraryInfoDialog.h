//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef MLIBRARYINFODIALOG_H
#define MLIBRARYINFODIALOG_H

#include "MDialog.h"

class MProject;
class MProjectLib;

class MLibraryInfoDialog : public MDialog
{
  public:
						MLibraryInfoDialog();

						~MLibraryInfoDialog();

	void				Initialize(
							MProject*		inProject,
							MProjectLib*	inLibrary);

	virtual bool		OKClicked();
	
  private:

	virtual void		ValueChanged(
							uint32			inID);

	MProject*			mProject;
	MProjectLib*		mLibrary;
};

#endif // MFINDANDOPENDIALOG_H
