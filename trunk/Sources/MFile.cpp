/*
	Copyright (c) 2006, Maarten L. Hekkelman
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


#include "MJapieG.h"

#include <unistd.h>
#include <sys/xattr.h>
//#include <sys/syslimits.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stack>
#include <fstream>
#include <cassert>
#include <cerrno>

#include "MFile.h"
#include "MError.h"
#include "MUnicode.h"
#include "MUtils.h"
//#include "MGlobals.h"

using namespace std;

ssize_t read_attribute(const MPath& inPath, const char* inName, void* outData, size_t inDataSize)
{
#if defined(XATTR_FINDERINFO_NAME)
	return ::getxattr(inPath.string().c_str(), inName, outData, inDataSize, 0, 0);
#else
	return ::getxattr(inPath.string().c_str(), inName, outData, inDataSize);
#endif
}

void write_attribute(const MPath& inPath, const char* inName, const void* inData, size_t inDataSize)
{
#if defined(XATTR_FINDERINFO_NAME)
	(void)::setxattr(inPath.string().c_str(), inName, inData, inDataSize, 0, 0);
#else
	(void)::setxattr(inPath.string().c_str(), inName, inData, inDataSize, 0);
#endif
}

// ------------------------------------------------------------------
//
//  MFileSystem
//

namespace MFileSystem
{

double GetModificationDate(
	const MPath&		inURL)
{
	return 0;
}

bool Exists(
	const MPath&		inURL)
{
	struct stat st;
	return stat(inURL.string().c_str(), &st) >= 0;
}

}

// ------------------------------------------------------------------
//
//  MFile
//

MFile::MFile(const MPath& inFileSpec, int inFD)
	: mFileSpec(inFileSpec)
	, mFD(inFD)
	, mIsOpen(inFD != -1)
{
}

MFile::~MFile()
{
	if (IsOpen())
		Close();
}

void MFile::GetFileSpec(MPath& outFileSpec) const
{
	outFileSpec = mFileSpec;
}

void MFile::Open(int inPermissions)
{
	mFD = open(mFileSpec.string().c_str(), inPermissions, 0644);
	THROW_IF_POSIX_ERROR(mFD);
	mIsOpen = true;
}

void MFile::Close()
{
	if (IsOpen())
	{
		close(mFD);
		mIsOpen = false;
	}
}

int32 MFile::Read(void* inBuffer, int32 inByteCount)
{
	if (not IsOpen())
		THROW(("File is not open"));
	
	return ::read(mFD, inBuffer, inByteCount);
}

int32 MFile::Write(const void* inBuffer, int32 inByteCount)
{
	if (not IsOpen())
		THROW(("File is not open"));
	
	return ::write(mFD, inBuffer, inByteCount);
}

int64 MFile::Seek(int64 inOffset, int inMode)
{
	return ::lseek(mFD, inOffset, inMode);
}

int64 MFile::GetSize() const
{
	int64 result = -1;
	
	struct stat st;
	if (stat(mFileSpec.string().c_str(), &st) >= 0)
		result = st.st_size;
	
	return result;
}

void MFile::SetSize(int64 inSize)
{
	THROW_IF_POSIX_ERROR(::ftruncate(mFD, inSize));
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

//bool FileNameMatches(
//	const char*		inPattern,
//	const MPath&		inFile)
//{
//	assert(false);
////	return FileNameMatches(inPattern, inFile.leaf());
//}
//
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

MSafeSaver::MSafeSaver(const MPath& inFile)
	: mDestFileSpec(inFile)
{
	struct stat st;
	
	if (stat(mDestFileSpec.string().c_str(), &st) == 0)
	{
		string path = mDestFileSpec.string() + "-XXXXXX";
		
		int fd = mkstemp(const_cast<char*>(path.c_str()));
		THROW_IF_POSIX_ERROR(fd);
		
		mTempFileSpec = path;
		mTempFile.reset(new MFile(mTempFileSpec, fd));
	}
	else
	{
		mTempFile.reset(new MFile(mDestFileSpec));
		
		mTempFile->GetFileSpec(mTempFileSpec);
		mDestFileSpec = mTempFileSpec;

		mTempFile->Open(O_RDWR | O_TRUNC | O_CREAT);
	}
}

MSafeSaver::~MSafeSaver()
{
	// if commit wasn't called...
	if (mTempFile.get() != nil and not (mDestFileSpec == mTempFileSpec))
	{
		try
		{
			mTempFile->Close();
			remove(mTempFileSpec.string().c_str());
		}
		catch (...) {}
	}
}

void MSafeSaver::Commit(MPath& outFileSpec)
{
	mTempFile->Close();
	mTempFile.reset(nil);

	if (not (mDestFileSpec == mTempFileSpec))
	{
		THROW_IF_POSIX_ERROR(remove(mDestFileSpec.string().c_str()));
		THROW_IF_POSIX_ERROR(
			rename(mTempFileSpec.string().c_str(), mDestFileSpec.string().c_str()));
	}

	outFileSpec = mDestFileSpec;
}

MFile* MSafeSaver::GetTempFile()
{
	return mTempFile.get();
}

//// ------------------------------------------------------------
//
//struct MFileIteratorImp
//{
//	struct MInfo
//	{
//		MPath			mParent;
//		DIR*			mDIR;
//		struct dirent	mEntry;
//	};
//	
//						MFileIteratorImp()
//							: mOnlyTEXT(false)
//							, mReturnDirs(false) {}
//	virtual				~MFileIteratorImp() {}
//	
//	virtual	bool		Next(MPath& outFile) = 0;
//	bool				IsTEXT(const MPath& inFile);
//	
//	string				mFilter;
//	bool				mOnlyTEXT;
//	bool				mReturnDirs;
//};
//
//bool MFileIteratorImp::IsTEXT(const MPath& inFile)
//{
//	bool result = false;
//	
//	FSRef ref;
//	LSItemInfoRecord outInfo = { };
//	
//	if (FSPathMakeRef(inFile, ref) == noErr and
//		::LSCopyItemInfoForRef(&ref, kLSRequestExtension | kLSRequestTypeCreator, &outInfo) == noErr)
//	{
//		CFStringRef itemUTI = nil;
//		if (outInfo.extension != nil)
//		{
//			itemUTI = ::UTTypeCreatePreferredIdentifierForTag(
//				kUTTagClassFilenameExtension, outInfo.extension, nil);
//			::CFRelease(outInfo.extension);
//		}
//		else
//		{
//			CFStringRef typeString = ::UTCreateStringForOSType(outInfo.filetype);
//			itemUTI = ::UTTypeCreatePreferredIdentifierForTag(
//				kUTTagClassFilenameExtension, typeString, NULL);
//			::CFRelease(typeString);
//		}
//
//		if (itemUTI != nil)
//		{
//			result = ::UTTypeConformsTo(itemUTI, CFSTR("public.text"));
//			::CFRelease(itemUTI);
//		}
//	}
//
//	return result;
//}
//
//struct MSingleFileIteratorImp : public MFileIteratorImp
//{
//						MSingleFileIteratorImp(const MPath& inDirectory);
//	virtual				~MSingleFileIteratorImp();
//	
//	virtual	bool		Next(MPath& outFile);
//	
//	MInfo				mInfo;
//};
//
//
//MSingleFileIteratorImp::MSingleFileIteratorImp(const MPath& inDirectory)
//{
//	mInfo.mParent = inDirectory;
//	mInfo.mDIR = opendir(inDirectory.string().c_str());
//	memset(&mInfo.mEntry, 0, sizeof(mInfo.mEntry));
//}
//
//MSingleFileIteratorImp::~MSingleFileIteratorImp()
//{
//	if (mInfo.mDIR != nil)
//		closedir(mInfo.mDIR);
//}
//
//bool MSingleFileIteratorImp::Next(MPath& outFile)
//{
//	bool result = false;
//	
//	while (not result)
//	{
//		struct dirent* e = nil;
//		
//		if (mInfo.mDIR != nil)
//			THROW_IF_POSIX_ERROR(::readdir_r(mInfo.mDIR, &mInfo.mEntry, &e));
//		
//		if (e == nil)
//			break;
//
//		if (strcmp(e->d_name, ".") == 0 or strcmp(e->d_name, "..") == 0)
//			continue;
//
//		outFile = mInfo.mParent / e->d_name;
//
////		struct stat statb;
////		
////		if (stat(outFile.string().c_str(), &statb) != 0)
////			continue;
////		
////		if (S_ISDIR(statb.st_mode) != mReturnDirs)
////			continue;
//
//		if (is_directory(outFile) and not mReturnDirs)
//			continue;
//		
//		if (mOnlyTEXT and not IsTEXT(outFile))
//			continue;
//		
//		if (mFilter.length() == 0 or
//			FileNameMatches(mFilter.c_str(), outFile))
//		{
//			result = true;
//		}
//	}
//	
//	return result;
//}
//
//struct MDeepFileIteratorImp : public MFileIteratorImp
//{
//						MDeepFileIteratorImp(const MPath& inDirectory);
//	virtual				~MDeepFileIteratorImp();
//
//	virtual	bool		Next(MPath& outFile);
//
//	stack<MInfo>		mStack;
//};
//
//MDeepFileIteratorImp::MDeepFileIteratorImp(const MPath& inDirectory)
//{
//	MInfo info;
//
//	info.mParent = inDirectory;
//	info.mDIR = opendir(inDirectory.string().c_str());
//	memset(&info.mEntry, 0, sizeof(info.mEntry));
//	
//	mStack.push(info);
//}
//
//MDeepFileIteratorImp::~MDeepFileIteratorImp()
//{
//	while (not mStack.empty())
//	{
//		closedir(mStack.top().mDIR);
//		mStack.pop();
//	}
//}
//
//bool MDeepFileIteratorImp::Next(MPath& outFile)
//{
//	bool result = false;
//	
//	while (not result and not mStack.empty())
//	{
//		struct dirent* e = nil;
//		
//		MInfo& top = mStack.top();
//		
//		if (top.mDIR != nil)
//			THROW_IF_POSIX_ERROR(::readdir_r(top.mDIR, &top.mEntry, &e));
//		
//		if (e == nil)
//		{
//			if (top.mDIR != nil)
//				closedir(top.mDIR);
//			mStack.pop();
//		}
//		else
//		{
//			outFile = top.mParent / e->d_name;
//			
//			if (exists(outFile) and is_directory(outFile))
//			{
//				if (strcmp(e->d_name, ".") and strcmp(e->d_name, ".."))
//				{
//					MInfo info;
//	
//					info.mParent = outFile;
//					info.mDIR = opendir(outFile.string().c_str());
//					memset(&info.mEntry, 0, sizeof(info.mEntry));
//					
//					mStack.push(info);
//				}
//				continue;
//			}
//
//			if (mOnlyTEXT and not IsTEXT(outFile))
//				continue;
//
//			if (mFilter.length() and not FileNameMatches(mFilter.c_str(), outFile))
//				continue;
//
//			result = true;
//		}
//	}
//	
//	return result;
//}
//
//MFileIterator::MFileIterator(const MPath& inDirectory, uint32 inFlags)
//{
//	if (inFlags & kFileIter_Deep)
//		mImpl = new MDeepFileIteratorImp(inDirectory);
//	else
//		mImpl = new MSingleFileIteratorImp(inDirectory);
//	
//	mImpl->mReturnDirs = (inFlags & kFileIter_ReturnDirectories) != 0;
//	mImpl->mOnlyTEXT = (inFlags & kFileIter_TEXTFilesOnly) != 0;
//}
//
//MFileIterator::~MFileIterator()
//{
//	delete mImpl;
//}
//
//bool MFileIterator::Next(MPath& outFile)
//{
//	return mImpl->Next(outFile);
//}
//
//void MFileIterator::SetFilter(const std::string& inFilter)
//{
//	mImpl->mFilter = inFilter;
//}

// ----------------------------------------------------------------------------
//	relative_path

MPath relative_path(const MPath& inFromDir, const MPath& inFile)
{
	assert(false);

//	fs::path::iterator d = inFromDir.begin();
//	fs::path::iterator f = inFile.begin();
//	
//	while (d != inFromDir.end() and f != inFile.end() and *d == *f)
//	{
//		++d;
//		++f;
//	}
//	
//	MPath result;
//	
//	if (d == inFromDir.end() and f == inFile.end())
//		result = ".";
//	else
//	{
//		while (d != inFromDir.end())
//		{
//			result /= "..";
//			++d;
//		}
//		
//		while (f != inFile.end())
//		{
//			result /= *f;
//			++f;
//		}
//	}
//
//	return result;
}
