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
