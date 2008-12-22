//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

/*	$Id: MKnownHosts.h,v 1.1 2003/11/06 13:43:37 maarten Exp $
	Copyright maarten
	Created Thursday November 06 2003 11:50:32
*/

#ifndef MKNOWNHOSTS_H
#define MKNOWNHOSTS_H

#include <string>
#include <map>

class MKnownHosts
{
  public:
	static MKnownHosts&	Instance();
	
	void			CheckHost(
						const std::string& inHost,
						const std::string& inHostKey);

  private:
					MKnownHosts();
	virtual			~MKnownHosts();

	typedef std::map<std::string,std::string>	MHostMap;
	MHostMap		fKnownHosts;
	bool			fUpdated;
};

#endif // MKNOWNHOSTS_H
