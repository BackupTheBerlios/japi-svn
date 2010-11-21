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
struct MSshPacket;
struct ZLibHelper;

class MSshConnection
{
	typedef std::vector<MSshChannel*>		ChannelList;
	
  public:
	
	static MSshConnection*
					Get(
						const std::string&	inIPAddress,
						const std::string&	inUserName,
						uint16				inPort);

	void			Disconnect();
	
	MSshChannel*	OpenChannel();

	void			Reference();
	void			Release();

//	std::string		UserName() const		{ return mUserName; }
//	std::string		IPAddress() const		{ return mIPAddress; }
//	uint16			PortNumber() const		{ return mPortNumber; }

	std::string		GetEncryptionParams() const;
	
	MEventOut<void(int)>				eConnectionEvent;
	MEventOut<void(const std::string&)>	eConnectionMessage;
	MEventOut<void(const std::string&)>	eConnectionBanner;
	
  private:

					MSshConnection(
						const std::string&	inIPAddress,
						const std::string&	inUserName,
						uint16				inPort);

					MSshConnection(
						const MSshConnection&);
	MSshConnection&	operator=(
						const MSshConnection&);

	virtual 		~MSshConnection();

	std::string		ChooseProtocol(
						const std::string&	inServer,
						const std::string&	inClient) const;

	void			DeriveKey(
						char*				inHash,
						int					inNr,
						int					inLength,
						byte*&				outKey);
	
	void			AdjustMyWindowSize(
						int32				inDelta);

	void			AdjustHostWindowSize(
						int32				inDelta);

//	void			Send(
//						std::string			inMessage);

	void			Error(
						int					inReason);

	bool			ProcessBuffer();
	void			ProcessPacket();

	// protocol handlers
	typedef void (MSshConnection::*ASIOHandler)(const boost::system::error_code&);

	void			Send(
						MSshPacket&			inMessage,
						ASIOHandler			inHandler = nil);

	void			HandleResolve(
						const boost::system::error_code& err,
						boost::asio::ip::tcp::resolver::iterator endpoint_iterator);
	void			HandleConnect(
						const boost::system::error_code& err,
						boost::asio::ip::tcp::resolver::iterator endpoint_iterator);
	void			HandleProtocolVersionExchange(
						const boost::system::error_code& err);
	void			HandleKexInitRequest(
						const boost::system::error_code& err);
	void			HandleKexInitResponse(
						const boost::system::error_code& err);
	void			HandleDataRequest(
						const boost::system::error_code& err);
	
	void			(MSshConnection::*mHandler)(
						uint8				inMessage,
						MSshPacket&			in,
						MSshPacket&			out);
	
	void			ProcessConnect();

	void			ProcessDisconnect(
						uint8				inMessage,
						MSshPacket&			in,
						MSshPacket&			out);

	void			ProcessKexInit(
						uint8				inMessage,
						MSshPacket&			in,
						MSshPacket&			out);

	void			ProcessKexdhReply(
						uint8				inMessage,
						MSshPacket&			in,
						MSshPacket&			out);

	void			ProcessNewKeys(
						uint8				inMessage,
						MSshPacket&			in,
						MSshPacket&			out);

	void			ProcessKeybInteract(
						uint8				inMessage,
						MSshPacket&			in,
						MSshPacket&			out);

	void			ProcessUserAuthInit(
						uint8				inMessage,
						MSshPacket&			in,
						MSshPacket&			out);

	void			ProcessUserAuthNone(
						uint8				inMessage,
						MSshPacket&			in,
						MSshPacket&			out);

	void			ProcessUserAuthPassword(
						uint8				inMessage,
						MSshPacket&			in,
						MSshPacket&			out);

	void			ProcessUserAuthKeyboardInteractive(
						uint8				inMessage,
						MSshPacket&			in,
						MSshPacket&			out);

	void			ProcessUserAuthPublicKey(
						uint8				inMessage,
						MSshPacket&			in,
						MSshPacket&			out);

	void			ProcessUserAuthSuccess(
						uint8				inMessage,
						MSshPacket&			in,
						MSshPacket&			out);

	void			ProcessChannelRequest(
						uint8				inMessage,
						MSshPacket&			in,
						MSshPacket&			out);

	void			TryNextIdentity();

	void			TryPassword();

	void			UserAuthSuccess();

	void			UserAuthFailed();

	void			ProcessConfirmChannel(
						uint8				inMessage,
						MSshPacket&			in,
						MSshPacket&			out);

	void			ProcessConfirmPTY(
						uint8				inMessage,
						MSshPacket&			in,
						MSshPacket&			out);

	void			ProcessChannel(
						uint8				inMessage,
						MSshPacket&			in,
						MSshPacket&			out);

	void			ProcessDebug(
						uint8				inMessage,
						MSshPacket&			in,
						MSshPacket&			out);

	void			ProcessUnimplemented(
						uint8				inMessage,
						MSshPacket&			in,
						MSshPacket&			out);
	
//	void			ProcessChannelOpen(
//						uint8				inMessage,
//						MSshPacket&			in,
//						MSshPacket&			out);

	MEventIn<void(std::vector<std::string>)>	eRecvAuthInfo;

	void			RecvAuthInfo(
						std::vector<std::string>
											inAuthInfo);

	MEventIn<void(std::vector<std::string>)>	eRecvPassword;
	
	void			RecvPassword(
						std::vector<std::string>
											inPassword);

	std::string					mUserName;
	std::string					mPassword;
	std::string					mIPAddress;
	uint16						mPortNumber;
	boost::asio::ip::tcp::resolver
								mResolver;
	boost::asio::ip::tcp::socket
								mSocket;
	boost::asio::streambuf		mRequest;
	boost::asio::streambuf		mResponse;
	uint32						mPasswordAttempts;

	std::unique_ptr<CryptoPP::BlockCipher>					mDecryptorCipher;
	std::unique_ptr<CryptoPP::StreamTransformation>			mDecryptorCBC;
	std::unique_ptr<CryptoPP::BlockCipher>					mEncryptorCipher;
	std::unique_ptr<CryptoPP::StreamTransformation>			mEncryptorCBC;
	std::unique_ptr<CryptoPP::MessageAuthenticationCode>	mSigner;
	std::unique_ptr<CryptoPP::MessageAuthenticationCode>	mVerifier;
//	std::unique_ptr<ZLibHelper>								mCompressor;
//	std::unique_ptr<ZLibHelper>								mDecompressor;
	
	CryptoPP::Integer			f_x;
	CryptoPP::Integer			f_e;
	std::string					mSessionId;
	CryptoPP::Integer			mSharedSecret;
	std::string					mHostVersion;
	std::string					mMyPayLoad;
	std::string					mHostPayLoad;
	std::string					mMyKexinitMessage;
	std::string					mHostKexinitMessage;

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
	
	bool						mAuthenticated;

//	std::unique_ptr<MCertificate>	mCertificate;
	std::unique_ptr<MSshAgent>	mSshAgent;

	MEventIn<void(MCertificate*)>	eCertificateDeleted;

	void						CertificateDeleted(
									MCertificate*	inCertificate);
	
	int32						mRefCount;
	ChannelList					mChannels;
	ChannelList					mOpeningChannels;
	std::string					mErrString;
	uint32						mErrCode;
	double						mOpenedAt;
	static uint32				sNextChannelId;

	static MSshConnection*		sFirstConnection;
	MSshConnection*				mNext;
};

#endif // MSSHCONNECTION_H
