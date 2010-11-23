//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

/*	$Id: MAuthDialog.cpp,v 1.3 2004/02/07 13:10:08 maarten Exp $
	Copyright maarten
	Created Friday November 21 2003 19:38:34
*/

#include "MLib.h"

#include <cmath>

#include "MAuthDialog.h"

using namespace std;

MAuthDialog::MAuthDialog(
	std::string		inTitle,
	std::string		inInstruction,
	int32			inFields,
	std::string		inPrompts[],
	bool			inEcho[])
	: MDialog("auth-dialog")
	, ePulse(this, &MAuthDialog::Pulse)
{
	mFields = inFields;

	SetTitle(inTitle);
	
	SetText("instruction", inInstruction);
	
	for (int32 id = 1; id <= mFields; ++id)
	{
		SetVisible("label-" + boost::lexical_cast<string>(id), true);
		SetVisible("edit-" + boost::lexical_cast<string>(id), true);

		SetText("label-" + boost::lexical_cast<string>(id), inPrompts[id - 1]);
		
//		SetPasswordField("edit-" + boost::lexical_cast<string>(id), inEcho[id - 1]);
	}

	for (int32 id = mFields + 1; id <= 5; ++id)
	{
		SetVisible("label-" + boost::lexical_cast<string>(id), false);
		SetVisible("edit-" + boost::lexical_cast<string>(id), false);
	}
}

bool MAuthDialog::OKClicked()
{
	vector<string> args;
	
	for (int32 id = 1; id <= mFields; ++id)
		args.push_back(GetText("edit-" + boost::lexical_cast<string>(id)));
	
	eAuthInfo(args);

	return true;
}

bool MAuthDialog::CancelClicked()
{
	vector<string> args;
	
	eAuthInfo(args);
	
	return true;
}

void MAuthDialog::Pulse(
	double		inTime)
{
//	SetNodeVisible('caps',
//		ModifierKeyDown(kAlphaLock) && (std::fmod(inTime, 1.0) <= 0.5));
}
