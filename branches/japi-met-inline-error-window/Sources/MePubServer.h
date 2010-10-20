//  Copyright Maarten L. Hekkelman, Radboud University 2008.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEPUBSERVER_H
#define MEPUBSERVER_H

#include <zeep/http/server.hpp>

class MePubServer : public zeep::http::server
{
  public:

	static MePubServer&
					Instance();

  private:
					MePubServer();
					~MePubServer();
	
	virtual void	handle_request(
						const zeep::http::request&	req,
						zeep::http::reply&			rep);

	boost::thread	mServerThread;
};

#endif
