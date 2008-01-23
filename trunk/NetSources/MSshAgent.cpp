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

#include "MJapi.h"

#include <sys/un.h>
#include <sys/socket.h>
#include <cerrno>

#include <cryptopp/integer.h>

#include "MSshUtil.h"
#include "MSshAgent.h"

using namespace std;
using namespace CryptoPP;

enum {
	
	/* Messages for the authentication agent connection. */
	SSH_AGENTC_REQUEST_RSA_IDENTITIES =	1,
	SSH_AGENT_RSA_IDENTITIES_ANSWER,
	SSH_AGENTC_RSA_CHALLENGE,
	SSH_AGENT_RSA_RESPONSE,
	SSH_AGENT_FAILURE,
	SSH_AGENT_SUCCESS,
	SSH_AGENTC_ADD_RSA_IDENTITY,
	SSH_AGENTC_REMOVE_RSA_IDENTITY,
	SSH_AGENTC_REMOVE_ALL_RSA_IDENTITIES,
	
	/* private OpenSSH extensions for SSH2 */
	SSH2_AGENTC_REQUEST_IDENTITIES = 11,
	SSH2_AGENT_IDENTITIES_ANSWER,
	SSH2_AGENTC_SIGN_REQUEST,
	SSH2_AGENT_SIGN_RESPONSE,
	SSH2_AGENTC_ADD_IDENTITY = 17,
	SSH2_AGENTC_REMOVE_IDENTITY,
	SSH2_AGENTC_REMOVE_ALL_IDENTITIES,
	
	/* smartcard */
	SSH_AGENTC_ADD_SMARTCARD_KEY,
	SSH_AGENTC_REMOVE_SMARTCARD_KEY,
	
	/* lock/unlock the agent */
	SSH_AGENTC_LOCK,
	SSH_AGENTC_UNLOCK,
	
	/* add key with constraints */
	SSH_AGENTC_ADD_RSA_ID_CONSTRAINED,
	SSH2_AGENTC_ADD_ID_CONSTRAINED,
	SSH_AGENTC_ADD_SMARTCARD_KEY_CONSTRAINED,
	
	SSH_AGENT_CONSTRAIN_LIFETIME = 1,
	SSH_AGENT_CONSTRAIN_CONFIRM,
	
	/* extended failure messages */
	SSH2_AGENT_FAILURE = 30,
	
	/* additional error code for ssh.com's ssh-agent2 */
	SSH_COM_AGENT2_FAILURE = 102,
	
	SSH_AGENT_OLD_SIGNATURE = 0x01
};

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
	, mCount(0)
{
}

MSshAgent::~MSshAgent()
{
	close(mSock);
}

bool MSshAgent::GetFirstIdentity(
	Integer&	e,
	Integer&	n,
	string&		outComment)
{
	bool result = false;
	
	mCount = 0;

	MSshPacket out;
	uint8 msg = SSH2_AGENTC_REQUEST_IDENTITIES;
	out << msg;
	
	if (RequestReply(out, mIdentities))
	{
//#if DEBUG
//		mIdentities.Dump();
//#endif
		mIdentities >> msg;
		
		if (msg == SSH2_AGENT_IDENTITIES_ANSWER)
		{
			mIdentities >> mCount;

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
	
	while (result == false and mCount-- > 0 and mIdentities.data.length() > 0)
	{
		MSshPacket blob;

		mIdentities >> blob.data >> outComment;
		
		string type;
		blob >> type;
		
		if (type != "ssh-rsa")
			continue;

		blob >> e >> n;

		result = true;
		PRINT(("++ returning identity %s", outComment.c_str()));
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
