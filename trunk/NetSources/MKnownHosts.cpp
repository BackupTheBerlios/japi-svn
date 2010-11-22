//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

/*	$Id: MKnownHosts.cpp,v 1.4 2004/02/07 13:10:08 maarten Exp $
	Copyright maarten
	Created Thursday November 06 2003 11:50:26
*/

#include "MJapi.h"

#include <fcntl.h>
#include <cctype>
#include <boost/filesystem/fstream.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>
#define foreach BOOST_FOREACH

#include "MKnownHosts.h"
#include "MGlobals.h"
#include "MUtils.h"
#include "MError.h"
#include "MAlerts.h"
#include "MSshPacket.h"

#include <cryptopp/base64.h>
#define CRYPTOPP_ENABLE_NAMESPACE_WEAK 1
#include <cryptopp/md5.h>

using namespace std;
namespace ba = boost::algorithm;

MKnownHosts& MKnownHosts::Instance()
{
	static MKnownHosts sInstance;
	return sInstance;
}

MKnownHosts::MKnownHosts()
	: mUpdated(false)
{
	try
	{
		fs::ifstream file(gPrefsDir / "known_hosts");
		
		while (file.is_open() and not file.eof())
		{
			string line;
			getline(file, line);
			
			vector<string> fields;
			ba::split(fields, line, ba::is_space());
			
			if (fields.size() < 3)
				continue;
			
			MKnownHost host = {
				fields[0],
				fields[1],
				fields[2]
			};
			
			mKnownHosts.push_back(host);
		}
	}
	catch (...) {}
}

MKnownHosts::~MKnownHosts()
{
	if (mUpdated)
	{
		try
		{
			fs::ofstream file(gPrefsDir / "known_hosts");
			
			foreach (auto known, mKnownHosts)
				file << known.host << ' ' << known.alg << ' ' << known.key << endl;
		}
		catch (...) {}
	}
}

void MKnownHosts::CheckHost(
	const string&		inHost,
	const string&		inAlgorithm,
	const MSshPacket&	inHostKey)
{
	string value;
	CryptoPP::Base64Encoder e(new CryptoPP::StringSink(value), false);
	e.Put(inHostKey.peek(), inHostKey.size());
	e.MessageEnd();

	string fingerprint;
	
#if CRYPTOPP_VERSION >= 552
	CryptoPP::Weak::MD5 hash;
#else
	CryptoPP::MD5 hash;
#endif
	uint32 dLen = hash.DigestSize();
	vector<char>	H(dLen);
	
	hash.Update(inHostKey.peek(), inHostKey.size());
	hash.Final(reinterpret_cast<byte*>(&H[0]));
	
	for (uint32 i = 0; i < dLen; ++i)
	{
		if (not fingerprint.empty())
			fingerprint += ':';
		
		char b = H[i];
		
		fingerprint += kHexChars[(b >> 4) & 0x0f];
		fingerprint += kHexChars[b & 0x0f];
	}
	
	MKnownHost host = { inHost, inAlgorithm, value };
	MKnownHostsList::iterator i = find(mKnownHosts.begin(), mKnownHosts.end(), host);
	if (i == mKnownHosts.end())
	{
		switch (DisplayAlert("unknown-host-alert", inHost, fingerprint))
		{
				// Add
			case 1:
				mKnownHosts.push_back(host);
				mUpdated = true;
				break;
			
				// Cancel
			case 2:
				THROW((0, 0));
				break;
				
				// Once
			case 3:
				break;
		}
	}
	else if (value != i->key)
	{
		if (DisplayAlert("host-key-changed-alert", inHost, fingerprint) != 2)
			THROW(("User cancelled"));
		
		i->key = value;
		mUpdated = true;
	}
}

