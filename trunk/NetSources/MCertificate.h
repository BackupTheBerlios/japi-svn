/*	$Id: MCertificate.h,v 1.5 2004/03/05 20:43:26 maarten Exp $
	Copyright maarten
	Created Tuesday December 09 2003 13:49:19
*/

#ifndef MCERTIFICATE_H
#define MCERTIFICATE_H

#include <string>
#include "MP2PEvents.h"

class MWindow;

namespace CryptoPP {
class Integer;
}

struct MSshPacket;

class MCertificate
{
  public:
					MCertificate();

					MCertificate(
						std::string		inSignature);

	virtual			~MCertificate();
	
	static bool		Next(
						MCertificate&	ioCertificate);
	
	static MCertificate*
					Select(
						MWindow* inParent);
	
	std::string		GetFriendlyName() const;
	std::string		GetComment() const;
	std::string		GetSignature() const;	// SHA1 hash
	std::string		GetPublicKeyFileContent() const;
	
	bool			GetPublicRSAKey(
						CryptoPP::Integer&	outExp,
						CryptoPP::Integer&	outN) const;
	
	bool			SignData(
						std::string			inData,
						std::string&		outSignature);

	static void		MandleAuthMessage(
						uint8				inMessage,
						MSshPacket&			in,
						MSshPacket&			out);
	
	static MEventOut<void()>				eCertificateDeleted;
	
  private:
					MCertificate(
						const MCertificate&	inOther);

	MCertificate&	operator=(
						const MCertificate&	inOther);

	struct MCertificateImp*	fImpl;
};

#endif // MCERTIFICATE_H
