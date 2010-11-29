//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef MWINSSHAGENT_H
#define MWINSSHAGENT_H

#include <wincrypt.h>

#include "../Ssh/MSshAgent.h"

class MWinSshAgentImpl : public MSshAgentImpl
{
  public:

  					MWinSshAgentImpl();
	virtual			~MWinSshAgentImpl();

	virtual bool	GetFirstIdentity(
						CryptoPP::Integer&	e,
						CryptoPP::Integer&	n,
						std::string&		outComment);
	
	virtual bool	GetNextIdentity(
						CryptoPP::Integer&	e,
						CryptoPP::Integer&	n,
						std::string&		outComment);
	
	virtual bool	SignData(
						const MSshPacket&	inBlob,
						const MSshPacket&	inData,
						std::vector<uint8>&	outSignature);

  private:
	bool			GetIdentity(
						CryptoPP::Integer&	e,
						CryptoPP::Integer&	n,
						std::string&		outComment);

	PCCERT_CONTEXT	mCertificateContext;
};

#endif
