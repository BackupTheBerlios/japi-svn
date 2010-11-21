//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

/*	$Id: MSshConnection.cpp,v 1.38 2004/03/05 20:43:26 maarten Exp $
	Copyright Maarten
	Created Sunday September 28 2003 11:35:17
*/

#include "MJapi.h"

#include <fstream>
#include <sstream>
#include <cerrno>

#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>

#include <cryptopp/gfpcrypt.h>
#include <cryptopp/rsa.h>
#include <cryptopp/osrng.h>
#include <cryptopp/aes.h>
#include <cryptopp/des.h>
#define CRYPTOPP_ENABLE_NAMESPACE_WEAK 1
#include <cryptopp/md5.h>
#include <cryptopp/blowfish.h>
#include <cryptopp/filters.h>
#include <cryptopp/files.h>
#include <cryptopp/factory.h>
#include <zlib.h>

#include "MSsh.h"
#include "MError.h"
#include "MSshConnection.h"
#include "MSshChannel.h"
#include "MSshAgent.h"
#include "MAuthDialog.h"
#include "MPreferences.h"
#include "MUtils.h"
#include "MKnownHosts.h"
#include "MStrings.h"
#include "MJapiApp.h"

using namespace std;
using namespace CryptoPP;
namespace ba = boost::algorithm;
using boost::asio::ip::tcp;

uint32 MSshConnection::sNextChannelId;
MSshConnection*	MSshConnection::sFirstConnection;

namespace {

const string
	kSSHVersionString = string("SSH-2.0-") + kAppName + '-' + kVersionString;

const int kDefaultCompressionLevel = 3;

const byte
	k_p_2[] = {
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xC9, 0x0F, 0xDA, 0xA2, 0x21, 0x68, 0xC2, 0x34,
		0xC4, 0xC6, 0x62, 0x8B, 0x80, 0xDC, 0x1C, 0xD1, 0x29, 0x02, 0x4E, 0x08, 0x8A, 0x67, 0xCC, 0x74,
		0x02, 0x0B, 0xBE, 0xA6, 0x3B, 0x13, 0x9B, 0x22, 0x51, 0x4A, 0x08, 0x79, 0x8E, 0x34, 0x04, 0xDD,
		0xEF, 0x95, 0x19, 0xB3, 0xCD, 0x3A, 0x43, 0x1B, 0x30, 0x2B, 0x0A, 0x6D, 0xF2, 0x5F, 0x14, 0x37,
		0x4F, 0xE1, 0x35, 0x6D, 0x6D, 0x51, 0xC2, 0x45, 0xE4, 0x85, 0xB5, 0x76, 0x62, 0x5E, 0x7E, 0xC6,
		0xF4, 0x4C, 0x42, 0xE9, 0xA6, 0x37, 0xED, 0x6B, 0x0B, 0xFF, 0x5C, 0xB6, 0xF4, 0x06, 0xB7, 0xED,
		0xEE, 0x38, 0x6B, 0xFB, 0x5A, 0x89, 0x9F, 0xA5, 0xAE, 0x9F, 0x24, 0x11, 0x7C, 0x4B, 0x1F, 0xE6,
		0x49, 0x28, 0x66, 0x51, 0xEC, 0xE6, 0x53, 0x81, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
	},
	k_p_14[] = {
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xC9, 0x0F, 0xDA, 0xA2, 0x21, 0x68, 0xC2, 0x34,
		0xC4, 0xC6, 0x62, 0x8B, 0x80, 0xDC, 0x1C, 0xD1, 0x29, 0x02, 0x4E, 0x08, 0x8A, 0x67, 0xCC, 0x74,
		0x02, 0x0B, 0xBE, 0xA6, 0x3B, 0x13, 0x9B, 0x22, 0x51, 0x4A, 0x08, 0x79, 0x8E, 0x34, 0x04, 0xDD,
		0xEF, 0x95, 0x19, 0xB3, 0xCD, 0x3A, 0x43, 0x1B, 0x30, 0x2B, 0x0A, 0x6D, 0xF2, 0x5F, 0x14, 0x37,
		0x4F, 0xE1, 0x35, 0x6D, 0x6D, 0x51, 0xC2, 0x45, 0xE4, 0x85, 0xB5, 0x76, 0x62, 0x5E, 0x7E, 0xC6,
		0xF4, 0x4C, 0x42, 0xE9, 0xA6, 0x37, 0xED, 0x6B, 0x0B, 0xFF, 0x5C, 0xB6, 0xF4, 0x06, 0xB7, 0xED,
		0xEE, 0x38, 0x6B, 0xFB, 0x5A, 0x89, 0x9F, 0xA5, 0xAE, 0x9F, 0x24, 0x11, 0x7C, 0x4B, 0x1F, 0xE6,
		0x49, 0x28, 0x66, 0x51, 0xEC, 0xE4, 0x5B, 0x3D, 0xC2, 0x00, 0x7C, 0xB8, 0xA1, 0x63, 0xBF, 0x05,
		0x98, 0xDA, 0x48, 0x36, 0x1C, 0x55, 0xD3, 0x9A, 0x69, 0x16, 0x3F, 0xA8, 0xFD, 0x24, 0xCF, 0x5F,
		0x83, 0x65, 0x5D, 0x23, 0xDC, 0xA3, 0xAD, 0x96, 0x1C, 0x62, 0xF3, 0x56, 0x20, 0x85, 0x52, 0xBB,
		0x9E, 0xD5, 0x29, 0x07, 0x70, 0x96, 0x96, 0x6D, 0x67, 0x0C, 0x35, 0x4E, 0x4A, 0xBC, 0x98, 0x04,
		0xF1, 0x74, 0x6C, 0x08, 0xCA, 0x18, 0x21, 0x7C, 0x32, 0x90, 0x5E, 0x46, 0x2E, 0x36, 0xCE, 0x3B,
		0xE3, 0x9E, 0x77, 0x2C, 0x18, 0x0E, 0x86, 0x03, 0x9B, 0x27, 0x83, 0xA2, 0xEC, 0x07, 0xA2, 0x8F,
		0xB5, 0xC5, 0x5D, 0xF0, 0x6F, 0x4C, 0x52, 0xC9, 0xDE, 0x2B, 0xCB, 0xF6, 0x95, 0x58, 0x17, 0x18,
		0x39, 0x95, 0x49, 0x7C, 0xEA, 0x95, 0x6A, 0xE5, 0x15, 0xD2, 0x26, 0x18, 0x98, 0xFA, 0x05, 0x10,
		0x15, 0x72, 0x8E, 0x5A, 0x8A, 0xAC, 0xAA, 0x68, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
	};

const char
	kKeyExchangeAlgorithms[] = "diffie-hellman-group14-sha1,diffie-hellman-group1-sha1",
	kServerHostKeyAlgorithms[] = "ssh-rsa,ssh-dss",
	kEncryptionAlgorithms[] = "aes256-cbc,aes192-cbc,aes128-cbc,blowfish-cbc,3des-cbc",
	kMacAlgorithms[] = "hmac-sha1,hmac-md5",
	kUseCompressionAlgorithms[] = "zlib,none",
	kDontUseCompressionAlgorithms[] = "none,zlib";

// implement as globals to keep things simple
Integer					p2(k_p_2, sizeof(k_p_2)), g2(2), q2((p2 - 1) / g2);
Integer					p14(k_p_14, sizeof(k_p_14)), g14(2), q14((p14 - 1) / g14);

AutoSeededRandomPool	rng;

class MIOService
{
  public:
	static MIOService&	Instance();

						operator boost::asio::io_service& ()	{ return mIOService; }

  private:
						MIOService();
						
	void				Idle(double);
	MEventIn<void(double)>
						eIdle;

	boost::asio::io_service
						mIOService;
};

MIOService&	MIOService::Instance()
{
	static MIOService* sIOService = new MIOService;
	return *sIOService;
}

MIOService::MIOService()
	: eIdle(this, &MIOService::Idle)
{
	AddRoute(gApp->eIdle, eIdle);
}

void MIOService::Idle(double)
{
	mIOService.poll();
}

} // end private namespace

struct ZLibHelper
{
						ZLibHelper(bool inInflate);
						~ZLibHelper();
	
	int					Process(string& ioData);
	
	z_stream_s			mStream;
	bool				mInflate;
	static const uint32	kBufferSize;
	static char			sBuffer[];
};

const uint32 ZLibHelper::kBufferSize = 1024;
char ZLibHelper::sBuffer[ZLibHelper::kBufferSize];	

ZLibHelper::ZLibHelper(
	bool	inInflate)
	: mInflate(inInflate)
{
	memset(&mStream, 0, sizeof(mStream));

	int err;

	if (inInflate)
		err = inflateInit(&mStream);
	else
		err = deflateInit(&mStream, kDefaultCompressionLevel);
//	if (err != Z_OK)
//		THROW(("Decompression error: %s", mStream.msg));
}

ZLibHelper::~ZLibHelper()
{
	if (mInflate)
		inflateEnd(&mStream);
	else
		deflateEnd(&mStream);
}

int ZLibHelper::Process(
	string&		ioData)
{
	string result;
	
	mStream.next_in = const_cast<byte*>(reinterpret_cast<const byte*>(ioData.c_str()));
	mStream.avail_in = ioData.length();
	mStream.total_in = 0;
	
	mStream.next_out = reinterpret_cast<byte*>(sBuffer);
	mStream.avail_out = kBufferSize;
	mStream.total_out = 0;

	int err;
	do
	{
		if (mInflate)
			err = inflate(&mStream, Z_SYNC_FLUSH);
		else
			err = deflate(&mStream, Z_SYNC_FLUSH);

		if (kBufferSize - mStream.avail_out > 0)
		{
			result.append(sBuffer, kBufferSize - mStream.avail_out);
			mStream.avail_out = kBufferSize;
			mStream.next_out = reinterpret_cast<byte*>(sBuffer);
		}
	}
	while (err >= Z_OK);
	
	ioData = result;
	return err;
}

MSshConnection::MSshConnection(
	const string&	inIPAddress,
	const string&	inUserName,
	uint16			inPortNr)
	: mHandler(nil)
	, eRecvAuthInfo(this, &MSshConnection::RecvAuthInfo)
	, eRecvPassword(this, &MSshConnection::RecvPassword)
	, mUserName(inUserName)
	, mIPAddress(inIPAddress)
	, mPortNumber(inPortNr)
//	, mIsConnected(false)
//	, mBusy(false)
//	, mInhibitIdle(false)
//	, mGotVersionString(false)
//	, mPacketLength(0)
	, mResolver(MIOService::Instance())
	, mSocket(MIOService::Instance())
	, mPasswordAttempts(0)
	, mDecryptorCipher(nil)
	, mDecryptorCBC(nil)
	, mEncryptorCipher(nil)
	, mEncryptorCBC(nil)
	, mSigner(nil)
	, mVerifier(nil)
	, mOutSequenceNr(0)
	, mInSequenceNr(0)
	, mAuthenticated(false)
	, eCertificateDeleted(this, &MSshConnection::CertificateDeleted)
	, mRefCount(1)
	, mOpenedAt(GetLocalTime())
{
	for (int i = 0; i < 6; ++i)
		mKeys[i] = nil;

	mNext = sFirstConnection;
	sFirstConnection = this;

	tcp::resolver::query query(inIPAddress, boost::lexical_cast<string>(inPortNr));
    mResolver.async_resolve(query,
        boost::bind(&MSshConnection::HandleResolve, this,
          boost::asio::placeholders::error,
          boost::asio::placeholders::iterator));
}

MSshConnection::~MSshConnection()
{
	assert(mRefCount == 0);

	if (this == sFirstConnection)
		sFirstConnection = mNext;
	else
	{
		MSshConnection* c = sFirstConnection;
		while (c != nil)
		{
			MSshConnection* next = c->mNext;
			if (next == this)
			{
				c->mNext = mNext;
				break;
			}
			c = next;
		}
	}
	
	for (int i = 0; i < 6; ++i)
		delete[] mKeys[i];

//	try
//	{
//		RemoveRoute(gApp->eIdle, eIdle);
//		
//		if (mIsConnected)
//			Disconnect();
//	}
//	catch (...) {}
}

void MSshConnection::Reference()
{
	++mRefCount;
}

void MSshConnection::Release()
{
	--mRefCount;
}

MSshConnection* MSshConnection::Get(
	const string&	inIPAddress,
	const string&	inUserName,
	uint16			inPort)
{
	string username = inUserName;
	
	if (username.length() == 0)
		username = GetUserName(true);
	
	if (inPort == 0)
		inPort = 22;
	
	MSshConnection* connection = sFirstConnection;
	while (connection != nil)
	{
		if (connection->mIPAddress == inIPAddress and
			connection->mUserName == username and
			connection->mPortNumber == inPort)
		{
			break;
		}
		
		connection = connection->mNext;
	}
	
	if (connection != nil)
		connection->Reference();
	else
		connection = new MSshConnection(inIPAddress, username, inPort);
	
	return connection;
}

void MSshConnection::Error(
	int			inReason)
{
#pragma warning("To lower case some day")
	const char* kErrors[] = {
		"HOST NOT ALLOWED TO CONNECT",
		"PROTOCOL ERROR",
		"KEY EXCHANGE FAILED",
		"RESERVED",
		"MAC ERROR",
		"COMPRESSION ERROR",
		"SERVICE NOT AVAILABLE",
		"PROTOCOL VERSION NOT SUPPORTED",
		"HOST KEY NOT VERIFIABLE",
		"CONNECTION LOST",
		"BY APPLICATION",
		"TOO MANY CONNECTIONS",
		"AUTH CANCELLED BY USER",
		"NO MORE AUTH METHODS AVAILABLE",
		"ILLEGAL USER NAME",
	};

	string reason = "Reason unknown, code is " + boost::lexical_cast<string>(inReason);
	if (inReason < sizeof(kErrors) / sizeof(char*))
		reason = kErrors[inReason];

	eConnectionMessage(FormatString("Error in SSH connection: ^0", reason));
	
	Disconnect();
	
	mErrCode = inReason;
}

//bool MSshConnection::Connect(
//	string		inIPAddress,
//	string		inUserName,
//	uint16		inPortNr)
//{
//	return true;
//	
//	/* Create socket */
//	mSocket = socket(AF_INET, SOCK_STREAM, 0);
//	if (mSocket < 0)
//	{
//		assert (false);
//		return false;
//	}
//	
//	eConnectionMessage(_("Looking up address"));
//
//	/* First do some ip address configuration */
//	sockaddr_in lAddr = {};
//
//	lAddr.sin_family = AF_INET;
//	lAddr.sin_port = htons(inPortNr);
//	lAddr.sin_addr.s_addr = inet_addr(inIPAddress.c_str());
//
//	/* Do DNS check. */
//	if (lAddr.sin_addr.s_addr == INADDR_NONE)
//	{
//		hostent* lHost = gethostbyname(inIPAddress.c_str());
//		if (lHost == nil)
//			return false;
//
//		assert(lHost->h_addrtype == AF_INET);
//		assert(lHost->h_length == sizeof(in_addr));
//		lAddr.sin_addr = *reinterpret_cast<in_addr*>(lHost->h_addr);
//	}
//
//	/* Set the sockets on non-blocking */
//	unsigned long lValue = 1;
//	ioctl(mSocket, FIONBIO, &lValue);
//
//	eConnectionMessage(_("Connecting…"));
//
//	/* Connect the control port. */
//	int lResult = connect(mSocket, (sockaddr*) &lAddr, sizeof(lAddr));
//	if (lResult < 0)
//	{
//		int err = errno;
//		if (err != EAGAIN and err != EINPROGRESS)
//			return false;
//	}
//
//	/* Even if the connection IS established we still Finish the connect non-blocking */
//	mBusy = true;
//	return true;
//}

void MSshConnection::Disconnect()
{
//	if (mIsConnected)
//	{
//		mIsConnected = false;
//	
//		RemoveRoute(eIdle, gApp->eIdle);
//		
//		close(mSocket);
//	
//		mBusy = false;

	mSocket.close();
		
		eConnectionMessage(_("Connection closed"));
		eConnectionEvent(SSH_CHANNEL_CLOSED);
//	}

	for_each(mChannels.begin(), mChannels.end(),
		boost::bind(&MSshChannel::SetChannelOpen, _1, false));
	mChannels.clear();
}

void MSshConnection::CertificateDeleted(
	MCertificate*	inCertificate)
{
//	if (inCertificate == mCertificate.get())
//	{
//		mCertificate.reset(nil);
//		Disconnect();
//	}
}

//void MSshConnection::Idle(
//	double	inSystemTime)
//{
//	if (mInhibitIdle)
//		return;
//	
//	MValueChanger<bool> saveFlag(mInhibitIdle, true);
//	
//	// avoid being deleted while processing this command
//	Reference();
//	
//	try
//	{
//		/* Idle processing... */
//	
//		if (mSocket < 0)
//			return;
//	
//		static fd_set sRead, sWrite, sExcept;
//		static timeval sTime = {0,0};
//	
//		/* Check of action on the socket */
//		FD_ZERO(&sRead);
//		FD_ZERO(&sWrite);
//		FD_ZERO(&sExcept);
//	
//		/* Setting the check flags */
//		if (mIsConnected == false )
//		{
//			FD_SET(mSocket, &sWrite);
//			FD_SET(mSocket, &sExcept);
//		}
//		else
//			FD_SET(mSocket, &sRead);
//	
//		/* Select */
//		int lResult = select(static_cast<int>(mSocket + 1), &sRead, &sWrite, &sExcept, &sTime);
//		
//		/* Something happened. */
//		if (lResult > 0)
//		{
//			if (FD_ISSET(mSocket, &sExcept))
//			{
//				mBusy = false;
//				Error(SSH_DISCONNECT_CONNECTION_LOST);
//			}
//	
//			if (FD_ISSET(mSocket, &sRead))
//			{
////				mIsConnected = true;
////				mBusy = false;
//
//				static char sReadBuf[2 * kMaxPacketSize + 256];
//				int lResult;
//
//				lResult = recv(mSocket, sReadBuf, sizeof(sReadBuf), 0);
//
//				/* Something wrong... no connection anymore */
//				if (lResult == 0 or lResult < 0)
//				{
////					assert(errno != EAGAIN);
//					Disconnect();
//				}
//				else
//					mBuffer.append(sReadBuf, static_cast<string::size_type>(lResult));
//			}
//	
//			if (FD_ISSET(mSocket, &sWrite))
//			{
//				// don't ask me...
////				delay(0.1);
//				
//				mIsConnected = true;
//				mBusy = false;
//	
//				string cmd = kSSHVersionString;
//				cmd += "\r\n";
//				
//				int lRes = send(mSocket, cmd.c_str(), cmd.length(), 0);
//				if (lRes != static_cast<int>(cmd.length()))
//					Disconnect();
//			}
//		}
//		
//		/* Process data from socket .. if needed */
//		while (not mBuffer.empty())
//		{
//			if (not ProcessBuffer())
//				break;
//			eConnectionEvent(SSH_CHANNEL_PACKET_DONE);
//		}
//		
//		// see if there's data to be sent
//		ChannelList::iterator ch;
//	
//		for (ch = mChannels.begin(); ch != mChannels.end(); ++ch)
//		{
//			MSshPacket p;
//
//			if ((*ch)->PopPending(p.data))
//				Send(p);
//		}
//
//		// if there's a time out, let the user decide what to do		
//		if ((mBusy or not mIsConnected) and GetLocalTime() > mOpenedAt + 30)
//		{
//			eConnectionEvent(SSH_CHANNEL_TIMEOUT);
//			
//			if (GetLocalTime() > mOpenedAt + 45)
//				Disconnect();
//		}
//	}
//	catch (Exception e)
//	{
//		Disconnect();
//		eConnectionMessage(e.what());
//		PRINT(("Catched cryptlib exception: %s", e.what()));
//	}
//	catch (exception e)
//	{
//		Disconnect();
//		eConnectionMessage(e.what());
//		PRINT(("Catched exception: %s", e.what()));
//	}
//	catch (...)
//	{
//		Disconnect();
//		eConnectionMessage("exception");
//		PRINT(("Catched unhandled exception"));
//	}
//	
//	Release();
//	
//	if (mRefCount <= 0 and not mIsConnected and not mBusy)
//		delete this;
//}

//void MSshConnection::Send(string inMessage)
//{
//	const char* msg = reinterpret_cast<const char*>(inMessage.c_str());
//	int lResult = send(mSocket, msg, inMessage.length(), 0);
//	if (lResult != static_cast<int>(inMessage.length()))
//		Error(SSH_DISCONNECT_CONNECTION_LOST);
//}

void MSshConnection::Send(MSshPacket& inPacket,
	ASIOHandler inHandler)
{
	uint32 blockSize = 8;

	if (mEncryptorCipher.get() != nil)
		blockSize = mEncryptorCipher->BlockSize();
	
//	if (mCompressor.get() != nil)
//		mCompressor->Process(inPacket.data);
	
	inPacket.Wrap(blockSize, rng);

HexDump(inPacket.peek(), inPacket.size(), cerr);
	
	ostream out(&mRequest);
	
	if (mEncryptorCipher.get() == nil)
		out << inPacket;
	else
	{
		StreamTransformationFilter f(
			*mEncryptorCBC.get(), new FileSink(out),
			StreamTransformationFilter::NO_PADDING);
		
		f.Put(inPacket.peek(), inPacket.size());
		f.Flush(true);
		
		net_swapper swap;
		uint32 seqNr = swap(mOutSequenceNr);
		mSigner->Update(reinterpret_cast<byte*>(&seqNr), sizeof(uint32));
		mSigner->Update(inPacket.peek(), inPacket.size());
		
		vector<byte> buf(mSigner->DigestSize());
		mSigner->Final(&buf[0]);
		
		out.write(reinterpret_cast<char*>(&buf[0]),
			mSigner->DigestSize());
	}

	++mOutSequenceNr;
	
	if (inHandler == nil)
		inHandler = &MSshConnection::HandleDataRequest;
	
	boost::asio::async_write(mSocket, mRequest,
		boost::bind(inHandler, this, boost::asio::placeholders::error));
}

void MSshConnection::HandleResolve(
	const boost::system::error_code& err,
	tcp::resolver::iterator endpoint_iterator)
{
    if (err)
    	Error(SSH_DISCONNECT_CONNECTION_LOST);

	// Attempt a connection to the first endpoint in the list. Each endpoint
	// will be tried until we successfully establish a connection.
	tcp::endpoint endpoint = *endpoint_iterator;
	mSocket.async_connect(endpoint,
		boost::bind(&MSshConnection::HandleConnect, this,
			boost::asio::placeholders::error, ++endpoint_iterator));
}

void MSshConnection::HandleConnect(const boost::system::error_code& err,
      tcp::resolver::iterator endpoint_iterator)
{
    if (!err)
    {
    	ostream out(&mRequest);
    	out << kSSHVersionString << "\r\n";
    	
		// The connection was successful. send protocol string
    	boost::asio::async_write(mSocket, mRequest,
			boost::bind(&MSshConnection::HandleProtocolVersionExchangeRequest, this,
				boost::asio::placeholders::error));
    }
    else if (endpoint_iterator != tcp::resolver::iterator())
    {
      // The connection failed. Try the next endpoint in the list.
      mSocket.close();
      tcp::endpoint endpoint = *endpoint_iterator;
      mSocket.async_connect(endpoint,
          boost::bind(&MSshConnection::HandleConnect, this,
            boost::asio::placeholders::error, ++endpoint_iterator));
    }
    else
    	Error(SSH_DISCONNECT_CONNECTION_LOST);
//    	THROW((err.message().c_str()));
}

void MSshConnection::HandleProtocolVersionExchangeRequest(
	const boost::system::error_code& err)
{
    if (err)
    	Error(SSH_DISCONNECT_PROTOCOL_ERROR);

	// The connection was successful. Receive initial string
	boost::asio::async_read_until(mSocket, mResponse, "\r\n",
		boost::bind(&MSshConnection::HandleProtocolVersionExchangeResponse, this,
			boost::asio::placeholders::error));
}

void MSshConnection::HandleProtocolVersionExchangeResponse(
	const boost::system::error_code& err)
{
    if (err)
    	Error(SSH_DISCONNECT_PROTOCOL_ERROR);

	istream response_stream(&mResponse);
	response_stream >> mHostVersion;
	
	if (not ba::starts_with(mHostVersion, "SSH-2.0"))
		Error(SSH_DISCONNECT_PROTOCOL_ERROR);

	MSshPacket out;
	out << uint8(SSH_MSG_KEXINIT);
	
	for (uint32 i = 0; i < 16; ++i)
		out << rng.GenerateByte();

	string compress;
//	if (Preferences::GetInteger("compress-sftp", true))
//		compress = kUseCompressionAlgorithms;
//	else
		compress = kDontUseCompressionAlgorithms;

	out << kKeyExchangeAlgorithms
		<< kServerHostKeyAlgorithms
		<< kEncryptionAlgorithms
		<< kEncryptionAlgorithms
		<< kMacAlgorithms
		<< kMacAlgorithms
		<< compress
		<< compress
		<< ""
		<< ""
		<< false
		<< uint32(0);

	// copy out the payload of this packet
	mMyPayLoad.assign(reinterpret_cast<const char*>(out.peek()), out.size());

	// Send the packet to the server
	Send(out, &MSshConnection::HandleKexInitRequest);
}

void MSshConnection::HandleKexInitRequest(
	const boost::system::error_code& err)
{
    if (err)
    	Error(SSH_DISCONNECT_CONNECTION_LOST);

	cout << "HandleKexInitRequest" << endl;
    
	boost::asio::async_read(mSocket, mResponse, boost::asio::transfer_at_least(1),
		boost::bind(&MSshConnection::HandleKexInitResponse, this,
			boost::asio::placeholders::error));
}

void MSshConnection::HandleKexInitResponse(
	const boost::system::error_code& err)
{
    if (err)
    	Error(SSH_DISCONNECT_CONNECTION_LOST);

	cout << "HandleKexInitResponse" << endl;

//	// capture the packet contents for the host payload
//
//	uint32 l = mResponse.in_avail();
//	vector<char> hostPayLoad(l);
//	mResponse.sgetn(&hostPayLoad[0], l);
//	mResponse.pubseekpos(0);
//
//	// Now handle the packet
//	
//	MSshIStream in(mResponse);
//	
//	uint8 msg;
//	in >> msg;
//	
//	if (msg != SSH_MSG_KEXINIT)
//		throw logic_error("protocol error"); // (SSH_DISCONNECT_PROTOCOL_ERROR);
//
//	uint8 b;
//	bool first_kex_packet_follows;
//	
//	for (uint32 i = 0; i < 16; ++i)
//		in >> b;
//		
//	in	>> mKexAlg
//		>> mServerHostKeyAlg
//		>> mEncryptionAlgC2S
//		>> mEncryptionAlgS2C
//		>> mMACAlgC2S
//		>> mMACAlgS2C
//		>> mCompressionAlgC2S
//		>> mCompressionAlgS2C
//		>> mLangC2S
//		>> mLangS2C
//		>> first_kex_packet_follows;
//
//	cout << "HandleKexInit: " << endl
//		 << mKexAlg << endl
//		 << mServerHostKeyAlg << endl
//		 << mEncryptionAlgC2S << endl
//		 << mEncryptionAlgS2C << endl
//		 << mMACAlgC2S << endl
//		 << mMACAlgS2C << endl
//		 << mCompressionAlgC2S << endl
//		 << mCompressionAlgS2C << endl
//		 << mLangC2S << endl
//		 << mLangS2C << endl
//		 << endl;
//
//	if (first_kex_packet_follows)
//		cout << "first_kex_packet_follows" << endl;
//
//	
////	f_e = 0;
////
////	if (ChooseProtocol(mKexAlg, kKeyExchangeAlgorithms) == "diffie-hellman-group14-sha1")
////	{
////		do
////		{
////			f_x.Randomize(*rng.get(), Integer(2), q14 - 1);
////			f_e = a_exp_b_mod_c(g14, f_x, p14);
////		}
////		while (f_e < 1 or f_e >= p14 - 1);
////	}
////	else if (ChooseProtocol(mKexAlg, kKeyExchangeAlgorithms) == "diffie-hellman-group1-sha1")
////	{
////		do
////		{
////			f_x.Randomize(*rng.get(), Integer(2), q2 - 1);
////			f_e = a_exp_b_mod_c(g2, f_x, p2);
////		}
////		while (f_e < 1 or f_e >= p2 - 1);
////	}
////	
////	if (f_e > 0)
////	{
////		out << uint8(SSH_MSG_KEXDH_INIT) << f_e;
////		mHandler = &MSshConnection::ProcessKexdhReply;
////	}
//
//	cerr << "HandleKexInit" << endl;
//
//	done = true;
}

void MSshConnection::HandleDataRequest(
	const boost::system::error_code& err)
{
	PRINT(("HandleDataRequest"));
}

bool MSshConnection::ProcessBuffer()
{
	bool result = true;

//	net_swapper swap;
//	
//	if (not mGotVersionString)
//	{
//		string::size_type n = mBuffer.find('\r');
//		if (n == string::npos)
//			n = mBuffer.find('\n');
//
//		if (n != string::npos)
//		{
//			mGotVersionString = true;
//			
//			mInPacket.assign(mBuffer, 0, n);
//			
//			if (mBuffer[n] == '\r' and mBuffer[n + 1] == '\n')
//				mBuffer.erase(0, n + 2);
//			else
//				mBuffer.erase(0, n + 1);
//			
//			ProcessConnect();
//			mInPacket.clear();
//		}
//		
//		result = mBuffer.length() > 0;
//	}
//	else
//	{
//		uint32 blockSize = 8;
//		
//		if (mDecryptorCipher.get())
//			blockSize = mDecryptorCipher->BlockSize();
//		
//			// first check if we need to add more data 
//		while (mBuffer.size() >= blockSize and
//			mInPacket.size() < mPacketLength + sizeof(uint32))
//		{
//			if (mDecryptorCBC.get())
//			{
//				mDecryptor->Put(
//					reinterpret_cast<const byte*>(mBuffer.c_str()), blockSize);
//				mDecryptor->Flush(true);
//			}
//			else
//				mInPacket.append(mBuffer.begin(), mBuffer.begin() + blockSize);
//			mBuffer.erase(0, blockSize);
//			
//			// If this is the first block for a new packet
//			// we determine the expected payload and padding length
//			if (mInPacket.size() == blockSize)
//			{
//				mPacketLength = swap(
//					*reinterpret_cast<const uint32*>(mInPacket.c_str()));
//			}
//		}
//
//		// if we have the entire packet, check the checksum
//		if (mInPacket.size() == mPacketLength + sizeof(uint32))
//		{
//			bool validMac = false;
//			bool done = false;
//			
//			if (mVerifier.get() == nil)
//			{
//				validMac = true;
//				done = true;
//			}
//			else if (mBuffer.length() >= mVerifier->DigestSize())
//			{
//				done = true;
//				
//				uint32 seqNr = swap(mInSequenceNr);
//				
//				mVerifier->Update(reinterpret_cast<byte*>(&seqNr), 4);
//				mVerifier->Update(reinterpret_cast<const byte*>(mInPacket.c_str()), mInPacket.length());
//				
//				validMac = mVerifier->Verify(reinterpret_cast<const byte*>(mBuffer.c_str()));
//
//				mBuffer.erase(0, mVerifier->DigestSize());
//				
//				if (not validMac)
//					Error(SSH_DISCONNECT_MAC_ERROR);
//			}
//			
//			if (done and validMac)
//			{
//				++mInSequenceNr;
//				
//				// and strip off the packet header and trailer
//
//				uint8 paddingLength = mInPacket[4];
//
//				mInPacket.erase(0, 5);
//				mInPacket.erase(mInPacket.length() - paddingLength, paddingLength);
//
//				// decompress now
//				
//				if (mDecompressor.get() != nil)
//					mDecompressor->Process(mInPacket);
//				
//				try
//				{
//					ProcessPacket();
//				}
//				catch (MSshPacket::MSshPacketError& e)
//				{
//					Error(SSH_DISCONNECT_PROTOCOL_ERROR);
//					eConnectionMessage(e.what());
//				}
//				catch (exception& e)
//				{
//					Error(SSH_DISCONNECT_PROTOCOL_ERROR);
//					eConnectionMessage(e.what());
//				}
//			}
//			else
//				result = false;
//		}
//		else
//			result = false;
//	}
	
	return result;
}

void MSshConnection::ProcessPacket()
{//
//	uint8 message = mInPacket[0];
//
//	MSshPacket in, out;
//	in.data = mInPacket;
//
//PRINT(("ProcessPacket %d", message));
//
//	switch (message)
//	{
//		case SSH_MSG_DISCONNECT:
//			ProcessDisconnect(message, in, out);
//			break;
//		
//		case SSH_MSG_DEBUG:
//			ProcessDebug(message, in, out);
//			break;
//		
//		case SSH_MSG_IGNORE:
//			break;
//		
//		case SSH_MSG_UNIMPLEMENTED:
//			ProcessUnimplemented(message, in, out);
//			break;
//		
//		case SSH_MSG_CHANNEL_OPEN_CONFIRMATION:
//		case SSH_MSG_CHANNEL_OPEN_FAILURE:
//			ProcessConfirmChannel(message, in, out);
//			break;
//
//		case SSH_MSG_CHANNEL_WINDOW_ADJUST:
//		case SSH_MSG_CHANNEL_DATA:
//		case SSH_MSG_CHANNEL_EXTENDED_DATA:
//		case SSH_MSG_CHANNEL_EOF:
//		case SSH_MSG_CHANNEL_CLOSE:
//		case SSH_MSG_CHANNEL_SUCCESS:
//		case SSH_MSG_CHANNEL_FAILURE:
//			if (mAuthenticated)
//				ProcessChannel(message, in, out);
//			else
//				Error(SSH_DISCONNECT_PROTOCOL_ERROR);
//			break;
//
//		case SSH_MSG_CHANNEL_REQUEST:
//			ProcessChannelRequest(message, in, out);
//			break;
//		
////		case SSH_MSG_CHANNEL_OPEN:
////			ProcessChannelOpen(message, in, out);
////			break;
//		
//		default:
//			if (mHandler != nil)
//				(this->*mHandler)(message, in, out);
//			else
//				PRINT(("This message should not have been received: %d", message));
//	}
//
//	mInPacket.clear();
//	
//	if (out.data.length() > 0)
//		Send(out);
//	
//	mInPacket.clear();
}

void MSshConnection::ProcessDisconnect(
	uint8		inMessage,
	MSshPacket&	in,
	MSshPacket&	out)
{
	uint8 message;
	string languageTag;
	
	in >> message >> mErrCode >> mErrString >> languageTag;
	
	Disconnect();
}

void MSshConnection::ProcessUnimplemented(
	uint8		inMessage,
	MSshPacket&	in,
	MSshPacket&	out)
{
	Disconnect();
}

void MSshConnection::ProcessChannelRequest(
	uint8		inMessage,
	MSshPacket&	in,
	MSshPacket&	out)
{
	uint8 message;
	uint32 channel;
	string request;
	bool wantReply;
	
	in >> message >> channel >> request >> wantReply;
	
	if (wantReply)
		out << uint8(SSH_MSG_CHANNEL_FAILURE) << channel;
}

void MSshConnection::ProcessDebug(
	uint8		inMessage,
	MSshPacket&	in,
	MSshPacket&	out)
{
	uint8 msg;
	bool always_display;
	string message, langTag;
	
	in >> msg >> always_display >> message >> langTag;
	PRINT(("Debug message from server: %s", message.c_str()));
}

void MSshConnection::ProcessConnect()
{//
//	if (mInPacket.substr(0, 4) != "SSH-")
//		Error(SSH_DISCONNECT_PROTOCOL_ERROR);
//
//	eConnectionMessage(_("Exchanging keys"));
//	
//	mHostVersion = mInPacket;
//
//	// create and send our KEXINIT packet
//
//	MSshPacket data;
//	
//	data << uint8(SSH_MSG_KEXINIT);
//	
//	for (uint32 i = 0; i < 16; ++i)
//		data << rng.GenerateByte();
//
//	string compress;
//	if (Preferences::GetInteger("compress-sftp", true))
//		compress = kUseCompressionAlgorithms;
//	else
//		compress = kDontUseCompressionAlgorithms;
//
//	data << kKeyExchangeAlgorithms
//		<< kServerHostKeyAlgorithms
//		<< kEncryptionAlgorithms
//		<< kEncryptionAlgorithms
//		<< kMacAlgorithms
//		<< kMacAlgorithms
//		<< compress
//		<< compress
//		<< ""
//		<< ""
//		<< false
//		<< uint32(0);
//
//	mMyPayLoad = data.data;
//	mHandler = &MSshConnection::ProcessKexInit;
//
//	Send(data);
}

void MSshConnection::ProcessKexInit(
	uint8		inMessage,
	MSshPacket&	in,
	MSshPacket&	out)
{//
//	if (inMessage != SSH_MSG_KEXINIT)
//		Error(SSH_DISCONNECT_PROTOCOL_ERROR);
//	else
//	{
//		mHostPayLoad = mInPacket;
//		
//		uint8 msg, b;
//		bool first_kex_packet_follows;
//	
//		in >> msg;
//
//		for (uint32 i = 0; i < 16; ++i)
//			in >> b;
//		
//		in	>> mKexAlg
//			>> mServerHostKeyAlg
//			>> mEncryptionAlgC2S
//			>> mEncryptionAlgS2C
//			>> mMACAlgC2S
//			>> mMACAlgS2C
//			>> mCompressionAlgC2S
//			>> mCompressionAlgS2C
//			>> mLangC2S
//			>> mLangS2C
//			>> first_kex_packet_follows;
//	
//		f_e = 0;
//
//		if (ChooseProtocol(mKexAlg, kKeyExchangeAlgorithms) == "diffie-hellman-group14-sha1")
//		{
//			do
//			{
//				f_x.Randomize(rng, Integer(2), q14 - 1);
//				f_e = a_exp_b_mod_c(g14, f_x, p14);
//			}
//			while (f_e < 1 or f_e >= p14 - 1);
//		}
//		else if (ChooseProtocol(mKexAlg, kKeyExchangeAlgorithms) == "diffie-hellman-group1-sha1")
//		{
//			do
//			{
//				f_x.Randomize(rng, Integer(2), q2 - 1);
//				f_e = a_exp_b_mod_c(g2, f_x, p2);
//			}
//			while (f_e < 1 or f_e >= p2 - 1);
//		}
//		
//		if (f_e > 0)
//		{
//			out << uint8(SSH_MSG_KEXDH_INIT) << f_e;
//			mHandler = &MSshConnection::ProcessKexdhReply;
//		}
//		// else we simply ignore this one
//	}
}

void MSshConnection::ProcessKexdhReply(
	uint8		inMessage,
	MSshPacket&	in,
	MSshPacket&	out)
{//
//	try
//	{
//		if (inMessage != SSH_MSG_KEXDH_REPLY)
//			throw uint32(SSH_DISCONNECT_PROTOCOL_ERROR);
//
//		uint8 msg;
//		string hostKey, signature;
//		Integer f;
//	
//		in >> msg >> hostKey >> f >> signature;
//		
//		string hostName = mIPAddress;
//		if (mPortNumber != 22)
//			hostName = hostName + ':' + NumToString(mPortNumber);
//
//		try
//		{
//			MKnownHosts::Instance().CheckHost(hostName, hostKey);
//		}
//		catch (...)
//		{
//			Disconnect();
//			throw;
//		}
//
//		Integer K;
//		if (ChooseProtocol(mKexAlg, kKeyExchangeAlgorithms) == "diffie-hellman-group14-sha1")
//			K = a_exp_b_mod_c(f, f_x, p14);
//		else
//			K = a_exp_b_mod_c(f, f_x, p2);
//
//		mSharedSecret = K;
//		
//		MSshPacket h_test;
//		h_test << kSSHVersionString << mHostVersion
//			<< mMyPayLoad << mHostPayLoad
//			<< hostKey
//			<< f_e << f << K;
//
//		SHA1 hash;
//		uint32 dLen = hash.DigestSize();
//		vector<char> H(dLen);
//		hash.Update(
//			reinterpret_cast<const byte*>(h_test.data.c_str()),
//			h_test.data.length());
//		hash.Final(reinterpret_cast<byte*>(&H[0]));
//		
//		if (mSessionId.length() == 0)
//			mSessionId.assign(&H[0], dLen);
//
//		string h_pk_type;
//		
//		MSshPacket packet;
//		packet.data = hostKey;
//		packet >> h_pk_type;
//
//		auto_ptr<PK_Verifier> h_key;
//
//		if (h_pk_type == "ssh-dss")
//		{
//			Integer h_p, h_q, h_g, h_y;
//			packet >> h_p >> h_q >> h_g >> h_y;
//
//			h_key.reset(new GDSA<SHA1>::Verifier(h_p, h_q, h_g, h_y));
//		}
//		else if (h_pk_type == "ssh-rsa")
//		{
//			Integer h_e, h_n;
//			packet >> h_e >> h_n;
//
//			h_key.reset(new RSASSA_PKCS1v15_SHA_Verifier(h_n, h_e));
//		}
//		else
//			throw uint32(SSH_DISCONNECT_PROTOCOL_ERROR);
//
//		string pk_type, pk_rs;
//		packet.data = signature;
//		packet >> pk_type >> pk_rs;
//
//		if (pk_type != h_pk_type)
//			throw uint32(SSH_DISCONNECT_PROTOCOL_ERROR);
//		
//		const byte* h_sig = reinterpret_cast<const byte*>(pk_rs.c_str());
//		if (not h_key->VerifyMessage(reinterpret_cast<byte*>(&H[0]), dLen, h_sig, pk_rs.length()))
//			throw uint32(SSH_DISCONNECT_PROTOCOL_ERROR);
//
//		out << uint8(SSH_MSG_NEWKEYS);
//		
//		int keyLen = 16;
//
//		if (keyLen < 20 and ChooseProtocol(mMACAlgC2S, kMacAlgorithms) == "hmac-sha1")
//			keyLen = 20;
//
//		if (keyLen < 20 and ChooseProtocol(mMACAlgS2C, kMacAlgorithms) == "hmac-sha1")
//			keyLen = 20;
//
//		if (keyLen < 24 and ChooseProtocol(mEncryptionAlgC2S, kEncryptionAlgorithms) == "3des-cbc")
//			keyLen = 24;
//
//		if (keyLen < 24 and ChooseProtocol(mEncryptionAlgS2C, kEncryptionAlgorithms) == "3des-cbc")
//			keyLen = 24;
//
//		if (keyLen < 24 and ChooseProtocol(mEncryptionAlgC2S, kEncryptionAlgorithms) == "aes192-cbc")
//			keyLen = 24;
//
//		if (keyLen < 24 and ChooseProtocol(mEncryptionAlgS2C, kEncryptionAlgorithms) == "aes192-cbc")
//			keyLen = 24;
//
//		if (keyLen < 32 and ChooseProtocol(mEncryptionAlgC2S, kEncryptionAlgorithms) == "aes256-cbc")
//			keyLen = 32;
//
//		if (keyLen < 32 and ChooseProtocol(mEncryptionAlgS2C, kEncryptionAlgorithms) == "aes256-cbc")
//			keyLen = 32;
//
//		for (int i = 0; i < 6; ++i)
//		{
//			delete[] mKeys[i];
//			DeriveKey(&H[0], i, keyLen, mKeys[i]);
//		}
//		
//		mHandler = &MSshConnection::ProcessNewKeys;
//	}
//	catch (uint32 err)
//	{
//		Error(err);
//		out.data.clear();
//		mHandler = nil;
//	}
}

void MSshConnection::ProcessNewKeys(
	uint8		inMessage,
	MSshPacket&	in,
	MSshPacket&	out)
{
	if (inMessage != SSH_MSG_NEWKEYS)
		Error(SSH_DISCONNECT_PROTOCOL_ERROR);
	else
	{
		string protocol;
		
		// Client to server encryption
		protocol = ChooseProtocol(mEncryptionAlgC2S, kEncryptionAlgorithms);
		
		if (protocol == "3des-cbc")
			mEncryptorCipher.reset(new DES_EDE3::Encryption(mKeys[2]));
		else if (protocol == "blowfish-cbc")
			mEncryptorCipher.reset(new BlowfishEncryption(mKeys[2]));
		else if (protocol == "aes128-cbc")
			mEncryptorCipher.reset(new AES::Encryption(mKeys[2], 16));
		else if (protocol == "aes192-cbc")
			mEncryptorCipher.reset(new AES::Encryption(mKeys[2], 24));
		else if (protocol == "aes256-cbc")
			mEncryptorCipher.reset(new AES::Encryption(mKeys[2], 32));

		mEncryptorCBC.reset(
			new CBC_Mode_ExternalCipher::Encryption(
				*mEncryptorCipher.get(), mKeys[0]));
		
//		mEncryptor.reset(new StreamTransformationFilter(
//			*mEncryptorCBC.get(), new StringSink(mOutPacket),
//			StreamTransformationFilter::NO_PADDING));

		// Server to client encryption
		protocol = ChooseProtocol(mEncryptionAlgS2C, kEncryptionAlgorithms);

		if (protocol == "3des-cbc")
			mDecryptorCipher.reset(new DES_EDE3_Decryption(mKeys[3]));
		else if (protocol == "blowfish-cbc")
			mDecryptorCipher.reset(new BlowfishDecryption(mKeys[3]));
		else if (protocol == "aes128-cbc")
			mDecryptorCipher.reset(new AESDecryption(mKeys[3], 16));
		else if (protocol == "aes192-cbc")
			mDecryptorCipher.reset(new AESDecryption(mKeys[3], 24));
		else if (protocol == "aes256-cbc")
			mDecryptorCipher.reset(new AESDecryption(mKeys[3], 32));

		mDecryptorCBC.reset(
			new CBC_Mode_ExternalCipher::Decryption(
				*mDecryptorCipher.get(), mKeys[1]));
		
//		mDecryptor.reset(new StreamTransformationFilter(
//			*mDecryptorCBC.get(), new StringSink(mInPacket),
//			StreamTransformationFilter::NO_PADDING));

		protocol = ChooseProtocol(mMACAlgC2S, kMacAlgorithms);
		if (protocol == "hmac-sha1")
			mSigner.reset(
				new HMAC<SHA1>(mKeys[4], 20));
		else
			mSigner.reset(
				new HMAC<Weak::MD5>(mKeys[4]));

		protocol = ChooseProtocol(mMACAlgS2C, kMacAlgorithms);
		if (protocol == "hmac-sha1")
			mVerifier.reset(
				new HMAC<SHA1>(mKeys[5], 20));
		else
			mVerifier.reset(
				new HMAC<Weak::MD5>(mKeys[5]));
	
		string compress;
//		if (Preferences::GetInteger("compress-sftp", true))
//			compress = kUseCompressionAlgorithms;
//		else
			compress = kDontUseCompressionAlgorithms;
	
//		protocol = ChooseProtocol(mCompressionAlgS2C, compress);
//		if (protocol == "zlib")
//			mDecompressor.reset(new ZLibHelper(true));
//		
//		protocol = ChooseProtocol(mCompressionAlgC2S, compress);
//		if (protocol == "zlib")
//			mCompressor.reset(new ZLibHelper(false));
		
		if (not mAuthenticated)
		{
			out << uint8(SSH_MSG_SERVICE_REQUEST) << "ssh-userauth";
			mHandler = &MSshConnection::ProcessUserAuthInit;
		}
		else
			mHandler = nil;
	}
}

void MSshConnection::ProcessUserAuthInit(
	uint8		inMessage,
	MSshPacket&	in,
	MSshPacket&	out)
{
	mHandler = nil;
	
	if (inMessage != SSH_MSG_SERVICE_ACCEPT)
		Error(SSH_DISCONNECT_SERVICE_NOT_AVAILABLE);
	else
	{
		eConnectionMessage(_("Starting authentication"));

		out << uint8(SSH_MSG_USERAUTH_REQUEST)
			<< mUserName << "ssh-connection" << "none";
		mHandler = &MSshConnection::ProcessUserAuthNone;
	}
}

void MSshConnection::ProcessUserAuthNone(
	uint8		inMessage,
	MSshPacket&	in,
	MSshPacket&	out)
{
	mHandler = nil;
	
	switch (inMessage)
	{
		case SSH_MSG_USERAUTH_BANNER:
		{
			string msg, lang;
			in >> inMessage >> msg >> lang;
			eConnectionBanner(msg);
			mHandler = &MSshConnection::ProcessUserAuthNone;
			break;
		}
		
		case SSH_MSG_USERAUTH_SUCCESS:	
			UserAuthSuccess();
			break;
		
		case SSH_MSG_USERAUTH_FAILURE:
		{
			string s, p;
			bool partial;
			uint8 msg;
			
			in >> msg >> s >> partial;
			
			p = "keyboard-interactive,password";

			Integer e, n;
			string comment;
			
			mSshAgent.reset(MSshAgent::Create());
			
			if (mSshAgent.get() != nil and mSshAgent->GetFirstIdentity(e, n, comment))
				p.insert(0, "publickey,");
			
//			try
//			{
//				if (Preferences::GetInteger("use-certificate", false) != 0)
//				{
//					mCertificate.reset(new MCertificate(
//						Preferences::GetString("auth-certificate", "")));
//				
//					AddRoute(MCertificate::eCertificateDeleted, eCertificateDeleted);
//
//					if (mCertificate->GetPublicRSAKey(e, n))
//						p.insert(0, "publickey,");
//				}
//			}
//			catch (exception& e)
//			{
//				DisplayError(e);
//			}
			
			s = ChooseProtocol(s, p);
			
			if (s == "publickey")
			{
				MSshPacket blob;
				blob << "ssh-rsa" << e << n;

				out << uint8(SSH_MSG_USERAUTH_REQUEST)
					<< mUserName << "ssh-connection" << "publickey" << false
					<< "ssh-rsa" << blob;

				mHandler = &MSshConnection::ProcessUserAuthPublicKey;
			}
			else if (s == "keyboard-interactive")
			{
				out << uint8(SSH_MSG_USERAUTH_REQUEST)
					<< mUserName << "ssh-connection" << "keyboard-interactive" << "" << "";
				mHandler = &MSshConnection::ProcessUserAuthKeyboardInteractive;
			}
			else if (s == "password")
				TryPassword();
			else
				UserAuthFailed();
			break;
		}
		
		default:
			UserAuthFailed();
			break;
	}
}

void MSshConnection::ProcessUserAuthPublicKey(
	uint8 inMessage, MSshPacket& in, MSshPacket& out)
{
	string instruction, title, lang;
	
	switch (inMessage)
	{
		case SSH_MSG_USERAUTH_BANNER:
		{
			string msg, lang;
			in >> inMessage >> msg >> lang;
			eConnectionBanner(msg);
			break;
		}

#define SSH_MSG_USERAUTH_PK_OK SSH_MSG_USERAUTH_INFO_REQUEST		

		case SSH_MSG_USERAUTH_PK_OK:
		{
			eConnectionMessage(_("Public key authentication"));

			string alg, blob;
			
			in >> inMessage >> alg >> blob;

			out << mSessionId << uint8(SSH_MSG_USERAUTH_REQUEST)
				<< mUserName << "ssh-connection" << "publickey" << true
				<< "ssh-rsa" << blob;

			string sig;
				// no matter what, send a bogus sig if needed
			mSshAgent->SignData(blob, reinterpret_cast<const char*>(out.peek()), sig);
			
			// strip off the session id from the beginning of the out packet
			string sink;
			out >> sink;

			out << sig;
			break;
		}
		
		case SSH_MSG_USERAUTH_FAILURE:
		{
//			string s;
//			bool partial;
			uint8 msg;
			
			in >> msg; // >> s >> partial;
			
			Integer e, n;
			string comment;
			
			if (mSshAgent->GetNextIdentity(e, n, comment))
			{
				MSshPacket blob;
				blob << "ssh-rsa" << e << n;

				out << uint8(SSH_MSG_USERAUTH_REQUEST)
					<< mUserName << "ssh-connection" << "publickey" << false
					<< "ssh-rsa" << blob;
			}
			else// if (ChooseProtocol(s, "password") == "password")
				TryPassword();
//			else
//				UserAuthFailed();
			break;
		}
		
		case SSH_MSG_USERAUTH_SUCCESS:
			UserAuthSuccess();
			break;
	}
}

void MSshConnection::ProcessUserAuthKeyboardInteractive(
	uint8 inMessage, MSshPacket& in, MSshPacket& out)
{
	string instruction, title, lang, p[5];
	bool e[5];
	uint8 msg;
	
	switch (inMessage)
	{
		case SSH_MSG_USERAUTH_BANNER:
		{
			string msg, lang;
			in >> inMessage >> msg >> lang;
			eConnectionBanner(msg);
			break;
		}
		
		case SSH_MSG_USERAUTH_INFO_REQUEST:
		{
			uint32 n;
			in >> msg >> title >> instruction >> lang >> n;
			
			if (n == 0)
				out << uint8(SSH_MSG_USERAUTH_INFO_RESPONSE) << uint32(0);
			else
			{
				if (title.length() == 0)
//					title = MStrings::GetIndString(1011, 0);
					title = "Logging in";
				
				if (instruction.length() == 0)
//					instruction = MStrings::GetFormattedIndString(1011, 1, mUserName, mIPAddress);
					instruction = "Please enter password for acount " + mUserName + " ip address " + mIPAddress;
				
				if (n > 5)
					THROW(("Invalid authentication protocol", 0));
				
				for (uint32 i = 0; i < n; ++i)
					in >> p[i] >> e[i];
				
				if (n == 0)
					n = 1;
				
				MWindow* docWindow = MWindow::GetFirstWindow();	// I hope...
				auto_ptr<MAuthDialog> dlog(new MAuthDialog(title, instruction, n, p, e));
				AddRoute(dlog->eAuthInfo, eRecvAuthInfo);
				dlog->Show(docWindow);
				dlog.release();
			}
			break;
		}
		
		case SSH_MSG_USERAUTH_FAILURE:
		{
			string s;
			bool partial;
			uint8 msg;
			
			in >> msg >> s >> partial;
			
			if (ChooseProtocol(s, "password") == "password")
				TryPassword();
			else
				UserAuthFailed();
			break;
		}
		
		case SSH_MSG_USERAUTH_SUCCESS:
			UserAuthSuccess();
			break;
	}
}

void MSshConnection::RecvAuthInfo(
	vector<string>		inAuthInfo)
{
	if (inAuthInfo.size() > 0)
	{
		MSshPacket out;
		out << uint8(SSH_MSG_USERAUTH_INFO_RESPONSE) << uint32(inAuthInfo.size());
		for (vector<string>::iterator s = inAuthInfo.begin(); s != inAuthInfo.end(); ++s)
			out << *s;
		Send(out);
	}
	else
		Disconnect();
}

void MSshConnection::TryPassword()
{
	eConnectionMessage(_("Password authentication"));

	string p[1];
	bool e[1];
	
//	p[0] = MStrings::GetIndString(1011, 2);
	p[0] = "Password";
	e[0] = false;

	MWindow* docWindow = MWindow::GetFirstWindow();	// I hope...
	unique_ptr<MAuthDialog> dlog(new MAuthDialog("Logging in",
		string("Please enter password for acount ") + mUserName + " ip address " + mIPAddress,
		1, p, e));

	AddRoute(dlog->eAuthInfo, eRecvPassword);

	dlog->Show(docWindow);
	dlog.release();	
}

void MSshConnection::RecvPassword(
	vector<string>	inPassword)
{
	if (inPassword.size() == 1 and inPassword[0].length() > 0)
	{
		MSshPacket out;
		
		out << uint8(SSH_MSG_USERAUTH_REQUEST)
			<< mUserName << "ssh-connection" << "password" << false << inPassword[0];
		
		Send(out);
		
		mHandler = &MSshConnection::ProcessUserAuthSuccess;
	}
	else
		Disconnect();
}

void MSshConnection::ProcessUserAuthSuccess(
	uint8		inMessage,
	MSshPacket&	in,
	MSshPacket&	out)
{
	switch (inMessage)
	{
		case SSH_MSG_USERAUTH_BANNER:
		{
			string msg, lang;
			in >> inMessage >> msg >> lang;
			eConnectionBanner(msg);
			break;
		}
		
		case SSH_MSG_USERAUTH_SUCCESS:
			UserAuthSuccess();
			break;
		
		case SSH_MSG_USERAUTH_FAILURE:
		{
			string s;
			bool partial;
			uint8 msg;
			
			in >> msg >> s >> partial;
			
			if (++mPasswordAttempts < 3)
				TryPassword();
			else
				UserAuthFailed();
			break;
		}
	
		default:
			UserAuthFailed();
			break;
	}
}

void MSshConnection::UserAuthSuccess()
{
//	mAuthenticated = true;
//	mSshAgent.release();
//
//	eConnectionMessage(_("Authenticated"));
//	
//	assert(mOpeningChannels.size() > 0);
//	
//	for_each(mOpeningChannels.begin(), mOpeningChannels.end(),
//		boost::bind(&MSshConnection::OpenChannel, this, _1));
//	
//	mOpeningChannels.clear();
//
//	mHandler = nil;
}

void MSshConnection::UserAuthFailed()
{
//	mAuthenticated = false;
//
//	Disconnect();
//
//	eConnectionMessage(
//		FormatString("Authentication to ^0 with user name ^1 failed",
//			mIPAddress, mUserName));
//	mHandler = nil;
}

//void MSshConnection::ProcessChannelOpen(
//	uint8		inMessage,
//	MSshPacket&	in,
//	MSshPacket&	out)
//{
//	string type;
//	uint32 channelId, windowSize, maxPacketSize;
//	
//	in >> inMessage >> type >> channelId >> windowSize >> maxPacketSize;
//	
//	if (type == "auth-agent@openssh.com" and Preferences::GetInteger("advertise_agent", 1))
//	{
////		mAgentChannel.reset(new MSshAgentChannel(*this));
//		
//		ChannelInfo info = {};
//		
////		info.mChannel = mAgentChannel.get();
//		info.mMyChannel = sNextChannelId++;
//		info.mHostChannel = channelId;
//		info.mMaxSendPacketSize = maxPacketSize;
//		info.mMyWindowSize = kWindowSize;
//		info.mHostWindowSize = windowSize;
//		info.mChannelOpen = true;
//		
//		mChannels.push_back(info);
//
//		out << uint8(SSH_MSG_CHANNEL_OPEN_CONFIRMATION) << channelId
//			<< info.mMyChannel << info.mMyWindowSize << kMaxPacketSize;
//	}
//	else
//	{
//		PRINT(("Wrong type of channel requested"));
//		
//		out << uint8(SSH_MSG_CHANNEL_OPEN_FAILURE) << channelId
//			<< uint8(SSH_MSG_CHANNEL_OPEN_FAILURE) << "unsupported" << "en";
//	}
//}

//void MSshConnection::OpenChannel(
//	MSshChannel*	inChannel)
//{
//	if (not mAuthenticated)
//	{
//		if (mOpenedAt + 30 < GetLocalTime())
//		{
//			Disconnect();
//			Connect(mUserName, mIPAddress, mPortNumber);
//			ResetTimer();
//		}
//
//		AddRoute(eConnectionEvent, inChannel->eConnectionEvent);
//		AddRoute(eConnectionMessage, inChannel->eConnectionMessage);
//		
//		mOpeningChannels.push_back(inChannel);
////		mOpeningChannel->eChannelBanner.AddProto(&eConnectionBanner);
//	}
//	else
//	{
//		// might be an opening channel
//		RemoveRoute(eConnectionEvent, inChannel->eConnectionEvent);
//		RemoveRoute(eConnectionMessage, inChannel->eConnectionMessage);
//
//		inChannel->SetMyChannelID(sNextChannelId++);
//
//		assert(find(mChannels.begin(), mChannels.end(), inChannel) == mChannels.end());
//		
////		if (mAuthenticated and mIsConnected)
////		{
//			MSshPacket out;
//			out << uint8(SSH_MSG_CHANNEL_OPEN) << "session"
//				<< inChannel->GetMyChannelID() << inChannel->GetMyWindowSize() << kMaxPacketSize;
//			Send(out);
//
//			mChannels.push_back(inChannel);
////		}
//	}
//}

//void MSshConnection::CloseChannel(
//	MSshChannel*	inChannel,
//	bool			inErase)
//{
//	ChannelList::iterator ch = find(mChannels.begin(), mChannels.end(), inChannel);
//
//	if (ch != mChannels.end())
//	{
////		if (mIsConnected)
////		{
//			MSshPacket p;
//			p << uint8(SSH_MSG_CHANNEL_CLOSE) << inChannel->GetHostChannelID();
//			Send(p);
////		}
//
//		if (inErase)
//			mChannels.erase(ch);
//		
//		if (not mAuthenticated)
//			Disconnect();
//	}
//}

void MSshConnection::ProcessConfirmChannel(
	uint8		inMessage,
	MSshPacket&	in,
	MSshPacket&	out)
{
	if (inMessage == SSH_MSG_CHANNEL_OPEN_CONFIRMATION)
	{
		uint8 msg;
		uint32 host_channel, my_channel, window_size, max_packet_size;
		
		in >> msg >> my_channel >> host_channel >> window_size >> max_packet_size;

		ChannelList::iterator ch = find_if(mChannels.begin(), mChannels.end(),
			boost::bind(&MSshChannel::GetMyChannelID, _1) == my_channel);

		if (ch == mChannels.end())
			Error(SSH_DISCONNECT_PROTOCOL_ERROR);
		else
		{
			MSshChannel* channel = *ch;
		
			channel->SetHostChannelID(host_channel);
			channel->SetHostWindowSize(window_size);
			channel->SetMaxSendPacketSize(max_packet_size);
			channel->SetChannelOpen(true);
			
			channel->HandleChannelEvent(SSH_CHANNEL_OPENED);
			eConnectionMessage(_("Connecting…"));
			
			if (channel->WantPTY())
			{
				MSshPacket p;
				
				p << uint8(SSH_MSG_CHANNEL_REQUEST)
					<< channel->GetHostChannelID()
					<< "pty-req"
					<< true
					<< "vt100"
					<< uint32(80) << uint32(24)
					<< uint32(0) << uint32(0)
					<< "";
				
				Send(p);
				
				mHandler = &MSshConnection::ProcessConfirmPTY;
			}

			out << uint8(SSH_MSG_CHANNEL_REQUEST)
				<< channel->GetHostChannelID()
				<< channel->GetRequest()
				<< true;
			
			if (strlen(channel->GetCommand()) > 0)
				out << channel->GetCommand();
		}
	}
	else if (inMessage == SSH_MSG_CHANNEL_OPEN_FAILURE)
	{
		uint8 msg;
		uint32 my_channel;
		
		in >> msg >> my_channel >> mErrCode >> mErrString;
		
		ChannelList::iterator ch = find_if(
			mChannels.begin(), mChannels.end(),
			boost::bind(&MSshChannel::GetMyChannelID, _1) == my_channel);
		
		if (ch != mChannels.end())
		{
			(*ch)->HandleChannelEvent(SSH_CHANNEL_ERROR);
			(*ch)->SetChannelOpen(false);
			mChannels.erase(ch);
		}
		
		Disconnect();
	}
}

void MSshConnection::ProcessConfirmPTY(
	uint8		inMessage,
	MSshPacket&	in,
	MSshPacket&	out)
{
	if (inMessage == SSH_MSG_REQUEST_SUCCESS)
		mHandler = nil;
	else
	{
#pragma message("tell user about error")
		Disconnect();
	}
}

void MSshConnection::ProcessChannel(
	uint8		inMessage,
	MSshPacket&	in,
	MSshPacket&	out)
{
	uint8 msg;
	uint32 channelId;
	
	in >> msg >> channelId;
	
	ChannelList::iterator ch = find_if(mChannels.begin(), mChannels.end(),
		boost::bind(&MSshChannel::GetMyChannelID, _1) == channelId);
	
	if (ch != mChannels.end())
	{
		MSshChannel* channel = *ch;
		
		uint32 type;
		MSshPacket p;
		
		switch (msg)
		{
			case SSH_MSG_CHANNEL_WINDOW_ADJUST:
			{
				int32 extra;
				in >> extra;
				channel->SetHostWindowSize(
					channel->GetHostWindowSize() + extra);
				break;
			}
			
			case SSH_MSG_CHANNEL_DATA:
			{
				MSshPacket data;
				in >> data;
				channel->SetMyWindowSize(
					channel->GetMyWindowSize() - data.size());
				channel->HandleData(data);
				break;
			}

			case SSH_MSG_CHANNEL_EXTENDED_DATA:
			{
				MSshPacket data;
				in >> type >> data;
				channel->SetMyWindowSize(
					channel->GetMyWindowSize() - data.size());
				channel->HandleExtraData(type, data);
				break;
			}
			
			case SSH_MSG_CHANNEL_CLOSE:
				mChannels.erase(ch);
				channel->HandleChannelEvent(SSH_CHANNEL_CLOSED);
				channel->SetChannelOpen(false);
				channel = nil;
				break;
			
			case SSH_MSG_CHANNEL_SUCCESS:
				channel->HandleChannelEvent(SSH_CHANNEL_SUCCESS);
				break;

			case SSH_MSG_CHANNEL_FAILURE:
				channel->HandleChannelEvent(SSH_CHANNEL_FAILURE);
				break;
		}

		if (channel != nil and channel->GetMyWindowSize() < kWindowSize - 2 * kMaxPacketSize)
		{
			uint32 adjust = kWindowSize - channel->GetMyWindowSize();
			out << uint8(SSH_MSG_CHANNEL_WINDOW_ADJUST) <<
				channel->GetHostChannelID() << adjust;
			channel->SetMyWindowSize(channel->GetMyWindowSize() + adjust);
		}
	}
}

//string MSshConnection::Wrap(string inData)
//{
//	net_swapper swap;
//	char padding = 0;
//	uint32 blockSize = 8;
//	if (mEncryptorCipher.get() != nil)
//		blockSize = mEncryptorCipher->BlockSize();
//	
//	if (mCompressor.get() != nil)
//		mCompressor->Process(inData);
//	
//	do
//	{
//		inData += rng.GenerateByte();
//		++padding;
//	}
//	while (inData.size() < blockSize or padding < 4 or
//		(inData.size() + 5) % blockSize);
//
//	inData.insert(0, &padding, 1);
//	
//	uint32 l = inData.size();
//	l = swap(l);
//
//	inData.insert(0, reinterpret_cast<char*>(&l), 4);
//	
//	string result;
//	
//	if (mEncryptorCipher.get() == nil)
//		result = inData;
//	else
//	{
//		mOutPacket.clear();
//		
//		mEncryptor->Put(reinterpret_cast<const byte*>(inData.c_str()),
//			inData.length());
//		mEncryptor->Flush(true);
//		
//		uint32 seqNr = swap(mOutSequenceNr);
//		mSigner->Update(reinterpret_cast<byte*>(&seqNr), sizeof(uint32));
//		mSigner->Update(
//			reinterpret_cast<const byte*>(inData.c_str()), inData.length());
//		
//		vector<char> buf(mSigner->DigestSize());
//		mSigner->Final(reinterpret_cast<byte*>(&buf[0]));
//		
//		result = mOutPacket;
//		result.append(&buf[0], mSigner->DigestSize());
//		
//		mOutPacket.clear();
//	}
//
//	++mOutSequenceNr;
//	
//	return result;
//}

string MSshConnection::ChooseProtocol(
	const string& inServer, const string& inClient) const
{
	string result;

	vector<string> server, client;
	ba::split(server, inServer, ba::is_any_of(","));
	ba::split(client, inClient, ba::is_any_of(","));
	
	vector<string>::iterator c, s;
	
	bool found = false;
	
	for (c = client.begin(); c != client.end() and not found; ++c)
	{
		for (s = server.begin(); s != server.end() and not found; ++s)
		{
			if (*s == *c)
			{
				result = *c;
				found = true;
			}
		}
	}
	
	return result;
}

void MSshConnection::DeriveKey(
	char*		inHash,
	int			inNr,
	int			inLength,
	byte*&		outKey)
{
//	MSshPacket p;
//	p << mSharedSecret;
//	
//	SHA1 hash;
//	uint32 dLen = hash.DigestSize();
//	vector<char> H(dLen);
//	
//	char ch = 'A' + inNr;
//	
//	hash.Update(reinterpret_cast<const byte*>(p.data.c_str()), p.data.length());
//	hash.Update(reinterpret_cast<const byte*>(inHash), 20);
//	hash.Update(reinterpret_cast<const byte*>(&ch), 1);
//	hash.Update(reinterpret_cast<const byte*>(mSessionId.c_str()), mSessionId.length());
//	hash.Final(reinterpret_cast<byte*>(&H[0]));
//
//	string result(&H[0], hash.DigestSize());
//	
//	for (inLength -= dLen; inLength > 0; inLength -= dLen)
//	{
//		hash.Update(reinterpret_cast<const byte*>(p.data.c_str()), p.data.length());
//		hash.Update(reinterpret_cast<const byte*>(inHash), 20);
//		hash.Update(reinterpret_cast<const byte*>(result.c_str()), result.length());
//		hash.Final(reinterpret_cast<byte*>(&H[0]));
//		result += string(&H[0], hash.DigestSize());
//	}
//	
//	outKey = new byte[result.length()];
//	copy(result.begin(), result.end(), outKey);
}

string MSshConnection::GetEncryptionParams() const
{
	string result =
		ChooseProtocol(mEncryptionAlgC2S, kEncryptionAlgorithms) + '-' +
		ChooseProtocol(mMACAlgC2S, kMacAlgorithms) + '-';
	
	if (Preferences::GetInteger("compress-sftp", true) != 0)
		result += ChooseProtocol(mCompressionAlgC2S, kUseCompressionAlgorithms);
	else
		result += ChooseProtocol(mCompressionAlgC2S, kDontUseCompressionAlgorithms);
	
	return result;
}
