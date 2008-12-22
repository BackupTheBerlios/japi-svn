//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

/*	$Id: MMarkMatchingDialog.h 75 2006-09-05 09:56:43Z maarten $
	Copyright Maarten L. Hekkelman
	Created Sunday August 15 2004 20:47:00
*/

#ifndef MMARKMATCHINGDIALOG_H
#define MMARKMATCHINGDIALOG_H

#include "MDialog.h"

class MTextDocument;

class MMarkMatchingDialog : public MDialog
{
  public:
						MMarkMatchingDialog(
							MTextDocument*	inDocument,
							MWindow*		inWindow);

	virtual bool		OKClicked();
	
  private:
	MTextDocument*		mDocument;
};

#endif // MMARKMATCHINGDIALOG_H
