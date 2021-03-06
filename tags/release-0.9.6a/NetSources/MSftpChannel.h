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
							MSftpChannel(
								std::string		inIPAddress,
								std::string		inUserName,
								uint16 inPort);

							MSftpChannel(
								const MUrl&		inURL);

	virtual					~MSftpChannel();

	virtual void			Send(
								std::string		inData);

	virtual const char*		GetRequest() const { return "subsystem"; }
	virtual const char*		GetCommand() const { return "sftp"; }
	
	void					SetCWD(
								std::string inDir);

	std::string				GetCWD() const;

	void					OpenDir();

	bool					NextFile(
								uint32&			ioCookie,
								std::string&	outName,
								uint64&			outSize,
								uint32&			outDate,
								char&			outType);

	void					MkDir(
								std::string		inPath);

	void					ReadFile(
								std::string		inPath,
								bool			inTextMode);

	void					WriteFile(
								std::string		inPath,
								bool			inTextMode);

	void					SendData(
								const std::string&
												inData);

	void					CloseFile();

	uint64					GetFileSize() const;

	std::string				GetData() const;
	
	uint32					GetStatusCode() const	{ return fStatusCode; }

  protected:
	
	friend struct MSftpChannelImp;

	virtual void			HandleData(
								std::string		inData);

	virtual void			HandleExtraData(
								int				inType,
								std::string		inData);

	void					HandleStatus(
								MSshPacket		in);
	
//	virtual void			Send(std::string inData)	{ MSshChannel::Send(inData); }

	virtual void			HandleChannelEvent(
								int			 inEvent);

	struct MSftpChannelImp*	fImpl;
	std::string				fPacket;
	std::string				fLeftOver;
	uint32					fPacketLength;
	uint32					fStatusCode;
};

#endif // MSFTPCHANNEL_H
