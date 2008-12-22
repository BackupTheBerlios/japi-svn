//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef MQUOTEDREWRAPDIALOG_H
#define MQUOTEDREWRAPDIALOG_H

#include "MDialog.h"

class MTextDocument;

class MQuotedRewrapDialog : public MDialog
{
  public:
					MQuotedRewrapDialog(
						MTextDocument*	inDocument,
						MWindow*		inWindow);

	virtual bool	OKClicked();
	
  private:
	MTextDocument*	mDocument;
};

#endif // MQUOTEDREWRAPDIALOG_H
