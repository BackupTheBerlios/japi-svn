//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "MLib.h"

//#include <unistd.h>
#include <cstring>

#include <sys/stat.h>
//#include <dirent.h>
#include <stack>
#include <fstream>
#include <cassert>
#include <cerrno>
#include <limits>

//#include <pcrecpp.h>

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
//#include "MJapiApp.h"
#include "MPreferences.h"
//#include "MSftpChannel.h"

using namespace std;
namespace io = boost::iostreams;
namespace ba = boost::algorithm;

int32 read_attribute(const fs::path& inPath, const char* inName, void* outData, size_t inDataSize);
int32 write_attribute(const fs::path& inPath, const char* inName, const void* inData, size_t inDataSize);

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
	std::time_t			inModDate)
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
	try
	{
		fs::path path(mFile.GetPath());
	
		if (not fs::exists(path))
			THROW(("File %s does not exist", path.string().c_str())); 
		
		time_t modTime = fs::last_write_time(path);
        bool readOnly = false;

		//struct stat st;

		//if (stat(path.string().c_str(), &st) == 0)
		//{
		//	// fetch user&group
		//	unsigned int gid = getgid();
		//	unsigned int uid = getuid();
		//	
		//	readOnly = not ((uid == st.st_uid and (S_IWUSR & st.st_mode)) or
		//					(gid == st.st_gid and (S_IWGRP & st.st_mode)) or
		//					(S_IWOTH & st.st_mode));
		//	
		//	if (readOnly and S_IWGRP & st.st_mode)
		//	{
		//		int ngroups = getgroups(0, nil);
		//		if (ngroups > 0)
		//		{
		//			vector<gid_t> groups(ngroups);
		//			if (getgroups(ngroups, &groups[0]) == 0)
		//				readOnly = find(groups.begin(), groups.end(), st.st_gid) == groups.end();
		//		}
		//	}
		//}
		
		fs::ifstream file(path, ios::binary);
		eReadFile(file);
		
		SetFileInfo(readOnly, modTime);
		
		eFileLoaded();
	}
	catch (exception& e)
	{
		eError(e.what());
	}
	
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
	std::time_t			inModDate)
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
	try
	{
		fs::path path = mFile.GetPath();
		
		bool save = true;
		
		if (fs::exists(path) and
		    (fs::last_write_time(path) <= mFile.GetModDate() or eAskOverwriteNewer()))
		{
			fs::ofstream file(path, ios::trunc|ios::binary);
			
			if (not file.is_open())
				THROW(("Could not open file %s for writing", path.leaf().c_str()));
			
			eWriteFile(file);
			
			file.close();
			
			eFileWritten();
			
			SetFileInfo(false, fs::last_write_time(path));
		}
	}
	catch (exception& e)
	{
		eError(e.what());
	}
	
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
								return mPath.filename();
							}
							
	virtual bool			IsLocal() const
							{
								return true;
							}
							
	virtual MFileImp*		GetParent() const
							{
								return new MPathImp(mPath.parent_path());
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

#if 0
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

	void			SFTPChannelEvent(
						int				inMessage);

	void			SFTPChannelMessage(
						string			inMessage);

	unique_ptr<MSftpChannel>
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
		this, &MSftpFileLoader::SFTPChannelEvent);
	SetCallback(mSFTPChannel->eChannelMessage,
		this, &MSftpFileLoader::SFTPChannelMessage);
	
	mFileSize = 0;
	mData.clear();
}

void MSftpFileLoader::Cancel()
{
	mSFTPChannel.release();
}

void MSftpFileLoader::SFTPChannelEvent(
	int		inMessage)
{
	switch (inMessage)
	{
		case SFTP_INIT_DONE:
			eProgress(0.f, _("Connected"));
			mSFTPChannel->ReadFile(mFile.GetPath().string(),
				Preferences::GetBoolean("text transfer", true));
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

			mSFTPChannel->CloseFile();
			
//			delete this;	// oohh, tricky?
			break;
		}
		
		case SFTP_FILE_CLOSED:
			eFileLoaded();
			mSFTPChannel->Close();
			break;

		case SSH_CHANNEL_TIMEOUT:
			eProgress(0, _("Timeout"));
			eError("SSH Channel Timeout");
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

// --------------------------------------------------------------------

class MSftpFileSaver : public MFileSaver
{
  public:
					MSftpFileSaver(
						MFile&					inFile);

	virtual void	DoSave();

  private:

	void			SFTPChannelEvent(
						int				inMessage);

	void			SFTPChannelMessage(
						string			inMessage);

	unique_ptr<MSftpChannel>
					mSFTPChannel;
	int64			mSFTPOffset;
	int64			mSFTPSize;
	string			mSFTPData;
};

MSftpFileSaver::MSftpFileSaver(
	MFile&			inFile)
	: MFileSaver(inFile)
{
}

void MSftpFileSaver::DoSave()
{
	mSFTPChannel.reset(new MSftpChannel(mFile));

	{
		io::filtering_ostream out(io::back_inserter(mSFTPData));
		eWriteFile(out);
	}

	SetCallback(mSFTPChannel->eChannelEvent,
		this, &MSftpFileSaver::SFTPChannelEvent);
	SetCallback(mSFTPChannel->eChannelMessage,
		this, &MSftpFileSaver::SFTPChannelMessage);

	mSFTPOffset = 0;
	mSFTPSize = mSFTPData.length();
}

void MSftpFileSaver::SFTPChannelEvent(
	int				inMessage)
{
	const uint32 kBufferSize = 10240;
	
	switch (inMessage)
	{
		case SFTP_INIT_DONE:
			eProgress(0.f, _("Connected"));
			mSFTPChannel->WriteFile(mFile.GetPath().string(),
				Preferences::GetBoolean("text transfer", true));
			break;
		
		case SFTP_CAN_SEND_DATA:
			if (mSFTPOffset < mSFTPSize)
			{
				uint32 k = mSFTPSize - mSFTPOffset;
				if (k > kBufferSize)
					k = kBufferSize;

				eProgress(
					float(mSFTPOffset) / mSFTPSize,
					_("Sending data"));

				mSFTPChannel->SendData(mSFTPData.substr(mSFTPOffset, k));
				mSFTPOffset += k;
			}
			else
			{
				eProgress(1.0f, _("Closing file"));

				mSFTPChannel->CloseFile();
//				SetModified(false);
//
//				if (Preferences::GetBoolean("loguploads", false) != 0)
//					LogUpload();
			}
			break;

		case SFTP_FILE_CLOSED:
//			eProgress(-1.f, _("done"));
			eFileWritten();
			mSFTPChannel->Close();
			
//			SetFileInfo(false, fs::last_write_time(path));
			break;

		case SSH_CHANNEL_TIMEOUT:
			eProgress(0, _("Timeout"));
			eError("SSH Channel Timeout");
			break;
	}
}

void MSftpFileSaver::SFTPChannelMessage(
	string	 	inMessage)
{
	float fraction = 0;
	if (mSFTPSize > 0)
		fraction = float(mSFTPData.size()) / mSFTPSize;
	eProgress(fraction, inMessage);
}

// --------------------------------------------------------------------

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

								s << "ssh://";
								if (not mUsername.empty())
								{
									s << mUsername;
									if (not mPassword.empty())
										s << ':' << mPassword;
									s << '@';
								}
								s << mHostname;
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
								return "ssh";
							}

	string					GetHost() const				{ return mHostname; }
	string					GetUser() const				{ return mUsername; }
	uint16					GetPort() const				{ return mPort; }
							
	virtual std::string		GetFileName() const
							{
								return mFilePath.filename();
							}
							
	virtual bool			IsLocal() const
							{
								return false;
							}
							
	virtual MFileImp*		GetParent() const
							{
								return new MSftpImp(mUsername, mPassword, mHostname, mPort, mFilePath.parent_path());
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
								return new MSftpFileSaver(inFile);
							}

  private:
	string					mUsername, mPassword, mHostname;
	uint16					mPort;
	fs::path				mFilePath;
};

#endif

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
	const char*			inURI,
	bool				isAbsoluteURI)
{
	THROW_IF_NIL(inURI);
	
	MFileImp* result = nil;

	if (ba::starts_with(inURI, "file://"))
	{
		string path(inURI + 7);
		URLDecode(path);
		result = new MPathImp(path);
	}
	else if (ba::starts_with(inURI, "ssh://") or ba::starts_with(inURI, "sftp://"))
	{
		//URLDecode(path);
		//
		//if (scheme == "file")
		//	result = new MPathImp(fs::system_complete(path));
		//else if (scheme == "sftp" or scheme == "ssh")
		//{
		//	pcrecpp::RE re2("^(([-$_.+!*'(),[:alnum:];?&=]+)(:([-$_.+!*'(),[:alnum:];?&=]+))?@)?([-[:alnum:].]+)(:\\d+)?/(.+)");
		//	
		//	string s1, s2, username, password, host, port, file;

		//	if (re2.FullMatch(path, &s1, &username, &s2, &password, &host, &port, &file))
		//	{
		//		if (port.empty())
		//			port = "22";

		//		if (isAbsoluteURI)
		//			file.insert(file.begin(), '/');

		//		result = new MSftpImp(username, password, host, boost::lexical_cast<uint16>(port), file);
		//	}
		//	else
		//		THROW(("Malformed URL: '%s'", inURI));
		//}
		//else
		//	THROW(("Unsupported URL scheme '%s'", scheme.c_str()));
	}
	else // assume it is a simple path
		result = new MPathImp(fs::system_complete(inURI));
	
	return result;
}
	
}

MFile::MFile(
	const char*			inURI,
	bool				isAbsoluteURI)
	: mImpl(CreateFileImpForURI(inURI, isAbsoluteURI))
	, mReadOnly(false)
	, mModDate(0)
{
}

MFile::MFile(
	const string&		inURI,
	bool				isAbsoluteURI)
	: mImpl(CreateFileImpForURI(inURI.c_str(), isAbsoluteURI))
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
	std::time_t			inModDate)
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
	
	//MSftpImp* imp = dynamic_cast<MSftpImp*>(mImpl);
	//if (imp != nil)
	//	result = imp->GetHost();
	
	return result;
}
	
string MFile::GetUser() const
{
	string result;
	
	//MSftpImp* imp = dynamic_cast<MSftpImp*>(mImpl);
	//if (imp != nil)
	//	result = imp->GetUser();
	
	return result;
}
	
uint16 MFile::GetPort() const
{
	uint16 result = 22;
	
	//MSftpImp* imp = dynamic_cast<MSftpImp*>(mImpl);
	//if (imp != nil)
	//	result = imp->GetPort() ;
	
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

time_t MFile::GetModDate() const
{
	return mModDate;
}

bool MFile::ReadOnly() const
{
	return mReadOnly;
}

int32 MFile::ReadAttribute(
	const char*			inName,
	void*				outData,
	uint32				inDataSize) const
{
	int32 result = 0;
	
	if (IsLocal())
		result = read_attribute(GetPath(), inName, outData, inDataSize);
	
	return result;
}

int32 MFile::WriteAttribute(
	const char*			inName,
	const void*			inData,
	uint32				inDataSize) const
{
	uint32 result = 0;
	
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
	return FileNameMatches(inPattern, inFile.filename());
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
						MFileIteratorImp()
							: mReturnDirs(false) {}
	virtual				~MFileIteratorImp() {}
	
	virtual	bool		Next(fs::path& outFile) = 0;
	
	string				mFilter;
	bool				mReturnDirs;
};

struct MSingleFileIteratorImp : public MFileIteratorImp
{
						MSingleFileIteratorImp(
							const fs::path&	inDirectory);

	virtual	bool		Next(
							fs::path&			outFile);
	
	fs::directory_iterator
						mIter;
};

MSingleFileIteratorImp::MSingleFileIteratorImp(
	const fs::path&	inDirectory)
	: mIter(inDirectory)
{
}

bool MSingleFileIteratorImp::Next(
	fs::path&			outFile)
{
	bool result = false;
	
	for (; result == false and mIter != fs::directory_iterator(); ++mIter)
	{
		if (fs::is_directory(*mIter) and not mReturnDirs)
			continue;
		
		if (mFilter.empty() or
			FileNameMatches(mFilter.c_str(), *mIter))
		{
			outFile = *mIter;
			result = true;
		}
	}
	
	return result;
}

struct MDeepFileIteratorImp : public MFileIteratorImp
{
						MDeepFileIteratorImp(
							const fs::path&	inDirectory);

	virtual	bool		Next(fs::path& outFile);

	fs::recursive_directory_iterator
						mIter;
};

MDeepFileIteratorImp::MDeepFileIteratorImp(const fs::path& inDirectory)
	: mIter(inDirectory)
{
}

bool MDeepFileIteratorImp::Next(
	fs::path&		outFile)
{
	bool result = false;
	
	for (; result == false and mIter != fs::recursive_directory_iterator(); ++mIter)
	{
		if (fs::is_directory(*mIter) and not mReturnDirs)
			continue;

		if (mFilter.empty() or
			FileNameMatches(mFilter.c_str(), *mIter))
		{
			outFile = *mIter;
			result = true;
		}
	}
	
	return result;
}

MFileIterator::MFileIterator(
	const fs::path&	inDirectory,
	uint32			inFlags)
	: mImpl(nil)
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

	bool unc = false;

	if (path.length() > 2 and path[0] == '/' and path[1] == '/')
	{
		unc = true;
		++i;
	}
	
	while (i < path.length())
	{
		while (i < path.length() and path[i] == '/')
		{
			++i;
			if (not dirs.empty())
				dirs.top() = i;
			else
				dirs.push(i);
		}
		
		if (path[i] == '.' and path[i + 1] == '.' and path[i + 2] == '/')
		{
			if (not dirs.empty())
				dirs.pop();
			if (dirs.empty())
				--r;
			i += 2;
			continue;
		}
		else if (path[i] == '.' and path[i + 1] == '/')
		{
			i += 1;
			if (not dirs.empty())
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
	
	if (not dirs.empty() and dirs.top() == path.length())
		ioPath.assign("/");
	else
		ioPath.erase(ioPath.begin(), ioPath.end());
	
	bool dir = false;
	while (not dirs.empty())
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
	else if (path.length() > 0 and path[0] == '/' and ioPath[0] != '/')
		ioPath.insert(0, "/");

	if (unc and ioPath[0] == '/')
		ioPath.insert(ioPath.begin(), '/');
}
