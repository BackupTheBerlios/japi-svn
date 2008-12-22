//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "MJapi.h"

#include <sstream>
#include <vector>
#include <sys/stat.h>

#include "MUrl.h"
#include "MUtils.h"
#include "MError.h"

using namespace std;

// --------------------------------------------------------------------

namespace {

unsigned char kURLAcceptable[96] =
{/* 0 1 2 3 4 5 6 7 8 9 A B C D E F */
    0,0,0,0,0,0,0,0,0,0,7,6,0,7,7,4,		/* 2x   !"#$%&'()*+,-./	 */
    7,7,7,7,7,7,7,7,7,7,0,0,0,0,0,0,		/* 3x  0123456789:;<=>?	 */
    7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,		/* 4x  @ABCDEFGHIJKLMNO  */
    7,7,7,7,7,7,7,7,7,7,7,0,0,0,0,7,		/* 5X  PQRSTUVWXYZ[\]^_	 */
    0,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,		/* 6x  `abcdefghijklmno	 */
    7,7,7,7,7,7,7,7,7,7,7,0,0,0,0,0			/* 7X  pqrstuvwxyz{\}~	DEL */
};

inline char ConvertHex(
	char		inChar)
{
	int	value = inChar - '0';

	if (inChar >= 'A' and inChar <= 'F') {
		value = inChar - 'A' + 10;
	} else if (inChar >= 'a' and inChar <= 'f') {
		value = inChar - 'a' + 10;
	}

	return char(value);
}
	
}

struct MUrlImp
{
	string			mScheme;
	string			mUser;
	string			mPassword;
	string			mHost;
	int16			mPort;
	fs::path		mPath;
	
	static void		Encode(
						std::string&		ioURL);

	void			Decode(
						std::string&		ioURL);

	void			Parse(
						const char*		inURL);
};

void MUrlImp::Parse(
	const char*			inUrl)
{
	if (inUrl == nil)
		THROW(("Invalid nil pointer passed for url"));
	
	// see RFC 1738
	
	string url(inUrl);

	Decode(url);

	string::size_type p;
	
	if ((p = url.find("://")) == string::npos)
		THROW(("Invalid url '%s'", inUrl));
	
	string scheme = url.substr(0, p);
	if (scheme != "file" and scheme != "sftp" and scheme != "ssh")
		THROW(("Unsupported scheme '%s'", scheme.c_str()));
	
	if (scheme == "file")
	{
		mScheme = "file";
		mUser.clear();
		mPassword.clear();
		mHost.clear();
		mPort = 0;
		mPath = url.substr(7, string::npos);
	}
	else
	{
		url.erase(0, scheme.length() + 3);
		
		p = url.find('/');
		if (p == string::npos)
			THROW(("Invalid url '%s'", inUrl));
		
		string path = url.substr(p, string::npos);
		NormalizePath(path);
		
		url = url.substr(0, p);
		
		string user, pass, host;
		int16 port = 0;
		
		if ((p = url.find('@')) != string::npos)
		{
			host = url.substr(p + 1, string::npos);
			user = url.substr(0, p);
			
			if ((p = user.find(':')) != string::npos)
			{
				pass = user.substr(p + 1, string::npos);
				user.erase(p, string::npos);
			}
		}
		else
			host = url;
		
		if ((p = host.find(':')) != string::npos)
		{
			port = atoi(host.c_str() + p + 1);
			host.erase(p, string::npos);
		}
		
		mScheme = scheme;
		mUser = user;
		mPassword = pass;
		mHost = host;
		mPort = port;
		mPath = path;
	}
}

void MUrlImp::Decode(
	string&		ioURL)
{
	vector<char> buf(ioURL.length() + 1);
	char* r = &buf[0];
	
	for (string::iterator p = ioURL.begin(); p != ioURL.end(); ++p)
	{
		char q = *p;

		if (q == '%' and ++p != ioURL.end())
		{
			q = (char) (ConvertHex(*p) * 16);

			if (++p != ioURL.end())
				q = (char) (q + ConvertHex(*p));
		}

		*r++ = q;
	}
	
	ioURL.assign(&buf[0], r);
}

void MUrlImp::Encode(
	string&		ioPath)
{
	string path;
	
	swap(path, ioPath);
	
	for (unsigned int i = 0; i < path.length(); ++i)
	{
		unsigned char a = (unsigned char)path[i];
		if (not (a >= 32 and a < 128 and (kURLAcceptable[a - 32] & 4)))
		{
			ioPath += '%';
			ioPath += kHexChars[a >> 4];
			ioPath += kHexChars[a & 15];
		}
		else
			ioPath += path[i];
	}
}

// --------------------------------------------------------------------

MUrl::MUrl()
	: mImpl(new MUrlImp)
{
}

MUrl::MUrl(
	const MUrl&			inUrl)
	: mImpl(new MUrlImp(*inUrl.mImpl))
{
}

MUrl::MUrl(
	const string&	inUrl)
	: mImpl(new MUrlImp)
{
	mImpl->Parse(inUrl.c_str());
}

MUrl::MUrl(
	const char*			inUrl)
	: mImpl(new MUrlImp)
{
	mImpl->Parse(inUrl);
}

MUrl::MUrl(
	const fs::path&		inFile)
	: mImpl(new MUrlImp)
{
	mImpl->mScheme = "file";
	mImpl->mPath = inFile;
	NormalizePath(mImpl->mPath);
}

MUrl& MUrl::operator=(
	const MUrl&			inRHS)
{
	*mImpl = *inRHS.mImpl;
	return *this;
}

MUrl& MUrl::operator=(
	const string&		inRHS)
{
	mImpl->Parse(inRHS.c_str());
	return *this;
}

bool MUrl::operator==(
	const MUrl&			inUrl) const
{
	bool result = false;
	
	if (GetScheme() == inUrl.GetScheme())
	{
		if (GetScheme() == "file")
		{
			string p1 = GetPath().string();
			string p2 = inUrl.GetPath().string();
			
			if (p1 == p2)
				result = true;
			else	// symlinks and stuff
			{
				struct stat st1, st2;
				
				if (stat(p1.c_str(), &st1) >= 0 and
					stat(p2.c_str(), &st2) >= 0)
				{
					result = st1.st_dev == st2.st_dev and st1.st_ino == st2.st_ino;
				}
			}
		}
		else
		{
			MUrlImp& a = *mImpl;
			MUrlImp& b = *inUrl.mImpl;
		
			result =
				a.mUser == b.mUser and
				a.mHost == b.mHost and
				a.mPort == b.mPort and
				a.mPath == b.mPath;
		}
	}

	return result;	
}

MUrl::~MUrl()
{
	delete mImpl;
}

bool MUrl::IsLocal() const
{
	return mImpl->mScheme == "file";
}

bool MUrl::IsValid() const
{
	return
		(mImpl->mScheme == "file" or mImpl->mScheme == "sftp" or mImpl->mScheme == "ssh") and
		mImpl->mPath.is_complete();
}

string MUrl::str(
	bool			inEncoded) const
{
	stringstream s;
	s << mImpl->mScheme << "://";
	
	if (mImpl->mScheme == "sftp" or mImpl->mScheme == "ssh")
	{
		if (mImpl->mUser.length() > 0)
		{
			s << mImpl->mUser;
			if (mImpl->mPassword.length() > 0)
				s << ':' << mImpl->mPassword;
			s << '@';
		}
		
		s << mImpl->mHost;
		
		if (mImpl->mPort != 0)
			s << ':' << mImpl->mPort;
		
		s << '/';
	}
	
	string path = mImpl->mPath.string();
	if (inEncoded)
		MUrlImp::Encode(path);
	
	s << path;
	
	return s.str();
}

fs::path MUrl::GetPath() const
{
	return mImpl->mPath;
}

void MUrl::SetPath(
	const fs::path&		inPath)
{
	mImpl->mPath = inPath;
	NormalizePath(mImpl->mPath);
}

std::string MUrl::GetFileName() const
{
	return mImpl->mPath.leaf();
}

void MUrl::SetFileName(
	const string&		inFileName)
{
	mImpl->mPath = mImpl->mPath.branch_path() / inFileName;
	NormalizePath(mImpl->mPath);
}

string MUrl::GetScheme() const
{
	return mImpl->mScheme;
}

void MUrl::SetScheme(
	const string&	inScheme)
{
	mImpl->mScheme = inScheme;
	
	if (inScheme == "file")
	{
		mImpl->mUser.clear();
		mImpl->mPassword.clear();
		mImpl->mHost.clear();
		mImpl->mPort = 0;
	}
}

string MUrl::GetHost() const
{
	assert(not IsLocal());
	return mImpl->mHost;
}

void MUrl::SetHost(
	const string&	inHost)
{
	assert(not IsLocal());
	mImpl->mHost = inHost;
}

string MUrl::GetUser() const
{
	assert(not IsLocal());
	return mImpl->mUser;
}

void MUrl::SetUser(
	const string&	inUser)
{
	assert(not IsLocal());
	mImpl->mUser = inUser;
}

string MUrl::GetPassword() const
{
	assert(not IsLocal());
	return mImpl->mPassword;
}

void MUrl::SetPassword(
	const string&	inPassword)
{
	assert(not IsLocal());
	mImpl->mPassword = inPassword;
}

int16 MUrl::GetPort() const
{
	assert(not IsLocal());
	return mImpl->mPort;
}

void MUrl::SetPort(
	int16				inPort)
{
	assert(not IsLocal());
	mImpl->mPort = inPort;
}

void MUrl::operator/=(
	const string&	inPartialPath)
{
	mImpl->mPath /= inPartialPath;
	NormalizePath(mImpl->mPath);
}

MUrl operator/(const MUrl& lhs, std::string rhs)
{
	MUrl result = lhs;
	result /= rhs;
	return result;
}

