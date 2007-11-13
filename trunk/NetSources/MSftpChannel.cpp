/*	$Id: MSftpChannel.cpp,v 1.22 2004/02/28 22:28:13 maarten Exp $
	Copyright Maarten
	Created Sunday September 28 2003 11:39:23
	
	Implementation of the Version 3 Secure File Transfer Protocol
*/

#include "MJapieG.h"

#include "MError.h"
#include "MSshUtil.h"
#include "MSftpChannel.h"

using namespace std;

#define SSH_FXP_INIT                1
#define SSH_FXP_VERSION             2
#define SSH_FXP_OPEN                3
#define SSH_FXP_CLOSE               4
#define SSH_FXP_READ                5
#define SSH_FXP_WRITE               6
#define SSH_FXP_LSTAT               7
#define SSH_FXP_FSTAT               8
#define SSH_FXP_SETSTAT             9
#define SSH_FXP_FSETSTAT           10
#define SSH_FXP_OPENDIR            11
#define SSH_FXP_READDIR            12
#define SSH_FXP_REMOVE             13
#define SSH_FXP_MKDIR              14
#define SSH_FXP_RMDIR              15
#define SSH_FXP_REALPATH           16
#define SSH_FXP_STAT               17
#define SSH_FXP_RENAME             18
#define SSH_FXP_READLINK           19
#define SSH_FXP_SYMLINK            20
#define SSH_FXP_STATUS            101
#define SSH_FXP_HANDLE            102
#define SSH_FXP_DATA              103
#define SSH_FXP_NAME              104
#define SSH_FXP_ATTRS             105
#define SSH_FXP_EXTENDED          200
#define SSH_FXP_EXTENDED_REPLY    201

#define SSH_FILEXFER_ATTR_SIZE          0x00000001
#define SSH_FILEXFER_ATTR_UIDGID        0x00000002
#define SSH_FILEXFER_ATTR_PERMISSIONS   0x00000004
#define SSH_FILEXFER_ATTR_ACMODTIME     0x00000008
#define SSH_FILEXFER_ATTR_EXTENDED      0x80000000

#define SSH_FX_OK                            0
#define SSH_FX_EOF                           1
#define SSH_FX_NO_SUCH_FILE                  2
#define SSH_FX_PERMISSION_DENIED             3
#define SSH_FX_FAILURE                       4
#define SSH_FX_BAD_MESSAGE                   5
#define SSH_FX_NO_CONNECTION                 6
#define SSH_FX_CONNECTION_LOST               7
#define SSH_FX_OP_UNSUPPORTED                8

#define SSH_FXF_READ            0x00000001
#define SSH_FXF_WRITE           0x00000002
#define SSH_FXF_APPEND          0x00000004
#define SSH_FXF_CREAT           0x00000008
#define SSH_FXF_TRUNC           0x00000010
#define SSH_FXF_EXCL            0x00000020

struct MSftpChannelImp
{
							MSftpChannelImp(MSftpChannel& inChannel);
	virtual					~MSftpChannelImp();
	
	static MSftpChannelImp*	CreateImpl(MSshPacket& in, MSshPacket& out, MSftpChannel& inChannel);
	
	virtual void			Init(MSshPacket& out) = 0;
	virtual void			MandlePacket(uint8 inMessage, MSshPacket& in, MSshPacket& out) = 0;
	
//	uint32					GetStatusCode(MSshPacket& inPacket);
	
	void					Send(string inData)			{ fChannel.Send(inData); }

	// action interface
	
	virtual void			SetCWD(string inPath) = 0;
	virtual void			OpenDir() = 0;
	virtual void			MkDir(string inPath) = 0;
	virtual void			ReadFile(string inPath) = 0;
	virtual void			WriteFile(string inPath) = 0;
	virtual void			SendData(const string& inData) = 0;
	virtual void			CloseFile() = 0;

	struct DirEntry {
		string			name;
		uint64				size;
		uint32				date;
		char				type;
	};
	typedef std::vector<DirEntry>	DirList;

	MSftpChannel&			fChannel;
	uint32					fRequestId;
	uint32					fPacketSize;
	string					fHandle;
	uint64					fFileSize;
	uint64					fOffset;
	DirList					fDirList;
	string					fCurrentDir;
	string					fData;
};

struct MSftpChannelImp3 : public MSftpChannelImp
{
							MSftpChannelImp3(MSftpChannel& inChannel);

	virtual void			Init(MSshPacket& out);
	virtual void			MandlePacket(uint8 inMessage, MSshPacket& in, MSshPacket& out);

	// action interface
	virtual void			SetCWD(string inPath);
	virtual void			OpenDir();
	virtual void			MkDir(string inPath);
	virtual void			ReadFile(string inPath);
	virtual void			WriteFile(string inPath);
	virtual void			SendData(const string& inData);
	virtual void			CloseFile();

	void (MSftpChannelImp3::*fHandler)(uint8 inMessage, MSshPacket& in, MSshPacket& out);
	void					ProcessRealPath(uint8 inMessage, MSshPacket& in, MSshPacket& out);
	void					ProcessOpenDir(uint8 inMessage, MSshPacket& in, MSshPacket& out);
	void					ProcessReadDir(uint8 inMessage, MSshPacket& in, MSshPacket& out);
	void					ProcessMkDir(uint8 inMessage, MSshPacket& in, MSshPacket& out);
	void					ProcessOpenFile(uint8 inMessage, MSshPacket& in, MSshPacket& out);
	void					ProcessFStat(uint8 inMessage, MSshPacket& in, MSshPacket& out);
	void					ProcessRead(uint8 inMessage, MSshPacket& in, MSshPacket& out);
	void					ProcessCreateFile(uint8 inMessage, MSshPacket& in, MSshPacket& out);
	void					ProcessWrite(uint8 inMessage, MSshPacket& in, MSshPacket& out);
	void					ProcessClose(uint8 inMessage, MSshPacket& in, MSshPacket& out);
};

struct MSftpChannelImp4 : public MSftpChannelImp3
{
							MSftpChannelImp4(MSftpChannel& inChannel);

	virtual void			Init(MSshPacket& out);
	virtual void			MandlePacket(uint8 inMessage, MSshPacket& in, MSshPacket& out);
};

/*
	Implementations
*/

MSftpChannelImp* MSftpChannelImp::CreateImpl(MSshPacket& in, MSshPacket& out, MSftpChannel& inChannel)
{
	uint32 version;
	in >> version;
	
	PRINT(("Connecting to a %d version SFTP server", version));
	
	MSftpChannelImp* result = nil;
	
	if (version == 4)
		result = new MSftpChannelImp4(inChannel);
	else if (version == 3)
		result = new MSftpChannelImp3(inChannel);
	else
		THROW(("Protocol version %d is not supported", version));
	
	return result;
}


MSftpChannelImp::MSftpChannelImp(MSftpChannel& inChannel)
	: fChannel(inChannel)
	, fRequestId(0)
{
}

MSftpChannelImp::~MSftpChannelImp()
{
}

/*
	Version 3 protocol
*/

MSftpChannelImp3::MSftpChannelImp3(MSftpChannel& inChannel)
	: MSftpChannelImp(inChannel)
	, fHandler(nil)
{
}

void MSftpChannelImp3::Init(MSshPacket& out)
{
	fChannel.eChannelEvent(SFTP_INIT_DONE);
}

void MSftpChannelImp3::MandlePacket(uint8 inMessage, MSshPacket& in, MSshPacket& out)
{
	PRINT(("SFTP: %d", inMessage));

	if (fHandler != nil)
		(this->*fHandler)(inMessage, in, out);
	else
		PRINT(("Mandler was nil, packet %d dropped", inMessage));
}

void MSftpChannelImp3::SetCWD(string inPath)
{
	if (inPath.length() == 0)
		inPath = ".";

	MSshPacket out;
	out << uint8(SSH_FXP_REALPATH) << fRequestId++ << inPath;
	Send(out.data);

	fHandler = &MSftpChannelImp3::ProcessRealPath;
}

void MSftpChannelImp3::OpenDir()
{
	fDirList.clear();
	
	MSshPacket p;
	p << uint8(SSH_FXP_OPENDIR) << fRequestId++ << fCurrentDir;
	Send(p.data);

	fHandler = &MSftpChannelImp3::ProcessOpenDir;
}

void MSftpChannelImp3::ProcessRealPath(uint8 inMessage, MSshPacket& in, MSshPacket& out)
{
	try
	{
		if (inMessage != SSH_FXP_NAME)
			throw -1;
		
		uint32 id, count;
		string dir;
		
		in >> id >> count >> dir;
		if (id != fRequestId - 1 or count != 1)
			throw -1;

		fCurrentDir = dir;
		fDirList.clear();

		fHandler = nil;
		fChannel.eChannelEvent(SFTP_SETCWD_OK);
	}
	catch (...)
	{
		fHandler = nil;
		fChannel.eChannelEvent(SFTP_ERROR);
	}	
}

void MSftpChannelImp3::ProcessOpenDir(uint8 inMessage, MSshPacket& in, MSshPacket& out)
{
	if (inMessage == SSH_FXP_HANDLE)
	{
		uint32 id;
		in >> id >> fHandle;
		out << uint8(SSH_FXP_READDIR) << fRequestId++ << fHandle;
		fHandler = &MSftpChannelImp3::ProcessReadDir;
	}
	else
		fHandler = nil;
}

void MSftpChannelImp3::ProcessReadDir(uint8 inMessage, MSshPacket& in, MSshPacket& out)
{
	bool done = true;
	
	if (inMessage == SSH_FXP_NAME)
	{
		uint32 id, count;
		string dir;
		
		in >> id >> count;
		
		PRINT(("Reading %d items", count));
		
		if (id != fRequestId - 1)
			;
		
		if (count != 0)
		{
			done = false;

			while (count-- > 0)
			{
				uint32 flags, dummy_i;
				
				string dummy_s;
				
				DirEntry e;
				in >> e.name >> dummy_s >> flags;

PRINT(("- %s\n", e.name.c_str()));
				
				e.type = dummy_s[0];
				
					// for now...
				if (e.type == 'l')
					e.type = 'd';
				
				if (flags & SSH_FILEXFER_ATTR_SIZE)
					in >> e.size;
	
				if (flags & SSH_FILEXFER_ATTR_UIDGID)
					in >> dummy_i >> dummy_i;
					
				if (flags & SSH_FILEXFER_ATTR_PERMISSIONS)
					in >> dummy_i;
				
				if (flags & SSH_FILEXFER_ATTR_ACMODTIME)
					in >> dummy_i;
				
				if (flags & SSH_FILEXFER_ATTR_ACMODTIME)
					in >> e.date;
				
				if (flags & SSH_FILEXFER_ATTR_EXTENDED)
				{
					in >> dummy_i;
					while (dummy_i-- > 0)
						in >> dummy_s >> dummy_s;
				}
				
				if (e.name != "." and e.name != "..")
					fDirList.push_back(e);
			}
			
			out << uint8(SSH_FXP_READDIR) << fRequestId++ << fHandle;
		}
	}

	if (done)
	{
		fChannel.eChannelEvent(SFTP_DIR_LISTING_AVAILABLE);
		
		out << uint8(SSH_FXP_CLOSE) << fRequestId++ << fHandle;
		
		fHandler = nil;
	}
}

void MSftpChannelImp3::MkDir(string inPath)
{
	MSshPacket p;
	p << uint8(SSH_FXP_MKDIR) << fRequestId++ << inPath << uint32(0);
	Send(p.data);
	fHandler = &MSftpChannelImp3::ProcessMkDir;
}

void MSftpChannelImp3::ProcessMkDir(uint8 inMessage, MSshPacket& in, MSshPacket& out)
{
	if (inMessage == SSH_FXP_STATUS)
	{
		if (fChannel.GetStatusCode() != SSH_FX_OK)
			fChannel.eChannelEvent(SFTP_ERROR);
	}
	
	fHandler = nil;
}

void MSftpChannelImp3::ReadFile(string inPath)
{
	assert(fHandle.size() == 0);
	
	MSshPacket p;
	p << uint8(SSH_FXP_OPEN) << fRequestId++ << inPath <<
		uint32(SSH_FXF_READ) << uint32(0);
	Send(p.data);
	fHandler = &MSftpChannelImp3::ProcessOpenFile;
}

void MSftpChannelImp3::ProcessOpenFile(uint8 inMessage, MSshPacket& in, MSshPacket& out)
{
	if (inMessage == SSH_FXP_HANDLE)
	{
		uint32 id;
		in >> id >> fHandle;
		
		assert(id == fRequestId - 1);
		
		out << uint8(SSH_FXP_FSTAT) << fRequestId++ << fHandle;
		fHandler = &MSftpChannelImp3::ProcessFStat;
	}
	else
	{
		assert(inMessage == SSH_FXP_STATUS);
		fHandler = nil;
	}
}

void MSftpChannelImp3::ProcessFStat(uint8 inMessage, MSshPacket& in, MSshPacket& out)
{
	if (inMessage == SSH_FXP_ATTRS)
	{
		fFileSize = 0;
		
		uint32 id, flags;
		in >> id >> flags;
		
		if (flags & SSH_FILEXFER_ATTR_SIZE)
			in >> fFileSize;
		
		fChannel.eChannelEvent(SFTP_FILE_SIZE_KNOWN);
		
		fOffset = 0;

		// we request all packets at once
		uint32 blockSize = kMaxPacketSize;
		for (int64 o = 0; o < fFileSize; o += blockSize)
		{
			MSshPacket p;
			p << uint8(SSH_FXP_READ) << fRequestId++ <<
				fHandle << o << blockSize;
			Send(p.data);
		}

		fHandler = &MSftpChannelImp3::ProcessRead;
	}
}

void MSftpChannelImp3::ProcessRead(uint8 inMessage, MSshPacket& in, MSshPacket& out)
{
	if (inMessage == SSH_FXP_DATA)
	{
		uint32 id;
		in >> id >> fData;
		
		fOffset += fData.length();

		fChannel.eChannelEvent(SFTP_DATA_AVAILABLE);
		
		int64 n = fFileSize - fOffset;
		if (n > kMaxPacketSize)
			n = kMaxPacketSize;
		
		if (n <= 0)
			fChannel.eChannelEvent(SFTP_DATA_DONE);
	}
	else
		fChannel.eChannelEvent(SFTP_ERROR);	
}

void MSftpChannelImp3::WriteFile(string inPath)
{
	assert(fHandle.size() == 0);

	MSshPacket p;
	p << uint8(SSH_FXP_OPEN) << fRequestId++ << inPath <<
		uint32(SSH_FXF_READ | SSH_FXF_WRITE | SSH_FXF_CREAT | SSH_FXF_TRUNC) << uint32(0);
	Send(p.data);
	fHandler = &MSftpChannelImp3::ProcessCreateFile;
}

void MSftpChannelImp3::ProcessCreateFile(uint8 inMessage, MSshPacket& in, MSshPacket& out)
{
	if (inMessage == SSH_FXP_HANDLE)
	{
		uint32 id;
		in >> id >> fHandle;
		
		assert(id == fRequestId - 1);
		
		fOffset = 0;
		fHandler = &MSftpChannelImp3::ProcessWrite;
		
		fChannel.eChannelEvent(SFTP_CAN_SEND_DATA);
	}
	else
	{
		assert(inMessage == SSH_FXP_STATUS);
		fHandler = nil;
	}
}

void MSftpChannelImp3::SendData(const string& inData)
{
	MSshPacket p;
	p << uint8(SSH_FXP_WRITE) << fRequestId++ << fHandle << fOffset << inData;
	Send(p.data);

	fOffset += inData.length();
	
	assert(fHandler == &MSftpChannelImp3::ProcessWrite);
}

void MSftpChannelImp3::ProcessWrite(uint8 inMessage, MSshPacket& in, MSshPacket& out)
{
	fChannel.eChannelEvent(SFTP_CAN_SEND_DATA);
}

void MSftpChannelImp3::CloseFile()
{
	MSshPacket p;
	p << uint8(SSH_FXP_CLOSE) << fRequestId++ << fHandle;
	Send(p.data);
	fHandle.clear();
	fHandler = &MSftpChannelImp3::ProcessClose;
}

void MSftpChannelImp3::ProcessClose(uint8 inMessage, MSshPacket& in, MSshPacket& out)
{
	assert(inMessage == SSH_FXP_STATUS);
	assert(fChannel.GetStatusCode() == 0);
	
	fHandler = nil;
	fChannel.eChannelEvent(SFTP_FILE_CLOSED);
}

/*
	Version 4 protocol
*/

MSftpChannelImp4::MSftpChannelImp4(MSftpChannel& inChannel)
	: MSftpChannelImp3(inChannel)
{
	assert(false);
}

void MSftpChannelImp4::Init(MSshPacket& out)
{
	assert(false);
}

void MSftpChannelImp4::MandlePacket(uint8 inMessage, MSshPacket& in, MSshPacket& out)
{
	assert(false);
}

/*
	The interface implementation
*/

MSftpChannel::MSftpChannel(string inIPAddress,
		string inUserName, uint16 inPort)
	: MSshChannel(inIPAddress, inUserName, inPort)
	, eChannelEventIn(this, &MSftpChannel::ChannelEvent)
	, fImpl(nil)		// we don't know yet what implementation to use
	, fPacketLength(0)
	, fStatusCode(0)

{
	AddRoute(eChannelEventIn, eChannelEvent);
}

MSftpChannel::~MSftpChannel()
{
	delete fImpl;
}

void MSftpChannel::Send(string inData)
{
	net_swapper swap;
	
	// wrap the packet adding the length
	uint32 l = inData.length();
	l = swap(l);
	inData.insert(0, reinterpret_cast<char*>(&l), sizeof(uint32));
	
	MSshChannel::Send(inData);
}

void MSftpChannel::ChannelEvent(
	int		inEvent)
{
	switch (inEvent)
	{
//		case SSH_CHANNEL_OPENED:
//			fImpl->fPacketSize = GetMaxPacketSize();
//			break;
		
		case SSH_CHANNEL_SUCCESS:
			if (fImpl == nil)
			{
#pragma message("Move to version 4 protocol someday")
				MSshPacket p;
				p << uint8(SSH_FXP_INIT) << uint32(3);
				Send(p.data);
			}
			break;
	}
}

void MSftpChannel::MandleData(string inData)
{
	if (fLeftOver.length() > 0)
	{
		fLeftOver.append(inData);
		inData = fLeftOver;
		fLeftOver.clear();
	}
	
	while (inData.length() > 0)
	{
		if (fPacket.length() > 0 or fPacketLength > 0)
		{	// continue to receive next packet
			assert(fPacketLength > 0);
			
			uint32 l = fPacketLength;
			if (l > inData.length())
				l = inData.length();
			
			fPacket.append(inData, 0, l);
			inData.erase(0, l);
			
			fPacketLength -= l;
		}
		else if (inData.length() >= sizeof(uint32))
		{
			MSshPacket in;
			in.data = inData;
		
			in >> fPacketLength;
			
			uint32 n = fPacketLength;
			if (n > in.data.length())
				n = in.data.length();
			
			if (n > 0)
			{
				fPacket.assign(inData, 4, n);
				inData.erase(0, n + 4);
				fPacketLength -= fPacket.length();
			}
			else
			{
				inData.erase(0, 4);
				fPacket.clear();
			}
		}
		else
		{
			fLeftOver = inData;
			inData.clear();
		}

		if (fPacket.length() > 0 and fPacketLength == 0)
		{
			MSshPacket in, out;
			in.data = fPacket;
			
			uint8 msg;
			in >> msg;
			
			if (msg == SSH_FXP_STATUS)
				MandleStatus(in);
			else
				fStatusCode = SSH_FX_OK;
			
			if (msg == SSH_FXP_VERSION)
			{
					// we now know what version of the server we're talking to
				fImpl = MSftpChannelImp::CreateImpl(in, out, *this);
				fImpl->fPacketSize = GetMaxPacketSize();
				
				fImpl->Init(out);
			}
			else
			{
				assert(fImpl != nil);
				if (fImpl != nil)
					fImpl->MandlePacket(msg, in, out);
			}
			
			if (out.data.length() > 0)
				Send(out.data);
			
			fPacket.clear();
			fPacketLength = 0;
		}
	}
}
	
void MSftpChannel::MandleExtraData(int inType, string inData)
{
}

void MSftpChannel::MandleStatus(MSshPacket in)
{
	uint32 id;
	string msg, lang;
	
	in >> id >> fStatusCode >> msg >> lang;
	
	if (fStatusCode > SSH_FX_EOF)
		eChannelMessage(msg);
}

void MSftpChannel::SetCWD(std::string inDir)
{
//	ThrowIfNil(fImpl);
	fImpl->SetCWD(inDir);
}

string MSftpChannel::GetCWD() const
{
//	ThrowIfNil(fImpl);
	return fImpl->fCurrentDir;
}

void MSftpChannel::OpenDir()
{
//	ThrowIfNil(fImpl);
	fImpl->OpenDir();
}

bool MSftpChannel::NextFile(uint32& ioCookie, string& outName,
							uint64& outSize, uint32& outDate, char& outType)
{
//	ThrowIfNil(fImpl);
	
	MSftpChannelImp::DirList& dirList = fImpl->fDirList;

	if (ioCookie < 0 or ioCookie >= dirList.size())
		return false;
	
	outName = dirList[ioCookie].name;
	outSize = dirList[ioCookie].size;
	outDate = dirList[ioCookie].date;
	outType = dirList[ioCookie].type;
	++ioCookie;
	
	return true;
}

void MSftpChannel::MkDir(string inPath)
{
//	ThrowIfNil(fImpl);
	fImpl->MkDir(inPath);
}

void MSftpChannel::ReadFile(string inPath, bool inTextMode)
{
//	ThrowIfNil(fImpl);
	fImpl->ReadFile(inPath);
}

void MSftpChannel::WriteFile(string inPath, bool inTextMode)
{
//	ThrowIfNil(fImpl);
	fImpl->WriteFile(inPath);
}

void MSftpChannel::SendData(const string& inData)
{
//	ThrowIfNil(fImpl);
	fImpl->SendData(inData);
}

void MSftpChannel::CloseFile()
{
//	ThrowIfNil(fImpl);
	fImpl->CloseFile();
}

uint64 MSftpChannel::GetFileSize() const
{
//	ThrowIfNil(fImpl);
	return fImpl->fFileSize;
}

string MSftpChannel::GetData() const
{
//	ThrowIfNil(fImpl);
	return fImpl->fData;
}

