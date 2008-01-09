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

#include <sys/stat.h>
#include <dirent.h>
#include <stack>
#include <fstream>
#include <cassert>
#include <cerrno>
#include <magic.h>

#include "MFile.h"
#include "MUrl.h"
#include "MError.h"
#include "MUnicode.h"
#include "MUtils.h"
#include "MStrings.h"

using namespace std;

// ------------------------------------------------------------------
//
//  libmagic support
//

class MLibMagic
{
  public:
	static MLibMagic&	Instance();
	
	bool		IsText(
					const MPath&	inPath);
	
  private:

				MLibMagic();
				~MLibMagic();

	magic_t		mCookie;
};

MLibMagic::MLibMagic()
{
	int flags = MAGIC_MIME | MAGIC_SYMLINK;
	flags |= MAGIC_NO_CHECK_COMPRESS;
	flags |= MAGIC_NO_CHECK_TAR;
	flags |= MAGIC_NO_CHECK_SOFT;
	flags |= MAGIC_NO_CHECK_APPTYPE;
	flags |= MAGIC_NO_CHECK_ELF;
	flags |= MAGIC_NO_CHECK_TROFF;
	flags |= MAGIC_NO_CHECK_TOKENS;
	
	mCookie = magic_open(flags);
	
	if (mCookie != nil)
		magic_load(mCookie, nil);
}

MLibMagic::~MLibMagic()
{
	magic_close(mCookie);
}

MLibMagic& MLibMagic::Instance()
{
	static MLibMagic sInstance;
	return sInstance;
}

bool MLibMagic::IsText(
	const MPath&	inPath)
{
	bool result = false;
	const char* t;
	
	if (mCookie != nil and (t = magic_file(mCookie, inPath.string().c_str())) != nil)
		result = strncmp(t, "text/", 5) == 0;
	
	return result;
}

// ------------------------------------------------------------------
//
//  Three different implementations of extended attributes...
//

// ------------------------------------------------------------------
//  FreeBSD

#if defined(__FreeBSD__) and (__FreeBSD__ > 0)

#include <sys/extattr.h>

ssize_t read_attribute(const MPath& inPath, const char* inName, void* outData, size_t inDataSize)
{
	string path = inPath.string();
	
	return extattr_get_file(path.c_str(), EXTATTR_NAMESPACE_USER,
		inName, outData, inDataSize);
}

void write_attribute(const MPath& inPath, const char* inName, const void* inData, size_t inDataSize)
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

ssize_t read_attribute(const MPath& inPath, const char* inName, void* outData, size_t inDataSize)
{
	string path = inPath.string();

	int length = inDataSize;
	int err = ::attr_get(path.c_str(), inName,
		reinterpret_cast<char*>(outData), &length, 0);
	
	if (err != 0)
		length = 0;
	
	return length;
}

void write_attribute(const MPath& inPath, const char* inName, const void* inData, size_t inDataSize)
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

ssize_t read_attribute(const MPath& inPath, const char* inName, void* outData, size_t inDataSize)
{
	string path = inPath.string();

	return ::getxattr(path.c_str(), inName, outData, inDataSize, 0, 0);
}

void write_attribute(const MPath& inPath, const char* inName, const void* inData, size_t inDataSize)
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
	const MPath&		inFile)
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
		MPath			mParent;
		DIR*			mDIR;
		struct dirent	mEntry;
	};
	
						MFileIteratorImp()
							: mOnlyTEXT(false)
							, mReturnDirs(false) {}
	virtual				~MFileIteratorImp() {}
	
	virtual	bool		Next(MPath& outFile) = 0;
	bool				IsTEXT(
							const MPath&	inFile);
	
	string				mFilter;
	bool				mOnlyTEXT;
	bool				mReturnDirs;
};

bool MFileIteratorImp::IsTEXT(const MPath& inFile)
{
	return MLibMagic::Instance().IsText(inFile);
}

struct MSingleFileIteratorImp : public MFileIteratorImp
{
						MSingleFileIteratorImp(
							const MPath&	inDirectory);

	virtual				~MSingleFileIteratorImp();
	
	virtual	bool		Next(
							MPath&			outFile);
	
	MInfo				mInfo;
};

MSingleFileIteratorImp::MSingleFileIteratorImp(
	const MPath&	inDirectory)
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
	MPath&			outFile)
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

//		struct stat statb;
//		
//		if (stat(outFile.string().c_str(), &statb) != 0)
//			continue;
//		
//		if (S_ISDIR(statb.st_mode) != mReturnDirs)
//			continue;

		if (is_directory(outFile) and not mReturnDirs)
			continue;
		
		if (mOnlyTEXT and not IsTEXT(outFile))
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
							const MPath&	inDirectory);

	virtual				~MDeepFileIteratorImp();

	virtual	bool		Next(MPath& outFile);

	stack<MInfo>		mStack;
};

MDeepFileIteratorImp::MDeepFileIteratorImp(const MPath& inDirectory)
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
	MPath&		outFile)
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

			if (mOnlyTEXT and not IsTEXT(outFile))
				continue;

			if (mFilter.length() and not FileNameMatches(mFilter.c_str(), outFile))
				continue;

			result = true;
		}
	}
	
	return result;
}

MFileIterator::MFileIterator(
	const MPath&	inDirectory,
	uint32			inFlags)
{
	if (inFlags & kFileIter_Deep)
		mImpl = new MDeepFileIteratorImp(inDirectory);
	else
		mImpl = new MSingleFileIteratorImp(inDirectory);
	
	mImpl->mReturnDirs = (inFlags & kFileIter_ReturnDirectories) != 0;
	mImpl->mOnlyTEXT = (inFlags & kFileIter_TEXTFilesOnly) != 0;
}

MFileIterator::~MFileIterator()
{
	delete mImpl;
}

bool MFileIterator::Next(
	MPath&			outFile)
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

MPath relative_path(const MPath& inFromDir, const MPath& inFile)
{
//	assert(false);

	fs::path::iterator d = inFromDir.begin();
	fs::path::iterator f = inFile.begin();
	
	while (d != inFromDir.end() and f != inFile.end() and *d == *f)
	{
		++d;
		++f;
	}
	
	MPath result;
	
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
	MPath&	outDirectory)
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
	
//		gtk_file_chooser_set_local_only(GTK_FILE_CHOOSER(dialog), true);
		
//		if (mCurrentFolder.length() > 0)
//		{
//			gtk_file_chooser_set_current_folder(
//				GTK_FILE_CHOOSER(dialog), mCurrentFolder.c_str());
//		}
		
		if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
		{
			char* uri = gtk_file_chooser_get_uri(GTK_FILE_CHOOSER(dialog));
			
			MUrl url(uri);
			outDirectory = url.GetPath();

			g_free(uri);

			result = true;
		}

//		char* cwd = gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER(dialog));
//		if (cwd != nil)
//		{
//			mCurrentFolder = cwd;
//			g_free(cwd);
//		}
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

bool ChooseOneFile(
	MUrl&	ioFile)
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
			gtk_file_chooser_set_uri(
				GTK_FILE_CHOOSER(dialog), ioFile.str().c_str());
		}
		
		if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
		{
			char* uri = gtk_file_chooser_get_uri(GTK_FILE_CHOOSER(dialog));	
			
			ioFile = uri;
			result = true;

			g_free(uri);
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
