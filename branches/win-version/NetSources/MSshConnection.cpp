//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

/*	$Id: MSshConnection.cpp,v 1.38 2004/03/05 20:43:26 maarten Exp $
	Copyright Maarten
	Created Sunday September 28 2003 11:35:17
*/

#include "MLib.h"

#include <fstream>
#include <sstream>
#include <cerrno>

#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/foreach.hpp>
#define foreach BOOST_FOREACH

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
//#include <zlib.h>

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
#include "MApplication.h"

#undef GetUserName

using namespace std;
using namespace CryptoPP;
namespace ba = boost::algorithm;
using boost::asio::ip::tcp;
namespace io = boost::iostreams;

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
Integer					p2(k_p_2, sizeof(k_p_2)), q2((p2 - 1) / 2);
Integer					p14(k_p_14, sizeof(k_p_14)), q14((p14 - 1) / 2);

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

#if 0
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
#endif

MSshConnection::MSshConnection(
	const string&	inIPAddress,
	const string&	inUserName,
	uint16			inPortNr)
	: eRecvAuthInfo(this, &MSshConnection::RecvAuthInfo)
	, eRecvPassword(this, &MSshConnection::RecvPassword)
	, eIdle(this, &MSshConnection::Idle)
	, mUserName(inUserName)
	, mIPAddress(inIPAddress)
	, mPortNumber(inPortNr)
	, mConnected(false)
	, mAuthenticated(false)
	, mPasswordAttempts(0)
	, mAuthenticationState(SSH_AUTH_STATE_NONE)
	, mResolver(MIOService::Instance())
	, mSocket(MIOService::Instance())
	, mPacketLength(0)
	, mOutSequenceNr(0)
	, mInSequenceNr(0)
	, eCertificateDeleted(this, &MSshConnection::CertificateDeleted)
{
	AddRoute(gApp->eIdle, eIdle);
	
	foreach (byte*& key, mKeys)
		key = nil;
	
	mNext = sFirstConnection;
	sFirstConnection = this;
}

MSshConnection::~MSshConnection()
{
	foreach (byte*& key, mKeys)
		delete key;

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
	
	if (connection == nil)
		connection = new MSshConnection(inIPAddress, username, inPort);

	return connection;
}

void MSshConnection::Error(
	uint32			inReason,
	const string&	inMessage)
{
#pragma warning("To lower case some day")
	const char* kErrors[] =
	{
		"No error",
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

	try
	{
		string reason = "Reason unknown, code is " + boost::lexical_cast<string>(inReason);
		if (inReason < sizeof(kErrors) / sizeof(char*))
			reason = kErrors[inReason];
	
		eConnectionMessage(FormatString("Error in SSH connection: ^0; ^1", reason, inMessage));
		
		Disconnect();
	}
	catch (...) {}
	
	THROW(("SSH Error (%s)", inMessage.c_str()));
}

void MSshConnection::Connect()
{
	if (not mConnected)
	{
		eConnectionMessage(_("Looking up address"));
	
		tcp::resolver::query query(mIPAddress, boost::lexical_cast<string>(mPortNumber));
	    mResolver.async_resolve(query,
	        boost::bind(&MSshConnection::HandleResolve, this,
	          boost::asio::placeholders::error,
	          boost::asio::placeholders::iterator));
	}
}

void MSshConnection::Disconnect()
{
	if (mConnected)
	{
		mSocket.close();
			
		eConnectionMessage(_("Connection closed"));
	
//		for_each(mChannels.begin(), mChannels.end(),
//			boost::bind(&MSshChannel::ConnectionClosed, _1));
		
		mConnected = false;
    	mAuthenticated = false;
	}
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

void MSshConnection::Idle(double)
{
	foreach (MSshChannel* channel, mChannels)
	{
		MSshPacket p;
		if (channel->PopPending(p))
			Send(p);
	}
}

void MSshConnection::Send(
	MSshPacket&		inPacket)
{
	uint32 blockSize = 8;

	if (mEncryptorCipher.get() != nil)
		blockSize = mEncryptorCipher->BlockSize();
	
//	if (mCompressor.get() != nil)
//		mCompressor->Process(inPacket.data);
	
	inPacket.Wrap(blockSize, rng);

HexDump(inPacket.peek(), inPacket.size(), cerr);
	
	boost::asio::streambuf* request = new boost::asio::streambuf;
	ostream out(request);
	
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

	mRequests.push_back(request);
	
	boost::asio::async_write(mSocket, *request,
		boost::bind(&MSshConnection::PacketSent, this, boost::asio::placeholders::error));
}

void MSshConnection::PacketSent(
	const boost::system::error_code& err)
{
	if (not mRequests.empty())
	{
		delete mRequests.front();
		mRequests.pop_front();
	}
	
    if (err)
    	Error(SSH_DISCONNECT_CONNECTION_LOST, err.message());
}

void MSshConnection::Receive(
	const boost::system::error_code& err)
{
    if (err)
    	Error(SSH_DISCONNECT_CONNECTION_LOST, err.message());

	for (;;)
    {
		uint32 blockSize = 8;
		if (mDecryptorCipher)
			blockSize = mDecryptorCipher->BlockSize();
	    
	    if (mResponse.size() < blockSize)
	    	break;

		istream in(&mResponse);

    	vector<byte> b(blockSize);
	    while (mResponse.size() >= blockSize and mPacket.size() < mPacketLength + sizeof(uint32))
	    {
	    	in.read(reinterpret_cast<char*>(&b[0]), blockSize);
	    	
	    	if (mDecryptorCBC)
	    	{
	    		vector<byte> data(blockSize);
	    		mDecryptorCBC->ProcessData(&data[0], &b[0], blockSize);    			
    			mPacket.insert(mPacket.end(), data.begin(), data.end());
	    	}
			else
				mPacket.insert(mPacket.end(), b.begin(), b.end());
			
			// If this is the first block for a new packet
			// we determine the expected payload and padding length
			if (mPacket.size() == blockSize)
			{
				mPacketLength = 0;
				for (uint32 i = 0; i < 4; ++i)
					mPacketLength = mPacketLength << 8 | mPacket[i];
			}
	    }

		// if we have the entire packet, check the checksum
		if (mPacket.size() == mPacketLength + sizeof(uint32))
		{
			if (mVerifier)
			{
				if (mResponse.size() < mVerifier->DigestSize())
					break;
				
				for (int32 i = 3; i >= 0; --i)
				{
					byte b = mInSequenceNr >> (i * 8);
					mVerifier->Update(&b, 1);
				}
				mVerifier->Update(&mPacket[0], mPacket.size());
				
				vector<byte> b2(mVerifier->DigestSize());
				in.read(reinterpret_cast<char*>(&b2[0]), mVerifier->DigestSize());
				
				if (not mVerifier->Verify(&b2[0]))
					Error(SSH_DISCONNECT_MAC_ERROR, "packet verification failed");
			}
			
			++mInSequenceNr;
				
			try
			{
				MSshPacket in(mPacket);
				ProcessPacket(mPacket[5], in);
			}
			catch (exception& e)
			{
				Error(SSH_DISCONNECT_PROTOCOL_ERROR, e.what());
				eConnectionMessage(e.what());
			}
			
			mPacket.clear();
			mPacketLength = 0;
		}
    }

	uint32 blockSize = 8;
	if (mDecryptorCipher)
		blockSize = mDecryptorCipher->BlockSize();

	boost::asio::async_read(mSocket, mResponse,
		boost::asio::transfer_at_least(blockSize),
		boost::bind(&MSshConnection::Receive, this, boost::asio::placeholders::error));	
}

void MSshConnection::ProcessPacket(
	uint8				inMessage,
	MSshPacket&			in)
{
PRINT(("ProcessPacket %d", inMessage));
	switch (inMessage)
	{
		case SSH_MSG_DISCONNECT:
			ProcessDisconnect(in);
			break;
		
		case SSH_MSG_IGNORE:
			break;
		
		case SSH_MSG_UNIMPLEMENTED:
			Error(SSH_DISCONNECT_PROTOCOL_ERROR, "Unimplemented SSH feature");
			break;
		
		case SSH_MSG_DEBUG:
			ProcessDebug(in);
			break;
		
		case SSH_MSG_SERVICE_ACCEPT:
			ProcessServiceAccept(in);
			break;

		case SSH_MSG_KEXINIT:
			ProcessKexInit(in);
			break;

		case SSH_MSG_NEWKEYS:
			ProcessNewKeys(in);
			break;
		
		case SSH_MSG_KEXDH_REPLY:
			ProcessKexdhReply(in);
			break;

		// authentication
		case SSH_MSG_USERAUTH_SUCCESS:
			ProcessUserAuthSuccess(in);
			break;

		case SSH_MSG_USERAUTH_FAILURE:
			ProcessUserAuthFailed(in);
			break;

		case SSH_MSG_USERAUTH_BANNER:
		{
			string msg, lang;
			in >> inMessage >> msg >> lang;
			eConnectionBanner(msg);
			break;
		}
		
		case SSH_MSG_USERAUTH_INFO_REQUEST:
			ProcessUserAuthInfoRequest(in);
			break;
		
//		case SSH_MSG_CHANNEL_OPEN:
//			ProcessChannelOpen(inMessage, in);
//			break;

		case SSH_MSG_CHANNEL_OPEN_CONFIRMATION:
		case SSH_MSG_CHANNEL_OPEN_FAILURE:
		case SSH_MSG_CHANNEL_WINDOW_ADJUST:
		case SSH_MSG_CHANNEL_DATA:
		case SSH_MSG_CHANNEL_EXTENDED_DATA:
		case SSH_MSG_CHANNEL_EOF:
		case SSH_MSG_CHANNEL_CLOSE:
		case SSH_MSG_CHANNEL_REQUEST:
		case SSH_MSG_CHANNEL_SUCCESS:
		case SSH_MSG_CHANNEL_FAILURE:
			if (not mAuthenticated)
				Error(SSH_DISCONNECT_PROTOCOL_ERROR, "invalid message, not authenticated yet");
			ProcessChannel(inMessage, in);
			break;
		
		default:
			PRINT(("This message should not have been received: %d", inMessage));
	}
}

void MSshConnection::HandleResolve(
	const boost::system::error_code& err,
	tcp::resolver::iterator endpoint_iterator)
{
    if (err)
    	Error(SSH_DISCONNECT_CONNECTION_LOST, err.message());

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
    	mConnected = true;
    	mAuthenticated = false;
    	mAuthenticationState = SSH_AUTH_STATE_NONE;
    	
		eConnectionMessage(_("Connected"));

		boost::asio::streambuf* request = new boost::asio::streambuf;
    	ostream out(request);
    	out << kSSHVersionString << "\r\n";
    	
		// The connection was successful. send protocol string
		mRequests.push_back(request);
		
    	boost::asio::async_write(mSocket, *request,
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
    	Error(SSH_DISCONNECT_CONNECTION_LOST, err.message());
}

void MSshConnection::HandleProtocolVersionExchangeRequest(
	const boost::system::error_code& err)
{
    if (err)
    	Error(SSH_DISCONNECT_PROTOCOL_ERROR, err.message());

	// The connection was successful. Receive initial string
	boost::asio::async_read_until(mSocket, mResponse, "\r\n",
		boost::bind(&MSshConnection::HandleProtocolVersionExchangeResponse, this,
			boost::asio::placeholders::error));
}

void MSshConnection::HandleProtocolVersionExchangeResponse(
	const boost::system::error_code& err)
{
    if (err)
    	Error(SSH_DISCONNECT_PROTOCOL_ERROR, err.message());

	eConnectionMessage(_("Exchanging protocol version"));

	istream response_stream(&mResponse);
	getline(response_stream, mHostVersion);
	mHostVersion.erase(mHostVersion.end() - 1, mHostVersion.end());
	
	if (not ba::starts_with(mHostVersion, "SSH-2.0"))
		Error(SSH_DISCONNECT_PROTOCOL_ERROR, err.message());

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
	Send(out);
	
	// start the read loop
	boost::asio::async_read(mSocket, mResponse, boost::asio::transfer_at_least(8),
		boost::bind(&MSshConnection::Receive, this, boost::asio::placeholders::error));
}

void MSshConnection::ProcessKexInit(
	MSshPacket&		in)
{
	eConnectionMessage(_("Exchanging keys"));

	// capture the packet contents for the host payload
	mHostPayLoad.assign(reinterpret_cast<const char*>(in.peek()), in.size());

	// Now handle the packet
	uint8 msg;
	in >> msg;
	
	if (msg != SSH_MSG_KEXINIT)
		Error(SSH_DISCONNECT_PROTOCOL_ERROR, "Unexpected SSH message");

	uint8 b;
	bool first_kex_packet_follows;
	
	for (uint32 i = 0; i < 16; ++i)
		in >> b;
		
	in	>> mKexAlg
		>> mServerHostKeyAlg
		>> mEncryptionAlgC2S
		>> mEncryptionAlgS2C
		>> mMACAlgC2S
		>> mMACAlgS2C
		>> mCompressionAlgC2S
		>> mCompressionAlgS2C
		>> mLangC2S
		>> mLangS2C
		>> first_kex_packet_follows;

	m_e = 0;

	if (ChooseProtocol(mKexAlg, kKeyExchangeAlgorithms) == "diffie-hellman-group14-sha1")
	{
		do
		{
			m_x.Randomize(rng, 2, q14 - 1);
			m_e = a_exp_b_mod_c(2, m_x, p14);
		}
		while (m_e < 1 or m_e >= p14 - 1);
	}
	else if (ChooseProtocol(mKexAlg, kKeyExchangeAlgorithms) == "diffie-hellman-group1-sha1")
	{
		do
		{
			m_x.Randomize(rng, 2, q2 - 1);
			m_e = a_exp_b_mod_c(2, m_x, p2);
		}
		while (m_e < 1 or m_e >= p2 - 1);
	}
	else
		Error(SSH_DISCONNECT_PROTOCOL_ERROR, "Unexpected key exchange protocol");
	
	MSshPacket out;
	out << uint8(SSH_MSG_KEXDH_INIT) << m_e;
	Send(out);
}

void MSshConnection::ProcessKexdhReply(
	MSshPacket&		in)
{
	MSshPacket hostKey, signature;
	uint8 msg;
	Integer f;

	in >> msg >> hostKey >> f >> signature;
	
	string hostName = mIPAddress;
	if (mPortNumber != 22)
		hostName = hostName + ':' + boost::lexical_cast<string>(mPortNumber);

	Integer K;
	if (ChooseProtocol(mKexAlg, kKeyExchangeAlgorithms) == "diffie-hellman-group14-sha1")
		K = a_exp_b_mod_c(f, m_x, p14);
	else
		K = a_exp_b_mod_c(f, m_x, p2);

	MSshPacket h_test;
	h_test << kSSHVersionString << mHostVersion
		<< mMyPayLoad << mHostPayLoad
		<< hostKey
		<< m_e << f << K;

	SHA1 hash;
	uint32 dLen = hash.DigestSize();
	vector<byte> H(dLen);
	hash.Update(h_test.peek(), h_test.size());
	hash.Final(&H[0]);

	if (mSessionId.empty())
		mSessionId = H;

	auto_ptr<PK_Verifier> h_key;

	string pk_type;
	MSshPacket pk_rs;
	signature >> pk_type >> pk_rs;

	MKnownHosts::Instance().CheckHost(hostName, pk_type, hostKey);

	string h_pk_type;
	hostKey >> h_pk_type;

	if (h_pk_type == "ssh-dss")
	{
		Integer h_p, h_q, h_g, h_y;
		hostKey >> h_p >> h_q >> h_g >> h_y;

		h_key.reset(new GDSA<SHA1>::Verifier(h_p, h_q, h_g, h_y));
	}
	else if (h_pk_type == "ssh-rsa")
	{
		Integer h_e, h_n;
		hostKey >> h_e >> h_n;

		h_key.reset(new RSASSA_PKCS1v15_SHA_Verifier(h_n, h_e));
	}
	else
		Error(SSH_DISCONNECT_KEY_EXCHANGE_FAILED, "Unexpected hostkey type");

	if (pk_type != h_pk_type)
		Error(SSH_DISCONNECT_KEY_EXCHANGE_FAILED, "Unexpected hostkey type");
	
	if (not h_key->VerifyMessage(&H[0], dLen, pk_rs.peek(), pk_rs.size()))
		Error(SSH_DISCONNECT_KEY_EXCHANGE_FAILED, "hostkey verification failed");

	MSshPacket out;
	out << uint8(SSH_MSG_NEWKEYS);
	
	int keyLen = 16;

	if (keyLen < 20 and ChooseProtocol(mMACAlgC2S, kMacAlgorithms) == "hmac-sha1")
		keyLen = 20;

	if (keyLen < 20 and ChooseProtocol(mMACAlgS2C, kMacAlgorithms) == "hmac-sha1")
		keyLen = 20;

	if (keyLen < 24 and ChooseProtocol(mEncryptionAlgC2S, kEncryptionAlgorithms) == "3des-cbc")
		keyLen = 24;

	if (keyLen < 24 and ChooseProtocol(mEncryptionAlgS2C, kEncryptionAlgorithms) == "3des-cbc")
		keyLen = 24;

	if (keyLen < 24 and ChooseProtocol(mEncryptionAlgC2S, kEncryptionAlgorithms) == "aes192-cbc")
		keyLen = 24;

	if (keyLen < 24 and ChooseProtocol(mEncryptionAlgS2C, kEncryptionAlgorithms) == "aes192-cbc")
		keyLen = 24;

	if (keyLen < 32 and ChooseProtocol(mEncryptionAlgC2S, kEncryptionAlgorithms) == "aes256-cbc")
		keyLen = 32;

	if (keyLen < 32 and ChooseProtocol(mEncryptionAlgS2C, kEncryptionAlgorithms) == "aes256-cbc")
		keyLen = 32;

	for (int i = 0; i < 6; ++i)
		DeriveKey(K, &H[0], i, keyLen, mKeys[i]);
	
	Send(out);
}

void MSshConnection::ProcessNewKeys(
	MSshPacket&		in)
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
	else
		Error(SSH_DISCONNECT_PROTOCOL_ERROR, "Invalid encryption cipher");

	mEncryptorCBC.reset(
		new CBC_Mode_ExternalCipher::Encryption(
			*mEncryptorCipher.get(), mKeys[0]));

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
	else
		Error(SSH_DISCONNECT_PROTOCOL_ERROR, "Invalid decryption cipher");

	mDecryptorCBC.reset(
		new CBC_Mode_ExternalCipher::Decryption(
			*mDecryptorCipher.get(), mKeys[1]));

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
//	if (Preferences::GetInteger("compress-sftp", true))
//		compress = kUseCompressionAlgorithms;
//	else
		compress = kDontUseCompressionAlgorithms;

//	protocol = ChooseProtocol(mCompressionAlgS2C, compress);
//	if (protocol == "zlib")
//		mDecompressor.reset(new ZLibHelper(true));
//	
//	protocol = ChooseProtocol(mCompressionAlgC2S, compress);
//	if (protocol == "zlib")
//		mCompressor.reset(new ZLibHelper(false));
	
	if (not mAuthenticated)
	{
		MSshPacket out;
		out << uint8(SSH_MSG_SERVICE_REQUEST) << "ssh-userauth";
		Send(out);
	}
}

void MSshConnection::ProcessDisconnect(
	MSshPacket&	in)
{
	uint8 message;
	uint32 errCode;
	string errString, languageTag;
	
	in >> message >> errCode >> errString >> languageTag;
	
	Error(errCode, errString);
}

void MSshConnection::ProcessDebug(
	MSshPacket&	in)
{
	uint8 msg;
	bool always_display;
	string message, langTag;
	
	in >> msg >> always_display >> message >> langTag;
	PRINT(("Debug message from server: %s", message.c_str()));
}

void MSshConnection::ProcessServiceAccept(
	MSshPacket&	in)
{
	eConnectionMessage(_("Starting authentication"));

	MSshPacket out;
	out << uint8(SSH_MSG_USERAUTH_REQUEST)
		<< mUserName << "ssh-connection" << "none";
	Send(out);
}

void MSshConnection::ProcessUserAuthSuccess(
	MSshPacket&	in)
{
	mAuthenticated = true;
	mSshAgent.release();

	eConnectionMessage(_("Authenticated"));
	
	foreach (MSshChannel* channel, mOpeningChannels)
	{
		channel->Open();
		mChannels.push_back(channel);
	}
	
	mOpeningChannels.clear();
}

void MSshConnection::ProcessUserAuthFailed(
	MSshPacket&	in)
{
	uint8 msg;
	string s;
	bool partial;
	
	in >> msg >> s >> partial;
	
	Integer e, n;
	string comment;
	bool done = false;
	
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

	while (not done)
	{
		switch (mAuthenticationState)
		{
			case SSH_AUTH_STATE_NONE:
				mSshAgent.reset(MSshAgent::Create());
				if (mSshAgent and ChooseProtocol(s, "publickey") == "publickey" and
					mSshAgent->GetFirstIdentity(e, n, comment))
				{
					mAuthenticationState = SSH_AUTH_STATE_PUBLIC_KEY;
					done = true;
				}
				else
					mAuthenticationState = SSH_AUTH_STATE_KEYBOARD_INTERACTIVE;
				break;
			
			case SSH_AUTH_STATE_PUBLIC_KEY:
				if (mSshAgent->GetNextIdentity(e, n, comment))
					done = true;
				else
					mAuthenticationState = SSH_AUTH_STATE_KEYBOARD_INTERACTIVE;
				break;
			
			case SSH_AUTH_STATE_KEYBOARD_INTERACTIVE:
				mAuthenticationState = SSH_AUTH_STATE_PASSWORD;
				break;
			
			case SSH_AUTH_STATE_PASSWORD:
				if (ChooseProtocol(s, "password") == "password" and ++mPasswordAttempts < 3)
					done = true;
				else
					Error(SSH_DISCONNECT_PROTOCOL_ERROR, "no suitable authentication methods");
				break;
		}
	}
	
	switch (mAuthenticationState)
	{
		case SSH_AUTH_STATE_PUBLIC_KEY:
		{
			MSshPacket blob;
			blob << "ssh-rsa" << e << n;

			MSshPacket out;
			out << uint8(SSH_MSG_USERAUTH_REQUEST)
				<< mUserName << "ssh-connection" << "publickey" << false
				<< "ssh-rsa" << blob;
			Send(out);
			break;
		}
		
		case SSH_AUTH_STATE_KEYBOARD_INTERACTIVE:
		{
			MSshPacket out;
			out << uint8(SSH_MSG_USERAUTH_REQUEST)
				<< mUserName << "ssh-connection" << "keyboard-interactive" << "" << "";
			Send(out);
			break;
		}
		
		case SSH_AUTH_STATE_PASSWORD:
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
			break;
		}
		
		default:
			break;
	}
}

void MSshConnection::ProcessUserAuthInfoRequest(
	MSshPacket&		in)
{
	if (mAuthenticationState == SSH_AUTH_STATE_PUBLIC_KEY)
	{
		eConnectionMessage(_("Public key authentication"));
	
		uint8 msg;
		string alg, blob;
	
		in >> msg >> alg >> blob;
	
		MSshPacket out;
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
		Send(out);
	}
	else if (mAuthenticationState == SSH_AUTH_STATE_KEYBOARD_INTERACTIVE)
	{
		uint8 msg;
		string title, instruction, lang, p[5];
		bool e[5];
		uint32 n;
		
		in >> msg >> title >> instruction >> lang >> n;
		
		if (n == 0)
		{
			MSshPacket out;
			out << uint8(SSH_MSG_USERAUTH_INFO_RESPONSE) << uint32(0);
			Send(out);
		}
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
	}
	else
		Error(SSH_DISCONNECT_PROTOCOL_ERROR, "unexpected SSH message");
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

void MSshConnection::RecvPassword(
	vector<string>	inPassword)
{
	if (inPassword.size() == 1 and not inPassword[0].empty())
	{
		MSshPacket out;
		out << uint8(SSH_MSG_USERAUTH_REQUEST)
			<< mUserName << "ssh-connection" << "password" << false << inPassword[0];
		Send(out);
	}
	else
		Disconnect();
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

void MSshConnection::OpenChannel(
	MSshChannel*	inChannel)
{
	if (not mConnected or not mAuthenticated)
	{
		AddRoute(eConnectionMessage, inChannel->eConnectionMessage);
		mOpeningChannels.push_back(inChannel);

		if (not mConnected)
			Connect();
	}
	else
	{
		// might be an opening channel
		RemoveRoute(eConnectionMessage, inChannel->eConnectionMessage);

		mChannels.push_back(inChannel);

		inChannel->Open();
	}
}

void MSshConnection::ProcessChannel(
	uint8		inMessage,
	MSshPacket&	in)
{
	uint8 msg;
	uint32 channelId;
	
	in >> msg >> channelId;
	
	ChannelList::iterator ch = find_if(mChannels.begin(), mChannels.end(),
		boost::bind(&MSshChannel::GetMyChannelID, _1) == channelId);
	
	if (ch == mChannels.end())
		PRINT(("Received a message for an unknown channel"));
	else
	{
		MSshChannel* channel = *ch;
		channel->Process(inMessage, in);
	}
}

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
	const Integer&	inSharedSecret,
	byte*			inHash,
	int				inNr,
	int				inLength,
	byte*&			outKey)
{
	MSshPacket p;
	p << inSharedSecret;
	
	SHA1 hash;
	uint32 dLen = hash.DigestSize();
	vector<byte> H(dLen);
	
	byte ch = 'A' + inNr;
	
	hash.Update(p.peek(), p.size());
	hash.Update(inHash, 20);
	hash.Update(&ch, 1);
	hash.Update(&mSessionId[0], mSessionId.size());
	hash.Final(&H[0]);

	vector<byte> key(H);
	
	for (inLength -= dLen; inLength > 0; inLength -= dLen)
	{
		hash.Update(p.peek(), p.size());
		hash.Update(inHash, 20);
		hash.Update(&key[0], key.size());

		hash.Final(&H[0]);
		
		copy(H.begin(), H.end(), back_inserter(key));
	}
	
	delete outKey;
	outKey = new byte[key.size()];
	copy(key.begin(), key.end(), outKey);
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
