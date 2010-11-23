//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

/*	$Id: MSshChannel.h,v 1.14 2004/03/05 20:43:26 maarten Exp $
	Copyright Maarten
	Created Sunday September 28 2003 10:52:44
*/

#ifndef MSSHCHANNEL_H
#define MSSHCHANNEL_H

#include "MP2PEvents.h"
#include "MCallbacks.h"

#include <deque>

#undef Success
#undef Failure

class MSshConnection;
struct MSshPacket;

// channel defaults

const uint32
	kMaxPacketSize = 0x8000,
	kWindowSize = 4 * kMaxPacketSize;

enum MSshChannelEvent {
	SSH_CHANNEL_OPENED,
	SSH_CHANNEL_CLOSED,
	SSH_CHANNEL_TIMEOUT,
	SSH_CHANNEL_USERAUTH_FAILURE,
	SSH_CHANNEL_ERROR,
	SSH_CHANNEL_SUCCESS,
	SSH_CHANNEL_FAILURE,
	SSH_CHANNEL_PACKET_DONE,	// sent when we've processed a packet
};

class MSshChannel
{
  public:
	uint32					GetMyChannelID() const		{ return mMyChannelID; }
	bool					IsChannelOpen() const		{ return mChannelOpen; }

	void					Open();
	void					Close();

	virtual void			ConnectionOpened();
	virtual void			ConnectionClosed();
	
	virtual void			Process(
								uint8				inMessage,
								MSshPacket&			in);

	bool					PopPending(
								MSshPacket&			outData);
	
	void					PushPending(
								const MSshPacket&	inData);

	MCallback<void(uint32)>	eChannelEvent;		// events in the enum range above
	MCallback<void(const std::string&)>
							eChannelMessage;	// for error strings and such
	MCallback<void(const std::string&)>
							eChannelBanner;		// sent by the authentication protocol

	MEventIn<void(const std::string&)>
							eConnectionMessage;

	std::string				GetEncryptionParams() const;
	
  protected:
							MSshChannel(
								MSshConnection&	inConnection);

	virtual					~MSshChannel();

	virtual void			GetRequestAndCommand(
								std::string&		outRequest,
								std::string&		outCommand) const = 0;

	// To send data through the channel using SSH_MSG_CHANNEL_DATA messages
	virtual void			Send(
								MSshPacket&			inData,
								uint32				inType = 0);

	virtual void			Receive(
								MSshPacket&			inData,
								int					inType = 0) = 0;

	void					SendWindowResize(
								uint32				inColumns,
								uint32				inRows);
	
	virtual void			HandleChannelRequest(
								const std::string&	inRequest,
								MSshPacket&			in,
								MSshPacket&			out);

	virtual void			HandleChannelEvent(
								uint32				inEventMessage);

	void					ConnectionMessage(
								const std::string&	inMessage);
	
  protected:

	static uint32			sNextChannelId;

	MSshConnection&			mConnection;
	uint32					mMyChannelID;
	uint32					mHostChannelID;
	uint32					mMaxSendPacketSize;
	uint32					mMyWindowSize;
	uint32					mHostWindowSize;
	bool					mChannelOpen;
	std::deque<MSshPacket>	mPending;
};

#endif // MSSHCHANNEL_H
