/*	$Id: MSshChannel.cpp,v 1.10 2004/03/05 20:43:26 maarten Exp $
	Copyright Maarten
	Created Sunday September 28 2003 11:38:57
*/

#include "MJapieG.h"

#include "MError.h"
#include "MSshChannel.h"
#include "MSshConnection.h"
#include "MSshConnectionPool.h"

using namespace std;

MSshChannel::MSshChannel(
	string		inIPAddress,
	string		inUserName,
	uint16		inPort)
	: eConnectionEvent(this, &MSshChannel::ConnectionEvent)
	, eConnectionMessage(this, &MSshChannel::ConnectionMessage)
{
	MSshConnection* connection =
		MSshConnectionPool::Instance().Get(inIPAddress, inUserName, inPort);

	if (connection == nil)
		throw MError("Could not open connection");

	AddRoute(eConnectionEvent, connection->eConnectionEvent);
	AddRoute(eConnectionMessage, connection->eConnectionMessage);

	connection->OpenChannel(this);
}

MSshChannel::MSshChannel(
	MSshConnection&		inConnection)
	: eConnectionEvent(this, &MSshChannel::ConnectionEvent)
	, eConnectionMessage(this, &MSshChannel::ConnectionMessage)
{
	AddRoute(eConnectionEvent, inConnection.eConnectionEvent);
	AddRoute(eConnectionMessage, inConnection.eConnectionMessage);
}

MSshChannel::~MSshChannel()
{
	try
	{
		MSshConnection* connection =
			MSshConnectionPool::Instance().Get(this);
		if (connection != nil)
			connection->CloseChannel(this);
	}
	catch (...) {}
}

void MSshChannel::Send(
	string		inData)
{
	MSshConnection* connection =
		MSshConnectionPool::Instance().Get(this);

	assert(connection);

	if (connection != nil)
		connection->SendChannelData(this, 0, inData);
}

void MSshChannel::SendExtra(
	uint32		inType,
	string		inData)
{
	MSshConnection* connection =
		MSshConnectionPool::Instance().Get(this);

	assert(connection);

	if (connection != nil)
		connection->SendChannelData(this, inType, inData);
}

uint32 MSshChannel::GetMaxPacketSize() const
{
	uint32 result = 1024;

	MSshConnection* connection =
		MSshConnectionPool::Instance().Get(this);

	assert(connection);

	if (connection != nil)
		result = connection->GetMaxPacketSize(this);

	return result;
}

void MSshChannel::ResetTimer()
{
	MSshConnection* connection =
		MSshConnectionPool::Instance().Get(this);

	assert(connection);

	if (connection != nil)
		connection->ResetTimer();
}

string MSshChannel::GetEncryptionParams() const
{
	string result;
	
	MSshConnection* connection =
		MSshConnectionPool::Instance().Get(this);

	assert(connection);

	if (connection != nil)
		result = connection->GetEncryptionParams();
	
	return result;
}

void MSshChannel::ConnectionEvent(
	int		inEvent)
{
	eChannelEvent(inEvent);
}

void MSshChannel::ConnectionMessage(
	string	inMessage)
{
	eChannelMessage(inMessage);
}
