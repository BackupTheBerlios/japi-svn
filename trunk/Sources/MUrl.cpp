#include "MJapieG.h"

#include <sstream>
#include <sys/stat.h>

#include "MUrl.h"

using namespace std;

// --------------------------------------------------------------------

struct MUrlImp
{
	string			mScheme;
	string			mUser;
	string			mPassword;
	string			mHost;
	int16			mPort;
	fs::path		mPath;
	
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
	string::size_type p;
	
	if ((p = url.find("://")) == string::npos)
		THROW(("Invalid url '%s'", inUrl));
	
	string scheme = url.substr(0, p);
	if (scheme != "file" and scheme != "sftp")
		THROW(("Unsupported scheme '%s'", scheme.c_str()));
	
	if (scheme == "file")
	{
		mScheme = "file";
		mUser.clear();
		mPassword.clear();
		mHost.clear();
		mPort = 0;
		mPath = inUrl + 7;
	}
	else
	{
		url.erase(0, 7);
		
		p = url.find('/');
		if (p == string::npos)
			THROW(("Invalid url '%s'", inUrl));
		
		string path = url.substr(p + 1, string::npos);
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
		
		mScheme = "sftp";
		mUser = user;
		mPassword = pass;
		mHost = host;
		mPort = port;
		mPath = path;
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
}

MUrl& MUrl::operator=(
	const MUrl&			inRHS)
{
	*mImpl = *inRHS.mImpl;
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

bool MUrl::IsLocal() const
{
	return mImpl->mScheme == "file";
}

string MUrl::str() const
{
	stringstream s;
	s << mImpl->mScheme << "://";
	
	if (mImpl->mScheme == "sftp")
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
	
	s << mImpl->mPath.string();
	
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
}

std::string MUrl::GetFileName() const
{
	return mImpl->mPath.leaf();
}

void MUrl::SetFileName(
	const string&		inFileName)
{
	mImpl->mPath = mImpl->mPath.branch_path() / inFileName;
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
}

MUrl operator/(const MUrl& lhs, std::string rhs)
{
	MUrl result = lhs;
	result /= rhs;
	return result;
}
