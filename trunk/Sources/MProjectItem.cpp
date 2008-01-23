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

#include "MJapi.h"

#include <vector>
#include <numeric>
#include <stack>

#include <boost/bind.hpp>
#include <boost/filesystem/fstream.hpp>

#include "MFile.h"
#include "MProjectItem.h"
#include "MObjectFile.h"

using namespace std;

namespace {

class MPathIterator : public boost::iterator_facade
<
	MPathIterator,
	string,
	boost::forward_traversal_tag,
	string
>
{
  public:
					MPathIterator();		// means 'end'
	
					MPathIterator(
						const string&		inData);
					
  private:
	
	friend class boost::iterator_core_access;

	string			dereference() const;

	void			increment();
	bool			equal(
						const MPathIterator&	rhs) const;

	const string*	mData;
	uint32			mOffset;
	uint32			mLength;
	bool			mQuoted;
};

MPathIterator::MPathIterator()
	: mData(nil)
	, mOffset(0)
	, mLength(0)
	, mQuoted(false)
{
}

MPathIterator::MPathIterator(
	const string&		inData)
	: mData(&inData)
	, mOffset(0)
	, mLength(0)
	, mQuoted(false)
{
	increment();
}

string MPathIterator::dereference() const
{
	string::const_iterator a = mData->begin() + mOffset;
	string::const_iterator b = a + mLength;
	
	string result;
	result.reserve(b - a);
	
	bool esc = false;
	for (; a != b; ++a)
	{
		if (not esc and *a == '\\')
			esc = true;
		else
		{
			result += *a;
			esc = false;
		}
	}

	return result;
}

void MPathIterator::increment()
{
	bool found = false;
	
	string::const_iterator s = mData->begin() + mOffset + mLength;
	if (mQuoted)
		s += 1;
	
	while (s != mData->end() and isspace(*s))
		++s;
	
	if (*s == '\'')
	{
		mQuoted = true;
		++s;
		
		mOffset = s - mData->begin();
		
		while (s != mData->end() and *s != '\'')
			++s;
		
		if (*s == '\'')
		{
			++s;
			found = true;
			mLength = (s - mData->begin()) - mOffset - 1;
		}
	}
	else if (s != mData->end())
	{
		found = true;
		mQuoted = false;
		mOffset = s - mData->begin();
		
		bool esc = false;
		for (; s != mData->end(); ++s)
		{
			if (esc or (*s == '\\' and not esc))
				esc = not esc;
			else if (isspace(*s))
				break;
		}
		
		mLength = (s - mData->begin()) - mOffset;
	}

	if (not found)
	{
		mData = nil;
		mOffset = 0;
		mLength = 0;
	}
}

bool MPathIterator::equal(const MPathIterator& rhs) const
{
	return
		mData == rhs.mData and
		mOffset == rhs.mOffset and
		mLength == rhs.mLength;
}

}

// ---------------------------------------------------------------------------
//	MProjectItem::MProjectItem

MProjectItem::MProjectItem(
	const string&	inName,
	MProjectGroup*	inParent)
	: mName(inName)
	, mParent(inParent)
{
}

// ---------------------------------------------------------------------------
//	MProjectItem::GetPosition

int32 MProjectItem::GetPosition() const
{
	int32 result = 0;
	
	if (mParent != nil)
		result = mParent->GetItemPosition(this);
	
	return result;
}

// ---------------------------------------------------------------------------
//	MProjectItem::GetSiblingPosition

int32 MProjectItem::GetSiblingPosition() const
{
	int32 result = 0;
	
	if (mParent != nil)
	{
		vector<MProjectItem*>::iterator i = find(
			mParent->GetItems().begin(), mParent->GetItems().end(),
			this);
		
		result = i - mParent->GetItems().begin();
	}

	return result;
}

// ---------------------------------------------------------------------------
//	MProjectItem::GetLevel

uint32 MProjectItem::GetLevel() const
{
	uint32 result = 0;
	if (mParent != nil)
		result = mParent->GetLevel() + 1;
	return result;
}

// ---------------------------------------------------------------------------
//	MProjectItem::GetNext

MProjectItem* MProjectItem::GetNextSibling() const
{
	MProjectItem* result = nil;

	if (mParent != nil)
	{
		vector<MProjectItem*>::iterator i = find(
			mParent->GetItems().begin(), mParent->GetItems().end(),
			this);
		
		if (i != mParent->GetItems().end() and
			i + 1 != mParent->GetItems().end())
		{
			result = *(i + 1);
		}
	}

	return result;
}

// ---------------------------------------------------------------------------
//	MProjectFile::MProjectFile

MProjectFile::MProjectFile(
	const string&		inName,
	MProjectGroup*		inParent,
	const MPath&		inParentDir)
	: MProjectItem(inName, inParent)
	, mParentDir(inParentDir)
	, mTextSize(0)
	, mDataSize(0)
	, mIsCompiling(false)
	, mIsOutOfDate(false)
	, mOptional(false)
{
	mIsOutOfDate = IsOutOfDate();
}

// ---------------------------------------------------------------------------
//	MProjectFile::GetProjectFileForPath

MProjectFile* MProjectFile::GetProjectFileForPath(
	const MPath&		inPath) const
{
	MProjectFile* result = nil;
	if (inPath == GetPath())
		result = const_cast<MProjectFile*>(this);
	return result;
}

// ---------------------------------------------------------------------------
//	MProjectFile::SetParentDir

void MProjectFile::SetParentDir(
	const MPath&		inParentDir)
{
	mParentDir = inParentDir;
}

// ---------------------------------------------------------------------------
//	MProjectFile::UpdatePaths

void MProjectFile::UpdatePaths(
	const MPath&		inObjectDir)
{
	string baseName = fs::basename(mName);
	
	mObjectPath = inObjectDir / (baseName + ".o");
	mDependsPath = inObjectDir / (baseName + ".d");
}

// ---------------------------------------------------------------------------
//	MProjectFile::SetOutOfDate

void MProjectFile::SetOutOfDate(
	bool			inIsOutOfDate)
{
	if (mIsOutOfDate != inIsOutOfDate)
	{
		mIsOutOfDate = inIsOutOfDate;
		eStatusChanged(this);
		
		MProjectItem* parent = GetParent();
		while (parent != nil)
		{
			parent->eStatusChanged(parent);
			parent = parent->GetParent();
		}
	}
}

// ---------------------------------------------------------------------------
//	MProjectFile::SetCompiling

void MProjectFile::SetCompiling(
	bool			inIsCompiling)
{
	if (mIsCompiling != inIsCompiling)
	{
		mIsCompiling = inIsCompiling;
		eStatusChanged(this);

		MProjectItem* parent = GetParent();
		while (parent != nil)
		{
			parent->eStatusChanged(parent);
			parent = parent->GetParent();
		}
	}
}

// ---------------------------------------------------------------------------
//	MProjectFile::CheckCompilationResult

void MProjectFile::CheckCompilationResult()
{
	mTextSize = 0;
	mDataSize = 0;
	
	if (not IsCompilable() or not fs::exists(mObjectPath))
		return;
	
	try
	{
		// first read object file and fetch __text and __data sizes
		MObjectFile objectFile(mObjectPath);
		
		mTextSize = objectFile.GetTextSize();
		mDataSize = objectFile.GetDataSize();

		// then read in the .d file and collect the included files
		fs::ifstream dependsFile(mDependsPath);
		string text;
		
		mIncludedFiles.clear();
		
		if (dependsFile.is_open())
		{
			char buffer[10240];
			
			dependsFile.getline(buffer, sizeof(buffer), ':');
			
			while (not dependsFile.eof())
			{
				string line;
				getline(dependsFile, line);
				
				if (line.length() > 0 and line[line.length() - 1] == '\\')
					line.erase(line.end() - 1);
				
				text += line;
			}
		}
		
		MPathIterator m1(text), m2;
		
		while (m1 != m2)
		{
			mIncludedFiles.push_back(*m1);
			++m1;
		}

		SetOutOfDate(false);
	}
	catch (std::exception& e)
	{
		SetOutOfDate(true);
	}
}

// ---------------------------------------------------------------------------
//	MProjectFile::CheckIsOutOfDate

void MProjectFile::CheckIsOutOfDate(
	MModDateCache&	ioModDateCache)
{
	bool isOutOfDate = false;
	
	MPath path = mParentDir / mName;
	
	if (not IsCompilable())
		return;

	if (not exists(mObjectPath) or not exists(mDependsPath))
	{
		isOutOfDate = true;
		mTextSize = 0;
		mDataSize = 0;
	}

	if (isOutOfDate == false)
		isOutOfDate = fs::last_write_time(path) > fs::last_write_time(mObjectPath);
	
	if (isOutOfDate == false)
		isOutOfDate = fs::last_write_time(path) > fs::last_write_time(mDependsPath);
	
	if (isOutOfDate == false)
	{
		double objectDate = fs::last_write_time(mObjectPath);
		
		vector<string>::iterator m1 = mIncludedFiles.begin();
		vector<string>::iterator m2 = mIncludedFiles.end();
		
		while (isOutOfDate == false and m1 != m2)
		{
			string path = *m1++;

			double depModDate = ioModDateCache[path];

			if (depModDate == 0)
			{
				try
				{
					MPath dependancy(path);
					ioModDateCache[path] = depModDate = fs::last_write_time(dependancy);
				}
				catch (...) {}
			}

			isOutOfDate = depModDate > objectDate;
		}
	}
	
	SetOutOfDate(isOutOfDate);
}

// ---------------------------------------------------------------------------
//	MProjectGroup::MProjectGroup

MProjectGroup::MProjectGroup(
	const string&	inName,
	MProjectGroup*	inParent)
	: MProjectItem(inName, inParent)
{
}
	
// ---------------------------------------------------------------------------
//	MProjectGroup::~MProjectGroup

MProjectGroup::~MProjectGroup()
{
	for (vector<MProjectItem*>::iterator i = mItems.begin(); i != mItems.end(); ++i)
		delete *i;
}

// ---------------------------------------------------------------------------
//	MProjectGroup::GetProjectFileForPath

MProjectFile* MProjectGroup::GetProjectFileForPath(
	const MPath&	inPath) const
{
	MProjectFile* result = nil;

	for (vector<MProjectItem*>::const_iterator i = mItems.begin(); result == nil and i != mItems.end(); ++i)
		result = (*i)->GetProjectFileForPath(inPath);

	return result;
}

// ---------------------------------------------------------------------------
//	MProjectGroup::UpdatePaths

void MProjectGroup::UpdatePaths(
	const MPath&	inObjectDir)
{
	for_each(mItems.begin(), mItems.end(),
		boost::bind(&MProjectItem::UpdatePaths, _1, inObjectDir));
}

// ---------------------------------------------------------------------------
//	MProjectGroup::CheckCompilationResult

void MProjectGroup::CheckCompilationResult()
{
	for_each(mItems.begin(), mItems.end(),
		boost::bind(&MProjectItem::CheckCompilationResult, _1));
}
	
// ---------------------------------------------------------------------------
//	MProjectGroup::CheckIsOutOfDate

void MProjectGroup::CheckIsOutOfDate(
	MModDateCache&	ioModDateCache)
{
	for (vector<MProjectItem*>::iterator i = mItems.begin(); i != mItems.end(); ++i)
		(*i)->CheckIsOutOfDate(ioModDateCache);
}
	
// ---------------------------------------------------------------------------
//	MProjectGroup::IsOutOfDate

bool MProjectGroup::IsOutOfDate() const
{
	return accumulate(mItems.begin(), mItems.end(), false,
		boost::bind(logical_or<bool>(), _1, boost::bind(&MProjectItem::IsOutOfDate, _2)));
}

// ---------------------------------------------------------------------------
//	MProjectGroup::SetOutOfDate

void MProjectGroup::SetOutOfDate(
	bool			inIsOutOfDate)
{
	for_each(mItems.begin(), mItems.end(),
		boost::bind(&MProjectItem::SetOutOfDate, _1, inIsOutOfDate));
}

// ---------------------------------------------------------------------------
//	MProjectGroup::IsCompiling

bool MProjectGroup::IsCompiling() const
{
	return accumulate(mItems.begin(), mItems.end(), false,
		boost::bind(logical_or<bool>(), _1, boost::bind(&MProjectItem::IsCompiling, _2)));
}

// ---------------------------------------------------------------------------
//	MProjectGroup::GetItemPosition

int32 MProjectGroup::GetItemPosition(
	const MProjectItem*	inItem) const
{
	int32 result = 0;
	
	vector<MProjectItem*>::const_iterator i = find(mItems.begin(), mItems.end(), inItem);
	if (i != mItems.end())
	{
		result = accumulate(mItems.begin(), i, 0,
			boost::bind(plus<uint32>(), _1, boost::bind(&MProjectItem::Count, _2)));
	}
	
	if (mParent != nil)
		result += mParent->GetItemPosition(this);
	
	return result;
}

// ---------------------------------------------------------------------------
//	MProjectGroup::AddProjectItem

void MProjectGroup::AddProjectItem(
	MProjectItem*	inItem,
	int32			inPosition)
{
	if (inPosition == -1)
		mItems.insert(mItems.end(), inItem);
	else
	{
		vector<MProjectItem*>::iterator i = mItems.begin();
		
		while (inPosition > 0 and i != mItems.end())
		{
			inPosition -= (*i)->Count();
			++i;
		}
		
		mItems.insert(i, inItem);
	}

	inItem->SetParent(this);
}

// ---------------------------------------------------------------------------
//	MProjectGroup::RemoveProjectItem

void MProjectGroup::RemoveProjectItem(
	MProjectItem*	inItem)
{
	mItems.erase(remove(mItems.begin(), mItems.end(), inItem), mItems.end());
	inItem->SetParent(nil);
}

// ---------------------------------------------------------------------------
//	MProjectGroup::GetTextSize

uint32 MProjectGroup::GetTextSize() const
{
	return accumulate(mItems.begin(), mItems.end(), 0,
		boost::bind(plus<uint32>(), _1, boost::bind(&MProjectItem::GetTextSize, _2)));
}

// ---------------------------------------------------------------------------
//	MProjectGroup::GetDataSize

uint32 MProjectGroup::GetDataSize() const
{
	return accumulate(mItems.begin(), mItems.end(), 0,
		boost::bind(plus<uint32>(), _1, boost::bind(&MProjectItem::GetDataSize, _2)));
}

// ---------------------------------------------------------------------------
//	MProjectGroup::GetDataSize

void MProjectGroup::Flatten(
	vector<MProjectItem*>&	outItems)
{
	if (mParent != nil)				// do not add the root
		outItems.push_back(this);
	
	for (vector<MProjectItem*>::iterator i = mItems.begin(); i != mItems.end(); ++i)
		(*i)->Flatten(outItems);
}

// ---------------------------------------------------------------------------
//	MProjectGroup::GetDataSize

uint32 MProjectGroup::Count() const
{
	return accumulate(mItems.begin(), mItems.end(), 1,
		boost::bind(plus<uint32>(), _1, boost::bind(&MProjectItem::Count, _2)));
}

// ---------------------------------------------------------------------------
//	MProjectGroup::GetItem

MProjectItem* MProjectGroup::GetItem(
	uint32				inIndex) const
{
	return mItems.at(inIndex);
}

// ---------------------------------------------------------------------------
//	MProjectCpFile::GetDestPath

MPath MProjectCpFile::GetDestPath(
	const MPath&		inPackageDir) const
{
	stack<MProjectMkDir*>	sp;
	
	MProjectMkDir* parent = dynamic_cast<MProjectMkDir*>(mParent);
	while (parent != nil)
	{
		sp.push(parent);
		parent = dynamic_cast<MProjectMkDir*>(parent->GetParent());
	}
	
	MPath result = inPackageDir;
	
	while (not sp.empty())
	{
		result /= sp.top()->GetName();
		sp.pop();
	}
	
	return result / mName;
}

// ---------------------------------------------------------------------------
//	MProjectCpFile::GetDataSize

uint32 MProjectCpFile::GetDataSize() const
{
	uint32 result = 0;
	try
	{
		if (is_directory(mPath))
		{
			MFileIterator iter(mPath, kFileIter_Deep);
			MPath p;
			
			while (iter.Next(p))
			{
				if (p.leaf() == ".svn" or FileNameMatches("*~.nib", p))
					continue;
	
				result += fs::file_size(p);
			}
		}
		else
			result = fs::file_size(mPath);
	}
	catch (...) {}
	return result;
}

// ---------------------------------------------------------------------------
//	MProjectResource::GetDataSize

string MProjectResource::GetResourceName() const
{
	stack<string> names;
	
	const MProjectItem* item = this;
	while (item != nil)
	{
		names.push(item->GetName());
		item = item->GetParent();
	}
	
	string result = names.top();
	
	while (not names.empty())
	{
		if (result.length() > 0)
			result += '/';
		result += names.top();
		names.pop();
	}
	
	return result;
}

// ---------------------------------------------------------------------------
//	MProjectResource::CheckIsOutOfDate

void MProjectResource::CheckIsOutOfDate(
	MModDateCache&	ioModDateCache)
{
	bool isOutOfDate = false;
	
	MPath path = GetPath();
	MPath object = GetObjectPath();
	
	if (not exists(object))
	{
		isOutOfDate = true;
		mTextSize = 0;
		mDataSize = 0;
	}

	if (isOutOfDate == false)
		isOutOfDate = fs::last_write_time(path) > fs::last_write_time(object);
	
	SetOutOfDate(isOutOfDate);
}

// ---------------------------------------------------------------------------
//	MProjectResource::CheckIsOutOfDate

void MProjectResource::UpdatePaths(
	const MPath&		inObjectDir)
{
	string baseName = GetResourceName();
	
	string::size_type p = 0;
	while ((p = baseName.find('/', p)) != string::npos)
		baseName[p] = '-';
	
	mObjectPath = inObjectDir / (baseName + ".o");
	mDependsPath = inObjectDir / (baseName + ".d");
}

