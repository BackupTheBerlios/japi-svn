//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

/*	$Id: MSftpChannel.cpp,v 1.22 2004/02/28 22:28:13 maarten Exp $
	Copyright Maarten
	Created Sunday September 28 2003 11:39:23
	
	Implementation of the Version 3 Secure File Transfer Protocol
*/

#include "MJapi.h"

#include "MError.h"
#include "MFile.h"

#include "MSsh.h"
#include "MSftpChannel.h"
#include "MSshConnection.h"

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

//struct MSftpChannelImp
//{
//							MSftpChannelImp(
//								MSftpChannel&	inChannel);
//	virtual					~MSftpChannelImp();
//	
//	static MSftpChannelImp*	CreateImpl(
//								MSshPacket&		in,
//								MSshPacket&		out,
//								MSftpChannel&	inChannel);
//	
//	virtual void			Init(
//								MSshPacket&		out) = 0;
//
//	virtual void			HandlePacket(
//								uint8			inMessage,
//								MSshPacket&		in,
//								MSshPacket&		out) = 0;
//	
////	uint32					GetStatusCode(MSshPacket& inPacket);
//	
//	void					Send(
//								MSshPacket&		out)
//							{
//								mChannel.Send(out);
//							}
//
//	// action interface
//	
//	virtual void			SetCWD(
//								string			inPath) = 0;
//
//	virtual void			OpenDir() = 0;
//
//	virtual void			MkDir(
//								string			inPath) = 0;
//
//	virtual void			ReadFile(
//								string			inPath) = 0;
//
//	virtual void			WriteFile(
//								string			inPath) = 0;
//
//	virtual void			SendData(
//								const string&	inData) = 0;
//
//	virtual void			CloseFile() = 0;
//
//	struct DirEntry {
//		string				name;
//		uint64				size;
//		uint32				date;
//		char				type;
//	};
//
//	typedef std::vector<DirEntry>	DirList;
//
//	MSftpChannel&			mChannel;
//	uint32					mRequestId;
//	uint32					mPacketSize;
//	string					mHandle;
//	int64					mFileSize;
//	int64					mOffset;
//	DirList					mDirList;
//	string					mCurrentDir;
//	string					mData;
//};
//
//struct MSftpChannelImp3 : public MSftpChannelImp
//{
//							MSftpChannelImp3(MSftpChannel& inChannel);
//
//	virtual void			Init(MSshPacket& out);
//	virtual void			HandlePacket(
//								uint8		inMessage,
//								MSshPacket&	in,
//								MSshPacket&	out);
//
//	void					CheckForError(
//								uint8		inMessage,
//								MSshPacket&	in);
//
//	// action interface
//	virtual void			SetCWD(string inPath);
//	virtual void			OpenDir();
//	virtual void			MkDir(string inPath);
//	virtual void			ReadFile(string inPath);
//	virtual void			WriteFile(string inPath);
//	virtual void			SendData(const string& inData);
//	virtual void			CloseFile();
//
//	void (MSftpChannelImp3::*mHandler)(
//								uint8		inMessage,
//								MSshPacket&	in,
//								MSshPacket&	out);
//
//	void					ProcessRealPath(
//								uint8		inMessage,
//								MSshPacket&	in,
//								MSshPacket&	out);
//
//	void					ProcessOpenDir(
//								uint8		inMessage,
//								MSshPacket&	in,
//								MSshPacket&	out);
//
//	void					ProcessReadDir(
//								uint8		inMessage,
//								MSshPacket&	in,
//								MSshPacket&	out);
//
//	void					ProcessMkDir(
//								uint8		inMessage,
//								MSshPacket&	in,
//								MSshPacket&	out);
//
//	void					ProcessOpenFile(
//								uint8		inMessage,
//								MSshPacket&	in,
//								MSshPacket&	out);
//
//	void					ProcessFStat(
//								uint8		inMessage,
//								MSshPacket&	in,
//								MSshPacket&	out);
//
//	void					ProcessRead(
//								uint8		inMessage,
//								MSshPacket&	in,
//								MSshPacket&	out);
//
//	void					ProcessCreateFile(
//								uint8		inMessage,
//								MSshPacket&	in,
//								MSshPacket&	out);
//
//	void					ProcessWrite(
//								uint8		inMessage,
//								MSshPacket&	in,
//								MSshPacket&	out);
//
//	void					ProcessClose(
//								uint8		inMessage,
//								MSshPacket&	in,
//								MSshPacket&	out);
//};
//
//struct MSftpChannelImp4 : public MSftpChannelImp3
//{
//							MSftpChannelImp4(MSftpChannel& inChannel);
//
//	virtual void			Init(MSshPacket& out);
//
//	virtual void			HandlePacket(
//								uint8		inMessage,
//								MSshPacket&	in,
//								MSshPacket&	out);
//};
//
///*
//	Implementations
//*/
//
//MSftpChannelImp* MSftpChannelImp::CreateImpl(MSshPacket& in, MSshPacket& out, MSftpChannel& inChannel)
//{
//	uint32 version;
//	in >> version;
//	
//	PRINT(("Connecting to a %d version SFTP server", version));
//	
//	MSftpChannelImp* result = nil;
//	
//	if (version == 4)
//		result = new MSftpChannelImp4(inChannel);
//	else if (version == 3)
//		result = new MSftpChannelImp3(inChannel);
//	else
//		THROW(("Protocol version %d is not supported", version));
//	
//	return result;
//}
//
//
//MSftpChannelImp::MSftpChannelImp(MSftpChannel& inChannel)
//	: mChannel(inChannel)
//	, mRequestId(0)
//{
//}
//
//MSftpChannelImp::~MSftpChannelImp()
//{
//}
//
///*
//	Version 3 protocol
//*/
//
//MSftpChannelImp3::MSftpChannelImp3(MSftpChannel& inChannel)
//	: MSftpChannelImp(inChannel)
//	, mHandler(nil)
//{
//}
//
//void MSftpChannelImp3::Init(MSshPacket& out)
//{
//	mChannel.HandleChannelEvent(SFTP_INIT_DONE);
//}
//
//void MSftpChannelImp3::HandlePacket(
//	uint8		inMessage,
//	MSshPacket&	in,
//	MSshPacket&	out)
//{
//	PRINT(("SFTP: %d", inMessage));
//
//	if (mHandler != nil)
//		(this->*mHandler)(inMessage, in, out);
//	else
//		PRINT(("Handler was nil, packet %d dropped", inMessage));
//}
//
//void MSftpChannelImp3::CheckForError(
//	uint8		inMessage,
//	MSshPacket&	in)
//{
//	if (inMessage == SSH_FXP_STATUS)
//	{
//		uint32 id, error_code;
//		string message;
//		in >> id >> error_code >> message;
//		if (error_code > SSH_FX_EOF)
//		{
//PRINT(("error code: %d, status message: %s", error_code, message.c_str()));
//			mChannel.HandleChannelEvent(SFTP_ERROR);
//			THROW(("Error in SFTP transfer: %s", message.c_str()));
//		}
//	}
//}
//
//void MSftpChannelImp3::SetCWD(string inPath)
//{
//	if (inPath.length() == 0)
//		inPath = ".";
//
//	MSshPacket out;
//	out << uint8(SSH_FXP_REALPATH) << mRequestId++ << inPath;
//	Send(out);
//
//	mHandler = &MSftpChannelImp3::ProcessRealPath;
//}
//
//void MSftpChannelImp3::OpenDir()
//{
//	mDirList.clear();
//	
//	MSshPacket out;
//	out << uint8(SSH_FXP_OPENDIR) << mRequestId++ << mCurrentDir;
//	Send(out);
//
//	mHandler = &MSftpChannelImp3::ProcessOpenDir;
//}
//
//void MSftpChannelImp3::ProcessRealPath(
//	uint8		inMessage,
//	MSshPacket&	in,
//	MSshPacket&	out)
//{
//	try
//	{
//		if (inMessage != SSH_FXP_NAME)
//			throw -1;
//		
//		uint32 id, count;
//		string dir;
//		
//		in >> id >> count >> dir;
//		if (id != mRequestId - 1 or count != 1)
//			throw -1;
//
//		mCurrentDir = dir;
//		mDirList.clear();
//
//		mHandler = nil;
//		mChannel.HandleChannelEvent(SFTP_SETCWD_OK);
//	}
//	catch (...)
//	{
//		mHandler = nil;
//		mChannel.HandleChannelEvent(SFTP_ERROR);
//	}	
//}
//
//void MSftpChannelImp3::ProcessOpenDir(
//	uint8		inMessage,
//	MSshPacket&	in,
//	MSshPacket&	out)
//{
//	if (inMessage == SSH_FXP_HANDLE)
//	{
//		uint32 id;
//		in >> id >> mHandle;
//		out << uint8(SSH_FXP_READDIR) << mRequestId++ << mHandle;
//		mHandler = &MSftpChannelImp3::ProcessReadDir;
//	}
//	else
//		mHandler = nil;
//}
//
//void MSftpChannelImp3::ProcessReadDir(
//	uint8		inMessage,
//	MSshPacket&	in,
//	MSshPacket&	out)
//{
//	bool done = true;
//	
//	if (inMessage == SSH_FXP_NAME)
//	{
//		uint32 id, count;
//		string dir;
//		
//		in >> id >> count;
//		
//		if (id != mRequestId - 1)
//			;
//		
//		if (count != 0)
//		{
//			done = false;
//
//			while (count-- > 0)
//			{
//				uint32 flags, dummy_i;
//				
//				string dummy_s;
//				
//				DirEntry e;
//				in >> e.name >> dummy_s >> flags;
//
//				e.type = dummy_s[0];
//				
//					// for now...
//				if (e.type == 'l')
//					e.type = 'd';
//				
//				if (flags & SSH_FILEXFER_ATTR_SIZE)
//					in >> e.size;
//	
//				if (flags & SSH_FILEXFER_ATTR_UIDGID)
//					in >> dummy_i >> dummy_i;
//					
//				if (flags & SSH_FILEXFER_ATTR_PERMISSIONS)
//					in >> dummy_i;
//				
//				if (flags & SSH_FILEXFER_ATTR_ACMODTIME)
//					in >> dummy_i;
//				
//				if (flags & SSH_FILEXFER_ATTR_ACMODTIME)
//					in >> e.date;
//				
//				if (flags & SSH_FILEXFER_ATTR_EXTENDED)
//				{
//					in >> dummy_i;
//					while (dummy_i-- > 0)
//						in >> dummy_s >> dummy_s;
//				}
//				
//				if (e.name != "." and e.name != "..")
//					mDirList.push_back(e);
//			}
//			
//			out << uint8(SSH_FXP_READDIR) << mRequestId++ << mHandle;
//		}
//	}
//
//	if (done)
//	{
//		mChannel.HandleChannelEvent(SFTP_DIR_LISTING_AVAILABLE);
//		
//		out << uint8(SSH_FXP_CLOSE) << mRequestId++ << mHandle;
//		
//		mHandler = nil;
//	}
//}
//
//void MSftpChannelImp3::MkDir(string inPath)
//{
//	MSshPacket out;
//	out << uint8(SSH_FXP_MKDIR) << mRequestId++ << inPath << uint32(0);
//	Send(out);
//	mHandler = &MSftpChannelImp3::ProcessMkDir;
//}
//
//void MSftpChannelImp3::ProcessMkDir(
//	uint8		inMessage,
//	MSshPacket&	in,
//	MSshPacket&	out)
//{
//	CheckForError(inMessage, in);
//	mHandler = nil;
//}
//
//void MSftpChannelImp3::ReadFile(string inPath)
//{
//	assert(mHandle.size() == 0);
//
//	MSshPacket out;
//	out << uint8(SSH_FXP_OPEN) << mRequestId++ << inPath <<
//		uint32(SSH_FXF_READ) << uint32(0);
//	Send(out);
//	mHandler = &MSftpChannelImp3::ProcessOpenFile;
//}
//
//void MSftpChannelImp3::ProcessOpenFile(
//	uint8		inMessage,
//	MSshPacket&	in,
//	MSshPacket&	out)
//{
//	if (inMessage != SSH_FXP_HANDLE)
//		CheckForError(inMessage, in);
//
//	uint32 id;
//	in >> id >> mHandle;
//	
//	assert(id == mRequestId - 1);
//	
//	out << uint8(SSH_FXP_FSTAT) << mRequestId++ << mHandle;
//	mHandler = &MSftpChannelImp3::ProcessFStat;
//}
//
//void MSftpChannelImp3::ProcessFStat(
//	uint8		inMessage,
//	MSshPacket&	in,
//	MSshPacket&	out)
//{
//	if (inMessage == SSH_FXP_ATTRS)
//	{
//		mFileSize = 0;
//		
//		uint32 id, flags;
//		in >> id >> flags;
//		
//		if (flags & SSH_FILEXFER_ATTR_SIZE)
//			in >> mFileSize;
//		
//		mChannel.HandleChannelEvent(SFTP_FILE_SIZE_KNOWN);
//		
//		mOffset = 0;
//
//		// we request all packets at once
//		uint32 blockSize = kMaxPacketSize;
//		for (int64 o = 0; o < mFileSize; o += blockSize)
//		{
//			MSshPacket out;
//			out << uint8(SSH_FXP_READ) << mRequestId++ <<
//				mHandle << o << blockSize;
//			Send(out);
//		}
//
//		mHandler = &MSftpChannelImp3::ProcessRead;
//	}
//}
//
//void MSftpChannelImp3::ProcessRead(
//	uint8		inMessage,
//	MSshPacket&	in,
//	MSshPacket&	out)
//{
//	if (inMessage == SSH_FXP_DATA)
//	{
//		uint32 id;
//		in >> id >> mData;
//		
//		mOffset += mData.length();
//
//		mChannel.HandleChannelEvent(SFTP_DATA_AVAILABLE);
//		
//		int64 n = mFileSize - mOffset;
//		if (n > kMaxPacketSize)
//			n = kMaxPacketSize;
//		
//		if (n <= 0)
//			mChannel.HandleChannelEvent(SFTP_DATA_DONE);
//	}
//	else
//		mChannel.HandleChannelEvent(SFTP_ERROR);	
//}
//
//void MSftpChannelImp3::WriteFile(string inPath)
//{
//	assert(mHandle.size() == 0);
//	MSshPacket out;
//	out << uint8(SSH_FXP_OPEN) << mRequestId++ << inPath <<
//		uint32(SSH_FXF_READ | SSH_FXF_WRITE | SSH_FXF_CREAT | SSH_FXF_TRUNC) << uint32(0);
//	Send(out);
//	mHandler = &MSftpChannelImp3::ProcessCreateFile;
//}
//
//void MSftpChannelImp3::ProcessCreateFile(
//	uint8		inMessage,
//	MSshPacket&	in,
//	MSshPacket&	out)
//{
//	CheckForError(inMessage, in);
//
//	uint32 id;
//	in >> id >> mHandle;
//	
//	assert(id == mRequestId - 1);
//	
//	mOffset = 0;
//	mHandler = &MSftpChannelImp3::ProcessWrite;
//	
//	mChannel.HandleChannelEvent(SFTP_CAN_SEND_DATA);
//}
//
//void MSftpChannelImp3::SendData(const string& inData)
//{
//	MSshPacket out;
//	out << uint8(SSH_FXP_WRITE) << mRequestId++ << mHandle << mOffset << inData;
//	Send(out);
//
//	mOffset += inData.length();
//	
//	assert(mHandler == &MSftpChannelImp3::ProcessWrite);
//}
//
//void MSftpChannelImp3::ProcessWrite(
//	uint8		inMessage,
//	MSshPacket&	in,
//	MSshPacket&	out)
//{
//	mChannel.HandleChannelEvent(SFTP_CAN_SEND_DATA);
//}
//
//void MSftpChannelImp3::CloseFile()
//{
//	MSshPacket out;
//	out << uint8(SSH_FXP_CLOSE) << mRequestId++ << mHandle;
//	Send(out);
//	mHandle.clear();
//	mHandler = &MSftpChannelImp3::ProcessClose;
//}
//
//void MSftpChannelImp3::ProcessClose(
//	uint8		inMessage,
//	MSshPacket&	in,
//	MSshPacket&	out)
//{
//	CheckForError(inMessage, in);
//	assert(mChannel.GetStatusCode() == 0);
//	
//	mHandler = nil;
//	mChannel.HandleChannelEvent(SFTP_FILE_CLOSED);
//}
//
///*
//	Version 4 protocol
//*/
//
//MSftpChannelImp4::MSftpChannelImp4(MSftpChannel& inChannel)
//	: MSftpChannelImp3(inChannel)
//{
//	assert(false);
//}
//
//void MSftpChannelImp4::Init(MSshPacket& out)
//{
//	assert(false);
//}
//
//void MSftpChannelImp4::HandlePacket(
//	uint8		inMessage,
//	MSshPacket&	in,
//	MSshPacket&	out)
//{
//	assert(false);
//}

/*
	The interface implementation
*/

MSftpChannel* MSftpChannel::Open(
	string		inIPAddress,
	string		inUserName,
	uint16		inPort)
{
	MSshConnection* connection = MSshConnection::Get(inIPAddress, inUserName, inPort);
	return new MSftpChannel(*connection);
}

MSftpChannel::MSftpChannel(
	MSshConnection&	inConnection)
	: MSshChannel(inConnection)
	, mImpl(nil)
//	, mPacketLength(0)
//	, mStatusCode(0)
{
}

MSftpChannel::~MSftpChannel()
{
//	delete mImpl;
}

//void MSftpChannel::Send(string inData)
//{
//	net_swapper swap;
//	
//	// wrap the packet adding the length
//	uint32 l = inData.length();
//	l = swap(l);
//	inData.insert(0, reinterpret_cast<char*>(&l), sizeof(uint32));
//	
//	MSshChannel::Send(inData);
//}

void MSftpChannel::HandleChannelEvent(
	int		inEvent)
{
//	switch (inEvent)
//	{
////		case SSH_CHANNEL_OPENED:
////			mImpl->mPacketSize = GetMaxPacketSize();
////			break;
//		
//		case SSH_CHANNEL_SUCCESS:
//			if (mImpl == nil)
//			{
////#warning("Move to version 4 protocol someday")
//				MSshPacket out;
//				out << uint8(SSH_FXP_INIT) << uint32(3);
//				Send(out);
//			}
//			break;
//	}
	
	MSshChannel::HandleChannelEvent(inEvent);
}

void MSftpChannel::HandleData(
	MSshPacket&	inData)
{//
//	if (mLeftOver.length() > 0)
//	{
//		mLeftOver.append(inData);
//		inData = mLeftOver;
//		mLeftOver.clear();
//	}
//	
//	while (inData.length() > 0)
//	{
//		if (mPacket.length() > 0 or mPacketLength > 0)
//		{	// continue to receive next packet
//			assert(mPacketLength > 0);
//			
//			uint32 l = mPacketLength;
//			if (l > inData.length())
//				l = inData.length();
//			
//			mPacket.append(inData, 0, l);
//			inData.erase(0, l);
//			
//			mPacketLength -= l;
//		}
//		else if (inData.length() >= sizeof(uint32))
//		{
//			MSshPacket in(inData);
//		
//			in >> mPacketLength;
//			
//			uint32 n = mPacketLength;
//			if (n > in.data.length())
//				n = in.data.length();
//			
//			if (n > 0)
//			{
//				mPacket.assign(inData, 4, n);
//				inData.erase(0, n + 4);
//				mPacketLength -= mPacket.length();
//			}
//			else
//			{
//				inData.erase(0, 4);
//				mPacket.clear();
//			}
//		}
//		else
//		{
//			mLeftOver = inData;
//			inData.clear();
//		}
//
//		if (mPacket.length() > 0 and mPacketLength == 0)
//		{
//			MSshPacket in, out;
//			in.data = mPacket;
//			
//			uint8 msg;
//			in >> msg;
//			
//			if (msg == SSH_FXP_STATUS)
//				HandleStatus(in);
//			else
//				mStatusCode = SSH_FX_OK;
//			
//			if (msg == SSH_FXP_VERSION)
//			{
//					// we now know what version of the server we're talking to
//				mImpl = MSftpChannelImp::CreateImpl(in, out, *this);
//				mImpl->mPacketSize = GetMaxSendPacketSize();
//				
//				mImpl->Init(out);
//			}
//			else
//			{
//				assert(mImpl != nil);
//				if (mImpl != nil)
//					mImpl->HandlePacket(msg, in, out);
//			}
//			
//			if (nout out.empty())
//				Send(out);
//			
//			mPacket.clear();
//			mPacketLength = 0;
//		}
//	}
}
	
void MSftpChannel::HandleExtraData(int inType, MSshPacket& inData)
{
}

void MSftpChannel::HandleStatus(MSshPacket in)
{//
//	uint32 id;
//	string msg, lang;
//	
//	in >> id >> mStatusCode >> msg >> lang;
//	
//	if (mStatusCode > SSH_FX_EOF)
//		eChannelMessage(msg);
}

void MSftpChannel::SetCWD(std::string inDir)
{
//	mImpl->SetCWD(inDir);
}

string MSftpChannel::GetCWD() const
{
//	return mImpl->mCurrentDir;
}

void MSftpChannel::OpenDir()
{
//	mImpl->OpenDir();
}

bool MSftpChannel::NextFile(uint32& ioCookie, string& outName,
							uint64& outSize, uint32& outDate, char& outType)
{//
//	MSftpChannelImp::DirList& dirList = mImpl->mDirList;
//
//	if (ioCookie < 0 or ioCookie >= dirList.size())
//		return false;
//	
//	outName = dirList[ioCookie].name;
//	outSize = dirList[ioCookie].size;
//	outDate = dirList[ioCookie].date;
//	outType = dirList[ioCookie].type;
//	++ioCookie;
//	
//	return true;
}

void MSftpChannel::MkDir(string inPath)
{
//	mImpl->MkDir(inPath);
}

void MSftpChannel::ReadFile(string inPath, bool inTextMode)
{
//	mImpl->ReadFile(inPath);
}

void MSftpChannel::WriteFile(string inPath, bool inTextMode)
{
//	mImpl->WriteFile(inPath);
}

void MSftpChannel::SendData(const string& inData)
{
//	mImpl->SendData(inData);
}

void MSftpChannel::CloseFile()
{
//	mImpl->CloseFile();
}

uint64 MSftpChannel::GetFileSize() const
{
//	return mImpl->mFileSize;
}

string MSftpChannel::GetData() const
{
//	return mImpl->mData;
}

