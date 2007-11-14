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
	: ePulse(this, &MAuthDialog::Pulse)
	, mFields(inFields)
{
	SetTitle(inTitle);
	
	AddStaticText('inst', inInstruction);
	
	AddTable('tbl1', 2, inFields);
	
	uint32 lblID = 'lbl1';
	uint32 edtID = 'edt1';
	
	for (int32 i = 0; i < mFields; ++i)
	{
		AddStaticText(lblID, inPrompts[i], 'tbl1');
		AddEditField(edtID, "", 'tbl1');
		
		if (inEcho[i] == false)
			SetPasswordField(edtID);
		
		++lblID;
		++edtID;
	}
	
	AddOKButton("OK");
	AddCancelButton("Cancel");

//	SetNodeVisible('caps', false);
//	AddRoute(gApp->ePulse, ePulse);
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
