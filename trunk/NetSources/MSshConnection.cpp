/*	$Id: MSshConnection.cpp,v 1.38 2004/03/05 20:43:26 maarten Exp $
	Copyright Maarten
	Created Sunday September 28 2003 11:35:17
*/

#include "MJapieG.h"

#include <fstream>
#include <sstream>
#include <cerrno>

#include <sys/un.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/ioctl.h>

#include <cryptopp/rng.h>
#include <cryptopp/aes.h>
#include <cryptopp/des.h>
#include <cryptopp/hmac.h>
//#include <cryptopp/md5.h>
#include <cryptopp/sha.h>
#include <cryptopp/dsa.h>
#include <cryptopp/rsa.h>
#include <cryptopp/blowfish.h>
#include <cryptopp/filters.h>
#include <cryptopp/factory.h>
#include <zlib.h>

#include "MSshUtil.h"
#include "MError.h"
#include "MSshConnection.h"
#include "MSshChannel.h"
#include "MSshAgent.h"
#include "MAuthDialog.h"
#include "MPreferences.h"
#include "MUtils.h"
#include "MKnownHosts.h"

using namespace std;
using namespace CryptoPP;

uint32 MSshConnection::sNextChannelId;
MSshConnection*	MSshConnection::sFirstConnection;

namespace {

const string
	kSSHVersionString = string("SSH-2.0-") + kAppName + '-' + kVersionString;

const int kDefaultCompressionLevel = 3;

const byte
	k_p[] = {
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xC9, 0x0F, 0xDA, 0xA2, 0x21, 0x68, 0xC2, 0x34,
		0xC4, 0xC6, 0x62, 0x8B, 0x80, 0xDC, 0x1C, 0xD1,
		0x29, 0x02, 0x4E, 0x08, 0x8A, 0x67, 0xCC, 0x74,
		0x02, 0x0B, 0xBE, 0xA6, 0x3B, 0x13, 0x9B, 0x22,
		0x51, 0x4A, 0x08, 0x79, 0x8E, 0x34, 0x04, 0xDD,
		0xEF, 0x95, 0x19, 0xB3, 0xCD, 0x3A, 0x43, 0x1B,
		0x30, 0x2B, 0x0A, 0x6D, 0xF2, 0x5F, 0x14, 0x37,
		0x4F, 0xE1, 0x35, 0x6D, 0x6D, 0x51, 0xC2, 0x45,
		0xE4, 0x85, 0xB5, 0x76, 0x62, 0x5E, 0x7E, 0xC6,
		0xF4, 0x4C, 0x42, 0xE9, 0xA6, 0x37, 0xED, 0x6B,
		0x0B, 0xFF, 0x5C, 0xB6, 0xF4, 0x06, 0xB7, 0xED,
		0xEE, 0x38, 0x6B, 0xFB, 0x5A, 0x89, 0x9F, 0xA5,
		0xAE, 0x9F, 0x24, 0x11, 0x7C, 0x4B, 0x1F, 0xE6,
		0x49, 0x28, 0x66, 0x51, 0xEC, 0xE6, 0x53, 0x81,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
	};

const char
	kKeyExchangeAlgorithms[] = "diffie-hellman-group1-sha1",
	kServerHostKeyAlgorithms[] = "ssh-dss",
	kEncryptionAlgorithms[] = "aes256-cbc,aes192-cbc,aes128-cbc,blowfish-cbc,3des-cbc",
	kMacAlgorithms[] = "hmac-sha1,hmac-sha256",
	kUseCompressionAlgorithms[] = "zlib,none",
	kDontUseCompressionAlgorithms[] = "none,zlib";

#if DEBUG

const char* LookupToken(
	const TokenNames	inTokens[],
	int					inValue)
{
	const TokenNames* t = inTokens;
	while (t->value != 0 and inValue != t->value)
		++t;
	return t->token;
}

#endif
	
// implement as globals to keep things simple
Integer				p, q, g;
auto_ptr<X917RNG>	rng;

} // end private namespace

struct ZLibHelper
{
						ZLibHelper(bool inInflate);
						~ZLibHelper();
	
	int					Process(string& ioData);
	
	z_stream_s			fStream;
	bool				fInflate;
	static const uint32	kBufferSize;
	static char			sBuffer[];
};

const uint32 ZLibHelper::kBufferSize = 1024;
char ZLibHelper::sBuffer[ZLibHelper::kBufferSize];	

ZLibHelper::ZLibHelper(
	bool	inInflate)
	: fInflate(inInflate)
{
	memset(&fStream, 0, sizeof(fStream));

	int err;

	if (inInflate)
		err = inflateInit(&fStream);
	else
		err = deflateInit(&fStream, kDefaultCompressionLevel);
//	if (err != Z_OK)
//		THROW(("Decompression error: %s", fStream.msg));
}

ZLibHelper::~ZLibHelper()
{
	if (fInflate)
		inflateEnd(&fStream);
	else
		deflateEnd(&fStream);
}

int ZLibHelper::Process(
	string&		ioData)
{
	string result;
	
	fStream.next_in = const_cast<byte*>(reinterpret_cast<const byte*>(ioData.c_str()));
	fStream.avail_in = ioData.length();
	fStream.total_in = 0;
	
	fStream.next_out = reinterpret_cast<byte*>(sBuffer);
	fStream.avail_out = kBufferSize;
	fStream.total_out = 0;

	int err;
	do
	{
		if (fInflate)
			err = inflate(&fStream, Z_SYNC_FLUSH);
		else
			err = deflate(&fStream, Z_SYNC_FLUSH);

		if (kBufferSize - fStream.avail_out > 0)
		{
			result.append(sBuffer, kBufferSize - fStream.avail_out);
			fStream.avail_out = kBufferSize;
			fStream.next_out = reinterpret_cast<byte*>(sBuffer);
		}
	}
	while (err >= Z_OK);
	
	ioData = result;
	return err;
}

MSshConnection::MSshConnection()
	: eIdle(this, &MSshConnection::Idle)
	, fHandler(nil)
	, eRecvAuthInfo(this, &MSshConnection::RecvAuthInfo)
	, eRecvPassword(this, &MSshConnection::RecvPassword)
	, fIsConnected(false)
	, fBusy(false)
	, fInhibitIdle(false)
	, fGotVersionString(false)
	, fPacketLength(0)
	, fPasswordAttempts(0)
	, fDecryptorCipher(nil)
	, fDecryptorCBC(nil)
	, fEncryptorCipher(nil)
	, fEncryptorCBC(nil)
	, fSigner(nil)
	, fVerifier(nil)
	, fOutSequenceNr(0)
	, fInSequenceNr(0)
	, fAuthenticated(false)
	, eCertificateDeleted(this, &MSshConnection::CertificateDeleted)
	, fOpeningChannel(nil)
	, fRefCount(1)
	, fOpenedAt(GetLocalTime())
{
	for (int i = 0; i < 6; ++i)
		fKeys[i] = nil;

	AddRoute(gApp->eIdle, eIdle);

	static bool sInited = false;
	if (not sInited)
	{
		byte seed[16];
		
		ifstream f("/dev/random");
		if (f.is_open())
			f.read(reinterpret_cast<char*>(seed), sizeof(seed));
		else
		{
			srand(int(time(nil) % 134567));
			for (int i = 0; i < 16; ++i)
				seed[i] = rand();
		}
		
		rng.reset(new X917RNG(new AESEncryption(seed, 16), seed));

		p = Integer(k_p, sizeof(k_p));
		g = Integer(2);
		q = ((p - 1) / g);
	}
	
	fNext = sFirstConnection;
	sFirstConnection = this;
}

MSshConnection::~MSshConnection()
{
	assert(fRefCount == 0);

	if (this == sFirstConnection)
		sFirstConnection = fNext;
	else
	{
		MSshConnection* c = sFirstConnection;
		while (c != nil)
		{
			MSshConnection* next = c->fNext;
			if (next == this)
			{
				c->fNext = fNext;
				break;
			}
			c = next;
		}
	}
	
	for (int i = 0; i < 6; ++i)
		delete[] fKeys[i];

	try
	{
		RemoveRoute(gApp->eIdle, eIdle);
		
		if (fIsConnected)
			Disconnect();
	}
	catch (...) {}
}

void MSshConnection::Reference()
{
	++fRefCount;
}

void MSshConnection::Release()
{
	--fRefCount;
}

MSshConnection* MSshConnection::Get(
	const string&	inIPAddress,
	const string&	inUserName,
	uint16			inPort)
{
	PRINT(("Connecting to %s:%d as user %s", inIPAddress.c_str(), inPort, inUserName.c_str())); 

	string username = inUserName;
	
	if (username.length() == 0)
		username = GetUserName(true);
	
	if (inPort == 0)
		inPort = 22;
	
	MSshConnection* connection = sFirstConnection;
	while (connection != nil)
	{
		if (connection->fIPAddress == inIPAddress and
			connection->fUserName == username and
			connection->fPortNumber == inPort)
		{
			if (connection->IsConnected() or connection->Busy())
				break;
		}
		
		connection = connection->fNext;
	}
	
	if (connection != nil)
		connection->Reference();
	else
	{
		connection = new MSshConnection();
		if (not connection->Connect(inIPAddress, username, inPort))
		{
			connection->Release();
			connection = nil;
		}
	}
	
	return connection;
}

void MSshConnection::Error(
	int			inReason)
{
//	string msg = MStrings::GetIndString(4008, inReason);
//	if (msg.length() == 0)
//		msg = MStrings::GetIndString(4008, 0);
	
	stringstream s;
	s << "Error in ssh connection: " << inReason;
	
	eConnectionMessage(s.str());
	
	PRINT(("Error: %s (%s)", LookupToken(kErrors, inReason), s.str().c_str()));
	Disconnect();
	
	fErrCode = inReason;
}

bool MSshConnection::Connect(
	string		inIPAddress,
	string		inUserName,
	uint16		inPortNr)
{
	fIPAddress = inIPAddress;
	fUserName = inUserName;
	fPortNumber = inPortNr;

	/* Create socket */
	fSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (fSocket < 0)
	{
		assert (false);
		return false;
	}
	
	eConnectionMessage("Lookink up address");

	/* First do some ip address configuration */
	sockaddr_in lAddr = {};

	lAddr.sin_family = AF_INET;
	lAddr.sin_port = htons(inPortNr);
	lAddr.sin_addr.s_addr = inet_addr(inIPAddress.c_str());

	/* Do DNS check. */
	if (lAddr.sin_addr.s_addr == INADDR_NONE)
	{
		hostent* lHost = gethostbyname(inIPAddress.c_str());
		if (lHost == nil)
			return false;

		assert(lHost->h_addrtype == AF_INET);
		assert(lHost->h_length == sizeof(in_addr));
		lAddr.sin_addr = *reinterpret_cast<in_addr*>(lHost->h_addr);
	}

	/* Set the sockets on non-blocking */
	unsigned long lValue = 1;
	ioctl(fSocket, FIONBIO, &lValue);

	eConnectionMessage("Connecting…");

	/* Connect the control port. */
	int lResult = connect(fSocket, (sockaddr*) &lAddr, sizeof(lAddr));
	if (lResult < 0)
	{
		int err = errno;
		if (err != EAGAIN and err != EINPROGRESS)
			return false;
	}

	/* Even if the connection IS established we still Finish the connect non-blocking */
	fBusy = true;
	return true;
}

void MSshConnection::Disconnect()
{
	if (fIsConnected)
	{
		fIsConnected = false;
	
		PRINT(("Disconnect %s", fIPAddress.c_str()));
		RemoveRoute(eIdle, gApp->eIdle);
		
		close(fSocket);
	
		fBusy = false;
		
		eConnectionMessage("Connection closed");
		eConnectionEvent(SSH_CHANNEL_CLOSED);
	}

	fChannels.clear();
}

void MSshConnection::ResetTimer()
{
	fOpenedAt = GetLocalTime();
}

void MSshConnection::CertificateDeleted(
	MCertificate*	inCertificate)
{
//	if (inCertificate == fCertificate.get())
//	{
//		fCertificate.reset(nil);
//		Disconnect();
//	}
}

void MSshConnection::Idle(
	double	inSystemTime)
{
	if (fInhibitIdle)
		return;
	
	MValueChanger<bool> saveFlag(fInhibitIdle, true);
	
	// avoid being deleted while processing this command
	Reference();
	
	try
	{
		/* Idle processing... */
	
		if (fSocket < 0)
			return;
	
		static fd_set sRead, sWrite, sExcept;
		static timeval sTime = {0,0};
	
		/* Check of action on the socket */
		FD_ZERO(&sRead);
		FD_ZERO(&sWrite);
		FD_ZERO(&sExcept);
	
		/* Setting the check flags */
		if (fIsConnected == false )
		{
			FD_SET(fSocket, &sWrite);
			FD_SET(fSocket, &sExcept);
		}
		else
			FD_SET(fSocket, &sRead);
	
		/* Select */
		int lResult = select(static_cast<int>(fSocket + 1), &sRead, &sWrite, &sExcept, &sTime);
		
		/* Something happened. */
		if (lResult > 0)
		{
			if (FD_ISSET(fSocket, &sExcept))
			{
				fBusy = false;
				Error(SSH_DISCONNECT_CONNECTION_LOST);
			}
	
			if (FD_ISSET(fSocket, &sRead))
			{
//				fIsConnected = true;
//				fBusy = false;

				static char sReadBuf[2 * kMaxPacketSize + 256];
				int lResult;

				lResult = recv(fSocket, sReadBuf, sizeof(sReadBuf), 0);

				PRINT(("-- Read %d bytes of data", lResult));

				/* Something wrong... no connection anymore */
				if (lResult == 0 or lResult < 0)
				{
//					assert(errno != EAGAIN);
					if (errno != EAGAIN)
						PRINT(("connection error %s", strerror(errno)));
					Disconnect();
				}
				else
					fBuffer.append(sReadBuf, static_cast<string::size_type>(lResult));
			}
	
			if (FD_ISSET(fSocket, &sWrite))
			{
				// don't ask me...
//				delay(0.1);
				
				fIsConnected = true;
				fBusy = false;
	
				string cmd = kSSHVersionString;
				cmd += "\r\n";
				
				int lRes = send(fSocket, cmd.c_str(), cmd.length(), 0);
				if (lRes != static_cast<int>(cmd.length()))
					Disconnect();
			}
		}
		
		/* Process data from socket .. if needed */
		while (not fBuffer.empty())
		{
			if (not ProcessBuffer())
				break;
			eConnectionEvent(SSH_CHANNEL_PACKET_DONE);
		}
		
		// see if there's data to be sent
		ChannelList::iterator ch;
	
		for (ch = fChannels.begin(); ch != fChannels.end(); ++ch)
		{
			if (ch->fPending.size() > 0 and
				ch->fPending.front().length() < ch->fHostWindowSize)
			{
				MSshPacket p;
				p.data = ch->fPending.front();
				ch->fPending.pop_front();
				Send(p);
			}
		}

		// if there's a time out, let the user decide what to do		
		if ((fBusy or not fIsConnected) and GetLocalTime() > fOpenedAt + 30)
		{
			eConnectionEvent(SSH_CHANNEL_TIMEOUT);
			
			if (GetLocalTime() > fOpenedAt + 45)
				Disconnect();
		}
	}
	catch (Exception e)
	{
		Disconnect();
		eConnectionMessage(e.what());
		PRINT(("Catched cryptlib exception: %s", e.what()));
	}
	catch (exception e)
	{
		Disconnect();
		eConnectionMessage(e.what());
		PRINT(("Catched exception: %s", e.what()));
	}
	catch (...)
	{
		Disconnect();
		eConnectionMessage("exception");
		PRINT(("Catched unhandled exception"));
	}
	
	Release();
	
	if (fRefCount <= 0 and not fIsConnected and not fBusy)
		delete this;
}

void MSshConnection::Send(string inMessage)
{
	const char* msg = reinterpret_cast<const char*>(inMessage.c_str());
	int lResult = send(fSocket, msg, inMessage.length(), 0);
	if (lResult != static_cast<int>(inMessage.length()))
		Error(SSH_DISCONNECT_CONNECTION_LOST);
}

void MSshConnection::Send(const MSshPacket& inPacket)
{
#if DEBUG
	inPacket.Dump();
#endif
	Send(Wrap(inPacket.data));
}

bool MSshConnection::ProcessBuffer()
{
	bool result = true;

	net_swapper swap;
	
	if (not fGotVersionString)
	{
		string::size_type n = fBuffer.find('\r');
		if (n == string::npos)
			n = fBuffer.find('\n');

		if (n != string::npos)
		{
			fGotVersionString = true;
			
			fInPacket.assign(fBuffer, 0, n);
			
			PRINT(("Version string from host: %s", fInPacket.c_str()));
			
			if (fBuffer[n] == '\r' and fBuffer[n + 1] == '\n')
				fBuffer.erase(0, n + 2);
			else
				fBuffer.erase(0, n + 1);
			
			ProcessConnect();
			fInPacket.clear();
		}
		
		result = fBuffer.length() > 0;
	}
	else
	{
		uint32 blockSize = 8;
		
		if (fDecryptorCipher.get())
			blockSize = fDecryptorCipher->BlockSize();
		
			// first check if we need to add more data 
		while (fBuffer.size() >= blockSize and
			fInPacket.size() < fPacketLength + sizeof(uint32))
		{
			if (fDecryptorCBC.get())
			{
				fDecryptor->Put(
					reinterpret_cast<const byte*>(fBuffer.c_str()), blockSize);
				fDecryptor->Flush(true);
			}
			else
				fInPacket.append(fBuffer.begin(), fBuffer.begin() + blockSize);
			fBuffer.erase(0, blockSize);
			
			// If this is the first block for a new packet
			// we determine the expected payload and padding length
			if (fInPacket.size() == blockSize)
			{
				fPacketLength = swap(
					*reinterpret_cast<const uint32*>(fInPacket.c_str()));
			}
		}

		// if we have the entire packet, check the checksum
		if (fInPacket.size() == fPacketLength + sizeof(uint32))
		{
			bool validMac = false;
			bool done = false;
			
			if (fVerifier.get() == nil)
			{
				validMac = true;
				done = true;
			}
			else if (fBuffer.length() >= fVerifier->DigestSize())
			{
				done = true;
				
				uint32 seqNr = swap(fInSequenceNr);
				
				fVerifier->Update(reinterpret_cast<byte*>(&seqNr), 4);
				fVerifier->Update(reinterpret_cast<const byte*>(fInPacket.c_str()), fInPacket.length());
				
				validMac = fVerifier->Verify(reinterpret_cast<const byte*>(fBuffer.c_str()));

				fBuffer.erase(0, fVerifier->DigestSize());
				
				if (not validMac)
					Error(SSH_DISCONNECT_MAC_ERROR);
			}
			
			if (done and validMac)
			{
				++fInSequenceNr;
				
				// and strip off the packet header and trailer

				uint8 paddingLength = fInPacket[4];

				fInPacket.erase(0, 5);
				fInPacket.erase(fInPacket.length() - paddingLength, paddingLength);

				// decompress now
				
				if (fDecompressor.get() != nil)
					fDecompressor->Process(fInPacket);
				
				try
				{
					ProcessPacket();
				}
				catch (MSshPacket::MSshPacketError& e)
				{
					Error(SSH_DISCONNECT_PROTOCOL_ERROR);
					eConnectionMessage(e.what());
				}
				catch (exception& e)
				{
					Error(SSH_DISCONNECT_PROTOCOL_ERROR);
					eConnectionMessage(e.what());
				}
			}
			else
				result = false;
		}
		else
			result = false;
	}
	
	return result;
}

void MSshConnection::ProcessPacket()
{
	uint8 message = fInPacket[0];

	PRINT((">> %s", LookupToken(kTokens, message)));

	MSshPacket in, out;
	in.data = fInPacket;

#if DEBUG
	in.Dump();
#endif

	switch (message)
	{
		case SSH_MSG_DISCONNECT:
			ProcessDisconnect(message, in, out);
			break;
		
		case SSH_MSG_DEBUG:
			ProcessDebug(message, in, out);
			break;
		
		case SSH_MSG_IGNORE:
			break;
		
		case SSH_MSG_UNIMPLEMENTED:
			ProcessUnimplemented(message, in, out);
			break;
		
		case SSH_MSG_CHANNEL_OPEN_CONFIRMATION:
		case SSH_MSG_CHANNEL_OPEN_FAILURE:
			ProcessConfirmChannel(message, in, out);
			break;

		case SSH_MSG_CHANNEL_WINDOW_ADJUST:
		case SSH_MSG_CHANNEL_DATA:
		case SSH_MSG_CHANNEL_EXTENDED_DATA:
		case SSH_MSG_CHANNEL_EOF:
		case SSH_MSG_CHANNEL_CLOSE:
		case SSH_MSG_CHANNEL_SUCCESS:
		case SSH_MSG_CHANNEL_FAILURE:
			if (fAuthenticated)
				ProcessChannel(message, in, out);
			else
				Error(SSH_DISCONNECT_PROTOCOL_ERROR);
			break;

		case SSH_MSG_CHANNEL_REQUEST:
			ProcessChannelRequest(message, in, out);
			break;
		
		case SSH_MSG_CHANNEL_OPEN:
			ProcessChannelOpen(message, in, out);
			break;
		
		default:
			if (fHandler != nil)
				(this->*fHandler)(message, in, out);
			else
				PRINT(("This message should not have been received: %d", message));
	}

	if (in.data.length() > 0)
		PRINT(("Data left in packet %d", message));

	fInPacket.clear();
	
	if (out.data.length() > 0)
		Send(out);
	
	fInPacket.clear();
}

void MSshConnection::ProcessDisconnect(
	uint8		inMessage,
	MSshPacket&	in,
	MSshPacket&	out)
{
	uint8 message;
	string languageTag;
	
	in >> message >> fErrCode >> fErrString >> languageTag;
	
	PRINT(("Disconnected: %s", fErrString.c_str()));

	Disconnect();
}

void MSshConnection::ProcessUnimplemented(
	uint8		inMessage,
	MSshPacket&	in,
	MSshPacket&	out)
{
	PRINT(("Unimplemented"));
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
	
	PRINT(("Channel request for %d: %s%s", channel,
		request.c_str(), wantReply ? " (wants reply)" : ""));
	
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
	
	PRINT(("Debug: %s", message.c_str()));
}

void MSshConnection::ProcessConnect()
{
	if (fInPacket.substr(0, 4) != "SSH-")
		Error(SSH_DISCONNECT_PROTOCOL_ERROR);

	eConnectionMessage("Exchanging keys");
	
	fHostVersion = fInPacket;

	// create and send our KEXINIT packet

	MSshPacket data;
	
	data << uint8(SSH_MSG_KEXINIT);
	
	for (uint32 i = 0; i < 16; ++i)
		data << rng->GenerateByte();

	string compress;
	if (Preferences::GetInteger("compress-sftp", true))
		compress = kUseCompressionAlgorithms;
	else
		compress = kDontUseCompressionAlgorithms;

	data << kKeyExchangeAlgorithms
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

	fMyPayLoad = data.data;
	fHandler = &MSshConnection::ProcessKexInit;

	Send(data);
}

void MSshConnection::ProcessKexInit(
	uint8		inMessage,
	MSshPacket&	in,
	MSshPacket&	out)
{
	if (inMessage != SSH_MSG_KEXINIT)
		Error(SSH_DISCONNECT_PROTOCOL_ERROR);
	else
	{
		fHostPayLoad = fInPacket;
		
		uint8 msg, b;
		bool first_kex_packet_follows;
	
		in >> msg;

		for (uint32 i = 0; i < 16; ++i)
			in >> b;
		
		in	>> fKexAlg
			>> fServerHostKeyAlg
			>> fEncryptionAlgC2S
			>> fEncryptionAlgS2C
			>> fMACAlgC2S
			>> fMACAlgS2C
			>> fCompressionAlgC2S
			>> fCompressionAlgS2C
			>> fLangC2S
			>> fLangS2C
			>> first_kex_packet_follows;
	
		do
		{
			f_x.Randomize(*rng.get(), Integer(2), q - 1);
			f_e = a_exp_b_mod_c(g, f_x, p);
		}
		while (f_e < 1);

		out << uint8(SSH_MSG_KEXDH_INIT) << f_e;
		
		fHandler = &MSshConnection::ProcessKexdhReply;
	}
}

void MSshConnection::ProcessKexdhReply(
	uint8		inMessage,
	MSshPacket&	in,
	MSshPacket&	out)
{
	try
	{
		if (inMessage != SSH_MSG_KEXDH_REPLY)
			throw uint32(SSH_DISCONNECT_PROTOCOL_ERROR);

		uint8 msg;
		string hostKey, signature;
		Integer f;
	
		in >> msg >> hostKey >> f >> signature;
		
		string hostName = fIPAddress;
		if (fPortNumber != 22)
			hostName = hostName + ':' + NumToString(fPortNumber);

		try
		{
			MKnownHosts::Instance().CheckHost(hostName, hostKey);
		}
		catch (...)
		{
			Disconnect();
			throw;
		}
		
		string h_pk_type;
		Integer h_p, h_q, h_g, h_y;
		
		MSshPacket packet;
		packet.data = hostKey;
		packet >> h_pk_type >> h_p >> h_q >> h_g >> h_y;
		
		DSA::Verifier h_key(h_p, h_q, h_g, h_y);
		
		if (h_pk_type != "ssh-dss")
			throw uint32(SSH_DISCONNECT_PROTOCOL_ERROR);
		
		string pk_type, pk_rs;
		
		packet.data = signature;
		packet >> pk_type >> pk_rs;
		const byte* h_sig = reinterpret_cast<const byte*>(pk_rs.c_str());
		
		if (pk_type != "ssh-dss" or pk_rs.length() != 40)
			throw uint32(SSH_DISCONNECT_PROTOCOL_ERROR);
		
		Integer K = a_exp_b_mod_c(f, f_x, p);
		
		fSharedSecret = K;
		
		MSshPacket h_test;
		h_test << kSSHVersionString << fHostVersion
			<< fMyPayLoad << fHostPayLoad
			<< hostKey
			<< f_e << f << K;
		
		SHA1 hash;
		uint32 dLen = hash.DigestSize();
		vector<char>	H(dLen);
		
		hash.Update(
			reinterpret_cast<const byte*>(h_test.data.c_str()),
			h_test.data.length());
		hash.Final(reinterpret_cast<byte*>(&H[0]));
		
		if (fSessionId.length() == 0)
			fSessionId.assign(&H[0], dLen);
	
		if (not h_key.VerifyMessage(reinterpret_cast<byte*>(&H[0]), dLen, h_sig, pk_rs.length()))
			throw uint32(SSH_DISCONNECT_PROTOCOL_ERROR);

		out << uint8(SSH_MSG_NEWKEYS);
		
		int keyLen = 16;

		if (keyLen < 20 and ChooseProtocol(fMACAlgC2S, kMacAlgorithms) == "hmac-sha1")
			keyLen = 24;

		if (keyLen < 20 and ChooseProtocol(fMACAlgS2C, kMacAlgorithms) == "hmac-sha1")
			keyLen = 24;

		if (keyLen < 24 and ChooseProtocol(fEncryptionAlgC2S, kEncryptionAlgorithms) == "3des-cbc")
			keyLen = 24;

		if (keyLen < 24 and ChooseProtocol(fEncryptionAlgS2C, kEncryptionAlgorithms) == "3des-cbc")
			keyLen = 24;

		if (keyLen < 24 and ChooseProtocol(fEncryptionAlgC2S, kEncryptionAlgorithms) == "aes192-cbc")
			keyLen = 24;

		if (keyLen < 24 and ChooseProtocol(fEncryptionAlgS2C, kEncryptionAlgorithms) == "aes192-cbc")
			keyLen = 24;

		if (keyLen < 32 and ChooseProtocol(fEncryptionAlgC2S, kEncryptionAlgorithms) == "aes256-cbc")
			keyLen = 32;

		if (keyLen < 32 and ChooseProtocol(fEncryptionAlgS2C, kEncryptionAlgorithms) == "aes256-cbc")
			keyLen = 32;

		for (int i = 0; i < 6; ++i)
		{
			delete[] fKeys[i];
			DeriveKey(&H[0], i, keyLen, fKeys[i]);
		}
		
		fHandler = &MSshConnection::ProcessNewKeys;
	}
	catch (uint32 err)
	{
		Error(err);
		out.data.clear();
		fHandler = nil;
	}
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
		protocol = ChooseProtocol(fEncryptionAlgC2S, kEncryptionAlgorithms);
		
		if (protocol == "3des-cbc")
			fEncryptorCipher.reset(new DES_EDE3::Encryption(fKeys[2]));
		else if (protocol == "blowfish-cbc")
			fEncryptorCipher.reset(new BlowfishEncryption(fKeys[2]));
		else if (protocol == "aes128-cbc")
			fEncryptorCipher.reset(new AES::Encryption(fKeys[2], 16));
		else if (protocol == "aes192-cbc")
			fEncryptorCipher.reset(new AES::Encryption(fKeys[2], 24));
		else if (protocol == "aes256-cbc")
			fEncryptorCipher.reset(new AES::Encryption(fKeys[2], 32));

		fEncryptorCBC.reset(
			new CBC_Mode_ExternalCipher::Encryption(
				*fEncryptorCipher.get(), fKeys[0]));
		
		fEncryptor.reset(new StreamTransformationFilter(
			*fEncryptorCBC.get(), new StringSink(fOutPacket),
			StreamTransformationFilter::NO_PADDING));

		// Server to client encryption
		protocol = ChooseProtocol(fEncryptionAlgS2C, kEncryptionAlgorithms);

		if (protocol == "3des-cbc")
			fDecryptorCipher.reset(new DES_EDE3_Decryption(fKeys[3]));
		else if (protocol == "blowfish-cbc")
			fDecryptorCipher.reset(new BlowfishDecryption(fKeys[3]));
		else if (protocol == "aes128-cbc")
			fDecryptorCipher.reset(new AESDecryption(fKeys[3], 16));
		else if (protocol == "aes192-cbc")
			fDecryptorCipher.reset(new AESDecryption(fKeys[3], 24));
		else if (protocol == "aes256-cbc")
			fDecryptorCipher.reset(new AESDecryption(fKeys[3], 32));

		fDecryptorCBC.reset(
			new CBC_Mode_ExternalCipher::Decryption(
				*fDecryptorCipher.get(), fKeys[1]));
		
		fDecryptor.reset(new StreamTransformationFilter(
			*fDecryptorCBC.get(), new StringSink(fInPacket),
			StreamTransformationFilter::NO_PADDING));

		protocol = ChooseProtocol(fMACAlgC2S, kMacAlgorithms);
		if (protocol == "hmac-sha1")
			fSigner.reset(
				new HMAC<SHA1>(fKeys[4], 20));
		else
			fSigner.reset(
				new HMAC<SHA256>(fKeys[4]));

		protocol = ChooseProtocol(fMACAlgS2C, kMacAlgorithms);
		if (protocol == "hmac-sha1")
			fVerifier.reset(
				new HMAC<SHA1>(fKeys[5], 20));
		else
			fVerifier.reset(
				new HMAC<SHA256>(fKeys[5]));
	
		string compress;
		if (Preferences::GetInteger("compress-sftp", true))
			compress = kUseCompressionAlgorithms;
		else
			compress = kDontUseCompressionAlgorithms;
	
		protocol = ChooseProtocol(fCompressionAlgS2C, compress);
		if (protocol == "zlib")
			fDecompressor.reset(new ZLibHelper(true));
		
		protocol = ChooseProtocol(fCompressionAlgC2S, compress);
		if (protocol == "zlib")
			fCompressor.reset(new ZLibHelper(false));
		
		if (not fAuthenticated)
		{
			out << uint8(SSH_MSG_SERVICE_REQUEST) << "ssh-userauth";
			fHandler = &MSshConnection::ProcessUserAuthInit;
		}
		else
			fHandler = nil;
	}
}

void MSshConnection::ProcessUserAuthInit(
	uint8		inMessage,
	MSshPacket&	in,
	MSshPacket&	out)
{
	fHandler = nil;
	
	if (inMessage != SSH_MSG_SERVICE_ACCEPT)
		Error(SSH_DISCONNECT_SERVICE_NOT_AVAILABLE);
	else
	{
		eConnectionMessage("Starting authentication");

		out << uint8(SSH_MSG_USERAUTH_REQUEST)
			<< fUserName << "ssh-connection" << "none";
		fHandler = &MSshConnection::ProcessUserAuthNone;
	}
}

void MSshConnection::ProcessUserAuthNone(
	uint8		inMessage,
	MSshPacket&	in,
	MSshPacket&	out)
{
	fHandler = nil;
	
	switch (inMessage)
	{
		case SSH_MSG_USERAUTH_BANNER:
		{
			string msg, lang;
			in >> inMessage >> msg >> lang;
			eConnectionBanner(msg);
			fHandler = &MSshConnection::ProcessUserAuthNone;
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
			
			PRINT(("UserAuth failure: %s (%s)", s.c_str(), (partial ? "partial" : "final")));
			
			p = "keyboard-interactive,password";

			Integer e, n;
			string comment;
			
			fSshAgent.reset(MSshAgent::Create());
			
			if (fSshAgent.get() != nil and fSshAgent->GetFirstIdentity(e, n, comment))
				p.insert(0, "publickey,");
			
//			try
//			{
//				if (Preferences::GetInteger("use-certificate", false) != 0)
//				{
//					fCertificate.reset(new MCertificate(
//						Preferences::GetString("auth-certificate", "")));
//				
//					AddRoute(MCertificate::eCertificateDeleted, eCertificateDeleted);
//
//					if (fCertificate->GetPublicRSAKey(e, n))
//						p.insert(0, "publickey,");
//				}
//			}
//			catch (exception& e)
//			{
//				MError::DisplayError(e);
//			}
			
			s = ChooseProtocol(s, p);
			
			if (s == "publickey")
			{
				MSshPacket blob;
				blob << "ssh-rsa" << e << n;

				out << uint8(SSH_MSG_USERAUTH_REQUEST)
					<< fUserName << "ssh-connection" << "publickey" << false
					<< "ssh-rsa" << blob.data;

				fHandler = &MSshConnection::ProcessUserAuthPublicKey;
			}
			else if (s == "keyboard-interactive")
			{
				out << uint8(SSH_MSG_USERAUTH_REQUEST)
					<< fUserName << "ssh-connection" << "keyboard-interactive" << "" << "";
				fHandler = &MSshConnection::ProcessUserAuthKeyboardInteractive;
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
			eConnectionMessage("Public key authentication");

			string alg, blob;
			
			in >> inMessage >> alg >> blob;

			out << fSessionId << uint8(SSH_MSG_USERAUTH_REQUEST)
				<< fUserName << "ssh-connection" << "publickey" << true
				<< "ssh-rsa" << blob;

			string sig;
				// no matter what, send a bogus sig if needed
			fSshAgent->SignData(blob, out.data, sig);
			
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
			
			if (fSshAgent->GetNextIdentity(e, n, comment))
			{
				MSshPacket blob;
				blob << "ssh-rsa" << e << n;

				out << uint8(SSH_MSG_USERAUTH_REQUEST)
					<< fUserName << "ssh-connection" << "publickey" << false
					<< "ssh-rsa" << blob.data;
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
//					instruction = MStrings::GetFormattedIndString(1011, 1, fUserName, fIPAddress);
					instruction = "Please enter password for acount " + fUserName + " ip address " + fIPAddress;
				
				if (n > 5)
					THROW(("Invalid authentication protocol", 0));
				
				for (uint32 i = 0; i < n; ++i)
					in >> p[i] >> e[i];
				
				if (n == 0)
					n = 1;
				
				auto_ptr<MAuthDialog> dlog(new MAuthDialog(title, instruction, n, p, e));
				AddRoute(dlog->eAuthInfo, eRecvAuthInfo);
				dlog->Show(nil);
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
	eConnectionMessage("Password authentication");

	string p[1];
	bool e[1];
	
//	p[0] = MStrings::GetIndString(1011, 2);
	p[0] = "Password";
	e[0] = false;

//	MAuthDialog* dlog = new MAuthDialog("Logging in",
//		string("Please enter password for acount ") + fUserName + " ip address " + fIPAddress,
//		1, p, e);

	auto_ptr<MAuthDialog> dlog(new MAuthDialog("Logging in",
		string("Please enter password for acount ") + fUserName + " ip address " + fIPAddress,
		1, p, e));

	AddRoute(dlog->eAuthInfo, eRecvPassword);

	dlog->Show(nil);
	dlog.release();	
}

void MSshConnection::RecvPassword(
	vector<string>	inPassword)
{
	if (inPassword.size() == 1 and inPassword[0].length() > 0)
	{
		MSshPacket out;
		
		out << uint8(SSH_MSG_USERAUTH_REQUEST)
			<< fUserName << "ssh-connection" << "password" << false << inPassword[0];
		
		Send(out);
		
		fHandler = &MSshConnection::ProcessUserAuthSuccess;
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
			
			if (++fPasswordAttempts < 3)
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
	fAuthenticated = true;
	fSshAgent.release();

	eConnectionMessage("Authenticated");
	
	assert(fOpeningChannel != nil);
	if (fOpeningChannel != nil)
	{
		OpenChannel(fOpeningChannel);
//		fOpeningChannel->eChannelBanner.RemoveProto(&eConnectionBanner);
	}
	fHandler = nil;
}

void MSshConnection::UserAuthFailed()
{
	fAuthenticated = false;

	Disconnect();

	eConnectionMessage(
//		MStrings::GetFormattedIndString(1011, 3, fUserName, fIPAddress));
		string("Authentication to ") + fIPAddress + " with user name " + fUserName + " failed");
	fHandler = nil;
}

void MSshConnection::ProcessChannelOpen(
	uint8		inMessage,
	MSshPacket&	in,
	MSshPacket&	out)
{
	string type;
	uint32 channelId, windowSize, maxPacketSize;
	
	in >> inMessage >> type >> channelId >> windowSize >> maxPacketSize;
	
	if (type == "auth-agent@openssh.com" and Preferences::GetInteger("advertise_agent", 1))
	{
//		fAgentChannel.reset(new MSshAgentChannel(*this));
		
		ChannelInfo info = {};
		
//		info.fChannel = fAgentChannel.get();
		info.fMyChannel = sNextChannelId++;
		info.fHostChannel = channelId;
		info.fMaxSendPacketSize = maxPacketSize;
		info.fMyWindowSize = kWindowSize;
		info.fHostWindowSize = windowSize;
		info.fChannelOpen = true;
		
		fChannels.push_back(info);

		out << uint8(SSH_MSG_CHANNEL_OPEN_CONFIRMATION) << channelId
			<< info.fMyChannel << info.fMyWindowSize << kMaxPacketSize;
	}
	else
	{
		PRINT(("Wrong type of channel requested"));
		
		out << uint8(SSH_MSG_CHANNEL_OPEN_FAILURE) << channelId
			<< uint8(SSH_MSG_CHANNEL_OPEN_FAILURE) << "unsupported" << "en";
	}
}

void MSshConnection::OpenChannel(
	MSshChannel*	inChannel)
{
	if (not fAuthenticated)
	{
		if (fOpenedAt + 30 < GetLocalTime())
		{
			Disconnect();
			Connect(fUserName, fIPAddress, fPortNumber);
			ResetTimer();
		}
		
//		assert(fOpeningChannel == nil);
		
//		if (fOpeningChannel != nil)
//			throw MError(pErrCouldNotOpenConnection);
		
		fOpeningChannel = inChannel;
//		fOpeningChannel->eChannelBanner.AddProto(&eConnectionBanner);
	}
	else
	{
		ChannelInfo info;
		
		info.fChannel = inChannel;
		info.fMyChannel = sNextChannelId++;
		info.fHostChannel = 0;
		info.fMaxSendPacketSize = 0;
		info.fMyWindowSize = kWindowSize;
		info.fHostWindowSize = 0;
		info.fChannelOpen = false;

		assert(find(fChannels.begin(), fChannels.end(), info) == fChannels.end());
		
		if (fAuthenticated and fIsConnected)
		{
			MSshPacket out;
			out << uint8(SSH_MSG_CHANNEL_OPEN) << "session"
				<< info.fMyChannel << info.fMyWindowSize << kMaxPacketSize;
			Send(out);

			fChannels.push_back(info);
		}
	}
}

void MSshConnection::CloseChannel(
	MSshChannel*	inChannel)
{
	ChannelInfo info;
	info.fChannel = inChannel;
	
	ChannelList::iterator i = find(fChannels.begin(), fChannels.end(), info);
	if (i != fChannels.end())
	{
		info = *i;
		
		fChannels.erase(i);
		
		if (fIsConnected)
		{
			MSshPacket p;
			p << uint8(SSH_MSG_CHANNEL_CLOSE) << info.fHostChannel;
			Send(p);
		}
		
		if (not fAuthenticated)
			Disconnect();
	}
}

void MSshConnection::SendChannelData(MSshChannel* inChannel, uint32 inType, string inData)
{
	ChannelList::iterator ch;
	
	for (ch = fChannels.begin(); ch != fChannels.end(); ++ch)
	{
		if (ch->fChannel == inChannel)
			break;
	}
	
	if (ch != fChannels.end())
	{
		assert(inData.length() < ch->fMaxSendPacketSize);

		MSshPacket p;
		if (inType == 0)
			p << uint8(SSH_MSG_CHANNEL_DATA) << ch->fHostChannel << inData;
		else
			p << uint8(SSH_MSG_CHANNEL_EXTENDED_DATA) << ch->fHostChannel
				<< inType << inData;
		
		if (inData.length() < ch->fHostWindowSize)
		{
			Send(p);
			ch->fHostWindowSize -= inData.length();
		}
		else
			ch->fPending.push_back(p.data);
	}
	else
		assert(false);
}

void MSshConnection::SendWindowResize(MSshChannel* inChannel, uint32 inColumns, uint32 inRows)
{
	ChannelList::iterator ch;
	
	for (ch = fChannels.begin(); ch != fChannels.end(); ++ch)
	{
		if (ch->fChannel == inChannel)
			break;
	}
	
	if (ch != fChannels.end())
	{
		MSshPacket p;
		
		p << uint8(SSH_MSG_CHANNEL_REQUEST) << ch->fHostChannel
			<< "window-change" << false
			<< inColumns << inRows
			<< uint32(0) << uint32(0);

		Send(p);
	}
	else
		assert(false);
}

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

		ChannelList::iterator ch;
		for (ch = fChannels.begin(); ch != fChannels.end(); ++ch)
		{
			if (ch->fMyChannel == my_channel)
			{
				ch->fHostChannel = host_channel;
				ch->fHostWindowSize = window_size;
				ch->fMaxSendPacketSize = max_packet_size;
				ch->fChannelOpen = true;
				
				ch->fChannel->eChannelEvent(SSH_CHANNEL_OPENED);
				eConnectionMessage("Connecting…");
				
				if (ch->fChannel->WantPTY())
				{
//					if (fAgentChannel.get() == nil and
//						Preferences::GetInteger("advertise_agent", 1))
//					{
//						MSshPacket p;
//						p << uint8(SSH_MSG_CHANNEL_REQUEST) << host_channel
//							<< "auth-agent-req@openssh.com" << false;
//						Send(p);
//					}
	
					MSshPacket p;
					
					p << uint8(SSH_MSG_CHANNEL_REQUEST)
						<< ch->fHostChannel
						<< "pty-req"
						<< true
						<< "vt100"
						<< uint32(80) << uint32(24)
						<< uint32(0) << uint32(0)
						<< "";
					
					Send(p);
					
					fHandler = &MSshConnection::ProcessConfirmPTY;
				}

				out << uint8(SSH_MSG_CHANNEL_REQUEST)
					<< ch->fHostChannel
					<< ch->fChannel->GetRequest()
					<< true;
				
				if (strlen(ch->fChannel->GetCommand()) > 0)
					out << ch->fChannel->GetCommand();

				break;
			}
		}
		
		if (ch == fChannels.end())
			Error(SSH_DISCONNECT_PROTOCOL_ERROR);
	}
	else if (inMessage == SSH_MSG_CHANNEL_OPEN_FAILURE)
	{
		uint8 msg;
		uint32 my_channel;
		
		in >> msg >> my_channel >> fErrCode >> fErrString;
		
		PRINT(("Channel open failed: %s", fErrString.c_str()));

		ChannelList::iterator ch;
		for (ch = fChannels.begin(); ch != fChannels.end(); ++ch)
		{
			if (ch->fMyChannel == my_channel)
			{
				ch->fChannel->eChannelEvent(SSH_CHANNEL_ERROR);
				
				fChannels.erase(ch);
				break;
			}
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
	{
		PRINT(("Got a pty!"));
		fHandler = nil;
	}
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
	
	PRINT(("%s for channel %d", LookupToken(kTokens, msg), channelId));
	
	MSshChannel* channel = nil;
	ChannelList::iterator ch;
	for (ch = fChannels.begin(); ch != fChannels.end(); ++ch)
	{
		if (ch->fMyChannel == channelId)
		{
			channel = ch->fChannel;
			break;
		}
	}
	
	if (channel == nil)
	{
		PRINT(("Received msg %d for closed channel %d", msg, channelId));
	}
	else
	{
		assert(ch != fChannels.end() or msg == SSH_MSG_CHANNEL_CLOSE);
		
		uint32 type;
		string data;
		MSshPacket p;
		
		switch (msg)
		{
			case SSH_MSG_CHANNEL_WINDOW_ADJUST: {
				int32 extra;
				in >> extra;
				ch->fHostWindowSize += extra;
				break;
			}
			
			case SSH_MSG_CHANNEL_DATA:
				in >> data;
				ch->fMyWindowSize -= data.length();
				channel->MandleData(data);
				break;

			case SSH_MSG_CHANNEL_EXTENDED_DATA:
				in >> type >> data;
				ch->fMyWindowSize -= data.length();
				channel->MandleExtraData(type, data);
				break;
			
			case SSH_MSG_CHANNEL_CLOSE:
				if (ch != fChannels.end())
					fChannels.erase(ch);
				if (channel != nil)
					channel->eChannelEvent(SSH_CHANNEL_CLOSED);
				break;
			
			case SSH_MSG_CHANNEL_SUCCESS:
				channel->eChannelEvent(SSH_CHANNEL_SUCCESS);
				break;

			case SSH_MSG_CHANNEL_FAILURE:
				channel->eChannelEvent(SSH_CHANNEL_FAILURE);
				break;
		}

		if (ch->fMyWindowSize < kWindowSize - 2 * kMaxPacketSize)
		{
			uint32 adjust = kWindowSize - ch->fMyWindowSize;
			out << uint8(SSH_MSG_CHANNEL_WINDOW_ADJUST) <<
				ch->fHostChannel << adjust;
			ch->fMyWindowSize += adjust;
		}
	}
}

string MSshConnection::Wrap(string inData)
{
	net_swapper swap;
	char padding = 0;
	uint32 blockSize = 8;
	if (fEncryptorCipher.get() != nil)
		blockSize = fEncryptorCipher->BlockSize();
	
	if (fCompressor.get() != nil)
		fCompressor->Process(inData);
	
	do
	{
		inData += rng->GenerateByte();
		++padding;
	}
	while (inData.size() < blockSize or padding < 4 or
		(inData.size() + 5) % blockSize);

	inData.insert(0, &padding, 1);
	
	uint32 l = inData.size();
	l = swap(l);

	inData.insert(0, reinterpret_cast<char*>(&l), 4);
	
	string result;
	
	if (fEncryptorCipher.get() == nil)
		result = inData;
	else
	{
		fOutPacket.clear();
		
		fEncryptor->Put(reinterpret_cast<const byte*>(inData.c_str()),
			inData.length());
		fEncryptor->Flush(true);
		
		uint32 seqNr = swap(fOutSequenceNr);
		fSigner->Update(reinterpret_cast<byte*>(&seqNr), sizeof(uint32));
		fSigner->Update(
			reinterpret_cast<const byte*>(inData.c_str()), inData.length());
		
		vector<char> buf(fSigner->DigestSize());
		fSigner->Final(reinterpret_cast<byte*>(&buf[0]));
		
		result = fOutPacket;
		result.append(&buf[0], fSigner->DigestSize());
		
		fOutPacket.clear();
	}

	++fOutSequenceNr;
	
	return result;
}

vector<string> MSshConnection::Split(string inData) const
{
	vector<string> result;
	
	string::size_type n;
	while ((n = inData.find(',')) != string::npos)
	{
		result.push_back(inData.substr(0, n));
		inData.erase(0, n + 1);
	}
	result.push_back(inData);
	
	return result;
}

string MSshConnection::ChooseProtocol(
	const string& inServer, const string& inClient) const
{
	string result;

	vector<string> server = Split(inServer);
	vector<string> client = Split(inClient);
	
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
	MSshPacket p;
	p << fSharedSecret;
	
	SHA1 hash;
	uint32 dLen = hash.DigestSize();
	vector<char> H(dLen);
	
	char ch = 'A' + inNr;
	
	hash.Update(reinterpret_cast<const byte*>(p.data.c_str()), p.data.length());
	hash.Update(reinterpret_cast<const byte*>(inHash), 20);
	hash.Update(reinterpret_cast<const byte*>(&ch), 1);
	hash.Update(reinterpret_cast<const byte*>(fSessionId.c_str()), fSessionId.length());
	hash.Final(reinterpret_cast<byte*>(&H[0]));

	string result(&H[0], hash.DigestSize());
	
	for (inLength -= dLen; inLength > 0; inLength -= dLen)
	{
		hash.Update(reinterpret_cast<const byte*>(p.data.c_str()), p.data.length());
		hash.Update(reinterpret_cast<const byte*>(inHash), 20);
		hash.Update(reinterpret_cast<const byte*>(result.c_str()), result.length());
		hash.Final(reinterpret_cast<byte*>(&H[0]));
		result += string(&H[0], hash.DigestSize());
	}
	
	outKey = new byte[result.length()];
	copy(result.begin(), result.end(), outKey);
}

uint32 MSshConnection::GetMaxPacketSize(const MSshChannel* inChannel) const
{
	uint32 result = 1024;
	
	ChannelInfo info;
	info.fChannel = const_cast<MSshChannel*>(inChannel);
	ChannelList::const_iterator i = find(fChannels.begin(), fChannels.end(), info);
	if (i != fChannels.end())
		result = (*i).fMaxSendPacketSize;
	return result;
}

string MSshConnection::GetEncryptionParams() const
{
	string result;
	
	if (fIsConnected)
	{
		result =
			ChooseProtocol(fEncryptionAlgC2S, kEncryptionAlgorithms) + '-' +
			ChooseProtocol(fMACAlgC2S, kMacAlgorithms) + '-';
		
		if (Preferences::GetInteger("compress-sftp", true) != 0)
			result += ChooseProtocol(fCompressionAlgC2S, kUseCompressionAlgorithms);
		else
			result += ChooseProtocol(fCompressionAlgC2S, kDontUseCompressionAlgorithms);
	}
	
	return result;
}
