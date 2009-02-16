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

#include <boost/filesystem/fstream.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/device/back_inserter.hpp>
#include <boost/range/iterator_range.hpp>

#include "MFile.h"
#include "MFile.h"
#include "MError.h"
#include "MUnicode.h"
#include "MUtils.h"
#include "MStrings.h"
#include "MJapiApp.h"

using namespace std;
namespace io = boost::iostreams;

namespace {

const char kRemoteQueryAttributes[] = \
	G_FILE_ATTRIBUTE_STANDARD_SIZE "," \
	G_FILE_ATTRIBUTE_STANDARD_TYPE "," \
	G_FILE_ATTRIBUTE_TIME_MODIFIED "," \
	G_FILE_ATTRIBUTE_ACCESS_CAN_WRITE;
	
}

// --------------------------------------------------------------------
// MFileLoader, used to load the contents of a file.
// This works strictly asynchronous. 

MFileLoader::MFileLoader(
	const MFile&		inFile)
	: mFile(inFile)
{
}

MFileLoader::~MFileLoader()
{
}

// --------------------------------------------------------------------

class MLocalFileLoader : public MFileLoader
{
  public:
					MLocalFileLoader(
						const MFile&	inFile);

	virtual void	DoLoad();
};

// --------------------------------------------------------------------

MLocalFileLoader::MLocalFileLoader(
	const MFile&	inFile)
	: MFileLoader(inFile)
{
}

void MLocalFileLoader::DoLoad()
{
	fs::path path(mFile.GetPath());

	if (not fs::exists(path))
		THROW(("File does not exist")); 
	
	fs::ifstream file(path);
	eReadFile(file);
}

// --------------------------------------------------------------------

class MGIOFileLoader : public MFileLoader
{
  public:
						MGIOFileLoader(
							const MFile&	inFile);
	
						~MGIOFileLoader();

	virtual void		DoLoad();
	
	static void			AsyncCallback(
							GObject*		source_object,
							GAsyncResult*	res,
							gpointer		user_data)
						{
							MGIOFileLoader* self = reinterpret_cast<MGIOFileLoader*>(user_data);
							self->Async(source_object, res);
						}

	void				Async(
							GObject*		inObject,
							GAsyncResult*	inResult);

	void				ProcessFileInfo(
							GFileInfo*		inFileInfo);

	uint32				mBytesRead;
	uint32				mExpectedFileSize;
	GCancellable*		mCancellable;
	GFileInputStream*	mStream;
	string				mBuffer;

	bool				mReadOnly;
	double				mModDate;
	
	char				mBufferBlock[4096];		// read 4 k blocks
	
	enum ELoadingState
	{
		eStateNone,
		eStateCancelled,
		eStateError,
		eStateBegin,
		eWaitAsyncReady,
		eWaitMountReady,
		eWaitGetInfoReady,
		eWaitGetFileInfoReady,
		eWaitReadReady,
		eWaitCloseReady
	}					mState;
	
	bool				mTriedMount;
};

MGIOFileLoader::MGIOFileLoader(
	const MFile&		inFile)
	: MFileLoader(inFile)
	, mBytesRead(0)
	, mCancellable(nil)
	, mStream(nil)
	, mState(eStateNone)
	, mTriedMount(false)
{
}

MGIOFileLoader::~MGIOFileLoader()
{
	if (mCancellable != nil)
	{
		g_cancellable_cancel(mCancellable);
		g_object_unref(mCancellable);
	}
	
	if (mStream != nil)
		g_object_unref(mStream);
}

void MGIOFileLoader::DoLoad()
{
	eProgress(0.f, "start");

	mCancellable = g_cancellable_new();
	
	mState = eWaitAsyncReady;
	g_file_read_async(mFile, G_PRIORITY_HIGH, mCancellable, &MGIOFileLoader::AsyncCallback, this);
}

void MGIOFileLoader::Async(
	GObject*		inObject,
	GAsyncResult*	inResult)
{
	if (g_cancellable_is_cancelled(mCancellable))
	{
		mState = eStateCancelled;
		return;
	}
	
	GError* error = nil;
	
	switch (mState)
	{
		case eWaitAsyncReady:
			mStream = g_file_read_finish(mFile, inResult, &error);
			if (mStream == nil)
			{
				// maybe we need to mount?
				if (error->code == G_IO_ERROR_NOT_MOUNTED and not mTriedMount)
				{
					g_error_free(error);
					error = nil;
					mTriedMount = true;
		
					GMountOperation* mountOperation = g_mount_operation_new();
					
					mState = eWaitMountReady;
					g_file_mount_enclosing_volume(mFile, G_MOUNT_MOUNT_NONE,
						mountOperation, mCancellable, &MGIOFileLoader::AsyncCallback, this);
		
					g_object_unref(mountOperation);
				}
			}
			else
			{
				mState = eWaitGetInfoReady;
				g_file_input_stream_query_info_async(mStream,
					const_cast<char*>(kRemoteQueryAttributes), G_PRIORITY_HIGH,
					mCancellable, &MGIOFileLoader::AsyncCallback, this);
			}
			break;

		case eWaitMountReady:
			if (g_file_mount_enclosing_volume_finish(mFile, inResult, &error))
			{
				mState = eWaitAsyncReady;
				g_file_read_async(mFile, G_PRIORITY_HIGH, mCancellable,
					&MGIOFileLoader::AsyncCallback, this);
			}
			break;
		
		case eWaitGetInfoReady:
		{
			GFileInfo* fileInfo = g_file_input_stream_query_info_finish(mStream, inResult, &error);
			if (fileInfo == nil)
			{
				if (error and error->code == G_IO_ERROR_NOT_SUPPORTED)
				{
					g_error_free(error);
					error = nil;
					
					mState = eWaitGetFileInfoReady;
					g_file_query_info_async(mFile, kRemoteQueryAttributes,
						G_FILE_QUERY_INFO_NONE, G_PRIORITY_HIGH,
						mCancellable, &MGIOFileLoader::AsyncCallback, this);
				}
			}
			else
				ProcessFileInfo(fileInfo);
			break;
		}
		
		case eWaitGetFileInfoReady:
		{
			GFileInfo* fileInfo = g_file_query_info_finish(mFile, inResult, &error);
			if (fileInfo != nil)
				ProcessFileInfo(fileInfo);
			break;
		}
		
		case eWaitReadReady:
		{
			int32 read = g_input_stream_read_finish(G_INPUT_STREAM(mStream), inResult, &error);
			
			if (read == 0)
			{
				mState = eWaitCloseReady;
				g_input_stream_close_async(G_INPUT_STREAM(mStream), G_PRIORITY_HIGH,
					mCancellable, &MGIOFileLoader::AsyncCallback, this);
				
				MGdkThreadBlock block;
				stringstream in(mBuffer);
				eReadFile(in);
			}
			else
			{
				mBuffer.append(mBufferBlock, read);
				
				mBytesRead += read;
				
				eProgress(float(mBytesRead) / mExpectedFileSize, "Receiving data");
				
				mState = eWaitReadReady;
				g_input_stream_read_async(G_INPUT_STREAM(mStream),
					mBufferBlock, sizeof(mBufferBlock),
					G_PRIORITY_HIGH, mCancellable, &MGIOFileLoader::AsyncCallback, this);
			}
			break;
		}
		
		case eWaitCloseReady:
			g_object_unref(mStream);
			mStream = nil;
			mState = eStateNone;
			break;
		
		default:
		{
			MGdkThreadBlock block;
			eError("Invalid state!");
			mState = eStateError;
		}
	}
	
	if (error != nil)
	{
		MGdkThreadBlock block;
		eError(error->message);
		mState = eStateError;
		
		g_error_free(error);
	}
}

void MGIOFileLoader::ProcessFileInfo(
	GFileInfo*		inFileInfo)
{
	try
	{
		if (g_file_info_has_attribute(inFileInfo, G_FILE_ATTRIBUTE_STANDARD_TYPE) and
			g_file_info_get_file_type(inFileInfo) != G_FILE_TYPE_REGULAR)
		{
			THROW(("Not a regular file"));
		}
		
		int64 size = g_file_info_get_size(inFileInfo);
		if (size > numeric_limits<uint32>::max())
			THROW(("File too large"));
		mExpectedFileSize = size;

		mState = eWaitReadReady;
		g_input_stream_read_async(G_INPUT_STREAM(mStream), mBufferBlock, sizeof(mBufferBlock),
			G_PRIORITY_HIGH, mCancellable, &MGIOFileLoader::AsyncCallback, this);
		
		mReadOnly =
			g_file_info_get_attribute_boolean(inFileInfo, G_FILE_ATTRIBUTE_ACCESS_CAN_WRITE) == false;
		
		GTimeVal tv;
		g_file_info_get_modification_time(inFileInfo, &tv);
		mModDate = tv.tv_sec + tv.tv_usec / 1e6;
	}
	catch (exception& e)
	{
		MGdkThreadBlock block;
		eError(e.what());
		mState = eStateError;
	}
	
	g_object_unref(inFileInfo);
}

// --------------------------------------------------------------------
// MFileSaver, used to save data to a file.

MFileSaver::MFileSaver(
	const MFile&		inFile)
	: mFile(inFile)
{
}

MFileSaver::~MFileSaver()
{
}

// --------------------------------------------------------------------

class MLocalFileSaver : public MFileSaver
{
  public:
					MLocalFileSaver(
						const MFile&			inFile);

	virtual void	DoSave();
};

// --------------------------------------------------------------------

MLocalFileSaver::MLocalFileSaver(
	const MFile&	inFile)
	: MFileSaver(inFile)
{
}

void MLocalFileSaver::DoSave()
{
	fs::ofstream file(mFile.GetPath(), ios::trunc|ios::binary);
	eWriteFile(file);
}

// --------------------------------------------------------------------

class MGIOFileSaver : public MFileSaver
{
  public:
						MGIOFileSaver(
							const MFile&	inFile);
	
						~MGIOFileSaver();

	virtual void		DoSave();

	static void			AsyncCallback(
							GObject*		source_object,
							GAsyncResult*	res,
							gpointer		user_data)
						{
							MGIOFileSaver* self = reinterpret_cast<MGIOFileSaver*>(user_data);
							self->Async(source_object, res);
						}

	void				Async(
							GObject*		inObject,
							GAsyncResult*	inResult);

	void				WriteChunk();

	GCancellable*		mCancellable;
	GFileOutputStream*	mStream;
	string				mBuffer;
	uint32				mOffset;
	
	enum
	{
		eStateNone,
		eStateCancelled,
		eStateError,
		eStateBegin,
		eWaitGetFileInfoReady,
		eWaitAsyncReady,
		eWaitMountReady,
		eWaitWriteReady,
		eWaitCloseReady
	}					mState;
	
	bool				mTriedMount;
};

MGIOFileSaver::MGIOFileSaver(
	const MFile&		inFile)
	: MFileSaver(inFile)
	, mCancellable(nil)
	, mStream(nil)
	, mOffset(0)
	, mTriedMount(false)
{
}

MGIOFileSaver::~MGIOFileSaver()
{
	if (mCancellable != nil)
	{
		g_cancellable_cancel(mCancellable);
		g_object_unref(mCancellable);
	}
	
	if (mStream != nil)
		g_object_unref(mStream);
}

void MGIOFileSaver::DoSave()
{
	eProgress(0.f, "start");

	mCancellable = g_cancellable_new();

	mState = eWaitGetFileInfoReady;
	g_file_query_info_async(mFile, G_FILE_ATTRIBUTE_TIME_MODIFIED,
		G_FILE_QUERY_INFO_NONE, G_PRIORITY_HIGH, mCancellable,
		&MGIOFileSaver::AsyncCallback, this);
}

void MGIOFileSaver::Async(
	GObject*		inSourceObject,
	GAsyncResult*	inResult)
{
	if (g_cancellable_is_cancelled(mCancellable))
	{
		mState = eStateCancelled;
		return;
	}
	
	GError* error = nil;
	
	switch (mState)
	{
		case eWaitGetFileInfoReady:
		{
			bool doSave = true;
			GFileInfo* fileInfo = g_file_query_info_finish(mFile, inResult, &error);
			if (fileInfo == nil)
			{
				doSave = false;
				
				// maybe we need to mount?
				if (error->code == G_IO_ERROR_NOT_MOUNTED and not mTriedMount)
				{
					g_error_free(error);
					error = nil;
					mTriedMount = true;
		
					GMountOperation* mountOperation = g_mount_operation_new();
					
					mState = eWaitMountReady;
					g_file_mount_enclosing_volume(mFile, G_MOUNT_MOUNT_NONE,
						mountOperation, mCancellable, &MGIOFileSaver::AsyncCallback, this);
		
					g_object_unref(mountOperation);
				}
				
				// file does not exist yet, not an error
				if (error->code == G_IO_ERROR_NOT_FOUND)
				{
					g_error_free(error);
					error = nil;
					
					doSave = true;
				}
			}
			else
			{
				// check modification time
				GTimeVal tv;
				g_file_info_get_modification_time(fileInfo, &tv);
				double modDate = tv.tv_sec + tv.tv_usec / 1e6;
				
//				doSave = modDate <= mFile.GetModDate() or
//					DisplayAlert("") == 1;
			}
			
			if (doSave)
			{
				mState = eWaitAsyncReady;
				g_file_replace_async(mFile, nil, false, G_FILE_CREATE_NONE,
					G_PRIORITY_HIGH, mCancellable, &MGIOFileSaver::AsyncCallback, this);
			}
			break;
		}
		
		case eWaitMountReady:
			if (g_file_mount_enclosing_volume_finish(mFile, inResult, &error))
			{
				mState = eWaitGetFileInfoReady;
				g_file_query_info_async(mFile, G_FILE_ATTRIBUTE_TIME_MODIFIED,
					G_FILE_QUERY_INFO_NONE, G_PRIORITY_HIGH, mCancellable,
					&MGIOFileSaver::AsyncCallback, this);
			}
			break;
		
		case eWaitAsyncReady:
			mStream = g_file_replace_finish(mFile, inResult, &error);
			if (mStream != nil)
			{
				MGdkThreadBlock block;
				
				io::filtering_ostream out(io::back_inserter(mBuffer));
				eWriteFile(out);

				WriteChunk();
			}
			break;
		
		case eWaitWriteReady:
		{
			int32 written = g_output_stream_write_finish(G_OUTPUT_STREAM(mStream), inResult, &error);
			if (written >= 0)
			{
				MGdkThreadBlock block;

				mOffset += written;
				eProgress(float(mOffset) / mBuffer.length(), "Writing data");
				
				if (mOffset < mBuffer.length())
					WriteChunk();
				else
				{
					mState = eWaitCloseReady;
					g_output_stream_close_async(G_OUTPUT_STREAM(mStream),
						G_PRIORITY_HIGH, mCancellable, &MGIOFileSaver::AsyncCallback, this);
				}
			}
			break;
		}
		
		case eWaitCloseReady:
			mState = eStateNone;
			g_output_stream_close_finish(G_OUTPUT_STREAM(mStream), inResult, &error);
			g_object_unref(mStream);
			mStream = nil;
			break;
		
		default:
		{
			MGdkThreadBlock block;
			eError("Invalid state!");
			mState = eStateError;
		}
	}
	
	if (error != nil)
	{
		MGdkThreadBlock block;
		eError(error->message);
		mState = eStateError;
		
		g_error_free(error);
	}
}
			
void MGIOFileSaver::WriteChunk()
{
	const uint32		kBlockSize = 4096;

	uint32 k = mBuffer.length() - mOffset;
	if (k > kBlockSize)
		k = kBlockSize;
	
	mState = eWaitWriteReady;
	g_output_stream_write_async(G_OUTPUT_STREAM(mStream),
		mBuffer.c_str() + mOffset, k,
		G_PRIORITY_HIGH, mCancellable, &MGIOFileSaver::AsyncCallback, this);
}

// --------------------------------------------------------------------
// MFile, something like a path or URI. 

MFile::MFile()
	: mFile(nil)
	, mLoaded(false)
	, mReadOnly(false)
	, mModDate(0)
{
}

MFile::MFile(
	const MFile&		rhs)
	: mFile(nil)
	, mLoaded(rhs.mLoaded)
	, mReadOnly(rhs.mReadOnly)
	, mModDate(rhs.mModDate)
{
	if (rhs.mFile != nil)
		mFile = static_cast<GFile*>(g_object_ref(rhs.mFile));
}

MFile::MFile(
	const fs::path&		inPath)
	: mFile(g_file_new_for_path(inPath.string().c_str()))
	, mLoaded(false)
	, mReadOnly(false)
	, mModDate(0)
{
}

MFile::MFile(
	const char*			inURI)
	: mFile(g_file_new_for_uri(inURI))
	, mLoaded(false)
	, mReadOnly(false)
	, mModDate(0)
{
}

MFile::MFile(
	const string&		inURI)
	: mFile(g_file_new_for_uri(inURI.c_str()))
	, mLoaded(false)
	, mReadOnly(false)
	, mModDate(0)
{
}

MFile::MFile(
	GFile*				inFile)
	: mFile(inFile)
	, mLoaded(false)
	, mReadOnly(false)
	, mModDate(0)
{
}

MFile& MFile::operator=(
	const MFile&		rhs)
{
	if (mFile != nil)
		g_object_unref(mFile);
	
	mFile = rhs.mFile;
	
	if (mFile != nil)
		mFile = static_cast<GFile*>(g_object_ref(mFile));

	mLoaded = rhs.mLoaded;
	mReadOnly = rhs.mReadOnly;
	mModDate = rhs.mModDate;

	return *this;
}

MFile& MFile::operator=(
	GFile*				rhs)
{
	if (mFile != nil)
		g_object_unref(mFile);
	
	mFile = rhs;
	
	if (mFile != nil)
		mFile = static_cast<GFile*>(g_object_ref(mFile));

	mLoaded = false;
	mReadOnly = false;
	mModDate = 0;

	return *this;
}

MFile& MFile::operator=(
	const fs::path&		rhs)
{
	if (mFile != nil)
		g_object_unref(mFile);
	
	mFile = g_file_new_for_path(rhs.string().c_str());

	mLoaded = false;
	mReadOnly = false;
	mModDate = 0;

	return *this;
}

bool MFile::operator==(
	const MFile&		rhs) const
{
	return g_file_equal(mFile, rhs.mFile);
}

bool MFile::operator!=(
	const MFile&		rhs) const
{
	return g_file_equal(mFile, rhs.mFile) == false;
}

MFile& MFile::operator/=(
	const char*			inSubPath)
{
	assert(strchr(inSubPath, '/') == nil);
	GFile* child = g_file_get_child(mFile, inSubPath);
	g_object_unref(mFile);
	mFile = child;

	return *this;
}

MFile& MFile::operator/=(
	const fs::path&		inSubPath)
{
	for (fs::path::iterator p = inSubPath.begin(); p != inSubPath.end(); ++p)
	{
		GFile* child = g_file_get_child(mFile, p->c_str());
		g_object_unref(mFile);
		mFile = child;
	}

	return *this;
}

fs::path MFile::GetPath() const
{
	if (mFile == nil)
		THROW(("Runtime error, file not specified"));
	
	char* path = g_file_get_path(mFile);
	if (path == nil)
		THROW(("Error getting path from file"));
	
	fs::path result(path);
	g_free(path);
	return result;
}

string MFile::GetURI() const
{
	char* uri = g_file_get_uri(mFile);
	if (uri == nil)
		THROW(("Failed to get uri"));

	string result(uri);
	g_free(uri);
	return result;
}

string MFile::GetScheme() const
{
	char* scheme = g_file_get_uri_scheme(mFile);
	if (scheme == nil)
		THROW(("Failed to get scheme"));

	string result(scheme);
	g_free(scheme);
	return result;
}

string MFile::GetFileName() const
{
	char* name = g_file_get_basename(mFile);
	if (name == nil)
		THROW(("Failed to get basename"));

	string result(name);
	g_free(name);
	return result;
}

MFile MFile::GetParent() const
{
	return MFile(g_file_get_parent(mFile));
}

MFile::operator GFile*() const
{
	return const_cast<GFile*>(mFile);
}

MFileLoader* MFile::Load() const
{
	MFileLoader* result;

	if (IsLocal())
		result = new MLocalFileLoader(*this);
	else
		result = new MGIOFileLoader(*this);
	
	return result;
}

MFileSaver* MFile::Save() const
{
	MFileSaver* result;

	if (IsLocal())
		result = new MLocalFileSaver(*this);
	else
		result = new MGIOFileSaver(*this);
	
	return result;
}

bool MFile::IsValid() const
{
	return mFile != nil;
}

bool MFile::IsLocal() const
{
	return g_file_has_uri_scheme(mFile, "file");
}

bool MFile::Exists() const
{
	assert(IsLocal());
	return IsLocal() and fs::exists(GetPath());
}

double MFile::GetModDate() const
{
//	if (not mLoaded)
//		THROW(("Runtime error, file not loaded yet"));
	
	return mModDate;
}

bool MFile::ReadOnly() const
{
//	if (mFile != nil and not mLoaded)
//		THROW(("Runtime error, file not loaded yet"));
	
	return mReadOnly;
}

void MFile::ReadAttribute(
	const char*			inName,
	std::string&		outData)
{
}

void MFile::WriteAttribute(
	const char*			inName,
	const std::string&	inData)
{
}

MFile operator/(const MFile& lhs, const fs::path& rhs)
{
	MFile result(lhs);
	result /= rhs;
	return result;
}

fs::path RelativePath(const MFile& lhs, const MFile& rhs)
{
	char* p = g_file_get_relative_path(lhs.mFile, rhs.mFile);
	if (p == nil)
		THROW(("Failed to get relative path"));
	
	fs::path result(p);
	g_free(p);
	return result;
}

ostream& operator<<(ostream& lhs, const MFile& rhs)
{
	lhs << rhs.GetURI();
	return lhs;
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

