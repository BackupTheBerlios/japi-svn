/*	$Id: MAuthDialog.cpp,v 1.3 2004/02/07 13:10:08 maarten Exp $
	Copyright maarten
	Created Friday November 21 2003 19:38:34
*/

#include "MJapieG.h"

#include <cmath>

#include "MAuthDialog.h"

//MAuthDialogBase::MAuthDialogBase(MRect inFrame, MWindowFlags inFlags,
//		std::string inTitle, MWindow* inOwner)
//	: MDialog(inFrame, inFlags, inTitle, inOwner)
//	, ePulse(this, &MAuthDialogBase::Pulse)
//{
//}
//
//void MAuthDialogBase::InitSelf()
//{
//	SetNodeVisible('caps', false);
//	AddRoute(gApp->ePulse, ePulse);
//}
//
//bool MAuthDialogBase::OKClicked()
//{
//	eOKClicked(true, this);
//	return true;
//}
//
//bool MAuthDialogBase::CancelClicked()
//{
//	eOKClicked(false, this);
//	return true;
//}
//
//void MAuthDialogBase::Pulse(const double& inTime, MEventSender*)
//{
//	SetNodeVisible('caps',
//		ModifierKeyDown(kAlphaLock) && (std::fmod(inTime, 1.0) <= 0.5));
//}
//
//void MAuthDialogBase::SetTexts(std::string inTitle, std::string inInstruction,
//							std::string inPrompts[], bool inEcho[])
//{
//}
//
//std::string MAuthDialogBase::GetField(int inFieldNr) const
//{
//	return kEmptyString;
//}
//
//uint32 MAuthDialogBase::GetN() const
//{
//	return 0;
//}
