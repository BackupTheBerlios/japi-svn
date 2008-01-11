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

/*	$Id: MKnownHosts.cpp,v 1.4 2004/02/07 13:10:08 maarten Exp $
	Copyright maarten
	Created Thursday November 06 2003 11:50:26
*/

#include "MJapieG.h"

#include <fcntl.h>
#include <cctype>
#include <boost/filesystem/fstream.hpp>

#include "MKnownHosts.h"
#include "MGlobals.h"
#include "MUtils.h"
#include "MError.h"
#include "MAlerts.h"

#include <cryptopp/base64.h>
#define CRYPTOPP_ENABLE_NAMESPACE_WEAK 1
#include <cryptopp/md5.h>

using namespace std;

MKnownHosts& MKnownHosts::Instance()
{
	static MKnownHosts sInstance;
	return sInstance;
}

MKnownHosts::MKnownHosts()
	: fUpdated(false)
{
	try
	{
		fs::ifstream file(gPrefsDir / "known_hosts");
		
		while (file.is_open() and not file.eof())
		{
			string line;
			getline(file, line);
			
			if (line.length() < 1 or line[0] == '#')
				continue;
			
			string host, key;
			
			string::size_type p = line.find(' ');
			if (p == string::npos)
				continue;
			
			host = line.substr(0, p);
			line.erase(0, p + 1);
			
			p = line.find(' ');
			if (p == string::npos or line.substr(0, p) != "ssh-dss")
				continue;
			
			key = line.substr(p + 1);
			
			while ((p = host.find(',')) != string::npos)
			{
				fKnownHosts[host.substr(0, p)] = key;
				host.erase(0, p + 1);
			}
			
			fKnownHosts[host] = key;
		}
	}
	catch (...) {}
}

MKnownHosts::~MKnownHosts()
{
	if (fUpdated)
	{
		try
		{
			fs::ofstream file(gPrefsDir / "known_hosts");
			
			for (MHostMap::iterator k = fKnownHosts.begin();
				k != fKnownHosts.end(); ++k)
			{
				file << (*k).first << " ssh-dss " << (*k).second << endl;
			}
		}
		catch (...) {}
	}
}

void MKnownHosts::CheckHost(
	const string&	inHost,
	const string&	inHostKey)
{
	string value;
	CryptoPP::Base64Encoder e(new CryptoPP::StringSink(value), false);
	e.Put(reinterpret_cast<const byte*>(inHostKey.c_str()), inHostKey.length());
	e.MessageEnd();

	string fingerprint;
	
	CryptoPP::Weak::MD5 hash;
	uint32 dLen = hash.DigestSize();
	vector<char>	H(dLen);
	
	hash.Update(reinterpret_cast<const byte*>(inHostKey.c_str()),
		inHostKey.length());
	hash.Final(reinterpret_cast<byte*>(&H[0]));
	
	for (uint32 i = 0; i < dLen; ++i)
	{
		if (fingerprint.length())
			fingerprint += ':';
		
		char b = H[i];
		
		fingerprint += kHexChars[(b >> 4) & 0x0f];
		fingerprint += kHexChars[b & 0x0f];
	}
	
	MHostMap::iterator i = fKnownHosts.find(inHost);
	if (i == fKnownHosts.end())
	{
		switch (DisplayAlert("unknown-host-alert", inHost, fingerprint))
		{
				// Add
			case 1:
				fKnownHosts[inHost] = value;
				fUpdated = true;
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
	else if (value != (*i).second)
	{
		if (DisplayAlert("host-key-changed-alert", inHost, fingerprint) != 2)
			THROW(("User cancelled"));
		
		fKnownHosts[inHost] = value;
		fUpdated = true;
	}
}

