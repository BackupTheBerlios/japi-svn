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
#include "MSshPacket.h"

class MSftpChannel : public MSshChannel
{
  public:

	void			ReadFile(
						const std::string&	inPath);

	void			WriteFile(
						const std::string&	inPath);

	virtual void	ReceiveData(
						const std::string&	inData,
						int64				inOffset,
						int64				inFileSize)
					{
					}

	virtual void	SendData(
						int64				inOffset,
						uint32				inMaxBlockSize,
						std::string&		outData)
					{
					}

	virtual void	FileClosed()
					{
					}

  protected:

	  using MSshChannel::SendData;

					MSftpChannel(
						const std::string&	inHost,
						const std::string&	inUser,
						uint16				inPort);

					MSftpChannel(
						MSshConnection&		inConnection);

	virtual void	SFTPInitialised();

	virtual void	Opened();
					
	virtual void	GetRequestAndCommand(
						std::string&		outRequest,
						std::string&		outCommand) const
					{
						outRequest = "subsystem";
						outCommand = "sftp";
					}
	
	virtual void	Send(
						MSshPacket&			inData);

	virtual void	ReceiveData(
						MSshPacket&			inData);

	void			ProcessPacket(
						uint8				msg,
						MSshPacket&			in);

	void			Match(
						uint8				inExpected,
						uint8				inReceived);

	void (MSftpChannel::*mHandler)(
						uint8				inMessage,
						MSshPacket&			in);

	void			ProcessOpenFile(
						uint8				inMessage,
						MSshPacket&			in);

	void			ProcessFStat(
						uint8				inMessage,
						MSshPacket&			in);

	void			ProcessRead(
						uint8				inMessage,
						MSshPacket&			in);

	void			ProcessCreateFile(
						uint8				inMessage,
						MSshPacket&			in);

	void			ProcessWrite(
						uint8				inMessage,
						MSshPacket&			in);

	void			ProcessClose(
						uint8				inMessage,
						MSshPacket&			in);

	std::deque<uint8>
					mPacket;
	uint32			mPacketLength;
	uint32			mRequestId;
	std::string		mHandle;
	int64			mFileSize;
	int64			mOffset;
	std::string		mData;
};

//	void					SetCWD(
//								const std::string& inDir);
//
//	std::string				GetCWD() const;
//
//	void					OpenDir();
//
//	bool					NextFile(
//								uint32&			ioCookie,
//								std::string&	outName,
//								uint64&			outSize,
//								uint32&			outDate,
//								char&			outType);
//
//	void					MkDir(
//								const std::string&
//												inPath);
//
//	void					ProcessRealPath(
//								uint8		inMessage,
//								MSshPacket&	in);
//
//	void					ProcessOpenDir(
//								uint8		inMessage,
//								MSshPacket&	in);
//
//	void					ProcessReadDir(
//								uint8		inMessage,
//								MSshPacket&	in);
//
//	void					ProcessMkDir(
//								uint8		inMessage,
//								MSshPacket&	in);
//
//	struct DirEntry
//	{
//		std::string			name;
//		uint64				size;
//		uint32				date;
//		char				type;
//	};
//
//	typedef std::vector<DirEntry>	DirList;
//
//	DirList					mDirList;
//	std::string				mCurrentDir;


#endif // MSFTPCHANNEL_H
