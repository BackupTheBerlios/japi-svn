//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

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

#if defined(BUILDING_TEMPORARY_JAPI)
const char gResourceIndex[] = "\0\0\0\0";
const char gResourceData[] = "\0\0\0\0";
const char gResourceName[] = "\0\0\0\0";
#endif

// --------------------------------------------------------------------

struct MResourceFileImp
{
	MTargetCPU				mTarget;
	vector<MResourceImp>	mIndex;
	vector<char>			mData, mName;

	void		AddEntry(
					fs::path	inPath,
					const char*	inData,
					uint32		inSize);
};

MResourceFile::MResourceFile(
	MTargetCPU			inTarget)
	: mImpl(new MResourceFileImp)
{
	mImpl->mTarget = inTarget;
	
	// push the root
	MResourceImp root = {};
	mImpl->mIndex.push_back(root);
	mImpl->mName.push_back(0);
}

MResourceFile::~MResourceFile()
{
	delete mImpl;
}

void MResourceFileImp::AddEntry(
	fs::path		inPath,
	const char*		inData,
	uint32			inSize)
{
	uint32 node = 0;	// start at root
	
	for (fs::path::iterator p = inPath.begin(); p != inPath.end(); ++p)
	{
		// no such child? Add it and continue
		if (mIndex[node].mChild == 0)
		{
			MResourceImp child = {};
			
			child.mName = mName.size();
			copy(p->begin(), p->end(), back_inserter(mName));
			mName.push_back(0);
			
			mIndex[node].mChild = mIndex.size();
			mIndex.push_back(child);
			
			node = mIndex[node].mChild;
			continue;
		}
		
		// lookup the path element in the current directory
		uint32 next = mIndex[node].mChild;
		for (;;)
		{
			const char* name = &mName[0] + mIndex[next].mName;
			
			// if this is the one we're looking for, break out of the loop
			if (*p == name)
			{
				node = next;
				break;
			}
			
			// if there is a next element, loop
			if (mIndex[next].mNext != 0)
			{
				next = mIndex[next].mNext;
				continue;
			}
			
			// not found, create it
			MResourceImp n = {};
			
			n.mName = mName.size();
			copy(p->begin(), p->end(), back_inserter(mName));
			mName.push_back(0);
			
			node = mIndex.size();
			mIndex[next].mNext = node;
			mIndex.push_back(n);

			break;
		}
	}
	
	assert(node != 0);
	assert(node < mIndex.size());
	
	mIndex[node].mSize = inSize;
	mIndex[node].mData = mData.size();
	
	copy(inData, inData + inSize, back_inserter(mData));
	while ((mData.size() % 8) != 0)
		mData.push_back('\0');
}

void MResourceFile::Add(
	const string&	inPath,
	const void*		inData,
	uint32			inSize)
{
	mImpl->AddEntry(inPath, static_cast<const char*>(inData), inSize);
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

	obj.AddGlobal("gResourceIndex",
		&mImpl->mIndex[0], mImpl->mIndex.size() * sizeof(MResourceImp));

	obj.AddGlobal("gResourceData",
		&mImpl->mData[0], mImpl->mData.size());

	obj.AddGlobal("gResourceName",
		&mImpl->mName[0], mImpl->mName.size());
	
	obj.Write(inFile);
}

