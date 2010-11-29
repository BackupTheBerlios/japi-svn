//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef MSSHAGENT_H
#define MSSHAGENT_H

#include <vector>
#include <cryptopp/integer.h>

class MSshPacket;

class MSshAgentImpl
{
  public:
	static MSshAgentImpl*
						Create();

	virtual				~MSshAgentImpl() {}

	virtual bool		GetFirstIdentity(
							CryptoPP::Integer&	e,
							CryptoPP::Integer&	n,
							std::string&		outComment) = 0;
	
	virtual bool		GetNextIdentity(
							CryptoPP::Integer&	e,
							CryptoPP::Integer&	n,
							std::string&		outComment) = 0;
	
	virtual bool		SignData(
							const MSshPacket&	inBlob,
							const MSshPacket&	inData,
							std::vector<uint8>&	outSignature) = 0;

  protected:
						MSshAgentImpl() {}
};

class MSshAgent
{
  public:
						MSshAgent();
	virtual 			~MSshAgent();

	bool				GetFirstIdentity(
							CryptoPP::Integer&	e,
							CryptoPP::Integer&	n,
							std::string&		outComment)
						{
							return mImpl->GetFirstIdentity(e, n, outComment);
						}
	
	bool				GetNextIdentity(
							CryptoPP::Integer&	e,
							CryptoPP::Integer&	n,
							std::string&		outComment)
						{
							return mImpl->GetNextIdentity(e, n, outComment);
						}
	
	void				SignData(
							const MSshPacket&	inBlob,
							const MSshPacket&	inData,
							std::vector<uint8>&	outSignature)
						{
							mImpl->SignData(inBlob, inData, outSignature);
						}
	
  private:
						MSshAgent(const MSshAgent&);
	MSshAgent&			operator=(const MSshAgent&);

	class MSshAgentImpl*
						mImpl;
};

#endif
