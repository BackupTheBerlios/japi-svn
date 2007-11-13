/*	$Id: MSshConnectionPool.h,v 1.1 2003/09/28 10:10:50 maarten Exp $
	Copyright Maarten
	Created Sunday September 28 2003 11:36:45
*/

#ifndef MSSHCONNECTIONPOOL_H
#define MSSHCONNECTIONPOOL_H

#include <string>
#include <boost/ptr_container/ptr_vector.hpp>

class MSshConnection;
class MSshChannel;

class MSshConnectionPool
{
  public:
	static MSshConnectionPool&
						Instance();
	
	MSshConnection*		Get(
							std::string			inIPAddress,
							std::string			inUserName,
							uint16				inPortNr);

	MSshConnection*		Get(
							const MSshChannel*	inChannel);

	void				Remove(
							MSshConnection*		inConnection);
	
  private:
						MSshConnectionPool();
	virtual				~MSshConnectionPool();

	typedef boost::ptr_vector<MSshConnection> MConnectionArray;

	MConnectionArray	fConnections;
};

#endif // MSSHCONNECTIONPOOL_H
