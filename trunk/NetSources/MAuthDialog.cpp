//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

/*	$Id: MAuthDialog.cpp,v 1.3 2004/02/07 13:10:08 maarten Exp $
	Copyright maarten
	Created Friday November 21 2003 19:38:34
*/

#include "MJapi.h"

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
	, mSentCredentials(false)
{
	mFields = inFields;

	SetTitle(inTitle);
	
	SetText('inst', inInstruction);
	
	uint32 lblID = 'lbl1';
	uint32 edtID = 'edt1';
	
	for (int32 i = 0; i < mFields; ++i)
	{
		SetVisible(lblID, true);
		SetVisible(edtID, true);

		SetText(lblID, inPrompts[i]);
		
		SetPasswordField(edtID, inEcho[i]);
		
		++lblID;
		++edtID;
	}

	for (int32 i = mFields; i < 5; ++i)
	{
		SetVisible(lblID++, false);
		SetVisible(edtID++, false);
	}
}

MAuthDialog::~MAuthDialog()
{
	if (not mSentCredentials)
		CancelClicked();
}

bool MAuthDialog::OKClicked()
{
	vector<string> args;
	
	uint32 edtID = 'edt1';
	for (int32 i = 0; i < mFields; ++i)
	{
		string a;
		GetText(edtID, a);
		args.push_back(a);
		++edtID;
	}
	
	eAuthInfo(args);
	mSentCredentials = true;

	return true;
}

bool MAuthDialog::CancelClicked()
{
	vector<string> args;
	
	eAuthInfo(args);
	mSentCredentials = true;
	
	return true;
}

void MAuthDialog::Pulse(
	double		inTime)
{
//	SetNodeVisible('caps',
//		ModifierKeyDown(kAlphaLock) && (std::fmod(inTime, 1.0) <= 0.5));
}
