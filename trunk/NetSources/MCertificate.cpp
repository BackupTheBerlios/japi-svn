/*	$Id: MCertificate.cpp,v 1.2 2004/03/05 20:43:26 maarten Exp $
	Copyright maarten
	Created Tuesday December 09 2003 14:24:21
*/

#include "MJapieG.h"

#include "MCertificate.h"

using namespace std;

MEventOut<void()>	MCertificate::eCertificateDeleted;

struct MCertificateImp {};

MCertificate::MCertificate()
	: fImpl(nil)
{
}

MCertificate::MCertificate(
	string		inSignature)
	: fImpl(nil)
{
}

MCertificate::~MCertificate()
{
	delete fImpl;
}

bool MCertificate::Next(
	MCertificate&	ioCertificate)
{
	return false;
}

MCertificate* MCertificate::Select(
	MWindow*		inParent)
{
	return nil;
}

string MCertificate::GetFriendlyName() const
{
	return "";
}

string MCertificate::GetSignature() const
{
	return "";
}

bool MCertificate::GetPublicRSAKey(
	CryptoPP::Integer&	outExp,
	CryptoPP::Integer&	outN) const
{
	return false;
}

string MCertificate::GetPublicKeyFileContent() const
{
	return "";
}

bool MCertificate::SignData(
	string		inData,
	string&		outSignature)
{
	return false;
}

