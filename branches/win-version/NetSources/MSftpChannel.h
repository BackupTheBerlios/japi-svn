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

	static MSftpChannel*	Open(
								const std::string&	inIPAddress,
								const std::string&	inUserName,
								uint16				inPort);

	static MSftpChannel*	Open(
								const MFile&		inURL)
							{
								return Open(inURL.GetHost(), inURL.GetUser(), inURL.GetPort());
							}

	void					ReadFile(
								const std::string&	inPath);

	void					WriteFile(
								const std::string&	inPath);

	// An SFTPChannelOpened event is sent when SFTP is set up.
	MEventOut<void()>								eSFTPChannelOpened;
	// An SFTPChannelClosed event is sent when the underlying channed
	// is about to be closed, don't use it after this event.
	MEventOut<void()>								eSFTPChannelClosed;

	// For reading, this object sends out a ReceiveData event
	// for each block read. First parameter is the data
	// in the block, second is the offset of this block
	// and the third the total size of the file.
	// The file is closed automatically after reading all data.
	MEventOut<void(const std::string&, int64, int64)>
													eReceiveData;

	// For writing a file, this object sends a SendData event
	// to collect the data. First parameter is the offset of
	// the next data block, second parameter indicates the
	// maximum amount of data to transfer and the third is the
	// data itself.
	// The file is closed automatically if the data passed back
	// is empty.
	MEventOut<void(int64, uint32, std::string&)>	eSendData;
	
  protected:

	virtual void			GetRequestAndCommand(
								std::string&		outRequest,
								std::string&		outCommand) const
							{
								outRequest = "subsystem";
								outCommand = "sftp";
							}
	
//	uint32					GetStatusCode() const	{ return mStatusCode; }

							MSftpChannel(
								MSshConnection&		inConnection);
							
							~MSftpChannel();
	
	virtual void			HandleChannelEvent(
								uint32				inEvent);

	virtual void			Send(
								MSshPacket&			inData);

	virtual void			Receive(
								MSshPacket&			inData,
								int					inType);

	void					ProcessPacket(
								uint8				msg,
								MSshPacket&			in);

	void					Match(
								uint8			inExpected,
								uint8			inReceived);

	void (MSftpChannel::*mHandler)(
								uint8			inMessage,
								MSshPacket&		in);

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

	std::deque<uint8>		mPacket;
	uint32					mPacketLength;
	uint32					mRequestId;
	std::string				mHandle;
	int64					mFileSize;
	int64					mOffset;
	std::string				mData;
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
