//
//	This is a GIO wrapper.
//

#include "MJapi.h"

#include <limits>

#include "MError.h"
#include "MFileLoader.h"

using namespace std;

#define REMOTE_QUERY_ATTRIBUTES G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE "," \
				G_FILE_ATTRIBUTE_STANDARD_TYPE "," \
				G_FILE_ATTRIBUTE_TIME_MODIFIED "," \
				G_FILE_ATTRIBUTE_STANDARD_SIZE "," \
				G_FILE_ATTRIBUTE_ACCESS_CAN_WRITE

struct MFileLoaderImpl
{
						MFileLoaderImpl(
							MFileLoader*	inLoader,
							const char*		inURI);
	
						~MFileLoaderImpl();
	
	static void			AsyncReadyCallback(
							GObject*		source_object,
							GAsyncResult*	res,
							gpointer		user_data)
						{
							MFileLoaderImpl* self = reinterpret_cast<MFileLoaderImpl*>(user_data);
							self->AsyncReady(source_object, res);
						}

	void				AsyncReady(
							GObject*		inSourceObject,
							GAsyncResult*	inResult);

	static void			RemoteGetInfoCallback(
							GObject*		source_object,
							GAsyncResult*	res,
							gpointer		user_data)
						{
							MFileLoaderImpl* self = reinterpret_cast<MFileLoaderImpl*>(user_data);
							self->RemoteGetInfo(source_object, res);
						}

	void				RemoteGetInfo(
							GObject*		inSourceObject,
							GAsyncResult*	inResult);

	static void			RemoteGetFileInfoCallback(
							GObject*		source_object,
							GAsyncResult*	res,
							gpointer		user_data)
						{
							MFileLoaderImpl* self = reinterpret_cast<MFileLoaderImpl*>(user_data);
							self->RemoteGetFileInfo(source_object, res);
						}

	void				RemoteGetFileInfo(
							GObject*		inSourceObject,
							GAsyncResult*	inResult);

	void				FinishQueryInfo();
	
	void				ReadChunk();

	static void			AsyncReadCallback(
							GObject*		source_object,
							GAsyncResult*	res,
							gpointer		user_data)
						{
							MFileLoaderImpl* self = reinterpret_cast<MFileLoaderImpl*>(user_data);
							self->AsyncRead(source_object, res);
						}

	void				AsyncRead(
							GObject*		inSourceObject,
							GAsyncResult*	inResult);

	MFileLoader*		mLoader;
	GFile*				mFile;
	GFileInfo*			mFileInfo;
	uint32				mBytesRead;
	uint32				mExpectedFileSize;
	GCancellable*		mCancellable;
	GFileInputStream*	mStream;
	string				mBuffer;
	GError*				mError;
	
	char				mBufferBlock[4096];		// read 4 k blocks
	
	bool				mTriedMount;
};

MFileLoaderImpl::MFileLoaderImpl(
	MFileLoader*		inLoader,
	const char*			inURI)
	: mLoader(inLoader)
	, mFile(nil)
	, mFileInfo(nil)
	, mBytesRead(0)
	, mCancellable(nil)
	, mStream(nil)
	, mError(nil)
	, mTriedMount(false)
{
	mFile = g_file_new_for_uri(inURI);
	
	mLoader->eProgress(0.f);

	mCancellable = g_cancellable_new();
	
	g_file_read_async(mFile, G_PRIORITY_HIGH, mCancellable,
		&MFileLoaderImpl::AsyncReadyCallback, this);
}

MFileLoaderImpl::~MFileLoaderImpl()
{
	if (mCancellable != nil)
	{
		g_cancellable_cancel(mCancellable);
		g_object_unref(mCancellable);
	}
	
	if (mFileInfo != nil)
		g_object_unref(mFileInfo);
	
	if (mStream != nil)
		g_object_unref(mStream);
	
	if (mFile != nil)
		g_object_unref(mFile);
	
	if (mError != nil)
		g_error_free(mError);
}

void MFileLoaderImpl::AsyncReady(
	GObject*		inSourceObject,
	GAsyncResult*	inResult)
{
	if (g_cancellable_is_cancelled(mCancellable))
		return;
	
	mStream = g_file_read_finish(mFile, inResult, &mError);
	
	// maybe we need to mount?
	if (mStream == nil)
	{
		if (mError != nil)
		{
			mLoader->eError(mError->message);
			g_error_free(mError);
			mError = nil;
		}
		else
			mLoader->eError("Unknown error reading file");
	}
	else
		g_file_input_stream_query_info_async(mStream,
			const_cast<char*>(REMOTE_QUERY_ATTRIBUTES), G_PRIORITY_HIGH,
			mCancellable, &MFileLoaderImpl::RemoteGetInfoCallback, this);
}

void MFileLoaderImpl::RemoteGetInfo(
	GObject*		inSourceObject,
	GAsyncResult*	inResult)
{
	if (g_cancellable_is_cancelled(mCancellable))
		return;
	
	mFileInfo = g_file_input_stream_query_info_finish(mStream, inResult, &mError);
	
	if (mFileInfo == nil)
	{
		if (mError and mError->code == G_IO_ERROR_NOT_SUPPORTED)
		{
			g_file_query_info_async(mFile, REMOTE_QUERY_ATTRIBUTES,
				G_FILE_QUERY_INFO_NONE, G_PRIORITY_HIGH,
				mCancellable, &MFileLoaderImpl::RemoteGetFileInfoCallback, this);
		}
		else if (mError != nil)
			mLoader->eError(mError->message);
		else
			mLoader->eError("Unknown error retrieving information");
	}
	else
		FinishQueryInfo();
}

void MFileLoaderImpl::RemoteGetFileInfo(
	GObject*		inSourceObject,
	GAsyncResult*	inResult)
{
	if (g_cancellable_is_cancelled(mCancellable))
		return;
	
	mFileInfo = g_file_query_info_finish(mFile, inResult, &mError);
	
	if (mFileInfo == nil)
	{
		if (mError != nil)
			mLoader->eError(mError->message);
		else
			mLoader->eError("Unknown error retrieving information");
	}
	else
		FinishQueryInfo();
}

void MFileLoaderImpl::FinishQueryInfo()
{
	if (g_file_info_has_attribute(mFileInfo, G_FILE_ATTRIBUTE_STANDARD_TYPE) and
		g_file_info_get_file_type(mFileInfo) != G_FILE_TYPE_REGULAR)
	{
		mLoader->eError("Not a regular file");
	}
	else
	{
		int64 size = g_file_info_get_size(mFileInfo);
		if (size > numeric_limits<uint32>::max())
			mLoader->eError("File is too large to load");
		else
		{
			mExpectedFileSize = size;
			ReadChunk();
		}
	}
}

void MFileLoaderImpl::ReadChunk()
{
	g_input_stream_read_async(G_INPUT_STREAM(mStream),
		mBufferBlock, sizeof(mBufferBlock),
		G_PRIORITY_HIGH, mCancellable, &MFileLoaderImpl::AsyncReadCallback, this);
}

void MFileLoaderImpl::AsyncRead(
	GObject*		inSourceObject,
	GAsyncResult*	inResult)
{
	if (g_cancellable_is_cancelled(mCancellable))
	{
		g_input_stream_close_async(G_INPUT_STREAM(mStream), G_PRIORITY_HIGH, nil, nil, nil);
		return;
	}
	
	int32 read = g_input_stream_read_finish(G_INPUT_STREAM(mStream), inResult, &mError);
	
	if (read < 0)
		mLoader->eError(mError->message);
	else if (read == 0)
		mLoader->eLoaded(mBuffer.c_str(), mBuffer.length());
	else
	{
		mBuffer.append(mBufferBlock, read);
		
		mBytesRead += read;
		
		mLoader->eProgress(float(mBytesRead) / mExpectedFileSize);
		
		ReadChunk();
	}
}

// --------------------------------------------------------------------

MFileLoader::MFileLoader(
	const char*		inURI)
	: mImpl(new MFileLoaderImpl(this, inURI))
{
}

MFileLoader::~MFileLoader()
{
	delete mImpl;
}
