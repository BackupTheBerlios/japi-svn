//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

/*	$Id: MSshChannel.cpp,v 1.10 2004/03/05 20:43:26 maarten Exp $
	Copyright Maarten
	Created Sunday September 28 2003 11:38:57
*/

#include "MLib.h"

#include "MError.h"
#include "MStrings.h"

#include "MSsh.h"
#include "MSshChannel.h"
#include "MSshConnection.h"

using namespace std;

uint32 MSshChannel::sNextChannelId = 1;

MSshChannel::MSshChannel(
	MSshConnection&	inConnection)
	: eConnectionMessage(this, &MSshChannel::ConnectionMessage)
	, mConnection(inConnection)
	, mMyChannelID(sNextChannelId++)
	, mHostChannelID(0)
	, mMaxSendPacketSize(0)
	, mMyWindowSize(kWindowSize)
	, mHostWindowSize(0)
	, mChannelOpen(false)
{
}

MSshChannel::~MSshChannel()
{
	PRINT(("Deleting SSH channel %d", mMyChannelID));
}

void MSshChannel::Open()
{
	mMyWindowSize = kWindowSize;
	mMyChannelID = sNextChannelId++;
	mConnection.OpenChannel(this, mMyChannelID);
}

void MSshChannel::Opened()
{
	mChannelOpen = true;
}

void MSshChannel::Close()
{
	ChannelMessage(_("Channel closed"));
	PRINT(("Closing SSH channel %d", mMyChannelID));
	mConnection.CloseChannel(this, mHostChannelID);
}

void MSshChannel::Closed()
{
	mChannelOpen = false;
}

void MSshChannel::Opened(
	MSshPacket&	in)
{
	in >> mHostChannelID >> mHostWindowSize >> mMaxSendPacketSize;

//			if (WantPTY())
//			{
//				MSshPacket p;
//				
//				p << uint8(SSH_MSG_CHANNEL_REQUEST)
//					<< mHostChannelID
//					<< "pty-req"
//					<< true
//					<< "vt100"
//					<< uint32(80) << uint32(24)
//					<< uint32(0) << uint32(0)
//					<< "";
//				
//				Send(p);
//				
//				mHandler = &MSshConnection::ProcessConfirmPTY;
//			}

	string request, command;
	GetRequestAndCommand(request, command);

	MSshPacket out;
	out << uint8(SSH_MSG_CHANNEL_REQUEST)
		<< mHostChannelID
		<< request
		<< true;
	
	if (not command.empty())
		out << command;

	mConnection.Send(out);
}

void MSshChannel::Process(
	uint8		inMessage,
	MSshPacket&	in)
{
PRINT(("Channel message %d for channel %d", inMessage, mMyChannelID));

	switch (inMessage)
	{
		case SSH_MSG_CHANNEL_WINDOW_ADJUST:
		{
			int32 extra;
			in >> extra;
			mHostWindowSize += extra;
			break;
		}

		case SSH_MSG_CHANNEL_SUCCESS:
			Opened();
			break;
		
		case SSH_MSG_CHANNEL_DATA:
		{
			MSshPacket data;
			in >> data;
			mMyWindowSize -= data.size();
			ReceiveData(data);
			break;
		}

		case SSH_MSG_CHANNEL_EXTENDED_DATA:
		{
			MSshPacket data;
			uint32 type;
			in >> type >> data;
			mMyWindowSize -= data.size();
			ReceiveExtendedData(data, type);
			break;
		}
		
		//case SSH_MSG_CHANNEL_SUCCESS:
		//	HandleChannelEvent(SSH_CHANNEL_SUCCESS);
		//	break;

		//case SSH_MSG_CHANNEL_FAILURE:
		//	HandleChannelEvent(SSH_CHANNEL_FAILURE);
		//	break;

		case SSH_MSG_CHANNEL_REQUEST:
		{
			string request;
			bool wantReply;
			
			in >> request >> wantReply;

			MSshPacket out;
			HandleChannelRequest(request, in, out);
			
			if (wantReply)
			{
				if (out.empty())
					out << uint8(SSH_MSG_CHANNEL_FAILURE) << mHostChannelID;
				mConnection.Send(out);
			}
			break;
		}
	}

	if (mChannelOpen and mMyWindowSize < kWindowSize - 2 * kMaxPacketSize)
	{
		MSshPacket out;
		uint32 adjust = kWindowSize - mMyWindowSize;
		out << uint8(SSH_MSG_CHANNEL_WINDOW_ADJUST) << mHostChannelID << adjust;
		mMyWindowSize += adjust;
		mConnection.Send(out);
	}
}

void MSshChannel::SendData(
	MSshPacket&		inData)
{
	assert(inData.size() < mMaxSendPacketSize);

	MSshPacket p;
	p << uint8(SSH_MSG_CHANNEL_DATA) << mHostChannelID << inData;
	PushPending(p);
}

void MSshChannel::SendExtendedData(
	MSshPacket&		inData,
	uint32			inType)
{
	assert(inData.size() < mMaxSendPacketSize);

	MSshPacket p;
	p << uint8(SSH_MSG_CHANNEL_EXTENDED_DATA) << mHostChannelID
		<< inType << inData;
	PushPending(p);
}

void MSshChannel::SendWindowResize(uint32 inColumns, uint32 inRows)
{
	MSshPacket p;
	
	p << uint8(SSH_MSG_CHANNEL_REQUEST) << mHostChannelID
		<< "window-change" << false
		<< inColumns << inRows
		<< uint32(0) << uint32(0);

	mConnection.Send(p);
}

string MSshChannel::GetEncryptionParams() const
{
	return mConnection.GetEncryptionParams();
}

void MSshChannel::ConnectionMessage(
	const string&		inMessage)
{
	ChannelMessage(inMessage);
}

void MSshChannel::ChannelMessage(const string& inMessage)
{
}

void MSshChannel::ChannelError(const string& inError)
{
}

void MSshChannel::ChannelBanner(const string& inBanner)
{
}

void MSshChannel::HandleChannelRequest(
	const string&		inRequest,
	MSshPacket&			in,
	MSshPacket&			out)
{
}

void MSshChannel::ReceiveData(
	MSshPacket&			in)
{
}

void MSshChannel::ReceiveExtendedData(
	MSshPacket&			in,
	uint32				inType)
{
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
