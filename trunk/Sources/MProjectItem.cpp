//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "MJapi.h"

#include <vector>
#include <numeric>
#include <stack>

#include <boost/bind.hpp>
#include <boost/filesystem/fstream.hpp>

#include "MFile.h"
#include "MProjectItem.h"
#include "MObjectFile.h"
#include "MError.h"
#include "MStrings.h"

using namespace std;

namespace {

class path_iterator : public boost::iterator_facade
<
	path_iterator,
	string,
	boost::forward_traversal_tag,
	string
>
{
  public:
					path_iterator();		// means 'end'
	
					path_iterator(
						const string&		inData);
					
  private:
	
	friend class boost::iterator_core_access;

	string			dereference() const;

	void			increment();
	bool			equal(
						const path_iterator&	rhs) const;

	const string*	mData;
	uint32			mOffset;
	uint32			mLength;
	bool			mQuoted;
};

path_iterator::path_iterator()
	: mData(nil)
	, mOffset(0)
	, mLength(0)
	, mQuoted(false)
{
}

path_iterator::path_iterator(
	const string&		inData)
	: mData(&inData)
	, mOffset(0)
	, mLength(0)
	, mQuoted(false)
{
	increment();
}

string path_iterator::dereference() const
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

void path_iterator::increment()
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

bool path_iterator::equal(const path_iterator& rhs) const
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
	{
		vector<MProjectItem*>::iterator i = find(
			mParent->GetItems().begin(), mParent->GetItems().end(),
			this);
		
		if (i == mParent->GetItems().end())
			THROW(("tree error"));
		
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

MProjectItem* MProjectItem::GetNext() const
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
	const fs::path&		inParentDir)
	: MProjectItem(inName, inParent)
	, mParentDir(inParentDir)
	, mTextSize(0)
	, mDataSize(0)
	, mIsCompiling(false)
	, mIsOutOfDate(false)
{
	mIsOutOfDate = IsOutOfDate();
}

// ---------------------------------------------------------------------------
//	MProjectFile::GetProjectFileForPath

MProjectFile* MProjectFile::GetProjectFileForPath(
	const fs::path&		inPath) const
{
	MProjectFile* result = nil;
	if (fs::exists(inPath) and fs::exists(GetPath()) and fs::equivalent(inPath, GetPath()))
		result = const_cast<MProjectFile*>(this);
	return result;
}

// ---------------------------------------------------------------------------
//	MProjectFile::SetParentDir

void MProjectFile::SetParentDir(
	const fs::path&		inParentDir)
{
	mParentDir = inParentDir;
}

// ---------------------------------------------------------------------------
//	MProjectFile::UpdatePaths

void MProjectFile::UpdatePaths(
	const fs::path&		inObjectDir)
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
	if (IsCompilable() and mIsOutOfDate != inIsOutOfDate)
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
	uint32 savedTextSize = mTextSize;
	uint32 savedDataSize = mDataSize;
	
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
		
		path_iterator m1(text), m2;
		
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

	if (savedDataSize != mDataSize or savedTextSize != mTextSize)
		eStatusChanged(this);
}

// ---------------------------------------------------------------------------
//	MProjectFile::CheckIsOutOfDate

void MProjectFile::CheckIsOutOfDate(
	MModDateCache&	ioModDateCache)
{
	bool isOutOfDate = false;
	
	fs::path path = mParentDir / mName;
	
	if (not IsCompilable())
		return;

	isOutOfDate = true;

	try
	{
		if (exists(path) and exists(mObjectPath) and exists(mDependsPath))
		{
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
							fs::path dependancy(path);
							ioModDateCache[path] = depModDate = fs::last_write_time(dependancy);
						}
						catch (...) {}
					}
		
					isOutOfDate = depModDate > objectDate;
				}
			}
		}
		else
		{
			mTextSize = 0;
			mDataSize = 0;
		}
	}
	catch (...)
	{
		isOutOfDate = true;
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
	const fs::path&	inPath) const
{
	MProjectFile* result = nil;

	for (vector<MProjectItem*>::const_iterator i = mItems.begin(); result == nil and i != mItems.end(); ++i)
		result = (*i)->GetProjectFileForPath(inPath);

	return result;
}

// ---------------------------------------------------------------------------
//	MProjectGroup::GetProjectResourceForPath

MProjectResource* MProjectGroup::GetProjectResourceForPath(
	const fs::path&		inPath) const
{
	MProjectResource* result = nil;

	for (vector<MProjectItem*>::const_iterator i = mItems.begin(); result == nil and i != mItems.end(); ++i)
		result = (*i)->GetProjectResourceForPath(inPath);

	return result;
}

// ---------------------------------------------------------------------------
//	MProjectGroup::UpdatePaths

void MProjectGroup::UpdatePaths(
	const fs::path&	inObjectDir)
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
		result = i - mItems.begin();
	
	return result;
}

// ---------------------------------------------------------------------------
//	MProjectGroup::AddProjectItem

void MProjectGroup::AddProjectItem(
	MProjectItem*	inItem,
	int32			inPosition)
{
	if (inPosition >= 0 and inPosition <= static_cast<int32>(mItems.size()))
		mItems.insert(mItems.begin() + inPosition, inItem);
	else
		mItems.insert(mItems.end(), inItem);

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
//	MProjectGroup::GetItem

MProjectItem* MProjectGroup::GetItem(
	uint32				inIndex) const
{
	return mItems.at(inIndex);
}

// ---------------------------------------------------------------------------
//	MProjectGroup::GetItem

MProjectItem* MProjectGroup::GetItem(
	const fs::path&		inPath) const
{
	const MProjectItem* result;
	
	fs::path::iterator pi = inPath.begin();
	
	if (pi == inPath.end())
		result = this;
	else
	{
		const MProjectGroup* group = this;
		
		while (group != nil and pi != inPath.end())
		{
			vector<MProjectItem*>::const_iterator item = find_if(
				group->mItems.begin(), group->mItems.end(), boost::bind(&MProjectItem::GetName, _1) == *pi);
		
			if (item == group->mItems.end())
			{
				result = nil;
				break;
			}
		
			++pi;
		
			group = dynamic_cast<MProjectGroup*>(*item);
			if (group != nil)
				result = group;
			else if (pi != inPath.end())
				THROW(("Item/group mismatch for %s", (*item)->GetName().c_str()));
			else
				result = *item;
		}
	}
	
	return const_cast<MProjectItem*>(result);
}

// ---------------------------------------------------------------------------
//	MProjectGroup::Contains

bool MProjectGroup::Contains(
	MProjectItem*		inItem) const
{
	return this == inItem or
		accumulate(mItems.begin(), mItems.end(), false,
			boost::bind(logical_or<bool>(), _1, boost::bind(&MProjectItem::Contains, _2, inItem)));
}

// ---------------------------------------------------------------------------
//	MProjectGroup::GetGroupForPath

MProjectGroup* MProjectGroup::GetGroupForPath(
	const fs::path&		inPath)
{
	MProjectGroup* result = const_cast<MProjectGroup*>(this);
	
	fs::path::iterator pi = inPath.begin();
	while (pi != inPath.end() and not pi->empty())
	{
		vector<MProjectItem*>::iterator item = find_if(
			result->mItems.begin(), result->mItems.end(), boost::bind(&MProjectItem::GetName, _1) == *pi);
		
		if (item == result->mItems.end())
		{
			MProjectGroup* group = new MProjectGroup(*pi, this);
			result->AddProjectItem(group);
			result = group;
		}
		else
		{
			result = dynamic_cast<MProjectGroup*>(*item);
			if (result == nil)
				THROW(("Item %s already exists as non group item", pi->c_str()));
		}

		++pi;
	}
	
	return result;
}

// ---------------------------------------------------------------------------
//	MProjectGroup::GetGroupPath

fs::path MProjectGroup::GetGroupPath() const
{
	fs::path result;
	if (mParent != nil)
		result = mParent->GetGroupPath();
	return result / GetName();
}

// ---------------------------------------------------------------------------
//	MProjectGroup::GetDepth

uint32 MProjectGroup::GetDepth() const
{
	uint32 result = 1;
	
	for (vector<MProjectItem*>::const_iterator i = mItems.begin(); i != mItems.end(); ++i)
	{
		uint32 d = (*i)->GetDepth();
		if (result < d + 1)
			result = d + 1;
	}
	
	return result;
}

// ---------------------------------------------------------------------------
//	MProjectGroup::iterator

//MProjectGroup::iterator::iterator()
//{
//}

MProjectGroup::iterator::iterator(
	const iterator&		inOther)
	: mBegin(inOther.mBegin)
	, mGroup(inOther.mGroup)
	, mOffset(inOther.mOffset)
{
}

MProjectGroup::iterator::iterator(
	MProjectGroup&		inGroup,
	uint32				inOffset)
	: mBegin(&inGroup)
	, mGroup(&inGroup)
	, mOffset(inOffset)
{
}

void MProjectGroup::iterator::increment()
{
	assert(mOffset <= mGroup->mItems.size());
	MProjectGroup* group = dynamic_cast<MProjectGroup*>(mGroup->mItems[mOffset]);
	if (group != nil)
	{
		mGroup = group;
		mOffset = 0;
	}
	else
		++mOffset;
		
	while (mOffset >= mGroup->mItems.size() and mGroup != mBegin)
	{
		MProjectGroup* parent = mGroup->mParent;
		vector<MProjectItem*>::iterator i =
			find(parent->mItems.begin(), parent->mItems.end(), mGroup);
		if (i == parent->mItems.end())
			THROW(("internal error in MProjectItem::iterator"));
		
		mGroup = parent;
		mOffset = (i - parent->mItems.begin()) + 1;
	}
}

MProjectItem& MProjectGroup::iterator::dereference() const
{
	assert(mOffset < mGroup->mItems.size());
	return *mGroup->mItems[mOffset];
}

bool MProjectGroup::iterator::equal(const iterator& inOther) const
{
	return
		mBegin == inOther.mBegin and
		mGroup == inOther.mGroup and
		mOffset == inOther.mOffset;
}

// ---------------------------------------------------------------------------
//	MProjectCpFile::GetDestPath

fs::path MProjectCpFile::GetDestPath(
	const fs::path&		inPackageDir) const
{
	stack<MProjectGroup*>	sp;
	
	MProjectGroup* parent = dynamic_cast<MProjectGroup*>(mParent);
	while (parent != nil)
	{
		sp.push(parent);
		parent = dynamic_cast<MProjectGroup*>(parent->GetParent());
	}
	
	fs::path result = inPackageDir;
	
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
			fs::path p;
			
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
//	MProjectLib::GetDisplayName

string MProjectLib::GetDisplayName() const
{
	string name = MProjectItem::GetDisplayName();
	
	if (FileNameMatches("lib*.a", name))
		name = name.substr(3, name.length() - 5);
	else if (FileNameMatches("lib*.so", name))
		name = name.substr(3, name.length() - 6);

	string tags;
	if (mShared)
		tags += _("shared");
	else
		tags += _("static");
	
	if (mOptional)
	{
		tags += ", ";
		tags += _("optional");
	}
	
	name += string(" (") + tags + ")";
	return name;
}

// ---------------------------------------------------------------------------
//	MProjectLib::GetLibraryName

string MProjectLib::GetLibraryName() const
{
	string name = MProjectItem::GetName();
	
	if (not FileNameMatches("lib*.a", name) and
		not FileNameMatches("lib*.so", name))
	{
		name.insert(0, "lib");
		if (mShared)
			name += ".so";
		else
			name += ".a";
	}

	return name;
}

// ---------------------------------------------------------------------------
//	MProjectLib::SetOptional

void MProjectLib::SetOptional(
	bool		inOptional)
{
	mOptional = inOptional;
	eStatusChanged(this);
}

// ---------------------------------------------------------------------------
//	MProjectLib::SetShared

void MProjectLib::SetShared(
	bool		inShared)
{
	mShared = inShared;
	eStatusChanged(this);
}

// ---------------------------------------------------------------------------
//	MProjectResource::GetResourceName

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
	
	fs::path path = GetPath();
	fs::path object = GetObjectPath();
	
	if (not fs::exists(object) or not fs::exists(path))
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
//	MProjectResource::GetProjectResourceForPath

MProjectResource* MProjectResource::GetProjectResourceForPath(
	const fs::path&		inPath) const
{
	MProjectResource* result = nil;
	if (fs::exists(inPath) and fs::exists(GetPath()) and fs::equivalent(inPath, GetPath()))
		result = const_cast<MProjectResource*>(this);
	return result;
}

// ---------------------------------------------------------------------------
//	MProjectResource::UpdatePaths

void MProjectResource::UpdatePaths(
	const fs::path&		inObjectDir)
{
	mObjectPath = inObjectDir / "__rsrc__.o";
}

// ---------------------------------------------------------------------------
//	MProjectResource::GetDataSize

uint32 MProjectResource::GetDataSize() const
{
	uint32 result = 0;
	try
	{
		result = fs::file_size(GetPath());
	}
	catch (...) {}
	return result;
}

