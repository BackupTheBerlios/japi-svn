/*	$Id: MSshTerminalChannel.cpp,v 1.4 2004/01/12 20:46:41 maarten Exp $
	Copyright Maarten
	Created Sunday September 28 2003 11:39:23
	
	Implementation of the Version 3 Secure File Transfer Protocol
*/

#include "MJapieG.h"

#include "MError.h"
#include "MSshUtil.h"
#include "MSshTerminalChannel.h"
#include "MSshConnectionPool.h"
#include "MSshConnection.h"

using namespace std;

MSshTerminalChannel::MSshTerminalChannel(
	string		inIPAddress,
	string		inUserName,
	uint16		inPort)
	: MSshChannel(inIPAddress, inUserName, inPort)
	, eChannelEventIn(this, &MSshTerminalChannel::ChannelEvent)
{
	AddRoute(eChannelEventIn, eChannelEvent);
}

MSshTerminalChannel::~MSshTerminalChannel()
{
}

void MSshTerminalChannel::ChannelEvent(
	int			inEvent)
{
	switch (inEvent)
	{
		case SSH_CHANNEL_OPENED:
			break;
		
		case SSH_CHANNEL_SUCCESS:
			break;
	}
}

void MSshTerminalChannel::MandleData(
	string		inData)
{
	eData(inData, this);
}
	
void MSshTerminalChannel::MandleExtraData(
	int			inType,
	string		inData)
{
	eData(inData, this);
}

void MSshTerminalChannel::SendWindowResize(
	int			inColumns,
	int			inRows)
{
	MSshConnection* connection =
		MSshConnectionPool::Instance().Get(this);

	assert(connection);

	if (connection != nil)
		connection->SendWindowResize(this, inColumns, inRows);
}
