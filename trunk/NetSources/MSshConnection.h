//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

/*	$Id: MSshConnection.h,v 1.21 2004/02/28 22:28:13 maarten Exp $
	Copyright Maarten
	Created Sunday September 28 2003 10:53:43
*/

#ifndef MSSHCONNECTION_H
#define MSSHCONNECTION_H

#include <vector>
#include <deque>

#include "MP2PEvents.h"
#include "MUtils.h"

#include <boost/asio.hpp>

#include <cryptopp/cryptlib.h>
#include <cryptopp/integer.h>
#include <cryptopp/modes.h>

class MSshChannel;
class MCertificate;
class MSshAgent;
class MSshPacket;
class MSshPacketCompressor;
class MSshPacketDecompressor;

class MSshConnection
{
	typedef std::vector<MSshChannel*>		ChannelList;
	
  public:
	
	static MSshConnection*
					Get(
						const std::string&	inIPAddress,
						const std::string&	inUserName,
						uint16				inPort);

	void			Connect();
	void			Disconnect();
	
	bool			IsConnected() const					{ return mConnected; }
	
	void			OpenChannel(
						MSshChannel*		inChannel,
						const uint32		inChannelID);
	void			CloseChannel(
						MSshChannel*		inChannel,
						const uint32		inChannelID);

	std::string		GetEncryptionParams() const;
	
	MEventOut<void(const std::string&)>	eConnectionMessage;
	MEventOut<void(const std::string&)>	eConnectionBanner;

	void			Error(
						uint32				inReason,
						const std::string&	inMessage);
	
  private:

	friend class MSshChannel;

					MSshConnection(
						const std::string&	inIPAddress,
						const std::string&	inUserName,
						uint16				inPort);

	virtual 		~MSshConnection();

	std::string		ChooseProtocol(
						const std::string&	inServer,
						const std::string&	inClient) const;

	void			DeriveKey(
						const CryptoPP::Integer&
											inSharedSecret,
						byte*				inHash,
						int					inNr,
						int					inLength,
						byte*&				outKey);
	
	// Network IO functions, we're using async boost::asio calls
	void			HandleResolve(
						const boost::system::error_code& err,
						boost::asio::ip::tcp::resolver::iterator endpoint_iterator);
	void			HandleConnect(
						const boost::system::error_code& err,
						boost::asio::ip::tcp::resolver::iterator endpoint_iterator);
	void			HandleProtocolVersionExchangeRequest(
						const boost::system::error_code& err);
	void			HandleProtocolVersionExchangeResponse(
						const boost::system::error_code& err);

	// To send a MSshPacket
	void			Send(
						MSshPacket&			inMessage);
	void			PacketSent(
						const boost::system::error_code& err);

	// Receive is some kind of eventloop, it receives packets from the
	// network and passes them on to ProcessPacket
	void			Receive(
						const boost::system::error_code& err);

	void			ProcessPacket(
						uint8				inMessage,
						MSshPacket&			in);

	void			ProcessDebug(
						MSshPacket&			in);

	void			ProcessUnimplemented(
						MSshPacket&			in);
	
	void			ProcessDisconnect(
						MSshPacket&			in);

	void			ProcessServiceAccept(
						MSshPacket&			in);

	void			ProcessKexInit(
						MSshPacket&			in);

	void			ProcessKexdhReply(
						MSshPacket&			in);

	void			ProcessNewKeys(
						MSshPacket&			in);

	void			ProcessUserAuthSuccess(
						MSshPacket&			in);

	void			ProcessUserAuthFailed(
						MSshPacket&			in);
	
	void			ProcessUserAuthInfoRequest(
						MSshPacket&			in);

	enum MAuthenticationState {
		SSH_AUTH_STATE_NONE,
		SSH_AUTH_STATE_PUBLIC_KEY,
		SSH_AUTH_STATE_KEYBOARD_INTERACTIVE,
		SSH_AUTH_STATE_PASSWORD
	};

	void			ProcessChannelOpen(
						MSshPacket&			in);

	void			ProcessChannelOpenConfirmation(
						MSshPacket&			in);

	void			ProcessChannelRequest(
						MSshPacket&			in);

	void			ProcessChannel(
						MSshPacket&			in);

	MEventIn<void(std::vector<std::string>&)>	eRecvAuthInfo;
	void			RecvAuthInfo(
						std::vector<std::string>&
											inAuthInfo);

	MEventIn<void(std::vector<std::string>&)>	eRecvPassword;
	void			RecvPassword(
						std::vector<std::string>&
											inPassword);

	MEventIn<void(MCertificate*)>				eCertificateDeleted;
	void			CertificateDeleted(
						MCertificate*		inCertificate);

	std::string					mUserName;
	std::string					mIPAddress;
	uint16						mPortNumber;

	bool						mConnected, mAuthenticated;
	uint32						mPasswordAttempts;
	MAuthenticationState		mAuthenticationState;

	boost::asio::ip::tcp::resolver
								mResolver;
	boost::asio::ip::tcp::socket
								mSocket;
	std::deque<boost::asio::streambuf*>
								mRequests;
	boost::asio::streambuf		mResponse;
	std::deque<MSshPacket>		mPending;
	std::vector<byte>			mPacket;
	uint32						mPacketLength;

	std::unique_ptr<CryptoPP::BlockCipher>					mDecryptorCipher;
	std::unique_ptr<CryptoPP::StreamTransformation>			mDecryptorCBC;
	std::unique_ptr<CryptoPP::BlockCipher>					mEncryptorCipher;
	std::unique_ptr<CryptoPP::StreamTransformation>			mEncryptorCBC;
	std::unique_ptr<CryptoPP::MessageAuthenticationCode>	mSigner;
	std::unique_ptr<CryptoPP::MessageAuthenticationCode>	mVerifier;
	std::unique_ptr<MSshPacketCompressor>					mCompressor;
	std::unique_ptr<MSshPacketDecompressor>					mDecompressor;

	bool						mDelayedCompress, mDelayedDecompress;
	
	CryptoPP::Integer			m_x;
	CryptoPP::Integer			m_e;
	std::vector<byte>			mSessionId;
	std::string					mHostVersion;
	std::string					mMyPayLoad;
	std::string					mHostPayLoad;

	std::string					mKexAlg;
	std::string					mServerHostKeyAlg;
	std::string					mEncryptionAlgC2S;
	std::string					mEncryptionAlgS2C;
	std::string					mMACAlgC2S;
	std::string					mMACAlgS2C;
	std::string					mCompressionAlgC2S;
	std::string					mCompressionAlgS2C;
	std::string					mLangC2S;
	std::string					mLangS2C;
	
	byte*						mKeys[6];
	
	uint32						mOutSequenceNr;
	uint32						mInSequenceNr;

//	std::unique_ptr<MCertificate>
//								mCertificate;
	std::unique_ptr<MSshAgent>	mSshAgent;
	ChannelList					mChannels;

	static boost::asio::io_service&
								GetIOService();

	friend class MIdleHandler;
	static void					Idle(double);

	static std::list<MSshConnection*>
								sConnectionList;

  private:
					MSshConnection(
						const MSshConnection&);
	MSshConnection&	operator=(
						const MSshConnection&);
};

#endif // MSSHCONNECTION_H
