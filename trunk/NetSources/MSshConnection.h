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

	void			Reference();
	void			Release();

	std::string		UserName() const		{ return fUserName; }
	std::string		IPAddress() const		{ return fIPAddress; }
	uint16			PortNumber() const		{ return fPortNumber; }
	double			OpenedAt() const		{ return fOpenedAt; }
	
	bool			IsConnected() const		{ return fIsConnected; }
	bool			Busy() const			{ return fBusy; }

	void			Disconnect();
	
	void			ResetTimer();
	
	std::string		GetEncryptionParams() const;
	
	void			OpenChannel(
						MSshChannel*		inChannel);

	void			CloseChannel(
						MSshChannel*		inChannel,
						bool				inErase);
						
	void			SendChannelData(
						MSshChannel*		inChannel,
						uint32				inType,
						std::string			inData);

	void			SendWindowResize(
						MSshChannel*		inChannel,
						uint32				inColumns,
						uint32				inRows);

	std::string		GetErrorString() const	{ return fErrString; }
	uint32			GetErrorCode() const	{ return fErrCode; }
	
	MEventOut<void(int)>			eConnectionEvent;
	MEventOut<void(std::string)>	eConnectionMessage;
	MEventOut<void(std::string)>	eConnectionBanner;
	
  private:

					MSshConnection();

					MSshConnection(
						const MSshConnection&);
	MSshConnection&	operator=(
						const MSshConnection&);

	virtual 		~MSshConnection();

	bool			Connect(
						std::string			inIPAddress,
						std::string			inUserName,
						uint16				inPortNr);

	std::string		Wrap(
						std::string inData);

	std::vector<std::string>
					Split(
						std::string inData) const;

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

	/* Idle loop for processing from the socket */
	MEventIn<void(double)>					eIdle;

	void			Idle (
						double				inSystemTime);
	
	void			Send(
						std::string			inMessage);

	void			Send(
						const MSshPacket&	inMessage);

	void			Error(
						int					inReason);

	bool			ProcessBuffer();
	void			ProcessPacket();

	// protocol handlers
	
	void			(MSshConnection::*fHandler)(
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

	std::string					fUserName;
	std::string					fPassword;
	std::string					fIPAddress;
	uint16						fPortNumber;
	int							fSocket;
	bool						fIsConnected;
	bool						fBusy;
	bool						fInhibitIdle;
	bool						fGotVersionString;
	std::string					fBuffer;
	std::string					fInPacket, fOutPacket;
	uint32						fPacketLength;
	uint32						fPasswordAttempts;

	std::unique_ptr<CryptoPP::BlockCipher>					fDecryptorCipher;
	std::unique_ptr<CryptoPP::StreamTransformation>			fDecryptorCBC;
	std::unique_ptr<CryptoPP::BufferedTransformation>		fDecryptor;
	std::unique_ptr<CryptoPP::BlockCipher>					fEncryptorCipher;
	std::unique_ptr<CryptoPP::StreamTransformation>			fEncryptorCBC;
	std::unique_ptr<CryptoPP::BufferedTransformation>		fEncryptor;
	std::unique_ptr<CryptoPP::MessageAuthenticationCode>	fSigner;
	std::unique_ptr<CryptoPP::MessageAuthenticationCode>	fVerifier;
	std::unique_ptr<ZLibHelper>								fCompressor;
	std::unique_ptr<ZLibHelper>								fDecompressor;
	
	CryptoPP::Integer			f_x;
	CryptoPP::Integer			f_e;
	std::string					fSessionId;
	CryptoPP::Integer			fSharedSecret;
	std::string					fHostVersion;
	std::string					fMyPayLoad;
	std::string					fHostPayLoad;
	std::string					fMyKexinitMessage;
	std::string					fHostKexinitMessage;

	std::string					fKexAlg;
	std::string					fServerHostKeyAlg;
	std::string					fEncryptionAlgC2S;
	std::string					fEncryptionAlgS2C;
	std::string					fMACAlgC2S;
	std::string					fMACAlgS2C;
	std::string					fCompressionAlgC2S;
	std::string					fCompressionAlgS2C;
	std::string					fLangC2S;
	std::string					fLangS2C;
	
	byte*						fKeys[6];
	
	uint32						fOutSequenceNr;
	uint32						fInSequenceNr;
	
	bool						fAuthenticated;

//	std::unique_ptr<MCertificate>	fCertificate;
	std::unique_ptr<MSshAgent>	fSshAgent;

	MEventIn<void(MCertificate*)>	eCertificateDeleted;

	void						CertificateDeleted(
									MCertificate*	inCertificate);
	
	int32						fRefCount;
	ChannelList					fChannels;
	ChannelList					fOpeningChannels;
	std::string					fErrString;
	uint32						fErrCode;
	double						fOpenedAt;
	static uint32				sNextChannelId;

	static MSshConnection*		sFirstConnection;
	MSshConnection*				fNext;
};

#endif // MSSHCONNECTION_H
