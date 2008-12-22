//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

/*	$Id: MSshChannel.cpp,v 1.10 2004/03/05 20:43:26 maarten Exp $
	Copyright Maarten
	Created Sunday September 28 2003 11:38:57
*/

#include "MJapi.h"

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
