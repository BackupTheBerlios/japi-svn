/*
	Copyright (c) 2007, Maarten L. Hekkelman
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

#include "MJapi.h"

#include <sstream>
#include <list>
#include <limits>

#include <boost/filesystem/fstream.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/bind.hpp>

#include "MObjectFile.h"
#include "MResources.h"
#include "MPatriciaTree.h"
#include "MError.h"

using namespace std;

extern const char gResourceIndex[];
extern const char gResourceData[];

#if defined(BUILDING_TEMPORARY_JAPI)
const char gResourceIndex[] = "\0\0\0\0";
const char gResourceData[] = "\0\0\0\0";
#endif

namespace
{
	
using namespace boost;

class MRSRCDirectory
{
  public:

						MRSRCDirectory();
	
						MRSRCDirectory(
							const char*		inData,
							uint32&			ioOffset);

	static MRSRCDirectory&	Instance();

	void				AddData(
							const string&	inPath,
							uint32			inOffset,
							uint32			inSize);
	
	bool				GetData(
							const char*		inPath,
							uint32&			outOffset,
							uint32&			outSize) const;

	void				Flatten(
							ostream&		inIndexStream);

  private:

	void				read(
							const char*		inData,
							uint32&			ioOffset,
							uint32&			outValue)
						{
							memcpy(&outValue, inData + ioOffset, sizeof(uint32));
							ioOffset += sizeof(uint32);
						}

	void				read(
							const char*		inData,
							uint32&			ioOffset,
							string&			outValue)
						{
							uint8 length = *reinterpret_cast<const uint8*>(inData + ioOffset);

							if (length > 0)
								outValue.assign(inData + ioOffset + 1, inData + ioOffset + 1 + length);
							else
								outValue.clear();

							ioOffset += length + 1;
						}

	void				write(
							uint32			inValue,
							ostream&		ioStream)
						{
							ioStream.write(reinterpret_cast<const char*>(&inValue), sizeof(inValue));
						}

	void				write(
							string			inValue,
							ostream&		ioStream)
						{
							if (inValue.length() > numeric_limits<uint8>::max())
								THROW(("Trying to store a resource with a name that is too long: %s", inValue.c_str()));
							
							uint8 length = inValue.length();
							
							ioStream.write(reinterpret_cast<const char*>(&length), sizeof(length));
							if (length > 0)
								ioStream.write(reinterpret_cast<const char*>(inValue.c_str()), length);
						}

#if DEBUG
  public:
	void				Print(
							int				inLevel);
#endif

	enum MRSRCType
	{
		eRSRC_File	= 'file',
		eRSRC_Dir	= 'dir '
	};

	struct MRSRCEntry
	{
		string			name;
		uint32			offset;
		uint32			size;
		uint32			type;
		
		const string&	GetName() const		{ return name; }
	};
	
	list<MRSRCEntry>			mEntries;
	ptr_vector<MRSRCDirectory>	mSubDirs;
};

MRSRCDirectory::MRSRCDirectory()
{
}

MRSRCDirectory::MRSRCDirectory(
	const char*		inData,
	uint32&			ioOffset)
{
	uint32 count;
	read(inData, ioOffset, count);
	
	while (count-- > 0)
	{
		MRSRCEntry e;

		read(inData, ioOffset, e.name);
		read(inData, ioOffset, e.type);
		
		if (e.type == eRSRC_Dir)
		{
			MRSRCDirectory* d = new MRSRCDirectory(inData, ioOffset);
			e.offset = mSubDirs.size();
			mSubDirs.push_back(d);
		}
		else
		{
			read(inData, ioOffset, e.offset);
			read(inData, ioOffset, e.size);
		}

		mEntries.push_back(e);
	}
}

#if DEBUG
void MRSRCDirectory::Print(
	int		inLevel)
{
	string tabs(inLevel, ' ');
	
	for (list<MRSRCEntry>::const_iterator e = mEntries.begin(); e != mEntries.end(); ++e)
	{
		cout << tabs << e->name << ' ';
		if (e->type == eRSRC_Dir)
		{
			cout << '/' << endl;
			mSubDirs[e->offset].Print(inLevel + 1);
		}
		else
			cout << e->offset << '-' << e->size << endl;
	}
}
#endif

void MRSRCDirectory::AddData(
	const string&	inPath,
	uint32			inOffset,
	uint32			inSize)
{
	string path(inPath);
	string::size_type p = path.find('/');
	string name = path.substr(0, p);

	list<MRSRCEntry>::const_iterator e = find_if(
		mEntries.begin(), mEntries.end(),
		boost::bind(&MRSRCEntry::GetName, _1) == name);
	
	if (e != mEntries.end())
	{
		if (p == string::npos)
			THROW(("Duplicate resource entry %s", inPath.c_str()));
		
		mSubDirs[e->offset].AddData(path.substr(p + 1), inOffset, inSize);
	}
	else
	{
		MRSRCEntry e;
		e.name = name;
		
		if (p != string::npos)
		{
			e.type = eRSRC_Dir;
			e.offset = mSubDirs.size();

			MRSRCDirectory* d = new MRSRCDirectory;
			mSubDirs.push_back(d);
			
			d->AddData(path.substr(p + 1), inOffset, inSize);
		}
		else
		{
			e.type = eRSRC_File;
			e.offset = inOffset;
			e.size = inSize;
		}
		
		mEntries.push_back(e);
	}
}

bool MRSRCDirectory::GetData(
	const char*		inPath,
	uint32&			outOffset,
	uint32&			outSize) const
{
	string path(inPath);
	string::size_type p = path.find('/');
	string name = path.substr(0, p);

	list<MRSRCEntry>::const_iterator e = find_if(
		mEntries.begin(), mEntries.end(),
		boost::bind(&MRSRCEntry::GetName, _1) == name);
	
	bool result = false;
	if (e != mEntries.end())
	{
		if (p != string::npos)
			result = mSubDirs[e->offset].GetData(inPath + p + 1, outOffset, outSize);
		else
		{
			outOffset = e->offset;
			outSize = e->size;
			result = true;
		}
	}
	
	return result;
}

void MRSRCDirectory::Flatten(
	ostream&		inIndexStream)
{
	uint32 count = mEntries.size();
	write(count, inIndexStream);
	
	for (list<MRSRCEntry>::iterator e = mEntries.begin(); e != mEntries.end(); ++e)
	{
		write(e->name, inIndexStream);
		write(e->type, inIndexStream);
		
		if (e->type == eRSRC_File)
		{
			write(e->offset, inIndexStream);
			write(e->size, inIndexStream);
		}
		else //if (e->type == eRSRC_Dir)
			mSubDirs[e->offset].Flatten(inIndexStream);
	}
}

MRSRCDirectory& MRSRCDirectory::Instance()
{
	uint32 offset = 0;
	static MRSRCDirectory sInstance(gResourceIndex, offset);
	return sInstance;
}

}

// --------------------------------------------------------------------

bool LoadResource(
		const string&	inName,
		const char*&	outData,
		uint32&			outSize)
{
	uint32 offset;
	MRSRCDirectory& dir = MRSRCDirectory::Instance();
	
	bool result = dir.GetData(inName.c_str(), offset, outSize);

	if (result == false)
	{
		string path;
		static const char* LANG = getenv("LANG");
		
		if (LANG != nil and strncmp(LANG, "nl", 2) == 0)
			path = "Dutch/";
		else
			path = "English/";
		
		result = dir.GetData((path + inName).c_str(), offset, outSize);
	}

#ifndef NDEBUG
	string::size_type p;
	if (not result and (p = inName.rfind('/')) != string::npos)
	{
		result = dir.GetData(inName.substr(p + 1).c_str(), offset, outSize);
	}
#endif

	if (result)
		outData = gResourceData + offset;

	return result;
}

// --------------------------------------------------------------------

struct MResourceFileImp
{
	MTargetCPU			mTarget;
	MRSRCDirectory		mIndex;
	stringstream		mData;
};

MResourceFile::MResourceFile(
	MTargetCPU			inTarget)
	: mImpl(new MResourceFileImp)
{
	mImpl->mTarget = inTarget;
}

MResourceFile::~MResourceFile()
{
	delete mImpl;
}

void MResourceFile::Add(
	const string&	inPath,
	const void*		inData,
	uint32			inSize)
{
	uint32 offset = mImpl->mData.tellp();
	mImpl->mData.write(reinterpret_cast<const char*>(inData), inSize);
	mImpl->mIndex.AddData(inPath, offset, inSize);
	
	while ((mImpl->mData.tellp() % 8) != 0)
		mImpl->mData.put('\0');
}

void MResourceFile::Add(
	const string&	inPath,
	const fs::path&	inFile)
{
	fs::ifstream f(inFile);

	if (not f.is_open())
		THROW(("Could not open resource file"));

	filebuf* b = f.rdbuf();
	
	uint32 size = b->pubseekoff(0, ios::end, ios::in);
	b->pubseekoff(0, ios::beg, ios::in);
	
	char* text = new char[size];
	
	b->sgetn(text, size);
	f.close();
	
	Add(inPath, text, size);
	
	delete[] text;
}

void MResourceFile::Write(
	const fs::path&		inFile)
{
	MObjectFile obj(mImpl->mTarget);

	stringstream s;
	mImpl->mIndex.Flatten(s);
	
	string index(s.str());
	obj.AddGlobal("gResourceIndex", index.c_str(), index.length());
	
	string data(mImpl->mData.str());
	obj.AddGlobal("gResourceData", data.c_str(), data.length());
	
	obj.Write(inFile);
}

