/*
	Copyright (c) 2007, Maarten L. Hekkelman
	All rights reserved.

	Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions
	are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in
      the documentation and/or other materials provided with the
      distribution.
    * Neither the name of the Maarten L. Hekkelman nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
	"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
	LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
	FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
	COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
	INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
	(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
	SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
	HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
	STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
	ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
	OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/*	$Id: MAuthDialog.cpp,v 1.3 2004/02/07 13:10:08 maarten Exp $
	Copyright maarten
	Created Friday November 21 2003 19:38:34
*/

#include "MJapieG.h"

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
	
	SetText('inst', inInstruction);
	
	uint32 lblID = 'lbl1';
	uint32 edtID = 'edt1';
	
	for (int32 i = 0; i < mFields; ++i)
	{
		SetVisible(lblID, true);
		SetVisible(edtID, true);

		SetText(lblID, inPrompts[i]);
		
//		if (inEcho[i] == false)
//			SetPasswordField(edtID);
		
		++lblID;
		++edtID;
	}

	for (int32 i = mFields; i < 5; ++i)
	{
		SetVisible(lblID++, false);
		SetVisible(edtID++, false);
	}
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

//void MAuthDialog::SetTexts(std::string inTitle, std::string inInstruction,
//							std::string inPrompts[], bool inEcho[])
//{
//}
//
//std::string MAuthDialog::GetField(int inFieldNr) const
//{
//	return kEmptyString;
//}
//
//uint32 MAuthDialog::GetN() const
//{
//	return 0;
//}

//template<int N>
//class MAuthDialog : public MAuthDialog
//{
//  public:
//	enum { res_id = 1010 + N };
//	
//						MAuthDialog(MRect inFrame, MWindowFlags inFlags,
//							std::string inTitle, MWindow* inOwner);
//
//	virtual void		SetTexts(std::string inTitle, std::string inInstruction,
//							std::string inPrompts[], bool inEcho[]);
//	virtual std::string	GetField(int inFieldNr) const;
//	virtual uint32		GetN() const			{ return N; }
//	
//  protected:
//	virtual void		InitSelf();
//
//	std::string			fFields[N];
//};
//
//// implementation
//
//template<int N>
//MAuthDialog<N>::MAuthDialog(MRect inFrame, MWindowFlags inFlags,
//		std::string inTitle, MWindow* inOwner)
//	: MAuthDialog(inFrame, inFlags, inTitle, inOwner)
//{
//}
//
//template<int N>
//void MAuthDialog<N>::InitSelf()
//{
//	MAuthDialog::InitSelf();
//	SwitchTarget('fld1');
//}
//
//template<int N>
//void MAuthDialog<N>::SetTexts(std::string inTitle, std::string inInstruction,
//	std::string inPrompts[], bool inEcho[])
//{
//	MNodeID promptID = 'pro1';
//	MNodeID fieldID = 'fld1';
//	
//	SetTitle(inTitle);
//	SetNodeText('inst', inInstruction);
//	
//	for (int i = 0; i < N; ++i, ++promptID, ++fieldID)
//	{
//		SetNodeText(promptID, inPrompts[i]);
//		if (inEcho[i])
//			FindNode<MEditTextNode>(fieldID)->SetPasswordChar(0);
//	}
//}
//
//template<int N>
//std::string MAuthDialog<N>::GetField(int inFieldNr) const
//{
//	MNodeID promptID = 'fld1' + inFieldNr;
//	return GetNodeText(promptID);
//}
