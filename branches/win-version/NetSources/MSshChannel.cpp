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
	mConnection.OpenChannel(this);
}

MSshChannel::~MSshChannel()
{
	try
	{
		Close();
	}
	catch (...) {}
}

void MSshChannel::Open()
{
	MSshPacket out;
	out << uint8(SSH_MSG_CHANNEL_OPEN) << "session"
		<< mMyChannelID << mMyWindowSize << kMaxPacketSize;
	mConnection.Send(out);
}

void MSshChannel::Close()
{
	MSshPacket out;
	out << uint8(SSH_MSG_CHANNEL_CLOSE) << mHostChannelID;
	mConnection.Send(out);
}

void MSshChannel::ConnectionOpened()
{
	
}

void MSshChannel::ConnectionClosed()
{
	
}

void MSshChannel::Process(
	uint8		inMessage,
	MSshPacket&	in)
{
PRINT(("Channel message %d for channel %d", inMessage, mMyChannelID));

	switch (inMessage)
	{
		case SSH_MSG_CHANNEL_OPEN_CONFIRMATION:
		{
			in >> mHostChannelID >> mHostWindowSize >> mMaxSendPacketSize;
			
			mChannelOpen = true;
			
			HandleChannelEvent(SSH_CHANNEL_OPENED);
			ConnectionMessage(_("Connecting…"));
			
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
			break;
		}
		
		case SSH_MSG_CHANNEL_OPEN_FAILURE:
		{
			uint32 errCode;
			string errString;

			in >> errCode >> errString;
			mConnection.Error(errCode, errString);
			break;
		}
			
		case SSH_MSG_CHANNEL_WINDOW_ADJUST:
		{
			int32 extra;
			in >> extra;
			mHostWindowSize += extra;
			break;
		}
		
		case SSH_MSG_CHANNEL_DATA:
		{
			MSshPacket data;
			in >> data;
			mMyWindowSize -= data.size();
			Receive(data);
			break;
		}

		case SSH_MSG_CHANNEL_EXTENDED_DATA:
		{
			MSshPacket data;
			uint32 type;
			in >> type >> data;
			mMyWindowSize -= data.size();
			Receive(data, type);
			break;
		}
		
		case SSH_MSG_CHANNEL_CLOSE:
			HandleChannelEvent(SSH_CHANNEL_CLOSED);
			mChannelOpen = false;
			break;
		
		case SSH_MSG_CHANNEL_SUCCESS:
			HandleChannelEvent(SSH_CHANNEL_SUCCESS);
			break;

		case SSH_MSG_CHANNEL_FAILURE:
			HandleChannelEvent(SSH_CHANNEL_FAILURE);
			break;

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

void MSshChannel::Send(
	MSshPacket&		inData,
	uint32			inType)
{
	assert(inData.size() < mMaxSendPacketSize);

	MSshPacket p;
	if (inType == 0)
		p << uint8(SSH_MSG_CHANNEL_DATA) << mHostChannelID << inData;
	else
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
	eChannelMessage(inMessage);
}

void MSshChannel::HandleChannelRequest(
	const string&		inRequest,
	MSshPacket&			in,
	MSshPacket&			out)
{
#if DEBUG
	cerr << "HandleChannelRequest " << inRequest << endl;
	if (not in.empty())
		HexDump(in.peek(), in.size(), cerr);
#endif
}

void MSshChannel::HandleChannelEvent(
	uint32				inEvent)
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
