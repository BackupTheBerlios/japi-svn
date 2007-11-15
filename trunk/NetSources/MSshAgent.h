#ifndef MSSHAGENT_H
#define MSSHAGENT_H

class MSshPacket;

class MSshAgent
{
  public:
	static MSshAgent*	Create();

	bool				GetFirstIdentity(
							CryptoPP::Integer&	e,
							CryptoPP::Integer&	n,
							std::string&		outComment);
	
	bool				GetNextIdentity(
							CryptoPP::Integer&	e,
							CryptoPP::Integer&	n,
							std::string&		outComment);
	
	void				SignData(
							const std::string&	inBlob,
							const std::string&	inData,
							std::string&		outSignature);
	
  public:
						MSshAgent(
							int			inSock);

						~MSshAgent();	

	bool				RequestReply(
							MSshPacket&	out,
							MSshPacket&	in);

	int					mSock;
	MSshPacket*			mPacket;
	uint32				mCount;
	CryptoPP::Integer	e, n;
};

#endif
