//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "MJapi.h"

#include <unistd.h>
#include <cstring>

#include <sys/stat.h>
#include <dirent.h>
#include <stack>
#include <fstream>
#include <cassert>
#include <cerrno>
#include <limits>

#include <pcrecpp.h>

#include <boost/filesystem/fstream.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/device/back_inserter.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

#include "MFile.h"
#include "MError.h"
#include "MUnicode.h"
#include "MUtils.h"
#include "MStrings.h"
#include "MJapiApp.h"
#include "MPreferences.h"
#include "MSftpChannel.h"

using namespace std;
namespace io = boost::iostreams;
namespace ba = boost::algorithm;

namespace {

// reserved characters in URL's

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

void URLEncode(
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

void URLDecode(
	string&		ioPath)
{
	vector<char> buf(ioPath.length() + 1);
	char* r = &buf[0];
	
	for (string::iterator p = ioPath.begin(); p != ioPath.end(); ++p)
	{
		char q = *p;

		if (q == '%' and ++p != ioPath.end())
		{
			q = (char) (ConvertHex(*p) * 16);

			if (++p != ioPath.end())
				q = (char) (q + ConvertHex(*p));
		}

		*r++ = q;
	}
	
	ioPath.assign(&buf[0], r);
}

// ------------------------------------------------------------------
//
//  Three different implementations of extended attributes...
//

// ------------------------------------------------------------------
//  FreeBSD

#if defined(__FreeBSD__) and (__FreeBSD__ > 0)

#include <sys/extattr.h>

ssize_t read_attribute(const fs::path& inPath, const char* inName, void* outData, size_t inDataSize)
{
	string path = inPath.string();
	
	return extattr_get_file(path.c_str(), EXTATTR_NAMESPACE_USER,
		inName, outData, inDataSize);
}

void write_attribute(const fs::path& inPath, const char* inName, const void* inData, size_t inDataSize)
{
	string path = inPath.string();
	
	time_t t = last_write_time(inPath);

	int r = extattr_set_file(path.c_str(), EXTATTR_NAMESPACE_USER,
		inName, inData, inDataSize);
	
	last_write_time(inPath, t);
}

#endif

// ------------------------------------------------------------------
//  Linux

#if defined(__linux__)

#include <attr/attributes.h>

ssize_t read_attribute(const fs::path& inPath, const char* inName, void* outData, size_t inDataSize)
{
	string path = inPath.string();

	int length = inDataSize;
	int err = ::attr_get(path.c_str(), inName,
		reinterpret_cast<char*>(outData), &length, 0);
	
	if (err != 0)
		length = 0;
	
	return length;
}

void write_attribute(const fs::path& inPath, const char* inName, const void* inData, size_t inDataSize)
{
	string path = inPath.string();
	
	(void)::attr_set(path.c_str(), inName,
		reinterpret_cast<const char*>(inData), inDataSize, 0);
}

#endif

// ------------------------------------------------------------------
//  MacOS X

#if defined(__APPLE__)

#include <sys/xattr.h>

ssize_t read_attribute(const fs::path& inPath, const char* inName, void* outData, size_t inDataSize)
{
	string path = inPath.string();

	return ::getxattr(path.c_str(), inName, outData, inDataSize, 0, 0);
}

void write_attribute(const fs::path& inPath, const char* inName, const void* inData, size_t inDataSize)
{
	string path = inPath.string();

	(void)::setxattr(path.c_str(), inName, inData, inDataSize, 0, 0);
}

#endif
	
}

// --------------------------------------------------------------------
// MFileLoader, used to load the contents of a file.
// This works strictly asynchronous. 

MFileLoader::MFileLoader(
	MFile&				inFile)
	: mFile(inFile)
{
}

MFileLoader::~MFileLoader()
{
}

void MFileLoader::Cancel()
{
}

void MFileLoader::SetFileInfo(
	bool				inReadOnly,
	double				inModDate)
{
	mFile.SetFileInfo(inReadOnly, inModDate);
}

// --------------------------------------------------------------------

class MLocalFileLoader : public MFileLoader
{
  public:
					MLocalFileLoader(
						MFile&			inFile);

	virtual void	DoLoad();
};

// --------------------------------------------------------------------

MLocalFileLoader::MLocalFileLoader(
	MFile&			inFile)
	: MFileLoader(inFile)
{
}

void MLocalFileLoader::DoLoad()
{
	fs::path path(mFile.GetPath());

	if (not fs::exists(path))
		THROW(("File %s does not exist", path.string().c_str())); 
	
	double modTime = fs::last_write_time(path);
	bool readOnly = false;
	
	fs::ifstream file(path);
	eReadFile(file);
	
	SetFileInfo(readOnly, modTime);
	
	eFileLoaded();
	
	delete this;
}

// --------------------------------------------------------------------
// MFileSaver, used to save data to a file.

MFileSaver::MFileSaver(
	MFile&				inFile)
	: mFile(inFile)
{
}

MFileSaver::~MFileSaver()
{
}

void MFileSaver::Cancel()
{
}

void MFileSaver::SetFileInfo(
	bool				inReadOnly,
	double				inModDate)
{
	mFile.SetFileInfo(inReadOnly, inModDate);
}

// --------------------------------------------------------------------

class MLocalFileSaver : public MFileSaver
{
  public:
					MLocalFileSaver(
						MFile&					inFile);

	virtual void	DoSave();
};

// --------------------------------------------------------------------

MLocalFileSaver::MLocalFileSaver(
	MFile&			inFile)
	: MFileSaver(inFile)
{
}

void MLocalFileSaver::DoSave()
{
	fs::path path = mFile.GetPath();
	
	bool save = true;
	
	if (fs::exists(path) and fs::last_write_time(path) > mFile.GetModDate())
		save = eAskOverwriteNewer();

	fs::ofstream file(path, ios::trunc|ios::binary);
	eWriteFile(file);
	
	eFileWritten();
	
	SetFileInfo(false, fs::last_write_time(path));
	
	delete this;
}

// --------------------------------------------------------------------
// MFile, something like a path or URI. 

// first we start with two implementations of the Impl

#include "MFileImp.h"

struct MPathImp : public MFileImp
{
							MPathImp(const fs::path& inPath) : mPath(inPath)
							{
								NormalizePath(mPath);
							}

	virtual bool			Equivalent(const MFileImp* rhs) const
							{
								const MPathImp* prhs = dynamic_cast<const MPathImp*>(rhs);
								return prhs != nil and fs::equivalent(mPath, prhs->mPath);
							}
							
	virtual std::string		GetURI() const
							{
								string path = fs::system_complete(mPath).string();
								URLEncode(path);
								return GetScheme() + "://" + path;
							}
							
	virtual fs::path		GetPath() const
							{
								return mPath;
							}

	virtual std::string		GetScheme() const
							{
								return "file";
							}
							
	virtual std::string		GetFileName() const
							{
								return mPath.leaf();
							}
							
	virtual bool			IsLocal() const
							{
								return true;
							}
							
	virtual MFileImp*		GetParent() const
							{
								return new MPathImp(mPath.branch_path());
							}
	
	virtual MFileImp*		GetChild(const fs::path& inSubPath) const
							{
								return new MPathImp(mPath / inSubPath);
							}
	
	virtual MFileLoader*	Load(MFile& inFile)
							{
								return new MLocalFileLoader(inFile);
							}
							
	virtual MFileSaver*		Save(MFile& inFile)
							{
								return new MLocalFileSaver(inFile);
							}

  private:
	fs::path				mPath;
};

// --------------------------------------------------------------------
// SFTP implementations

class MSftpFileLoader : public MFileLoader
{
  public:
					MSftpFileLoader(
						MFile&			inUrl);
	
	virtual void	DoLoad();
	
	virtual void	Cancel();

  private:

	void			SFTPGetChannelEvent(
						int				inMessage);

	void			SFTPChannelMessage(
						string			inMessage);

	auto_ptr<MSftpChannel>
					mSFTPChannel;
	int64			mFileSize;
	string			mData;
};

MSftpFileLoader::MSftpFileLoader(
	MFile&			inUrl)
	: MFileLoader(inUrl)
{
}

void MSftpFileLoader::DoLoad()
{
	mSFTPChannel.reset(new MSftpChannel(mFile));

	SetCallback(mSFTPChannel->eChannelEvent,
		this, &MSftpFileLoader::SFTPGetChannelEvent);
	SetCallback(mSFTPChannel->eChannelMessage,
		this, &MSftpFileLoader::SFTPChannelMessage);
	
	mFileSize = 0;
	mData.clear();
}

void MSftpFileLoader::Cancel()
{
	mSFTPChannel.release();
}

void MSftpFileLoader::SFTPGetChannelEvent(
	int		inMessage)
{
	switch (inMessage)
	{
		case SFTP_INIT_DONE:
			eProgress(0.f, _("Connected"));
			mSFTPChannel->ReadFile(mFile.GetPath().string(),
				Preferences::GetInteger("text transfer", true));
			break;
		
		case SFTP_FILE_SIZE_KNOWN:
			eProgress(0.f, _("File size known"));
			mFileSize = mSFTPChannel->GetFileSize();
			break;
		
		case SFTP_DATA_AVAILABLE:
			mData += mSFTPChannel->GetData();
			if (mFileSize > 0)
			{
				eProgress(
					float(mData.length()) / mFileSize,
					_("Receiving data"));
			}
			break;
		
		case SFTP_DATA_DONE:
		{
			eProgress(1.0f, _("Data received"));

			stringstream data(mData);
			eReadFile(data);

			mData.clear();

			mSFTPChannel->CloseFile();
			break;
		}
		
		case SFTP_FILE_CLOSED:
			eFileLoaded();
			mSFTPChannel->Close();
			break;

		case SSH_CHANNEL_TIMEOUT:
			eProgress(0, _("Timeout"));
			break;
	}
}

void MSftpFileLoader::SFTPChannelMessage(
	string	 	inMessage)
{
	float fraction = 0;
	if (mFileSize > 0)
		fraction = float(mData.size()) / mFileSize;
	eProgress(fraction, inMessage);
}

struct MSftpImp : public MFileImp
{
							MSftpImp(
								const string&	inUsername,
								const string&	inPassword,
								const string&	inHostname,
								uint16			inPort,
								const fs::path&	inFilePath)
								: mUsername(inUsername)
								, mPassword(inPassword)
								, mHostname(inHostname)
								, mPort(inPort)
								, mFilePath(inFilePath)
							{
								NormalizePath(mFilePath);
							}

	virtual bool			Equivalent(const MFileImp* rhs) const
							{
								const MSftpImp* prhs = dynamic_cast<const MSftpImp*>(rhs);
								return prhs != nil and
									mUsername == prhs->mUsername and
									mPassword == prhs->mPassword and
									mHostname == prhs->mHostname and
									mPort == prhs->mPort and
									mFilePath == prhs->mFilePath;
							}
							
	virtual std::string		GetURI() const
							{
								stringstream s;

								s << "sftp://" << mUsername;
								if (not mPassword.empty())
									s << ':' << mPassword;
								s << '@' << mHostname;
								if (mPort != 22)
									s << ':' << mPort;
								s << '/' << mFilePath;

								return s.str();
							}
							
	virtual fs::path		GetPath() const
							{
								return mFilePath;
							}

	virtual std::string		GetScheme() const
							{
								return "sftp";
							}

	string					GetHost() const				{ return mHostname; }
	string					GetUser() const				{ return mUsername; }
	uint16					GetPort() const				{ return mPort; }
							
	virtual std::string		GetFileName() const
							{
								return mFilePath.leaf();
							}
							
	virtual bool			IsLocal() const
							{
								return false;
							}
							
	virtual MFileImp*		GetParent() const
							{
								return new MSftpImp(mUsername, mPassword, mHostname, mPort, mFilePath.branch_path());
							}
	
	virtual MFileImp*		GetChild(const fs::path& inSubPath) const
							{
								return new MSftpImp(mUsername, mPassword, mHostname, mPort, mFilePath / inSubPath);
							}
	
	virtual MFileLoader*	Load(MFile& inFile)
							{
								return new MSftpFileLoader(inFile);
							}
							
	virtual MFileSaver*		Save(MFile& inFile)
							{
								THROW(("Unimplemented"));
//								return new MSftpFileSaver(inFile);
								return nil;
							}

  private:
	string					mUsername, mPassword, mHostname;
	uint16					mPort;
	fs::path				mFilePath;
};

// --------------------------------------------------------------------

MFile::MFile()
	: mImpl(nil)
	, mReadOnly(false)
	, mModDate(0)
{
}

MFile::MFile(
	MFileImp*			impl)
	: mImpl(impl)
	, mReadOnly(false)
	, mModDate(0)
{
}

MFile::~MFile()
{
	if (mImpl != nil)
		mImpl->Release();
}

MFile::MFile(
	const MFile&		rhs)
	: mImpl(rhs.mImpl)
	, mReadOnly(rhs.mReadOnly)
	, mModDate(rhs.mModDate)
{
	if (mImpl != nil)
		mImpl->Reference();
}

MFile::MFile(
	const fs::path&		inPath)
	: mImpl(new MPathImp(inPath))
	, mReadOnly(false)
	, mModDate(0)
{
}

namespace
{

MFileImp* CreateFileImpForURI(
	const std::string&	inURI)
{
	MFileImp* result = nil;

	pcrecpp::RE re("^(\\w+)://(.+)");
	string scheme, path;
	
	if (re.FullMatch(inURI, &scheme, &path))
	{
		URLDecode(path);
		
		if (scheme == "file")
			result = new MPathImp(fs::system_complete(path));
		else if (scheme == "sftp" or scheme == "ssh")
		{
			pcrecpp::RE re2("^(([-$_.+!*'(),[:alnum:];?&=]+)(:([-$_.+!*'(),[:alnum:];?&=]+))?@)?([-[:alnum:].]+)(:\\d+)?(/.+)");
			
			string s1, s2, username, password, host, port, file;

			if (re2.FullMatch(path, &s1, &username, &s2, &password, &host, &port, &file))
			{
				if (port.empty())
					port = "22";
				result = new MSftpImp(username, password, host, boost::lexical_cast<uint16>(port), file);
			}
			else
				THROW(("Malformed URL: '%s'", inURI.c_str()));
		}
		else
			THROW(("Unsupported URL scheme %s", path.c_str()));
	}
	else // assume it is a simple path
		result = new MPathImp(fs::system_complete(inURI));
	
	return result;
}
	
}

MFile::MFile(
	const char*			inURI)
	: mImpl(CreateFileImpForURI(inURI))
	, mReadOnly(false)
	, mModDate(0)
{
}

MFile::MFile(
	const string&		inURI)
	: mImpl(CreateFileImpForURI(inURI))
	, mReadOnly(false)
	, mModDate(0)
{
}

MFile& MFile::operator=(
	const MFile&		rhs)
{
	if (mImpl != nil)
		mImpl->Release();
	
	mImpl = rhs.mImpl;
	
	if (mImpl != nil)
		mImpl->Reference();

	mReadOnly = rhs.mReadOnly;
	mModDate = rhs.mModDate;

	return *this;
}

MFile& MFile::operator=(
	const fs::path&		rhs)
{
	if (mImpl != nil)
		mImpl->Release();
	
	mImpl = new MPathImp(rhs);

	mReadOnly = false;
	mModDate = 0;

	return *this;
}

bool MFile::operator==(
	const MFile&		rhs) const
{
	return mImpl == rhs.mImpl or
		(mImpl != nil and rhs.mImpl != nil and mImpl->Equivalent(rhs.mImpl));
}

MFile& MFile::operator/=(
	const char*			inSubPath)
{
	if (mImpl == nil)
		THROW(("Invalid file for /="));
	
	MFileImp* imp = mImpl->GetChild(inSubPath);
	mImpl->Release();
	mImpl = imp;

	return *this;
}

MFile& MFile::operator/=(
	const fs::path&		inSubPath)
{
	if (mImpl == nil)
		THROW(("Invalid file for /="));
	
	MFileImp* imp = mImpl->GetChild(inSubPath);
	mImpl->Release();
	mImpl = imp;

	return *this;
}

void MFile::SetFileInfo(
	bool				inReadOnly,
	double				inModDate)
{
	mReadOnly = inReadOnly;
	mModDate = inModDate;
}

fs::path MFile::GetPath() const
{
	fs::path result;
	if (mImpl != nil)
		result = mImpl->GetPath();
	return result;
}

string MFile::GetURI() const
{
	string result;
	if (mImpl != nil)
		result = mImpl->GetURI();
	return result;
}

string MFile::GetScheme() const
{
	string result;
	if (mImpl != nil)
		result = mImpl->GetScheme();
	return result;
}

string MFile::GetHost() const
{
	string result;
	
	MSftpImp* imp = dynamic_cast<MSftpImp*>(mImpl);
	if (imp != nil)
		result = imp->GetHost();
	
	return result;
}
	
string MFile::GetUser() const
{
	string result;
	
	MSftpImp* imp = dynamic_cast<MSftpImp*>(mImpl);
	if (imp != nil)
		result = imp->GetUser();
	
	return result;
}
	
uint16 MFile::GetPort() const
{
	uint16 result;
	
	MSftpImp* imp = dynamic_cast<MSftpImp*>(mImpl);
	if (imp != nil)
		result = imp->GetPort() ;
	
	return result;
}

string MFile::GetFileName() const
{
	string result;
	if (mImpl != nil)
		result = mImpl->GetFileName();
	return result;
}

MFile MFile::GetParent() const
{
	MFile result;
	if (mImpl != nil)
		result = MFile(mImpl->GetParent());
	return result;
}

MFileLoader* MFile::Load()
{
	THROW_IF_NIL(mImpl);
	return mImpl->Load(*this);
}

MFileSaver* MFile::Save()
{
	THROW_IF_NIL(mImpl);
	return mImpl->Save(*this);
}

bool MFile::IsValid() const
{
	return mImpl != nil;
}

bool MFile::IsLocal() const
{
	return mImpl != nil and mImpl->IsLocal();
}

bool MFile::Exists() const
{
	assert(IsLocal());
	return IsLocal() and fs::exists(GetPath());
}

double MFile::GetModDate() const
{
	return mModDate;
}

bool MFile::ReadOnly() const
{
	return mReadOnly;
}

ssize_t MFile::ReadAttribute(
	const char*			inName,
	void*				outData,
	size_t				inDataSize) const
{
	ssize_t result = 0;
	
	if (IsLocal())
		result = read_attribute(GetPath(), inName, outData, inDataSize);
	
	return result;
}

size_t MFile::WriteAttribute(
	const char*			inName,
	const void*			inData,
	size_t				inDataSize) const
{
	size_t result = 0;
	
	if (IsLocal())
	{
		write_attribute(GetPath(), inName, inData, inDataSize);
		result = inDataSize;
	}
	
	return result;
}

MFile operator/(const MFile& lhs, const fs::path& rhs)
{
	MFile result(lhs);
	result /= rhs;
	return result;
}

//fs::path RelativePath(const MFile& lhs, const MFile& rhs)
//{
//	char* p = g_file_get_relative_path(lhs.mFile, rhs.mFile);
//	if (p == nil)
//		THROW(("Failed to get relative path"));
//	
//	fs::path result(p);
//	g_free(p);
//	return result;
//}

ostream& operator<<(ostream& lhs, const MFile& rhs)
{
	lhs << rhs.GetURI();
	return lhs;
}

namespace {

bool Match(const char* inPattern, const char* inName);

bool Match(
	const char*		inPattern,
	const char*		inName)
{
	for (;;)
	{
		char op = *inPattern;

		switch (op)
		{
			case 0:
				return *inName == 0;
			case '*':
			{
				if (inPattern[1] == 0)	// last '*' matches all 
					return true;

				const char* n = inName;
				while (*n)
				{
					if (Match(inPattern + 1, n))
						return true;
					++n;
				}
				return false;
			}
			case '?':
				if (*inName)
					return Match(inPattern + 1, inName + 1);
				else
					return false;
			default:
				if (tolower(*inName) == tolower(op))
				{
					++inName;
					++inPattern;
				}
				else
					return false;
				break;
		}
	}
}

}

bool FileNameMatches(
	const char*		inPattern,
	const MFile&	inFile)
{
	return FileNameMatches(inPattern, inFile.GetPath());
}

bool FileNameMatches(
	const char*		inPattern,
	const fs::path&		inFile)
{
	return FileNameMatches(inPattern, inFile.leaf());
}

bool FileNameMatches(
	const char*		inPattern,
	const string&	inFile)
{
	bool result = false;
	
	if (inFile.length() > 0)
	{
		string p(inPattern);
	
		while (not result and p.length())
		{
			string::size_type s = p.find(';');
			string pat;
			
			if (s == string::npos)
			{
				pat = p;
				p.clear();
			}
			else
			{
				pat = p.substr(0, s);
				p.erase(0, s + 1);
			}
			
			result = Match(pat.c_str(), inFile.c_str());
		}
	}
	
	return result;	
}

// ------------------------------------------------------------

struct MFileIteratorImp
{
	struct MInfo
	{
		fs::path			mParent;
		DIR*			mDIR;
		struct dirent	mEntry;
	};
	
						MFileIteratorImp()
							: mReturnDirs(false) {}
	virtual				~MFileIteratorImp() {}
	
	virtual	bool		Next(fs::path& outFile) = 0;
	bool				IsTEXT(
							const fs::path&	inFile);
	
	string				mFilter;
	bool				mReturnDirs;
};

struct MSingleFileIteratorImp : public MFileIteratorImp
{
						MSingleFileIteratorImp(
							const fs::path&	inDirectory);

	virtual				~MSingleFileIteratorImp();
	
	virtual	bool		Next(
							fs::path&			outFile);
	
	MInfo				mInfo;
};

MSingleFileIteratorImp::MSingleFileIteratorImp(
	const fs::path&	inDirectory)
{
	mInfo.mParent = inDirectory;
	mInfo.mDIR = opendir(inDirectory.string().c_str());
	memset(&mInfo.mEntry, 0, sizeof(mInfo.mEntry));
}

MSingleFileIteratorImp::~MSingleFileIteratorImp()
{
	if (mInfo.mDIR != nil)
		closedir(mInfo.mDIR);
}

bool MSingleFileIteratorImp::Next(
	fs::path&			outFile)
{
	bool result = false;
	
	while (not result)
	{
		struct dirent* e = nil;
		
		if (mInfo.mDIR != nil)
			THROW_IF_POSIX_ERROR(::readdir_r(mInfo.mDIR, &mInfo.mEntry, &e));
		
		if (e == nil)
			break;

		if (strcmp(e->d_name, ".") == 0 or strcmp(e->d_name, "..") == 0)
			continue;

		outFile = mInfo.mParent / e->d_name;

		if (is_directory(outFile) and not mReturnDirs)
			continue;
		
		if (mFilter.length() == 0 or
			FileNameMatches(mFilter.c_str(), outFile))
		{
			result = true;
		}
	}
	
	return result;
}

struct MDeepFileIteratorImp : public MFileIteratorImp
{
						MDeepFileIteratorImp(
							const fs::path&	inDirectory);

	virtual				~MDeepFileIteratorImp();

	virtual	bool		Next(fs::path& outFile);

	stack<MInfo>		mStack;
};

MDeepFileIteratorImp::MDeepFileIteratorImp(const fs::path& inDirectory)
{
	MInfo info;

	info.mParent = inDirectory;
	info.mDIR = opendir(inDirectory.string().c_str());
	memset(&info.mEntry, 0, sizeof(info.mEntry));
	
	mStack.push(info);
}

MDeepFileIteratorImp::~MDeepFileIteratorImp()
{
	while (not mStack.empty())
	{
		closedir(mStack.top().mDIR);
		mStack.pop();
	}
}

bool MDeepFileIteratorImp::Next(
	fs::path&		outFile)
{
	bool result = false;
	
	while (not result and not mStack.empty())
	{
		struct dirent* e = nil;
		
		MInfo& top = mStack.top();
		
		if (top.mDIR != nil)
			THROW_IF_POSIX_ERROR(::readdir_r(top.mDIR, &top.mEntry, &e));
		
		if (e == nil)
		{
			if (top.mDIR != nil)
				closedir(top.mDIR);
			mStack.pop();
		}
		else
		{
			outFile = top.mParent / e->d_name;
			
			struct stat st;
			if (stat(outFile.string().c_str(), &st) < 0 or S_ISLNK(st.st_mode))
				continue;
			
			if (S_ISDIR(st.st_mode))
			{
				if (strcmp(e->d_name, ".") and strcmp(e->d_name, ".."))
				{
					MInfo info;
	
					info.mParent = outFile;
					info.mDIR = opendir(outFile.string().c_str());
					memset(&info.mEntry, 0, sizeof(info.mEntry));
					
					mStack.push(info);
				}
				continue;
			}

			if (mFilter.length() and not FileNameMatches(mFilter.c_str(), outFile))
				continue;

			result = true;
		}
	}
	
	return result;
}

MFileIterator::MFileIterator(
	const fs::path&	inDirectory,
	uint32			inFlags)
{
	if (inFlags & kFileIter_Deep)
		mImpl = new MDeepFileIteratorImp(inDirectory);
	else
		mImpl = new MSingleFileIteratorImp(inDirectory);
	
	mImpl->mReturnDirs = (inFlags & kFileIter_ReturnDirectories) != 0;
}

MFileIterator::~MFileIterator()
{
	delete mImpl;
}

bool MFileIterator::Next(
	fs::path&			outFile)
{
	return mImpl->Next(outFile);
}

void MFileIterator::SetFilter(
	const string&	inFilter)
{
	mImpl->mFilter = inFilter;
}

// ----------------------------------------------------------------------------
//	relative_path

fs::path relative_path(const fs::path& inFromDir, const fs::path& inFile)
{
//	assert(false);

	fs::path::iterator d = inFromDir.begin();
	fs::path::iterator f = inFile.begin();
	
	while (d != inFromDir.end() and f != inFile.end() and *d == *f)
	{
		++d;
		++f;
	}
	
	fs::path result;
	
	if (d == inFromDir.end() and f == inFile.end())
		result = ".";
	else
	{
		while (d != inFromDir.end())
		{
			result /= "..";
			++d;
		}
		
		while (f != inFile.end())
		{
			result /= *f;
			++f;
		}
	}

	return result;
}

bool ChooseDirectory(
	fs::path&	outDirectory)
{
	GtkWidget* dialog = nil;
	bool result = false;

	try
	{
		dialog = 
			gtk_file_chooser_dialog_new(_("Select Folder"), nil,
				GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
				GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
				NULL);
		
		THROW_IF_NIL(dialog);
	
		string currentFolder = gApp->GetCurrentFolder();
	
		if (currentFolder.length() > 0)
		{
			gtk_file_chooser_set_current_folder_uri(
				GTK_FILE_CHOOSER(dialog), currentFolder.c_str());
		}
		
		if (fs::exists(outDirectory) and outDirectory != fs::path())
		{
			gtk_file_chooser_select_filename(GTK_FILE_CHOOSER(dialog),
				outDirectory.string().c_str());
		}
		
		if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
		{
			char* uri = gtk_file_chooser_get_uri(GTK_FILE_CHOOSER(dialog));
			
			MFile url(uri);
			outDirectory = url.GetPath();

			g_free(uri);

			result = true;
//
//			gApp->SetCurrentFolder(
//				gtk_file_chooser_get_current_folder_uri(GTK_FILE_CHOOSER(dialog)));
		}
	}
	catch (exception& e)
	{
		if (dialog)
			gtk_widget_destroy(dialog);
		
		throw;
	}
	
	gtk_widget_destroy(dialog);

	return result;
}

//bool ChooseDirectory(
//	fs::path&			outDirectory)
//{
//	bool result = true; 
//	
//	MFile dir(outDirectory);
//
//	if (ChooseDirectory(dir))
//	{
//		outDirectory = dir.GetPath();
//		result = true;
//	}
//	
//	return result; 
//}

bool ChooseOneFile(
	MFile&	ioFile)
{
	GtkWidget* dialog = nil;
	bool result = false;
	
	try
	{
		dialog = 
			gtk_file_chooser_dialog_new(_("Select File"), nil,
				GTK_FILE_CHOOSER_ACTION_OPEN,
				GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
				NULL);
		
		THROW_IF_NIL(dialog);
	
		gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(dialog), false);
		gtk_file_chooser_set_local_only(GTK_FILE_CHOOSER(dialog), false);
		
		if (ioFile.IsValid())
		{
			gtk_file_chooser_select_uri(GTK_FILE_CHOOSER(dialog),
				ioFile.GetURI().c_str());
		}
		else if (gApp->GetCurrentFolder().length() > 0)
		{
			gtk_file_chooser_set_current_folder_uri(
				GTK_FILE_CHOOSER(dialog), gApp->GetCurrentFolder().c_str());
		}
		
		if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
		{
			char* uri = gtk_file_chooser_get_uri(GTK_FILE_CHOOSER(dialog));	
			
			ioFile = MFile(uri);
			g_free(uri);

			gApp->SetCurrentFolder(
				gtk_file_chooser_get_current_folder_uri(GTK_FILE_CHOOSER(dialog)));

			result = true;
		}
	}
	catch (exception& e)
	{
		if (dialog)
			gtk_widget_destroy(dialog);
		
		throw;
	}
	
	gtk_widget_destroy(dialog);
	
	return result;
}

bool ChooseFiles(
	bool				inLocalOnly,
	std::vector<MFile>&	outFiles)
{
	GtkWidget* dialog = nil;
	
	try
	{
		dialog = 
			gtk_file_chooser_dialog_new(_("Open"), nil,
				GTK_FILE_CHOOSER_ACTION_OPEN,
				GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
				NULL);
		
		THROW_IF_NIL(dialog);
	
		gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(dialog), true);
		gtk_file_chooser_set_local_only(GTK_FILE_CHOOSER(dialog), inLocalOnly);
		
		if (gApp->GetCurrentFolder().length() > 0)
		{
			gtk_file_chooser_set_current_folder_uri(
				GTK_FILE_CHOOSER(dialog), gApp->GetCurrentFolder().c_str());
		}
		
		if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
		{
			GSList* uris = gtk_file_chooser_get_uris(GTK_FILE_CHOOSER(dialog));	
			
			GSList* file = uris;	
			
			while (file != nil)
			{
				MFile url(reinterpret_cast<char*>(file->data));

				g_free(file->data);
				file->data = nil;

				outFiles.push_back(url);

				file = file->next;
			}
			
			g_slist_free(uris);
		}
		
		char* cwd = gtk_file_chooser_get_current_folder_uri(GTK_FILE_CHOOSER(dialog));
		if (cwd != nil)
		{
			gApp->SetCurrentFolder(cwd);
			g_free(cwd);
		}
	}
	catch (exception& e)
	{
		if (dialog)
			gtk_widget_destroy(dialog);
		
		throw;
	}
	
	gtk_widget_destroy(dialog);
	
	return outFiles.size() > 0;
}

void NormalizePath(
	fs::path&	ioPath)
{
	string p = ioPath.string();
	NormalizePath(p);
	ioPath = p;
}

void NormalizePath(string& ioPath)
{
	string path(ioPath);	
	stack<unsigned long> dirs;
	int r = 0;
	unsigned long i = 0;
	
	dirs.push(0);
	
	while (i < path.length())
	{
		while (i < path.length() && path[i] == '/')
		{
			++i;
			if (dirs.size() > 0)
				dirs.top() = i;
			else
				dirs.push(i);
		}
		
		if (path[i] == '.' && path[i + 1] == '.' && path[i + 2] == '/')
		{
			if (dirs.size() > 0)
				dirs.pop();
			if (dirs.size() == 0)
				--r;
			i += 2;
			continue;
		}
		else if (path[i] == '.' && path[i + 1] == '/')
		{
			i += 1;
			if (dirs.size() > 0)
				dirs.top() = i;
			else
				dirs.push(i);
			continue;
		}

		unsigned long d = path.find('/', i + 1);

		if (d == string::npos)
			break;
		
		i = d + 1;
		dirs.push(i);
	}
	
	if (dirs.size() > 0 && dirs.top() == path.length())
		ioPath.assign("/");
	else
		ioPath.erase(ioPath.begin(), ioPath.end());
	
	bool dir = false;
	while (dirs.size() > 0)
	{
		unsigned long l, n;
		n = path.find('/', dirs.top());
		if (n == string::npos)
			l = path.length() - dirs.top();
		else
			l = n - dirs.top();
		
		if (l > 0)
		{
			if (dir)
				ioPath.insert(0, "/");
			ioPath.insert(0, path.c_str() + dirs.top(), l);
			dir = true;
		}
		
		dirs.pop();
	}
	
	if (r < 0)
	{
		ioPath.insert(0, "../");
		while (++r < 0)
			ioPath.insert(0, "../");
	}
	else if (path.length() > 0 && path[0] == '/' && ioPath[0] != '/')
		ioPath.insert(0, "/");
}
