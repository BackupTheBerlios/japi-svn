//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

/*	$Id: MAuthDialog.h,v 1.2 2003/12/14 21:10:07 maarten Exp $
	Copyright maarten
	Created Thursday November 20 2003 21:36:39
*/

#ifndef MAUTHDIALOG_H
#define MAUTHDIALOG_H

#include "MDialog.h"

class MAuthDialog : public MDialog
{
  public:
						MAuthDialog(
							std::string		inTitle,
							std::string		inInstruction,
							int32			inFields,
							std::string		inPrompts[],
							bool			inEcho[]);

	virtual				~MAuthDialog();

	MEventOut<void(std::vector<std::string>&)>
						eAuthInfo;
	
  protected:
	virtual bool		OKClicked();
	virtual bool		CancelClicked();
	
	MEventIn<void(double)>		ePulse;

	void				Pulse(
							double	inSystemTime);
	
	int32				mFields;
	bool				mSentCredentials;
};

#endif // MAUTHDIALOG_H
