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
#include "MSsh.h"
#include "MSshChannel.h"
#include "MSshConnection.h"

using namespace std;

MSshChannel::MSshChannel(
	MSshConnection&	inConnection)
	: eConnectionEvent(this, &MSshChannel::ConnectionEvent)
	, eConnectionMessage(this, &MSshChannel::ConnectionMessage)
	, mConnection(inConnection)
	, mMyChannelID(0)
	, mHostChannelID(0)
	, mMaxSendPacketSize(0)
	, mMyWindowSize(kWindowSize)
	, mHostWindowSize(0)
	, mChannelOpen(false)
{
//	mConnection.OpenChannel(this);
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
//	mConnection.CloseChannel(this);
}

void MSshChannel::SetChannelOpen(
	bool	inChannelOpen)
{
	mChannelOpen = inChannelOpen;
	
	if (not mChannelOpen)
	{
		mConnection.Release();
		delete this;
	}
}

void MSshChannel::Send(
	MSshPacket&		inData,
	uint32			inType)
{
	assert(inData.size() < GetMaxSendPacketSize());

	MSshPacket p;
	if (inType == 0)
		p << uint8(SSH_MSG_CHANNEL_DATA) << GetHostChannelID() << inData;
	else
		p << uint8(SSH_MSG_CHANNEL_EXTENDED_DATA) << GetHostChannelID()
			<< inType << inData;
		
	PushPending(p);
}

void MSshChannel::SendWindowResize(uint32 inColumns, uint32 inRows)
{
	MSshPacket p;
	
	p << uint8(SSH_MSG_CHANNEL_REQUEST) << GetHostChannelID()
		<< "window-change" << false
		<< inColumns << inRows
		<< uint32(0) << uint32(0);

	PushPending(p);
}

//void MSshChannel::ResetTimer()
//{
//	mConnection.ResetTimer();
//}

string MSshChannel::GetEncryptionParams() const
{
	return mConnection.GetEncryptionParams();
}

void MSshChannel::ConnectionEvent(
	int		inEvent)
{
	if (inEvent == SSH_CHANNEL_CLOSED)
		mConnection.Release();
	
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
	MSshPacket&	outData)
{
	bool result = false;

	if (mPending.size() > 0 and mPending.front().size() < mHostWindowSize)
	{
		result = true;
		outData = mPending.front();
		mPending.pop_front();
		mHostWindowSize -= outData.size();
	}
	
	return result;
}
	
void MSshChannel::PushPending(
	const MSshPacket& inData)
{
	mPending.push_back(inData);
}
