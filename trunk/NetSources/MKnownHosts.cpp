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

#include <cryptopp/base64.h>
#include <cryptopp/md5.h>

using namespace std;

MKnownHosts& MKnownHosts::Instance()
{
	static MKnownHosts sInstance;
	return sInstance;
}

MKnownHosts::MKnownHosts()
	: fUpdated(false)
{//
//	MPath file(gPrefsDir / "known_hosts");
//	
//	try
//	{
//		boost::ifstream f(file);
//		
//		while (not f.eof())
//		{
//			string key, kind, value;
//			
//			f >> 
//			
//		}		
//			// extremely simple parser:
//		
//		char c;
//		int state = 0;
//		
//		while (size-- > 0)
//		{
//			f >> c;
//			
//			switch (state)
//			{
//				case 0:
//					if (isspace(c))
//						++state;
//					else
//						key += c;
//					break;
//				
//				case 1:
//					if (isspace(c))
//						++state;
//					else
//						kind += c;
//					break;
//				
//				case 2:
//					if (isspace(c))
//					{
//						if (kind == "ssh-dss")
//							fKnownHosts[key] = value;
//	
//						state = 0;
//						key.clear();
//						kind.clear();
//						value.clear();
//					}
//					else
//						value += c;
//					break;
//			}
//		}
//	}
//	catch (...) {}
}

MKnownHosts::~MKnownHosts()
{//
//	if (fUpdated)
//	{
//		try
//		{
//			MUrl url = gPrefsURL.GetChild("known_hosts");
//			
//			MTextWriter f(url);
//
//			for (MHostMap::iterator k = fKnownHosts.begin();
//				k != fKnownHosts.end(); ++k)
//			{
//				f << (*k).first << " ssh-dss " << (*k).second << "\n";
//			}
//		}
//		catch (...) {}
//	}
}

void MKnownHosts::CheckHost(
	const string&	inHost,
	const string&	inHostKey)
{//
//	string value;
//	CryptoPP::Base64Encoder e(new CryptoPP::StringSink(value), false);
//	e.Put(reinterpret_cast<const byte*>(inHostKey.c_str()), inHostKey.length());
//	e.MessageEnd();
//
//	string fingerprint;
//	
//	CryptoPP::MD5 hash;
//	uint32 dLen = hash.DigestSize();
//	MAutoBuf<char>	H(new char[dLen]);
//	
//	hash.Update(reinterpret_cast<const byte*>(inHostKey.c_str()),
//		inHostKey.length());
//	hash.Final(reinterpret_cast<byte*>(H.get()));
//	
//	for (uint32 i = 0; i < dLen; ++i)
//	{
//		if (fingerprint.length())
//			fingerprint += ':';
//		
//		char b = *(H.get() + i);
//		
//		fingerprint += kHexChars[(b >> 4) & 0x0f];
//		fingerprint += kHexChars[b & 0x0f];
//	}
//	
//	MHostMap::iterator i = fKnownHosts.find(inHost);
//	if (i == fKnownHosts.end())
//	{
//		switch (MAlerts::Alert(nil, 505, inHost, fingerprint))
//		{
//				// Add
//			case 1:
//				fKnownHosts[inHost] = value;
//				fUpdated = true;
//				break;
//			
//				// Cancel
//			case 2:
//				THROW((0, 0));
//				break;
//				
//				// Once
//			case 3:
//				break;
//		}
//	}
//	else if (value != (*i).second)
//	{
//		if (MAlerts::Alert(nil, 506, inHost, fingerprint) != 2)
//			THROW((0, 0));
//		
//		fKnownHosts[inHost] = value;
//		fUpdated = true;
//	}
}

