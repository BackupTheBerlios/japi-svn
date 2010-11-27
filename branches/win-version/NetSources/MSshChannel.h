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
#include "MSshPacket.h"

#include <deque>

#undef Success
#undef Failure

class MSshConnection;

// channel defaults

const uint32
	kMaxPacketSize = 0x8000,
	kWindowSize = 4 * kMaxPacketSize;

class MSshChannel
{
  public:
	uint32					GetMyChannelID() const		{ return mMyChannelID; }
	bool					IsChannelOpen() const		{ return mChannelOpen; }

	virtual void			Open();
	virtual void			Opened();

	virtual void			Close();
	virtual void			Closed();
	
	bool					IsOpen() const				{ return mChannelOpen; }

	virtual void			ChannelMessage(const std::string&);	// for status messages
	virtual void			ChannelError(const std::string&);	// for error messages
	virtual void			ChannelBanner(const std::string&);	// sent by the authentication protocol

	std::string				GetEncryptionParams() const;
	
  protected:
	friend class MSshConnection;

							MSshChannel(
								MSshConnection&	inConnection);

	virtual					~MSshChannel();

	virtual void			GetRequestAndCommand(
								std::string&		outRequest,
								std::string&		outCommand) const = 0;

	virtual void			Opened(
								MSshPacket&			in);

	// To send data through the channel using SSH_MSG_CHANNEL_DATA messages
	virtual void			SendData(
								MSshPacket&			inData);

	virtual void			SendExtendedData(
								MSshPacket&			inData,
								uint32				inType);

	virtual void			ReceiveData(
								MSshPacket&			inData);

	virtual void			ReceiveExtendedData(
								MSshPacket&			inData,
								uint32				inType);

	virtual void			Process(
								uint8				inMessage,
								MSshPacket&			in);

	bool					PopPending(
								MSshPacket&			outData);
	
	void					PushPending(
								const MSshPacket&	inData);

	void					SendWindowResize(
								uint32				inColumns,
								uint32				inRows);
	
	virtual void			HandleChannelRequest(
								const std::string&	inRequest,
								MSshPacket&			in,
								MSshPacket&			out);

	void					ConnectionMessage(
								const std::string&	inMessage);

	MEventIn<void(const std::string&)>
							eConnectionMessage;
	
  protected:
	uint32					mMaxSendPacketSize;
	bool					mChannelOpen;

  private:
	  
	static uint32			sNextChannelId;

	MSshConnection&			mConnection;
	uint32					mMyChannelID;
	uint32					mHostChannelID;
	uint32					mMyWindowSize;
	uint32					mHostWindowSize;
	std::deque<MSshPacket>	mPending;
};

#endif // MSSHCHANNEL_H
