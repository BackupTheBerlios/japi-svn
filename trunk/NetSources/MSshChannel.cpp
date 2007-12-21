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
{
	fConnection = MSshConnection::Get(inIPAddress, inUserName, inPort);
	
	if (fConnection == nil)
		THROW(("Could not open connection"));

	AddRoute(eConnectionEvent, fConnection->eConnectionEvent);
	AddRoute(eConnectionMessage, fConnection->eConnectionMessage);

	fConnection->OpenChannel(this);
}

MSshChannel::~MSshChannel()
{
	try
	{
		if (fConnection != nil)
		{
			fConnection->CloseChannel(this);
			fConnection->Release();
		}
	}
	catch (...) {}
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

uint32 MSshChannel::GetMaxPacketSize() const
{
	uint32 result = 1024;

	assert(fConnection);

	if (fConnection != nil)
		result = fConnection->GetMaxPacketSize(this);

	return result;
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


