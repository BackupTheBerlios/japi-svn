/*
	Copyright (c) 2007, Maarten L. Hekkelman
	All rights reserved.

	Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions
	are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in
      the documentation and/or other materials provided with the
      distribution.
    * Neither the name of the Maarten L. Hekkelman nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
	"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
	LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
	FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
	COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
	INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
	(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
	SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
	HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
	STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
	ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
	OF THE POSSIBILITY OF SUCH DAMAGE.
*/

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
