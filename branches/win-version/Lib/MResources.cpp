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
const mrsrc::rsrc_imp gResourceIndex[0] = {};
const char gResourceData[] = "\0\0\0\0";
const char gResourceName[] = "\0\0\0\0";
#endif

// --------------------------------------------------------------------

struct MResourceFileImp
{
	MTargetCPU				mTarget;
	vector<mrsrc::rsrc_imp>	m_index;
	vector<char>			m_data, m_name;

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
	mrsrc::rsrc_imp root = {};
	mImpl->m_index.push_back(root);
	mImpl->m_name.push_back(0);
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
		if (m_index[node].m_child == 0)
		{
			mrsrc::rsrc_imp child = {};
			
			child.m_name = m_name.size();
			copy(p->begin(), p->end(), back_inserter(m_name));
			m_name.push_back(0);
			
			m_index[node].m_child = m_index.size();
			m_index.push_back(child);
			
			node = m_index[node].m_child;
			continue;
		}
		
		// lookup the path element in the current directory
		uint32 next = m_index[node].m_child;
		for (;;)
		{
			const char* name = &m_name[0] + m_index[next].m_name;
			
			// if this is the one we're looking for, break out of the loop
			if (*p == name)
			{
				node = next;
				break;
			}
			
			// if there is a next element, loop
			if (m_index[next].m_next != 0)
			{
				next = m_index[next].m_next;
				continue;
			}
			
			// not found, create it
			mrsrc::rsrc_imp n = {};
			
			n.m_name = m_name.size();
			copy(p->begin(), p->end(), back_inserter(m_name));
			m_name.push_back(0);
			
			node = m_index.size();
			m_index[next].m_next = node;
			m_index.push_back(n);

			break;
		}
	}
	
	assert(node != 0);
	assert(node < m_index.size());
	
	m_index[node].m_size = inSize;
	m_index[node].m_data = m_data.size();
	
	copy(inData, inData + inSize, back_inserter(m_data));
	while ((m_data.size() % 8) != 0)
		m_data.push_back('\0');
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
	fs::ifstream f(inFile, ios::binary);

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
		&mImpl->m_index[0], mImpl->m_index.size() * sizeof(mrsrc::rsrc_imp));

	obj.AddGlobal("gResourceData",
		&mImpl->m_data[0], mImpl->m_data.size());

	obj.AddGlobal("gResourceName",
		&mImpl->m_name[0], mImpl->m_name.size());
	
	obj.Write(inFile);
}

