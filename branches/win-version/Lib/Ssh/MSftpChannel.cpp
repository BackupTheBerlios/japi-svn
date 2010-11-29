//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

/*	$Id: MSftpChannel.cpp,v 1.22 2004/02/28 22:28:13 maarten Exp $
	Copyright Maarten
	Created Sunday September 28 2003 11:39:23
	
	Implementation of the Version 3 Secure File Transfer Protocol
*/

#include "MLib.h"

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

MSftpChannel::MSftpChannel(
	MSshConnection&	inConnection)
	: MSshChannel(inConnection)
	, mHandler(0)
	, mPacketLength(0)
	, mRequestId(0)
	, mFileSize(0)
	, mOffset(0)
{
}

MSftpChannel::MSftpChannel(
	const string&		inHost,
	const string&		inUser,
	uint16				inPort)
	: MSshChannel(*MSshConnection::Get(inHost, inUser, inPort))
	, mHandler(0)
	, mPacketLength(0)
	, mRequestId(0)
	, mFileSize(0)
	, mOffset(0)
{
}

void MSftpChannel::Opened()
{
	MSshChannel::Opened();

	MSshPacket out;
	out << uint8(SSH_FXP_INIT) << uint32(3);
	Send(out);
}

void MSftpChannel::SFTPInitialised()
{
}

void MSftpChannel::Send(
	MSshPacket&		inData)
{
	// wrap the packet again...
	MSshPacket out;
	out << inData;
	MSshChannel::SendData(out);
}

void MSftpChannel::ReceiveData(
	MSshPacket&		inData)
{
	copy(inData.peek(), inData.peek() + inData.size(), back_inserter(mPacket));

	while (not mPacket.empty())
	{
		if (mPacketLength > 0 and mPacketLength <= mPacket.size())
		{
			MSshPacket in(mPacket, mPacketLength), out;

			uint8 msg = mPacket.front();

			mPacket.erase(mPacket.begin(), mPacket.begin() + mPacketLength);
			mPacketLength = 0;

			ProcessPacket(msg, in);
			continue;
		}
		
		if (mPacketLength == 0 and mPacket.size() >= sizeof(uint32))
		{
			for (uint32 i = 0; i < 4; ++i)
			{
				mPacketLength = mPacketLength << 8 | mPacket.front();
				mPacket.pop_front();
			}
			
			continue;
		}
		
		break;
	}
}

void MSftpChannel::ProcessPacket(
	uint8		msg,
	MSshPacket&	in)
{
	for (;;)
	{
		if (msg == SSH_FXP_VERSION)
		{
			SFTPInitialised();
			break;
		}

		if (msg == SSH_FXP_STATUS)
		{
			uint8 msg;
			uint32 id, statusCode;
			string message, lang;
			
			in >> msg >> id >> statusCode >> message >> lang;
			
			if (statusCode > SSH_FX_EOF)
			{
				ChannelError(message);
				Close();
				break;
			}
		}
		
		if (mHandler != nil)
			(this->*mHandler)(msg, in);
		else if (msg != SSH_FXP_STATUS)
			PRINT(("Unhandled SFTP Message %d", msg));
		
		break;
	}
}

void MSftpChannel::ReadFile(const string& inPath)
{
	MSshPacket out;
	out << uint8(SSH_FXP_OPEN) << ++mRequestId << inPath <<
		uint32(SSH_FXF_READ) << uint32(0);
	
	mHandler = &MSftpChannel::ProcessOpenFile;
	Send(out);
}

void MSftpChannel::ProcessOpenFile(
	uint8		inMessage,
	MSshPacket&	in)
{
	Match(inMessage, SSH_FXP_HANDLE);

	uint8 msg;
	uint32 id;
	in >> msg >> id >> mHandle;

	if (id != mRequestId)
		THROW(("Invalid request ID"));
	
	MSshPacket out;	
	out << uint8(SSH_FXP_FSTAT) << ++mRequestId << mHandle;
	mHandler = &MSftpChannel::ProcessFStat;
	Send(out);
}

void MSftpChannel::ProcessFStat(
	uint8		inMessage,
	MSshPacket&	in)
{
	Match(inMessage, SSH_FXP_ATTRS);

	mFileSize = 0;
	
	uint8 msg;
	uint32 id, flags;
	in >> msg >> id >> flags;
	
	if (id != mRequestId)
		THROW(("Invalid request ID"));
	
	if (flags & SSH_FILEXFER_ATTR_SIZE)
		in >> mFileSize;
	
	mOffset = 0;

	// we request all packets at once
	uint32 blockSize = kMaxPacketSize;
	if (blockSize > mMaxSendPacketSize - 4 * sizeof(uint32))
		blockSize = mMaxSendPacketSize - 4 * sizeof(uint32);

	for (int64 o = 0; o < mFileSize; o += blockSize)
	{
		MSshPacket out;
		out << uint8(SSH_FXP_READ) << ++mRequestId <<
			mHandle << o << blockSize;
		Send(out);
		mHandler = &MSftpChannel::ProcessRead;
	}
}

void MSftpChannel::ProcessRead(
	uint8		inMessage,
	MSshPacket&	in)
{
	Match(inMessage, SSH_FXP_DATA);

	uint8 msg;
	uint32 id;
	in >> msg >> id >> mData;
	
	ReceiveData(mData, mOffset, mFileSize);
	mOffset += mData.length();
	
	if (mOffset == mFileSize)
	{
		MSshPacket out;
		out << uint8(SSH_FXP_CLOSE) << ++mRequestId << mHandle;
		mHandle.clear();
		mHandler = &MSftpChannel::ProcessClose;
		Send(out);
	}
}

void MSftpChannel::WriteFile(const string& inPath)
{
	MSshPacket out;
	out << uint8(SSH_FXP_OPEN) << ++mRequestId << inPath <<
		uint32(SSH_FXF_READ | SSH_FXF_WRITE | SSH_FXF_CREAT | SSH_FXF_TRUNC) << uint32(0);
	mHandler = &MSftpChannel::ProcessCreateFile;
	Send(out);
}

void MSftpChannel::ProcessCreateFile(
	uint8		inMessage,
	MSshPacket&	in)
{
	uint8 msg;
	uint32 id;
	in >> msg >> id >> mHandle;
	
	mOffset = 0;
	mData.clear();
	SendData(mOffset, mMaxSendPacketSize - 4 * sizeof(uint32), mData);

	MSshPacket out;
	if (not mData.empty())
	{
		out << uint8(SSH_FXP_WRITE) << mRequestId++ << mHandle << mOffset << mData;
		mHandler = &MSftpChannel::ProcessWrite;
	}
	else
	{
		out << uint8(SSH_FXP_CLOSE) << ++mRequestId << mHandle;
		mHandle.clear();
		mHandler = &MSftpChannel::ProcessClose;
	}
	Send(out);
}

void MSftpChannel::ProcessWrite(
	uint8		inMessage,
	MSshPacket&	in)
{
	mOffset += mData.length();
	mData.clear();
	SendData(mOffset, mMaxSendPacketSize - 4 * sizeof(uint32), mData);
	
	MSshPacket out;
	if (not mData.empty())
		out << uint8(SSH_FXP_WRITE) << mRequestId++ << mHandle << mOffset << mData;
	else
	{
		out << uint8(SSH_FXP_CLOSE) << ++mRequestId << mHandle;
		mHandle.clear();
		mHandler = &MSftpChannel::ProcessClose;
	}
	Send(out);
}

void MSftpChannel::ProcessClose(
	uint8		inMessage,
	MSshPacket&	in)
{
	FileClosed();
	mHandler = nil;
	Close();
}

void MSftpChannel::Match(
	uint8		inExpected,
	uint8		inReceived)
{
	if (inExpected != inReceived)
		THROW(("Unexpected SFTP message"));
}

//struct MSftpChannelImp
//{
//							MSftpChannelImp(
//								MSftpChannel&	inChannel,
//								uint32 inMaxPacketSize);
//
//	virtual					~MSftpChannelImp();
//	
//	static MSftpChannelImp*	CreateImpl(
//								MSshPacket&		in,
//								MSshPacket&		out,
//								uint32			inMaxPacketSize,
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
//	uint32					GetStatusCode(MSshPacket& inPacket);
//	
//	// action interface
//	virtual void			SetCWD(
//								const string&			inPath) = 0;
//
//	virtual void			OpenDir() = 0;
//
//	virtual void			MkDir(
//								const string&			inPath) = 0;
//
//	virtual void			ReadFile(
//								const string&			inPath) = 0;
//
//	virtual void			WriteFile(
//								const string&			inPath) = 0;
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
//							MSftpChannelImp3(MSftpChannel& inChannel, uint32 inMaxPacketSize);
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
//	virtual void			SetCWD(const string& inPath);
//	virtual void			OpenDir();
//	virtual void			MkDir(const string& inPath);
//	virtual void			ReadFile(const string& inPath);
//	virtual void			WriteFile(const string& inPath);
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
//							MSftpChannelImp4(MSftpChannel& inChannel, uint32 inMaxPacketSize);
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
//MSftpChannelImp* MSftpChannelImp::CreateImpl(MSshPacket& in, MSshPacket& out, 
//	uint32 inMaxPacketSize, MSftpChannel& inChannel)
//{
//	uint32 version;
//	in >> version;
//	
//	PRINT(("Connecting to a %d version SFTP server", version));
//	
//	MSftpChannelImp* result = nil;
//	
//	if (version == 4)
//		result = new MSftpChannelImp4(inChannel, inMaxPacketSize);
//	else if (version == 3)
//		result = new MSftpChannelImp3(inChannel, inMaxPacketSize);
//	else
//		THROW(("Protocol version %d is not supported", version));
//	
//	return result;
//}
//
//
//MSftpChannelImp::MSftpChannelImp(MSftpChannel& inChannel, uint32 inMaxPacketSize)
//	: mChannel(inChannel)
//	, mRequestId(0)
//	, mPacketSize(inMaxPacketSize)
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
//MSftpChannelImp3::MSftpChannelImp3(MSftpChannel& inChannel, uint32 inMaxPacketSize)
//	: MSftpChannelImp(inChannel, inMaxPacketSize)
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
//void MSftpChannelImp3::SetCWD(const string& inPath)
//{
//	MSshPacket out;
//	out << uint8(SSH_FXP_REALPATH) << mRequestId++;
//	if (inPath.empty())
//		out << ".";
//	else
//		out << inPath;
//	mChannel.Send(out);
//	mHandler = &MSftpChannelImp3::ProcessRealPath;
//}
//
//void MSftpChannelImp3::OpenDir()
//{
//	mDirList.clear();
//	
//	MSshPacket out;
//	out << uint8(SSH_FXP_OPENDIR) << mRequestId++ << mCurrentDir;
//	mChannel.Send(out);
//
//	mHandler = &MSftpChannelImp3::ProcessOpenDir;
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
//void MSftpChannelImp3::MkDir(const string& inPath)
//{
//	MSshPacket out;
//	out << uint8(SSH_FXP_MKDIR) << mRequestId++ << inPath << uint32(0);
//	mChannel.Send(out);
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
//			mChannel.Send(out);
//		}
//
//		mHandler = &MSftpChannelImp3::ProcessRead;
//	}
//}
//
//void MSftpChannelImp3::SendData(const string& inData)
//{
//	MSshPacket out;
//	out << uint8(SSH_FXP_WRITE) << mRequestId++ << mHandle << mOffset << inData;
//	mChannel.Send(out);
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
//	mChannel.Send(out);
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
////	assert(mChannel.GetStatusCode() == 0);
//	
//	mHandler = nil;
//	mChannel.HandleChannelEvent(SFTP_FILE_CLOSED);
//}
//
///*
//	Version 4 protocol
//*/
//
//MSftpChannelImp4::MSftpChannelImp4(MSftpChannel& inChannel, uint32 inMaxPacketSize)
//	: MSftpChannelImp3(inChannel, inMaxPacketSize)
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
//void MSftpChannel::SetCWD(const string& inDir)
//{
//	MSshPacket out;
//	out << uint8(SSH_FXP_REALPATH) << ++mRequestId;
//	if (inDir.empty())
//		out << ".";
//	else
//		out << inDir;
//	Send(out);
//	mHandler = &MSftpChannel::ProcessRealPath;
//}
//
//string MSftpChannel::GetCWD() const
//{
//	return mCurrentDir;
//}
//
//void MSftpChannel::OpenDir()
//{
////	mImpl->OpenDir();
//}
//
//bool MSftpChannel::NextFile(uint32& ioCookie, string& outName,
//							uint64& outSize, uint32& outDate, char& outType)
//{
////	MSftpChannelImp::DirList& dirList = mImpl->mDirList;
////
////	if (ioCookie < 0 or ioCookie >= dirList.size())
//		return false;
//	
////	outName = dirList[ioCookie].name;
////	outSize = dirList[ioCookie].size;
////	outDate = dirList[ioCookie].date;
////	outType = dirList[ioCookie].type;
////	++ioCookie;
////	
////	return true;
//}
//
//void MSftpChannel::MkDir(const string& inPath)
//{
////	mImpl->MkDir(inPath);
//}
//
//void MSftpChannel::ProcessRealPath(
//	uint8		inMessage,
//	MSshPacket&	in)
//{
//	Match(inMessage, SSH_FXP_NAME);
//	
//	uint32 id, count;
//	string dir;
//	
//	in >> id >> count >> dir;
//	if (id == mRequestId - 1 and count == 1)
//	{
//		mCurrentDir = dir;
//		mDirList.clear();
//	
//		HandleChannelEvent(SFTP_SETCWD_OK);
//	}
//
//	mHandler = nil;
//}
//
//void MSftpChannel::ProcessOpenDir(
//	uint8		inMessage,
//	MSshPacket&	in)
//{
//}
//
//void MSftpChannel::ProcessReadDir(
//	uint8		inMessage,
//	MSshPacket&	in)
//{
//}
//
//void MSftpChannel::ProcessMkDir(
//	uint8		inMessage,
//	MSshPacket&	in)
//{
//}
