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
	uint32					GetMyChannelID() const		{ return fMyChannelID; }
	
	void					SetMyChannelID(
								uint32	inChannelID)	{ fMyChannelID = inChannelID; }

	uint32					GetHostChannelID() const	{ return fHostChannelID; }
	
	void					SetHostChannelID(
								uint32	inChannelID)	{ fHostChannelID = inChannelID; }

	uint32					GetMaxSendPacketSize() const{ return fMaxSendPacketSize; }
	
	void					SetMaxSendPacketSize(
								uint32	inSize)			{ fMaxSendPacketSize = inSize; }

	uint32					GetMyWindowSize() const		{ return fMyWindowSize; }
	
	void					SetMyWindowSize(
								uint32	inSize)			{ fMyWindowSize = inSize; }

	uint32					GetHostWindowSize() const	{ return fHostWindowSize; }
	
	void					SetHostWindowSize(
								uint32	inSize)			{ fHostWindowSize = inSize; }

	bool					IsChannelOpen() const		{ return fChannelOpen; }
	
	void					SetChannelOpen(
								bool	inChannelOpen);

	bool					PopPending(
								std::string&	outData);
	
	void					PushPending(
								const std::string&	inData);

	void					Close();

	MCallback<void(int)>	eChannelEvent;		// events in the enum range above
	MCallback<void(std::string)>
							eChannelMessage;	// for error strings and such
	MCallback<void(std::string)>
							eChannelBanner;		// sent by the authentication protocol

	MEventIn<void(int)>		eConnectionEvent;
	MEventIn<void(std::string)>
							eConnectionMessage;

	// these are called by the connection class:
	virtual void			HandleData(
								std::string		inData) = 0;

	virtual void			HandleExtraData(
								int				inType,
								std::string		inData) = 0;

	virtual void			HandleChannelEvent(
								int				inEventMessage);
	
	// Override to finish creating the channel
	virtual const char*		GetRequest() const = 0;
	virtual const char*		GetCommand() const = 0;
	virtual bool			WantPTY() const				{ return false; }
	
	void					ResetTimer();

	// To send data through the channel using SSH_MSG_CHANNEL_DATA messages
	virtual void			Send(
								std::string		inData);

	virtual void			SendExtra(
								uint32			inType,
								std::string		inData);
	
	std::string				GetEncryptionParams() const;
	
  protected:
							MSshChannel(
								std::string		inIPAddress,
								std::string		inUserName,
								uint16			inPort);

	virtual					~MSshChannel();

	void					ConnectionEvent(
								int				inEvent);

	void					ConnectionMessage(
								std::string		inMessage);
	
  private:

	MSshConnection*			fConnection;
	uint32					fMyChannelID;
	uint32					fHostChannelID;
	uint32					fMaxSendPacketSize;
	uint32					fMyWindowSize;
	uint32					fHostWindowSize;
	bool					fChannelOpen;
	std::deque<std::string>	fPending;
};

#endif // MSSHCHANNEL_H
