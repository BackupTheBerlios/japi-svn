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
#include <boost/algorithm/string.hpp>

#include "MFile.h"
#include "MError.h"
#include "MUnicode.h"
#include "MUtils.h"
#include "MStrings.h"
#include "MJapiApp.h"

using namespace std;
namespace io = boost::iostreams;
namespace ba = boost::algorithm;

namespace {

const char kRemoteQueryAttributes[] = \
	G_FILE_ATTRIBUTE_STANDARD_SIZE "," \
	G_FILE_ATTRIBUTE_STANDARD_TYPE "," \
	G_FILE_ATTRIBUTE_TIME_MODIFIED "," \
	G_FILE_ATTRIBUTE_ACCESS_CAN_WRITE;

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
		THROW(("File does not exist")); 
	
	double modTime = fs::last_write_time(path);
	bool readOnly = false;
	
	fs::ifstream file(path);
	eReadFile(file);
	
	SetFileInfo(readOnly, modTime);
	
	eFileLoaded();
	
	delete this;
}

// --------------------------------------------------------------------

class MGIOFileLoader : public MFileLoader
{
  public:
						MGIOFileLoader(
							MFile&			inFile);
	
						~MGIOFileLoader();

	virtual void		DoLoad();
	
	virtual void		Cancel();
	
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
	
	char				mBufferBlock[4096];		// read 4 k blocks
	
	enum ELoadingState
	{
		eStateNone,
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
	MFile&				inFile)
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

void MGIOFileLoader::Cancel()
{
	if (mCancellable != nil)
		g_cancellable_cancel(mCancellable);
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
		delete this;
		return;
	}
	
	GError* error = nil;
	
	switch (mState)
	{
		case eStateNone:
			assert(false);
		
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
			
			eFileLoaded();

			delete this;
			break;
	}
	
	if (error != nil)
	{
		MGdkThreadBlock block;
		eError(error->message);
		
		g_error_free(error);
		
		delete this;
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
		
		bool readOnly =
			g_file_info_has_attribute(inFileInfo, G_FILE_ATTRIBUTE_ACCESS_CAN_WRITE) and
			g_file_info_get_attribute_boolean(inFileInfo, G_FILE_ATTRIBUTE_ACCESS_CAN_WRITE) == false;
		
		GTimeVal tv;
		g_file_info_get_modification_time(inFileInfo, &tv);
		double modDate = tv.tv_sec + tv.tv_usec / 1e6;
		
		SetFileInfo(readOnly, modDate);
	}
	catch (exception& e)
	{
		MGdkThreadBlock block;
		eError(e.what());
		
		delete this;
	}
	
	g_object_unref(inFileInfo);
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

class MGIOFileSaver : public MFileSaver
{
  public:
						MGIOFileSaver(
							MFile&			inFile);
	
						~MGIOFileSaver();

	virtual void		DoSave();
	
	virtual void		Cancel();

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
		eWaitGetFileInfoReady,
		eWaitAsyncReady,
		eWaitMountReady,
		eWaitWriteReady,
		eWaitCloseReady,
		eWaitGetFileInfoReady2
	}					mState;
	
	bool				mTriedMount;
};

MGIOFileSaver::MGIOFileSaver(
	MFile&				inFile)
	: MFileSaver(inFile)
	, mCancellable(nil)
	, mStream(nil)
	, mOffset(0)
	, mState(eStateNone)
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

void MGIOFileSaver::Cancel()
{
	if (mCancellable != nil)
		g_cancellable_cancel(mCancellable);
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
		delete this;
		return;
	}
	
	GError* error = nil;
	
	switch (mState)
	{
		case eStateNone:
			assert(false);
		
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
				
				if (modDate > mFile.GetModDate())
				{
					MGdkThreadBlock block;
					doSave = eAskOverwriteNewer();
				}
				else
					doSave = true;
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
			if (g_output_stream_close_finish(G_OUTPUT_STREAM(mStream), inResult, &error))
			{
				mState = eWaitGetFileInfoReady2;
				g_file_query_info_async(mFile, G_FILE_ATTRIBUTE_TIME_MODIFIED,
					G_FILE_QUERY_INFO_NONE, G_PRIORITY_HIGH, mCancellable,
					&MGIOFileSaver::AsyncCallback, this);
			}
			g_object_unref(mStream);
			mStream = nil;
			break;
		
		case eWaitGetFileInfoReady2:
		{
			GFileInfo* fileInfo = g_file_query_info_finish(mFile, inResult, &error);
			if (fileInfo != nil)
			{
				GTimeVal tv;
				g_file_info_get_modification_time(fileInfo, &tv);
				double modDate = tv.tv_sec + tv.tv_usec / 1e6;
				
				SetFileInfo(false, modDate);
				eFileWritten();
			
				delete this;
			}
			break;
		}
	}
	
	if (error != nil)
	{
		MGdkThreadBlock block;
		eError(error->message);
		
		g_error_free(error);
		
		delete this;
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

// first we start with two implementations of the Impl

#include "MFileImp.h"

struct MGFileImp : public MFileImp
{
						MGFileImp(GFile* inFile) : mGFile(inFile) {}

						~MGFileImp()
						{
							g_object_unref(mGFile);
						}
	
	virtual bool		Equivalent(const MFileImp* rhs) const
						{
							const MGFileImp* grhs = dynamic_cast<const MGFileImp*>(rhs);
							return grhs != nil and g_file_equal(mGFile, grhs->mGFile);
						}
							
	virtual string		GetURI() const
						{
							string result;
							char* s = g_file_get_uri(mGFile);
							if (s != nil)
							{
								result = s;
								g_free(s);
							}
							return result;
						}
						
	virtual fs::path	GetPath() const
						{
							string result;
							char* s = g_file_get_path(mGFile);
							if (s != nil)
							{
								result = s;
								g_free(s);
							}
							return result;
						}
							
	virtual string		GetScheme() const
						{
							string result;
							char* s = g_file_get_uri_scheme(mGFile);
							if (s != nil)
							{
								result = s;
								g_free(s);
							}
							return result;
						}
							
	virtual string		GetFileName() const
						{
							string result;
							char* s = g_file_get_basename(mGFile);
							if (s != nil)
							{
								result = s;
								g_free(s);
							}
							return result;
						}
							
	virtual bool		IsLocal() const
						{
							return false;
						}
							
	virtual MFileImp*	GetParent() const
						{
							return new MGFileImp(g_file_get_parent(mGFile));
						}
	
	virtual MFileImp*	GetChild(const fs::path& inSubPath) const
						{
							GFile* file = G_FILE(g_object_ref(mGFile));
							
							for (fs::path::iterator p = inSubPath.begin(); p != inSubPath.end(); ++p)
							{
								GFile* child = g_file_get_child(mGFile, p->c_str());
								g_object_unref(file);
								file = child;
							}
							
							return new MGFileImp(file);
						}
	
	virtual MFileLoader* Load(MFile& inFile)
						{
							return new MGIOFileLoader(inFile);
						}
							
	virtual MFileSaver*	Save(MFile& inFile)
						{
							return new MGIOFileSaver(inFile);
						}

	GFile*				mGFile;					
};

struct MPathImp : public MFileImp
{
							MPathImp(const fs::path& inPath) : mPath(inPath) {}

	virtual bool			Equivalent(const MFileImp* rhs) const
							{
								const MPathImp* prhs = dynamic_cast<const MPathImp*>(rhs);
								return prhs != nil and fs::equivalent(mPath, prhs->mPath);
							}
							
	virtual std::string		GetURI() const
							{
								return GetScheme() + "://" + mPath.string();
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

MFile::MFile(
	const char*			inURI)
	: mImpl(nil)
	, mReadOnly(false)
	, mModDate(0)
{
	if (strncmp(inURI, "file://", 7) == 0)
		mImpl = new MPathImp(fs::system_complete(inURI + 7));
	else
	{
		GFile* file = g_file_new_for_uri(inURI);
		if (g_file_has_uri_scheme(file, "file"))
		{
			fs::path path(g_file_get_path(file));
			mImpl = new MPathImp(path);
			g_object_unref(file);
		}
		else
			mImpl = new MGFileImp(file);
	}
}

MFile::MFile(
	const string&		inURI)
	: mImpl(nil)
	, mReadOnly(false)
	, mModDate(0)
{
	if (ba::starts_with(inURI, "file://"))
		mImpl = new MPathImp(fs::system_complete(inURI.substr(7)));
	else
	{
		GFile* file = g_file_new_for_uri(inURI.c_str());
		if (g_file_has_uri_scheme(file, "file"))
		{
			fs::path path(g_file_get_path(file));
			mImpl = new MPathImp(path);
			g_object_unref(file);
		}
		else
			mImpl = new MGFileImp(file);
	}
}

MFile::MFile(
	GFile*				inFile)
	: mImpl(new MGFileImp(inFile))
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
	GFile*				rhs)
{
	if (mImpl != nil)
		mImpl->Release();
	
	if (rhs == nil)
		mImpl = nil;
	else
		mImpl = new MGFileImp(rhs);

	mReadOnly = false;
	mModDate = 0;

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
	return mImpl->GetPath();
}

string MFile::GetURI() const
{
	return mImpl->GetURI();
}

string MFile::GetScheme() const
{
	return mImpl->GetScheme();
}

string MFile::GetFileName() const
{
	return mImpl->GetFileName();
}

MFile MFile::GetParent() const
{
	return MFile(mImpl->GetParent());
}

MFile::operator GFile*() const
{
	const MGFileImp* imp = dynamic_cast<const MGFileImp*>(mImpl);
	THROW_IF_NIL(imp);
	return imp->mGFile;
}

MFileLoader* MFile::Load()
{
	return mImpl->Load(*this);
}

MFileSaver* MFile::Save()
{
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

