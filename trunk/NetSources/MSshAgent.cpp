#include "MJapieG.h"

#include <sys/un.h>
#include <sys/socket.h>
#include <cerrno>

#include <cryptopp/integer.h>

#include "MSshUtil.h"
#include "MSshAgent.h"

using namespace std;
using namespace CryptoPP;

/* Messages for the authentication agent connection. */
#define SSH_AGENTC_REQUEST_RSA_IDENTITIES	1
#define SSH_AGENT_RSA_IDENTITIES_ANSWER		2
#define SSH_AGENTC_RSA_CHALLENGE		3
#define SSH_AGENT_RSA_RESPONSE			4
#define SSH_AGENT_FAILURE			5
#define SSH_AGENT_SUCCESS			6
#define SSH_AGENTC_ADD_RSA_IDENTITY		7
#define SSH_AGENTC_REMOVE_RSA_IDENTITY		8
#define SSH_AGENTC_REMOVE_ALL_RSA_IDENTITIES	9

/* private OpenSSH extensions for SSH2 */
#define SSH2_AGENTC_REQUEST_IDENTITIES		11
#define SSH2_AGENT_IDENTITIES_ANSWER		12
#define SSH2_AGENTC_SIGN_REQUEST		13
#define SSH2_AGENT_SIGN_RESPONSE		14
#define SSH2_AGENTC_ADD_IDENTITY		17
#define SSH2_AGENTC_REMOVE_IDENTITY		18
#define SSH2_AGENTC_REMOVE_ALL_IDENTITIES	19

/* smartcard */
#define SSH_AGENTC_ADD_SMARTCARD_KEY		20
#define SSH_AGENTC_REMOVE_SMARTCARD_KEY		21

/* lock/unlock the agent */
#define SSH_AGENTC_LOCK				22
#define SSH_AGENTC_UNLOCK			23

/* add key with constraints */
#define SSH_AGENTC_ADD_RSA_ID_CONSTRAINED	24
#define SSH2_AGENTC_ADD_ID_CONSTRAINED		25
#define SSH_AGENTC_ADD_SMARTCARD_KEY_CONSTRAINED 26

#define	SSH_AGENT_CONSTRAIN_LIFETIME		1
#define	SSH_AGENT_CONSTRAIN_CONFIRM		2

/* extended failure messages */
#define SSH2_AGENT_FAILURE			30

/* additional error code for ssh.com's ssh-agent2 */
#define SSH_COM_AGENT2_FAILURE			102

#define	SSH_AGENT_OLD_SIGNATURE			0x01


MSshAgent* MSshAgent::Create()
{
	auto_ptr<MSshAgent> result;

	const char* authSock = getenv("SSH_AUTH_SOCK");
	
	if (authSock != nil)
	{
		struct sockaddr_un addr = {};
		addr.sun_family = AF_LOCAL;
		strcpy(addr.sun_path, authSock);
		
		int sock = socket(AF_LOCAL, SOCK_STREAM, 0);
		if (sock >= 0)
		{
			if (fcntl(sock, F_SETFD, 1) < 0)
				close(sock);
			else if (connect(sock, (const sockaddr*)&addr, sizeof(addr)) < 0)
				close(sock);
			else
				result.reset(new MSshAgent(sock));
		}
	}

	return result.release();
}

MSshAgent::MSshAgent(
	int			inSock)
	: mSock(inSock)
	, mPacket(nil)
{
}

MSshAgent::~MSshAgent()
{
	delete mPacket;
	
	close(mSock);
}

bool MSshAgent::GetFirstIdentity(
	Integer&	e,
	Integer&	n,
	string&		outComment)
{
	bool result = false;
	
	delete mPacket;
	mPacket = new MSshPacket;
	mCount = 0;

	MSshPacket out;
	uint8 msg = SSH2_AGENTC_REQUEST_IDENTITIES;
	out << msg;
	
	if (not RequestReply(out, *mPacket))
	{
		delete mPacket;
		mPacket = nil;
	}
	
	if (mPacket != nil)
	{
		*mPacket >> msg;
		
		if (msg != SSH2_AGENT_IDENTITIES_ANSWER)
		{
			delete mPacket;
			mPacket = nil;
		}
		else
		{
			*mPacket >> mCount;

			PRINT(("++ SSH_AGENT returned %d identities", mCount));
			
			if (mCount > 0 and mCount < 1024)
				result = GetNextIdentity(e, n, outComment);
		}
	}
	
	return result;
}

bool MSshAgent::GetNextIdentity(
	Integer&	e,
	Integer&	n,
	string&		outComment)
{
	bool result = false;
	
	while (mCount > 0 and result == false)
	{
		MSshPacket p;
		
		*mPacket >> p.data;
		
		string type;
		
		p >> type;
		
		if (type == "ssh-rsa")
		{
			p >> e >> n;
			
			if (p.data.length() > 0)
				p >> outComment;
			else
				outComment.clear();
			
			PRINT(("++ returning yet another identity"));

			result = true;
		}
		
		--mCount;
	}
	
	return result;
}

bool MSshAgent::RequestReply(
	MSshPacket&		out,
	MSshPacket&		in)
{
	net_swapper swap;
	
	uint32 l = out.data.length();
	l = swap(l);
	
	bool result = false;
	
	if (write(mSock, &l, sizeof(l)) == sizeof(l) and
		write(mSock, out.data.c_str(), out.data.length()) == int32(out.data.length()) and
		read(mSock, &l, sizeof(l)) == sizeof(l))
	{
		l = swap(l);
		
		if (l < 256 * 1024)
		{
			char b[1024];

			uint32 k = l;
			if (k > sizeof(b))
				k = sizeof(b);
			
			while (l > 0)
			{
				if (read(mSock, b, k) != k)
					break;
				
				in.data.append(b, k);
				
				l -= k;
			}
			
			result = (l == 0);
		}
	}
	
	return result;
}

void MSshAgent::SignData(
	const string&	inBlob,
	const string&	inData,
	string&			outSignature)
{
	MSshPacket out;
	uint8 msg = SSH2_AGENTC_SIGN_REQUEST;

	uint32 flags = 0;
	
	out << msg << inBlob << inData << flags;
	
	MSshPacket in;
	if (RequestReply(out, in))
	{
		in >> msg;
		
		if (msg == SSH2_AGENT_SIGN_RESPONSE)
			in >> outSignature;
	}
}
