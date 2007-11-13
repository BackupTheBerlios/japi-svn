/*	$Id: MAuthDialog.h,v 1.2 2003/12/14 21:10:07 maarten Exp $
	Copyright maarten
	Created Thursday November 20 2003 21:36:39
*/

#ifndef MAUTHDIALOG_H
#define MAUTHDIALOG_H

#include "MDialog.h"

//class MAuthDialogBase : public MDialog
//{
//  public:
//	enum { res_id = 1010 };
//	
//						MAuthDialogBase(MRect inFrame, MWindowFlags inFlags,
//							std::string inTitle, MWindow* inOwner);
//
//	MEventOut<bool>		eOKClicked;
//	
//	virtual void		SetTexts(std::string inTitle, std::string inInstruction,
//							std::string inPrompts[], bool inEcho[]);
//	virtual std::string	GetField(int inFieldNr) const;
//	virtual uint32		GetN() const;
//	
//  protected:
//	virtual void		InitSelf();
//
//	virtual bool		OKClicked();
//	virtual bool		CancelClicked();
//	
//	MEventIn<MAuthDialogBase,double>
//						ePulse;
//	void				Pulse(const double&, MEventSender*);
//};
//
//template<int N>
//class MAuthDialog : public MAuthDialogBase
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
//	: MAuthDialogBase(inFrame, inFlags, inTitle, inOwner)
//{
//}
//
//template<int N>
//void MAuthDialog<N>::InitSelf()
//{
//	MAuthDialogBase::InitSelf();
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

#endif // MAUTHDIALOG_H
