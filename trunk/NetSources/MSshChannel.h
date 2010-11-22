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
	
	void					SetMyChannelID(
								uint32	inChannelID)	{ mMyChannelID = inChannelID; }

	uint32					GetHostChannelID() const	{ return mHostChannelID; }
	
	void					SetHostChannelID(
								uint32	inChannelID)	{ mHostChannelID = inChannelID; }

	uint32					GetMaxSendPacketSize() const{ return mMaxSendPacketSize; }
	
	void					SetMaxSendPacketSize(
								uint32	inSize)			{ mMaxSendPacketSize = inSize; }

	uint32					GetMyWindowSize() const		{ return mMyWindowSize; }
	
	void					SetMyWindowSize(
								uint32	inSize)			{ mMyWindowSize = inSize; }

	uint32					GetHostWindowSize() const	{ return mHostWindowSize; }
	
	void					SetHostWindowSize(
								uint32	inSize)			{ mHostWindowSize = inSize; }

	bool					IsChannelOpen() const		{ return mChannelOpen; }
	
	void					SetChannelOpen(
								bool	inChannelOpen);

	bool					PopPending(
								MSshPacket& outData);
	
	void					PushPending(
								const MSshPacket& inData);

	void					Close();

	MCallback<void(int)>	eChannelEvent;		// events in the enum range above
	MCallback<void(const std::string&)>
							eChannelMessage;	// for error strings and such
	MCallback<void(const std::string&)>
							eChannelBanner;		// sent by the authentication protocol

	MEventIn<void(int)>		eConnectionEvent;
	MEventIn<void(const std::string&)>
							eConnectionMessage;

	// these are called by the connection class:
	virtual void			HandleData(
								MSshPacket&		inData) = 0;

	virtual void			HandleExtraData(
								int				inType,
								MSshPacket&		inData) = 0;

	virtual void			HandleChannelEvent(
								int				inEventMessage);
	
	// Override to finish creating the channel
	virtual const char*		GetRequest() const = 0;
	virtual const char*		GetCommand() const = 0;
	virtual bool			WantPTY() const				{ return false; }
	
	void					ResetTimer();

	// To send data through the channel using SSH_MSG_CHANNEL_DATA messages
	virtual void			Send(
								MSshPacket&		inData,
								uint32			inType = 0);

	void					SendWindowResize(
								uint32			inColumns,
								uint32			inRows);
	
	std::string				GetEncryptionParams() const;
	
  protected:
							MSshChannel(
								MSshConnection&	inConnection);

	virtual					~MSshChannel();

	void					ConnectionEvent(
								int				inEvent);

	void					ConnectionMessage(
								const std::string&
												inMessage);
	
  private:

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
