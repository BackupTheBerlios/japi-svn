//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

/*	$Id: MSftpChannel.h,v 1.12 2004/01/04 20:40:29 maarten Exp $
	Copyright Maarten
	Created Sunday September 28 2003 10:54:43
*/

#ifndef MSFTPCHANNEL_H
#define MSFTPCHANNEL_H

#include "MSshChannel.h"

enum MSftpEvent {
	SFTP_INIT_DONE = 400,
	SFTP_SETCWD_OK,
	SFTP_DIR_LISTING_AVAILABLE,
	SFTP_MKDIR_OK,
	SFTP_ERROR,
	SFTP_FILE_SIZE_KNOWN,
	SFTP_DATA_AVAILABLE,
	SFTP_DATA_DONE,
	SFTP_CAN_SEND_DATA,
	SFTP_FILE_CLOSED,
};

class MSftpChannel : public MSshChannel
{
  public:

	static MSftpChannel*	Open(
								const std::string&	inIPAddress,
								const std::string&	inUserName,
								uint16				inPort);

	static MSftpChannel*	Open(
								const MFile&	inURL)
							{
								return Open(inURL.GetHost(), inURL.GetUser(), inURL.GetPort());
							}

	void					SetCWD(
								const std::string& inDir);

	std::string				GetCWD() const;

	void					OpenDir();

	bool					NextFile(
								uint32&			ioCookie,
								std::string&	outName,
								uint64&			outSize,
								uint32&			outDate,
								char&			outType);

	void					MkDir(
								const std::string&
												inPath);

	void					ReadFile(
								const std::string&
												inPath,
								bool			inTextMode);

	void					WriteFile(
								const std::string&
												inPath,
								bool			inTextMode);

	void					CloseFile();

	uint64					GetFileSize() const;

	void					SendData(
								const std::string&
												inData);

	std::string				GetData() const;
	
  protected:

	virtual void			GetRequestAndCommand(
								std::string&	outRequest,
								std::string&	outCommand) const
							{
								outRequest = "subsystem";
								outCommand = "sftp";
							}
	
//	uint32					GetStatusCode() const	{ return mStatusCode; }

							MSftpChannel(
								MSshConnection&	inConnection);
	
	virtual void			HandleChannelEvent(
								uint32			inEvent);

	virtual void			Send(
								MSshPacket&		inData);

	virtual void			Receive(
								MSshPacket&		inData,
								int				inType);

	void					ProcessPacket(
								uint8			msg,
								MSshPacket&		in);

	void					ProcessStatus(
								MSshPacket&		in);

	void					Match(
								uint8		inExpected,
								uint8		inReceived);

	void (MSftpChannel::*mHandler)(
								uint8		inMessage,
								MSshPacket&	in);

	void					ProcessRealPath(
								uint8		inMessage,
								MSshPacket&	in);

	void					ProcessOpenDir(
								uint8		inMessage,
								MSshPacket&	in);

	void					ProcessReadDir(
								uint8		inMessage,
								MSshPacket&	in);

	void					ProcessMkDir(
								uint8		inMessage,
								MSshPacket&	in);

	void					ProcessOpenFile(
								uint8		inMessage,
								MSshPacket&	in);

	void					ProcessFStat(
								uint8		inMessage,
								MSshPacket&	in);

	void					ProcessRead(
								uint8		inMessage,
								MSshPacket&	in);

	void					ProcessCreateFile(
								uint8		inMessage,
								MSshPacket&	in);

	void					ProcessWrite(
								uint8		inMessage,
								MSshPacket&	in);

	void					ProcessClose(
								uint8		inMessage,
								MSshPacket&	in);

	struct DirEntry
	{
		std::string			name;
		uint64				size;
		uint32				date;
		char				type;
	};

	typedef std::vector<DirEntry>	DirList;

	std::deque<uint8>		mPacket;
	uint32					mPacketLength;
	uint32					mStatusCode;
	uint32					mRequestId;
	uint32					mPacketSize;
	std::string				mHandle;
	int64					mFileSize;
	int64					mOffset;
	DirList					mDirList;
	std::string				mCurrentDir;
	std::string				mData;
};

#endif // MSFTPCHANNEL_H
