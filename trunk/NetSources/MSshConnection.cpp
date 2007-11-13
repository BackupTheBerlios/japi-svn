/*	$Id: MSshConnection.cpp,v 1.38 2004/03/05 20:43:26 maarten Exp $
	Copyright Maarten
	Created Sunday September 28 2003 11:35:17
*/

#include "MJapieG.h"

#include "MSshUtil.h"
#include "MError.h"
#include "MSshConnection.h"
#include "MSshConnectionPool.h"
#include "MSshChannel.h"
//#include "MSshAgentChannel.h"
#include "MAuthDialog.h"
#include "MCertificate.h"
#include "MPreferences.h"

#include "MKnownHosts.h"

#include <rng.h>
#include <aes.h>
#include <des.h>
#include <hmac.h>
#include <md5.h>
#include <sha.h>
#include <dsa.h>
#include <rsa.h>
#include <blowfish.h>
#include <filters.h>
#include <zlib.h>

using namespace std;
using namespace CryptoPP;

uint32 MSshConnection::sNextChannelId;

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
	kMacAlgorithms[] = "hmac-md5,hmac-sha1",
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
static Integer				p, q, g;
static auto_ptr<X917RNG>	rng;

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
	inflateEnd(&fStream);
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
	: eRecvAuthInfo(this, &MSshConnection::RecvAuthInfo)
	, eRecvPassword(this, &MSshConnection::RecvPassword)
	, eIdle(this, &MSshConnection::Idle)
	, fHandler(nil)
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
	, fOpenedAt(system_time())
{
	AddRoute(MApplication::ePulse, eIdle);

	static bool sInited = false;
	if (not sInited)
	{
		byte seed[16];

		int fd = open("/dev/random", R_ONLY);
		if (fd >= 0)
		{
			read(fd, seed, sizeof(seed));
			close(fd);
		}
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
}

MSshConnection::~MSshConnection()
{
	try
	{
		RemoveRoute(MApplication::ePulse, eIdle);
		
		if (fIsConnected)
			Disconnect();
	}
	catch (...) {}
}

void MSshConnection::Error(
	int			inReason)
{
//	string msg = MStrings::GetIndString(4008, inReason);
//	if (msg.length() == 0)
//		msg = MStrings::GetIndString(4008, 0);
	
	stringstream s;
	s << "Error in ssh connection: " << inReason;
	
	eConnectionMessage(s.str(), this);
	
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
	
	eConnectionMessage("Lookink up address", this);

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

	eConnectionMessage("Connecting…", this);

	/* Connect the control port. */
	int lResult = connect(fSocket, (MBSD::sockaddr*) &lAddr, sizeof(lAddr));
	if (lResult == SOCKET_ERROR)
	{
		int err = errno;
		if (err != MBSD_EAGAIN and err != MBSD_EINPROGRESS)
			return false;
	}

	/* Even if the connection IS established we still Finish the connect non-blocking */
	fBusy = true;
	return true;
}

void MSshConnection::Disconnect()
{
	PRINT(("Disconnect %s", fIPAddress.c_str()));
	RemoveRoute(eIdle, MApplication::ePulse);
	MSshConnectionPool::Instance().Remove(this);
	
	close(fSocket);

	fIsConnected = false;
	fBusy = false;
	
	eConnectionEvent(SSH_CHANNEL_CLOSED, this);
	eConnectionMessage("Connection closed", this);
	
	fChannels.clear();
}

void MSshConnection::ResetTimer()
{
	fOpenedAt = system_time();
}

void MSshConnection::CertificateDeleted(const bool&, MEventSender* inSender)
{
	if (inSender == fCertificate.get())
	{
		fCertificate.reset(nil);
		Disconnect();
	}
}

void MSshConnection::Idle(const double&, MEventSender*)
{
	if (fInhibitIdle)
		return;
	
	MValueChanger<bool> saveFlag(fInhibitIdle, true);
	
	try
	{
		/* Idle processing... */
	
		if (fSocket == kInvalidSocket)
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
				if (lResult == 0 || lResult == SOCKET_ERROR)
				{
					assert(errno != MBSD_EAGAIN);
					Disconnect();
				}
				else
					fBuffer.append(sReadBuf, static_cast<string::size_type>(lResult));
			}
	
			if (FD_ISSET(fSocket, &sWrite))
			{
				// don't ask me...
				delay(0.1);
				
				fIsConnected = true;
				fBusy = false;
	
				string cmd = kSSHVersionString;
				cmd += "\r\n";
				
				int lRes = send(fSocket, cmd.c_str(), cmd.length(), 0);
				if (lRes != cmd.length())
					Disconnect();
			}
		}
		
		/* Process data from socket .. if needed */
		while (not fBuffer.empty())
		{
			if (not ProcessBuffer())
				break;
		}
		
		eConnectionEvent(SSH_CHANNEL_PACKET_DONE, this);
		
		// see if there's data to be sent
		ChannelList::iterator ch;
	
		for (ch = fChannels.begin(); ch != fChannels.end(); ++ch)
		{
			if ((*ch).fPending.size() > 0 and
				(*ch).fPending.front().length() < (*ch).fHostWindowSize)
			{
				MSshPacket p;
				p.data = (*ch).fPending.front();
				(*ch).fPending.pop_front();
				Send(p);
			}
		}

		// if there's a time out, let the user decide what to do		
		if ((fBusy or not fIsConnected) and system_time() > fOpenedAt + 30)
		{
			eConnectionEvent(SSH_CHANNEL_TIMEOUT, this);
			
			if (system_time() > fOpenedAt + 45)
				Disconnect();
		}
	}
	catch (Exception e)
	{
		Disconnect();
		eConnectionMessage(e.what(), this);
		PRINT(("Catched cryptlib exception: %s", e.what()));
	}
	catch (exception e)
	{
		Disconnect();
		eConnectionMessage(e.what(), this);
		PRINT(("Catched exception: %s", e.what()));
	}
	catch (...)
	{
		Disconnect();
		eConnectionMessage("exception", this);
		PRINT(("Catched unhandled exception"));
	}
}

void MSshConnection::Send(string inMessage)
{
	const char* msg = reinterpret_cast<const char*>(inMessage.c_str());
	int lResult = send(fSocket, msg, inMessage.length(), 0);
	if (lResult != inMessage.length())
		Error(SSH_DISCONNECT_CONNECTION_LOST);
}

void MSshConnection::Send(const MSshPacket& inPacket)
{
	Send(Wrap(inPacket.data));
}

bool MSshConnection::ProcessBuffer()
{
	bool result = true;
	
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
				fDecryptorCBC->Put(
					reinterpret_cast<const byte*>(fBuffer.c_str()), blockSize);
				fDecryptorCBC->Flush(true);
			}
			else
				fInPacket.append(fBuffer.begin(), fBuffer.begin() + blockSize);
			fBuffer.erase(0, blockSize);
			
			// If this is the first block for a new packet
			// we determine the expected payload and padding length
			if (fInPacket.size() == blockSize)
			{
				fPacketLength = net_swapper::swap(
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
				
				uint32 seqNr = net_swapper::swap(fInSequenceNr);
				
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
					eConnectionMessage(e.what(), this);
				}
				catch (exception& e)
				{
					Error(SSH_DISCONNECT_PROTOCOL_ERROR);
					eConnectionMessage(e.what(), this);
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

	PRINT((LookupToken(kTokens, message)));

	MSshPacket in, out;
	in.data = fInPacket;

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

void MSshConnection::ProcessDisconnect(uint8 inMessage, MSshPacket& in, MSshPacket& out)
{
	uint8 message;
	string languageTag;
	
	in >> message >> fErrCode >> fErrString >> languageTag;
	
	PRINT(("Disconnected: %s", fErrString.c_str()));

	Disconnect();
}

void MSshConnection::ProcessUnimplemented(uint8 inMessage, MSshPacket& in, MSshPacket& out)
{
	PRINT(("Unimplemented"));
	Disconnect();
}

void MSshConnection::ProcessChannelRequest(uint8 inMessage, MSshPacket& in, MSshPacket& out)
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

void MSshConnection::ProcessDebug(uint8 inMessage, MSshPacket& in, MSshPacket& out)
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

	eConnectionMessage("Exchanging keys", this);
	
	fHostVersion = fInPacket;

	// create and send our KEXINIT packet

	MSshPacket data;
	
	data << uint8(SSH_MSG_KEXINIT);
	
	for (uint32 i = 0; i < 16; ++i)
		data << rng->GenerateByte();

	string compress;
	if (gPrefs->GetPrefInt("compress-sftp", true) != 0)
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
		<< 0UL;

	fMyPayLoad = data.data;
	fHandler = &MSshConnection::ProcessKexInit;

	Send(data);
}

void MSshConnection::ProcessKexInit(uint8 inMessage, MSshPacket& in, MSshPacket& out)
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

void MSshConnection::ProcessKexdhReply(uint8 inMessage, MSshPacket& in, MSshPacket& out)
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
		
		DSAPublicKey h_key(h_p, h_q, h_g, h_y);
		
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
		MAutoBuf<char>	H(new char[dLen]);
		
		hash.Update(
			reinterpret_cast<const byte*>(h_test.data.c_str()),
			h_test.data.length());
		hash.Final(reinterpret_cast<byte*>(H.get()));
		
		if (fSessionId.length() == 0)
			fSessionId.assign(H.get(), dLen);
	
		if (not h_key.VerifyMessage(reinterpret_cast<byte*>(H.get()), dLen, h_sig))
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
			fKeys[i] = DeriveKey(H.get(), i, keyLen);
		
		fHandler = &MSshConnection::ProcessNewKeys;
	}
	catch (uint32 err)
	{
		Error(err);
		out.data.clear();
		fHandler = nil;
	}
}

void MSshConnection::ProcessNewKeys(uint8 inMessage, MSshPacket& in, MSshPacket& out)
{
	if (inMessage != SSH_MSG_NEWKEYS)
		Error(SSH_DISCONNECT_PROTOCOL_ERROR);
	else
	{
		string protocol;
		
		// Client to server encryption
		protocol = ChooseProtocol(fEncryptionAlgC2S, kEncryptionAlgorithms);
		 
		if (protocol == "3des-cbc")
			fEncryptorCipher.reset(new DES_EDE3_Encryption(
				reinterpret_cast<const byte*>(fKeys[2].c_str())));
		else if (protocol == "blowfish-cbc")
			fEncryptorCipher.reset(new BlowfishEncryption(
				reinterpret_cast<const byte*>(fKeys[2].c_str())));
		else if (protocol == "aes128-cbc")
			fEncryptorCipher.reset(new AESEncryption(
				reinterpret_cast<const byte*>(fKeys[2].c_str())));
		else if (protocol == "aes192-cbc")
			fEncryptorCipher.reset(new AESEncryption(
				reinterpret_cast<const byte*>(fKeys[2].c_str()), 24));
		else if (protocol == "aes256-cbc")
			fEncryptorCipher.reset(new AESEncryption(
				reinterpret_cast<const byte*>(fKeys[2].c_str()), 32));

		fEncryptorCBC.reset(
			new CBCRawEncryptor(*fEncryptorCipher.get(),
				reinterpret_cast<const byte*>(fKeys[0].c_str()),
				new StringSink(fOutPacket)));
	
		// Server to client encryption
		protocol = ChooseProtocol(fEncryptionAlgS2C, kEncryptionAlgorithms);

		if (protocol == "3des-cbc")
			fDecryptorCipher.reset(new DES_EDE3_Decryption(
				reinterpret_cast<const byte*>(fKeys[3].c_str())));
		else if (protocol == "blowfish-cbc")
			fDecryptorCipher.reset(new BlowfishDecryption(
				reinterpret_cast<const byte*>(fKeys[3].c_str())));
		else if (protocol == "aes128-cbc")
			fDecryptorCipher.reset(new AESDecryption(
				reinterpret_cast<const byte*>(fKeys[3].c_str()), 16));
		else if (protocol == "aes192-cbc")
			fDecryptorCipher.reset(new AESDecryption(
				reinterpret_cast<const byte*>(fKeys[3].c_str()), 24));
		else if (protocol == "aes256-cbc")
			fDecryptorCipher.reset(new AESDecryption(
				reinterpret_cast<const byte*>(fKeys[3].c_str()), 32));

		fDecryptorCBC.reset(
			new CBCRawDecryptor(*fDecryptorCipher.get(),
				reinterpret_cast<const byte*>(fKeys[1].c_str()),
				new StringSink(fInPacket)));
		
		protocol = ChooseProtocol(fMACAlgC2S, kMacAlgorithms);
		if (protocol == "hmac-sha1")
			fSigner.reset(
				new MMAC<SHA1>(
					reinterpret_cast<const byte*>(fKeys[4].c_str()), 20));
		else
			fSigner.reset(
				new MMAC<MD5>(
					reinterpret_cast<const byte*>(fKeys[4].c_str())));

		protocol = ChooseProtocol(fMACAlgS2C, kMacAlgorithms);
		if (protocol == "hmac-sha1")
			fVerifier.reset(
				new MMAC<SHA1>(
					reinterpret_cast<const byte*>(fKeys[5].c_str()), 20));
		else
			fVerifier.reset(
				new MMAC<MD5>(
					reinterpret_cast<const byte*>(fKeys[5].c_str())));
	
		string compress;
		if (gPrefs->GetPrefInt("compress-sftp", true) != 0)
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

void MSshConnection::ProcessUserAuthInit(uint8 inMessage, MSshPacket& in, MSshPacket& out)
{
	fHandler = nil;
	
	if (inMessage != SSH_MSG_SERVICE_ACCEPT)
		Error(SSH_DISCONNECT_SERVICE_NOT_AVAILABLE);
	else
	{
		eConnectionMessage("Starting authentication", this);

		out << uint8(SSH_MSG_USERAUTH_REQUEST)
			<< fUserName << "ssh-connection" << "none";
		fHandler = &MSshConnection::ProcessUserAuthNone;
	}
}

void MSshConnection::ProcessUserAuthNone(uint8 inMessage, MSshPacket& in, MSshPacket& out)
{
	fHandler = nil;
	
	switch (inMessage)
	{
		case SSH_MSG_USERAUTH_BANNER:
		{
			string msg, lang;
			in >> inMessage >> msg >> lang;
			eConnectionBanner(msg, this);
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
			
			try
			{
				if (gPrefs->GetPrefInt("use-certificate", false) != 0)
				{
					fCertificate.reset(new MCertificate(
						gPrefs->GetPrefString("auth-certificate", "")));
					ThrowIfNil(fCertificate.get());
				
					AddRoute(MCertificate::eCertificateDeleted, eCertificateDeleted);
					if (fCertificate->GetPublicRSAKey(e, n))
						p.insert(0, "publickey,");
				}
			}
			catch (exception& e)
			{
				DisplayError(e);
			}
			
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
			eConnectionBanner(msg, this);
			break;
		}

#define SSH_MSG_USERAUTH_PK_OK SSH_MSG_USERAUTH_INFO_REQUEST		

		case SSH_MSG_USERAUTH_PK_OK:
		{
			eConnectionMessage("Public key authentication", this);

			string alg, blob;
			
			in >> inMessage >> alg >> blob;

			out << fSessionId << uint8(SSH_MSG_USERAUTH_REQUEST)
				<< fUserName << "ssh-connection" << "publickey" << true
				<< "ssh-rsa" << blob;

			string sig;
				// no matter what, send a bogus sig if needed
			(void)fCertificate->SignData(out.data, sig);

			string sink;
			out >> sink;
					
			MSshPacket ps;
			ps << "ssh-rsa" << sig;
			
			out << ps.data;
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
			eConnectionBanner(msg, this);
			break;
		}
		
		case SSH_MSG_USERAUTH_INFO_REQUEST:
		{
			uint32 n;
			in >> msg >> title >> instruction >> lang >> n;
			
			if (n == 0)
				out << uint8(SSH_MSG_USERAUTH_INFO_RESPONSE) << 0UL;
			else
			{
				if (title.length() == 0)
					title = MStrings::GetIndString(1011, 0);
				
				if (instruction.length() == 0)
					instruction = MStrings::GetFormattedIndString(1011, 1, fUserName, fIPAddress);
				
				if (n > 5)
					THROW((pErrAuthenticationProtocol, 0));
				
				for (int i = 0; i < n; ++i)
					in >> p[i] >> e[i];
				
				MAuthDialogBase* dlog = nil;
				switch (n)
				{
					case 0: dlog = CreateHDialog<MAuthDialogBase>(nil); break;
					case 1: dlog = CreateHDialog<MAuthDialog<1> >(nil); break;
					case 2: dlog = CreateHDialog<MAuthDialog<2> >(nil); break;
					case 3: dlog = CreateHDialog<MAuthDialog<3> >(nil); break;
					case 4: dlog = CreateHDialog<MAuthDialog<4> >(nil); break;
					case 5: dlog = CreateHDialog<MAuthDialog<5> >(nil); break;
				}
	
				if (dlog != nil)
				{
					dlog->SetTexts(title, instruction, p, e);
					AddRoute(dlog->eOKClicked, eRecvAuthInfo);
					dlog->Show();
				}
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

void MSshConnection::RecvAuthInfo(const bool& inOK, MEventSender* inSender)
{
	if (inOK)
	{
		MAuthDialogBase* dlog = dynamic_cast<MAuthDialogBase*>(inSender);
		
		ThrowIfNil(dlog);
		uint32 n = dlog->GetN();
		
		MSshPacket out;
		out << uint8(SSH_MSG_USERAUTH_INFO_RESPONSE) << n;
		for (int i = 0; i < n; ++i)
			out << dlog->GetField(i);
		Send(out);
	}
	else
		Disconnect();
}

void MSshConnection::TryPassword()
{
	eConnectionMessage("Password authentication", this);

	string p[1];
	bool e[1];
	
	p[0] = MStrings::GetIndString(1011, 2);
	e[0] = false;

	MAuthDialog<1>* dlog = CreateHDialog<MAuthDialog<1> >(nil);
	dlog->SetTexts(MStrings::GetIndString(1011, 0),
		MStrings::GetFormattedIndString(1011, 1, fUserName, fIPAddress),
		p, e);
	AddRoute(dlog->eOKClicked, eRecvPassword);
	dlog->Show();	
}

void MSshConnection::RecvPassword(const bool& inOK, MEventSender* inSender)
{
	if (inOK)
	{
		MAuthDialog<1>* dlog = dynamic_cast<MAuthDialog<1>*>(inSender);
		
		MSshPacket out;
		
		out << uint8(SSH_MSG_USERAUTH_REQUEST)
			<< fUserName << "ssh-connection" << "password" << false << dlog->GetField(0);
		
		Send(out);
		
		fHandler = &MSshConnection::ProcessUserAuthSuccess;
	}
	else
		Disconnect();
}

void MSshConnection::ProcessUserAuthSuccess(uint8 inMessage, MSshPacket& in, MSshPacket& out)
{
	switch (inMessage)
	{
		case SSH_MSG_USERAUTH_BANNER:
		{
			string msg, lang;
			in >> inMessage >> msg >> lang;
			eConnectionBanner(msg, this);
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

	eConnectionMessage("Authenticated", this);
	
	assert(fOpeningChannel != nil);
	if (fOpeningChannel != nil)
	{
		OpenChannel(fOpeningChannel);
		fOpeningChannel->eChannelBanner.RemoveProto(&eConnectionBanner);
	}
	fHandler = nil;
}

void MSshConnection::UserAuthFailed()
{
	fAuthenticated = false;

	Disconnect();

	eConnectionMessage(
		MStrings::GetFormattedIndString(1011, 3, fUserName, fIPAddress), this);
	fHandler = nil;
}

void MSshConnection::ProcessChannelOpen(uint8 inMessage, MSshPacket& in, MSshPacket& out)
{
	string type;
	uint32 channelId, windowSize, maxPacketSize;
	
	in >> inMessage >> type >> channelId >> windowSize >> maxPacketSize;
	
	if (type == "auth-agent@openssh.com" and gPrefs->GetPrefInt("advertise_agent", 1))
	{
		fAgentChannel.reset(new MSshAgentChannel(*this));
		
		ChannelInfo info;
		
		info.fChannel = fAgentChannel.get();
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

bool MSshConnection::IsConnectionForChannel(const MSshChannel* inChannel)
{
	bool result = (fOpeningChannel == inChannel);
	if (result == false)
	{
		ChannelInfo info;
		info.fChannel = const_cast<MSshChannel*>(inChannel);
		result = find(fChannels.begin(), fChannels.end(), info) != fChannels.end();
	}
	return result;
}

void MSshConnection::OpenChannel(MSshChannel* inChannel)
{
	if (not fAuthenticated)
	{
		if (fOpenedAt + 30 < ::system_time())
		{
			Disconnect();
			Connect(fUserName, fIPAddress, fPortNumber);
			ResetTimer();
		}
		
//		assert(fOpeningChannel == nil);
		
//		if (fOpeningChannel != nil)
//			throw MError(pErrCouldNotOpenConnection);
		
		fOpeningChannel = inChannel;
		fOpeningChannel->eChannelBanner.AddProto(&eConnectionBanner);
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

void MSshConnection::CloseChannel(MSshChannel* inChannel)
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
		if ((*ch).fChannel == inChannel)
			break;
	}
	
	if (ch != fChannels.end())
	{
		assert(inData.length() < (*ch).fMaxSendPacketSize);

		MSshPacket p;
		if (inType == 0)
			p << uint8(SSH_MSG_CHANNEL_DATA) << (*ch).fHostChannel << inData;
		else
			p << uint8(SSH_MSG_CHANNEL_EXTENDED_DATA) << (*ch).fHostChannel
				<< inType << inData;
		
		if (inData.length() < (*ch).fHostWindowSize)
		{
			Send(p);
			(*ch).fHostWindowSize -= inData.length();
		}
		else
			(*ch).fPending.push_back(p.data);
	}
	else
		assert(false);
}

void MSshConnection::SendWindowResize(MSshChannel* inChannel, uint32 inColumns, uint32 inRows)
{
	ChannelList::iterator ch;
	
	for (ch = fChannels.begin(); ch != fChannels.end(); ++ch)
	{
		if ((*ch).fChannel == inChannel)
			break;
	}
	
	if (ch != fChannels.end())
	{
		MSshPacket p;
		
		p << uint8(SSH_MSG_CHANNEL_REQUEST) << (*ch).fHostChannel
			<< "window-change" << false
			<< inColumns << inRows
			<< 0UL << 0UL;

		Send(p);
	}
	else
		assert(false);
}

void MSshConnection::ProcessConfirmChannel(uint8 inMessage, MSshPacket& in, MSshPacket& out)
{
	if (inMessage == SSH_MSG_CHANNEL_OPEN_CONFIRMATION)
	{
		uint8 msg;
		uint32 host_channel, my_channel, window_size, max_packet_size;
		
		in >> msg >> my_channel >> host_channel >> window_size >> max_packet_size;

		ChannelList::iterator ch;
		for (ch = fChannels.begin(); ch != fChannels.end(); ++ch)
		{
			if ((*ch).fMyChannel == my_channel)
			{
				(*ch).fHostChannel = host_channel;
				(*ch).fHostWindowSize = window_size;
				(*ch).fMaxSendPacketSize = max_packet_size;
				(*ch).fChannelOpen = true;
				
				(*ch).fChannel->eChannelEvent(SSH_CHANNEL_OPENED, (*ch).fChannel);
				eConnectionMessage("Connecting…", this);
				
				if ((*ch).fChannel->WantPTY())
				{
					if (fAgentChannel.get() == nil and
						gPrefs->GetPrefInt("advertise_agent", 1))
					{
						MSshPacket p;
						p << uint8(SSH_MSG_CHANNEL_REQUEST) << host_channel
							<< "auth-agent-req@openssh.com" << false;
						Send(p);
					}
	
					MSshPacket p;
					
					p << uint8(SSH_MSG_CHANNEL_REQUEST)
						<< (*ch).fHostChannel
						<< "pty-req"
						<< true
						<< "vt100"
						<< 80UL << 24UL << 0UL << 0UL <<
						"";
					
					Send(p);
					
					fHandler = &MSshConnection::ProcessConfirmPTY;
				}

				out << uint8(SSH_MSG_CHANNEL_REQUEST)
					<< (*ch).fHostChannel
					<< (*ch).fChannel->GetRequest()
					<< true;
				
				if (strlen((*ch).fChannel->GetCommand()) > 0)
					out << (*ch).fChannel->GetCommand();

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
			if ((*ch).fMyChannel == my_channel)
			{
				(*ch).fChannel->eChannelEvent(SSH_CHANNEL_ERROR, (*ch).fChannel);
				
				fChannels.erase(ch);
				break;
			}
		}
		
		Disconnect();
	}
}

void MSshConnection::ProcessConfirmPTY(uint8 inMessage, MSshPacket& in, MSshPacket& out)
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

void MSshConnection::ProcessChannel(uint8 inMessage, MSshPacket& in, MSshPacket& out)
{
	uint8 msg;
	uint32 channelId;
	
	in >> msg >> channelId;
	
	PRINT(("%s for channel %d", LookupToken(kTokens, msg), channelId));
	
	MSshChannel* channel = nil;
	ChannelList::iterator ch;
	for (ch = fChannels.begin(); ch != fChannels.end(); ++ch)
	{
		if ((*ch).fMyChannel == channelId)
		{
			channel = (*ch).fChannel;
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
				(*ch).fHostWindowSize += extra;
				break;
			}
			
			case SSH_MSG_CHANNEL_DATA:
				in >> data;
				(*ch).fMyWindowSize -= data.length();
				channel->MandleData(data);
				break;

			case SSH_MSG_CHANNEL_EXTENDED_DATA:
				in >> type >> data;
				(*ch).fMyWindowSize -= data.length();
				channel->MandleExtraData(type, data);
				break;
			
			case SSH_MSG_CHANNEL_CLOSE:
				if (ch != fChannels.end())
					fChannels.erase(ch);
				if (channel != nil)
					channel->eChannelEvent(SSH_CHANNEL_CLOSED, channel);
				break;
			
			case SSH_MSG_CHANNEL_SUCCESS:
				channel->eChannelEvent(SSH_CHANNEL_SUCCESS, channel);
				break;

			case SSH_MSG_CHANNEL_FAILURE:
				channel->eChannelEvent(SSH_CHANNEL_FAILURE, channel);
				break;
		}

		if ((*ch).fMyWindowSize < kWindowSize - 2 * kMaxPacketSize)
		{
			uint32 adjust = kWindowSize - (*ch).fMyWindowSize;
			out << uint8(SSH_MSG_CHANNEL_WINDOW_ADJUST) <<
				(*ch).fHostChannel << adjust;
			(*ch).fMyWindowSize += adjust;
		}
	}
}

string MSshConnection::Wrap(string inData)
{
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
	while (inData.size() < 8 or padding < 4 or
		(inData.size() + 5) % blockSize);

	inData.insert(0, &padding, 1);
	
	uint32 l = inData.size();
	l = net_swapper::swap(l);

	inData.insert(0, reinterpret_cast<char*>(&l), 4);
	
	string result;
	
	if (fEncryptorCipher.get() == nil)
		result = inData;
	else
	{
		fOutPacket.clear();
		
		fEncryptorCBC->Put(reinterpret_cast<const byte*>(inData.c_str()),
			inData.length());
		fEncryptorCBC->Flush(true);
		
		uint32 seqNr = net_swapper::swap(fOutSequenceNr);
		fSigner->Update(reinterpret_cast<byte*>(&seqNr), sizeof(uint32));
		fSigner->Update(
			reinterpret_cast<const byte*>(inData.c_str()), inData.length());
		
		MAutoBuf<char> buf(new char[fSigner->DigestSize()]);
		fSigner->Final(reinterpret_cast<byte*>(buf.get()));
		
		result = fOutPacket;
		result.append(buf.get(), fSigner->DigestSize());
		
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

string MSshConnection::DeriveKey(char* inHash, int inNr, int inLength)
{
	MSshPacket p;
	p << fSharedSecret;
	
	SHA1 hash;
	uint32 dLen = hash.DigestSize();
	MAutoBuf<char> H(new char[dLen]);
	
	char ch = 'A' + inNr;
	
	hash.Update(reinterpret_cast<const byte*>(p.data.c_str()), p.data.length());
	hash.Update(reinterpret_cast<const byte*>(inHash), 20);
	hash.Update(reinterpret_cast<const byte*>(&ch), 1);
	hash.Update(reinterpret_cast<const byte*>(fSessionId.c_str()), fSessionId.length());
	hash.Final(reinterpret_cast<byte*>(H.get()));

	string result(H.get(), hash.DigestSize());
	
	for (inLength -= dLen; inLength > 0; inLength -= dLen)
	{
		hash.Update(reinterpret_cast<const byte*>(p.data.c_str()), p.data.length());
		hash.Update(reinterpret_cast<const byte*>(inHash), 20);
		hash.Update(reinterpret_cast<const byte*>(result.c_str()), result.length());
		hash.Final(reinterpret_cast<byte*>(H.get()));
		result += string(H.get(), hash.DigestSize());
	}
	
	return result;
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
		
		if (gPrefs->GetPrefInt("compress-sftp", true) != 0)
			result += ChooseProtocol(fCompressionAlgC2S, kUseCompressionAlgorithms);
		else
			result += ChooseProtocol(fCompressionAlgC2S, kDontUseCompressionAlgorithms);
	}
	
	return result;
}
