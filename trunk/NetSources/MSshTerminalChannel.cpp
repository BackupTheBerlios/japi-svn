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

/*	$Id: MSshTerminalChannel.cpp,v 1.4 2004/01/12 20:46:41 maarten Exp $
	Copyright Maarten
	Created Sunday September 28 2003 11:39:23
	
	Implementation of the Version 3 Secure File Transfer Protocol
*/

#include "MJapi.h"

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
