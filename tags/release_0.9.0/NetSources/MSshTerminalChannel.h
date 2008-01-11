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

/*	$Id: MSshTerminalChannel.h,v 1.4 2004/01/12 20:46:41 maarten Exp $
	Copyright maarten
	Created Friday January 02 2004 19:54:09
*/

#ifndef MSSHTERMINALCHANNEL_H
#define MSSHTERMINALCHANNEL_H

#include "MSshChannel.h"

class MSshTerminalChannel : public MSshChannel
{
  public:
							MSshTerminalChannel(
								std::string		inIPAddress,
								std::string		inUserName,
								uint16			inPort = 22);

	virtual					~MSshTerminalChannel();
	
	void					SendWindowResize(
								int				inColumns,
								int				inRows);

	virtual const char*		GetRequest() const { return "shell"; }

	virtual const char*		GetCommand() const { return ""; }

	virtual bool			WantPTY() const { return true; }
	
	MEventOut<void(std::string)>
							eData;

  protected:
	
	friend struct MSshTerminalChannelImp;

	virtual void			MandleData(
								std::string		inData);

	virtual void			MandleExtraData(
								int				inType,
								std::string		inData);

	void					ChannelEvent(
								int				inEvent);

	MEventIn<void(int)>		eChannelEventIn;

	std::string				fData;
};

#endif // MSSHTERMINALCHANNEL_H
