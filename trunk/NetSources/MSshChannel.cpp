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

/*	$Id: MSshChannel.cpp,v 1.10 2004/03/05 20:43:26 maarten Exp $
	Copyright Maarten
	Created Sunday September 28 2003 11:38:57
*/

#include "MJapieG.h"

#include "MError.h"
#include "MSshChannel.h"
#include "MSshConnection.h"

using namespace std;

MSshChannel::MSshChannel(
	string		inIPAddress,
	string		inUserName,
	uint16		inPort)
	: eConnectionEvent(this, &MSshChannel::ConnectionEvent)
	, eConnectionMessage(this, &MSshChannel::ConnectionMessage)
	, fMyChannelID(0)
	, fHostChannelID(0)
	, fMaxSendPacketSize(0)
	, fMyWindowSize(kWindowSize)
	, fHostWindowSize(0)
	, fChannelOpen(false)
{
	fConnection = MSshConnection::Get(inIPAddress, inUserName, inPort);
	
	if (fConnection == nil)
		THROW(("Could not open connection"));

	fConnection->OpenChannel(this);
}

MSshChannel::~MSshChannel()
{
	try
	{
		Close();
	}
	catch (...) {}
}

void MSshChannel::Close()
{
	if (fConnection != nil)
	{
		fConnection->CloseChannel(this);
		fConnection->Release();
		fConnection = nil;
	}

	fChannelOpen = false;
}

void MSshChannel::SetChannelOpen(
	bool	inChannelOpen)
{
	fChannelOpen = inChannelOpen;
	
	if (not fChannelOpen)
	{
		fConnection->Release();
		fConnection = nil;
	}
}

void MSshChannel::Send(
	string		inData)
{
	assert(fConnection != nil);

	if (fConnection != nil)
		fConnection->SendChannelData(this, 0, inData);
}

void MSshChannel::SendExtra(
	uint32		inType,
	string		inData)
{
	assert(fConnection);

	if (fConnection != nil)
		fConnection->SendChannelData(this, inType, inData);
}

void MSshChannel::ResetTimer()
{
	assert(fConnection);

	if (fConnection != nil)
		fConnection->ResetTimer();
}

string MSshChannel::GetEncryptionParams() const
{
	string result;
	
	assert(fConnection);

	if (fConnection != nil)
		result = fConnection->GetEncryptionParams();
	
	return result;
}

void MSshChannel::ConnectionEvent(
	int		inEvent)
{
	if (inEvent == SSH_CHANNEL_CLOSED)
	{
		fConnection->Release();
		fConnection = nil;
	}
	
	HandleChannelEvent(inEvent);
}

void MSshChannel::ConnectionMessage(
	string	inMessage)
{
	eChannelMessage(inMessage);
}

void MSshChannel::HandleChannelEvent(
	int		inEvent)
{
	eChannelEvent(inEvent);
}

bool MSshChannel::PopPending(
	string&	outData)
{
	bool result = false;

	if (fPending.size() > 0 and fPending.front().length() < fHostWindowSize)
	{
		result = true;
		outData = fPending.front();
		fPending.pop_front();
		fHostWindowSize -= outData.length();
	}
	
	return result;
}
	
void MSshChannel::PushPending(
	const string&	inData)
{
	fPending.push_back(inData);
}
