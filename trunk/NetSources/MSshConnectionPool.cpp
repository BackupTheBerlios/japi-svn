/*	$Id: MSshConnectionPool.cpp,v 1.4 2003/10/12 19:31:04 maarten Exp $
	Copyright Maarten
	Created Sunday September 28 2003 11:37:32
*/

#include "MJapieG.h"

#include "MError.h"
#include "MSshConnection.h"
#include "MSshConnectionPool.h"

using namespace std;

MSshConnectionPool& MSshConnectionPool::Instance()
{
	static MSshConnectionPool sInstance;
	return sInstance;
}

MSshConnection* MSshConnectionPool::Get(
	string		inIPAddress,
	string		inUserName,
	uint16 inPortNr)
{
	MSshConnection* result = nil;
	
	for (MConnectionArray::iterator i = fConnections.begin();
		i != fConnections.end(); ++i)
	{
		if (i->IPAddress() == inIPAddress and
			i->UserName() == inUserName and
			i->PortNumber() == inPortNr)
		{
			if (i->IsConnected() or i->Busy())
				result = *i;
			else
				fConnections.erase(i);

			break;
		}
	}
	
	if (result == nil)
	{
		result = new MSshConnection();
		if (result->Connect(inIPAddress, inUserName, inPortNr))
			fConnections.push_back(result);
		else
			THROW(("Failed to connect to %s", inIPAddress.c_str()));
	}
	
	return result;
}

MSshConnection* MSshConnectionPool::Get(
	const MSshChannel*	inChannel)
{
	MSshConnection* result = nil;
	
	for (MConnectionArray::iterator i = fConnections.begin();
		i != fConnections.end(); ++i)
	{
		if (i->IsConnectionForChannel(inChannel))
		{
			if (i->IsConnected() or i->Busy())
				result = *i;
			else
				i = fConnections.erase(i);

			break;
		}
	}
	
	return result;
}

void MSshConnectionPool::Remove(
	MSshConnection*	inConnection)
{
	fConnections.erase(
		remove(fConnections.begin(), fConnections.end(), inConnection),
		fConnections.end());
}

MSshConnectionPool::MSshConnectionPool()
{
}

MSshConnectionPool::~MSshConnectionPool()
{
}
