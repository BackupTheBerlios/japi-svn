//  Copyright Maarten L. Hekkelman, Radboud University 2008.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "MJapi.h"

#include <iostream>

#include <boost/algorithm/string.hpp>
#include <boost/filesystem/operations.hpp>

#include "MePubServer.h"
#include "MePubDocument.h"

using namespace std;
using namespace zeep;
namespace ba = boost::algorithm;
namespace fs = boost::filesystem;

MePubServer& MePubServer::Instance()
{
	static MePubServer sInstance;
	return sInstance;
}

MePubServer::MePubServer()
	: http::server("0.0.0.0", 9090)
	, mServerThread(boost::bind(&MePubServer::run, this, "0.0.0.0", 9090, 1))
{
}

MePubServer::~MePubServer()
{
	stop();
	mServerThread.join();
}
	
void MePubServer::handle_request(
	const http::request&		req,
	http::reply&				rep)
{
	rep = http::reply::stock_reply(http::not_found);
	
	if (req.method == "GET")
	{
		// start by sanitizing the request's URI
		string uri = req.uri;

		// strip off the http part including hostname and such
		if (ba::starts_with(uri, "http://"))
		{
			string::size_type s = uri.find_first_of('/', 7);
			if (s != string::npos)
				uri.erase(0, s);
		}
		
		// now make the path relative to the root
		while (uri.length() > 0 and uri[0] == '/')
			uri.erase(uri.begin());

		if (uri.empty())	// root requested, return an index
		{
			rep = http::reply::stock_reply(http::ok);
			xml::node_ptr index_html(new xml::node("html"));
			xml::node_ptr index_body(new xml::node("body"));
			index_html->add_child(index_body);
			xml::node_ptr index_h1(new xml::node("h1"));
			index_h1->content("List of open ePub documents");
			index_body->add_child(index_h1);
			
			MePubDocument* doc = MePubDocument::GetFirstEPubDocument();
			while (doc != nil)
			{
				xml::node_ptr index_a(new xml::node("a"));
				index_a->add_attribute("href", doc->GetDocumentID());
				index_a->content(doc->GetFile().GetPath().leaf());
				index_body->add_child(index_a);
				
				doc = doc->GetNextEPubDocument();
			}
			
			rep.set_content(index_html);
			if (rep.headers[1].name == "Content-Type")
				rep.headers[1].value = "text/html; charset=utf-8";
			
			log() << "index of epub documents";
		}
		else
		{
			fs::path path(uri);
			fs::path::iterator p = path.begin();
			
			if (p == path.end())
				throw http::bad_request;
			
			string docID = *p++;
			
			MePubDocument* doc = MePubDocument::GetFirstEPubDocument();
			while (doc != nil)
			{
				if (doc->GetDocumentID() == docID)
					break;
				
				doc = doc->GetNextEPubDocument();
			}
			
			if (doc != nil)
			{
				// listing of first level of ePub document
				
				MProjectItem* root = doc->GetFiles();

				rep = http::reply::stock_reply(http::ok);
				xml::node_ptr index_html(new xml::node("html"));
				xml::node_ptr index_body(new xml::node("body"));
				index_html->add_child(index_body);
				xml::node_ptr index_h1(new xml::node("h1"));
				index_h1->content("List of files in epub document");
				index_body->add_child(index_h1);
				
//				while (doc != nil)
//				{
//					xml::node_ptr index_a(new xml::node("a"));
//					index_a->add_attribute("href", doc->GetDocumentID());
//					index_a->content(doc->GetFile().GetPath().leaf());
//					index_body->add_child(index_a);
//					
//					doc = doc->GetNextEPubDocument();
//				}
				
				rep.set_content(index_html);
				if (rep.headers[1].name == "Content-Type")
					rep.headers[1].value = "text/html; charset=utf-8";
				
				log() << "index of epub documents";

			}
		}
	}
	else
		rep = http::reply::stock_reply(http::bad_request);
}
