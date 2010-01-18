//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef MPROJECTITEM_H
#define MPROJECTITEM_H

#include <vector>

#include "MPatriciaTree.h"
#include "MP2PEvents.h"

typedef MPatriciaTree<double> MModDateCache;

// ---------------------------------------------------------------------------
//	MProjectItem

class MProjectFile;
class MProjectGroup;

class MProjectItem
{
  public:
					MProjectItem(
						const std::string&	inName,
						MProjectGroup*		inParent);
	virtual			~MProjectItem()	{}

	MProjectGroup*	GetParent() const						{ return mParent; }
	void			SetParent(
						MProjectGroup*		inParent)		{ mParent = inParent; }

	virtual int32	GetPosition() const;
	
	MProjectItem*	GetNext() const;

	virtual MProjectFile*
					GetProjectFileForPath(
						const fs::path&		inPath) const	{ return nil; }

	virtual void	UpdatePaths(
						const fs::path&		inObjectDir)	{}

	virtual void	CheckCompilationResult()				{}

	virtual void	CheckIsOutOfDate(
						MModDateCache&		ioModDateCache)	{}

	virtual bool	IsCompilable() const					{ return false; }
	virtual bool	IsCompiling() const						{ return false; }
	
	virtual bool	IsOutOfDate() const						{ return false; }
	virtual void	SetOutOfDate(
						bool				inIsOutOfDate)	{}

	MEventOut<void(MProjectItem*)>			eStatusChanged;
	
	virtual uint32	GetTextSize() const						{ return 0; }
	virtual uint32	GetDataSize() const						{ return 0; }

	std::string		GetName() const							{ return mName; }
	void			SetName(
						const std::string&	inName)			{ mName = inName; }
	
	virtual std::string
					GetDisplayName() const					{ return GetName(); }
	
	uint32			GetLevel() const;

	virtual void	Flatten(
						std::vector<MProjectItem*>&
											outItems)		{ outItems.push_back(this); }

	virtual bool	Contains(
						MProjectItem*		inItem) const	{ return this == inItem; }

	virtual uint32	GetDepth() const						{ return 1; }

  protected:
					
	std::string		mName;
	MProjectGroup*	mParent;

  private:
					MProjectItem(
						const MProjectItem&);
	MProjectItem&	operator=(
						const MProjectItem&);
};

// ---------------------------------------------------------------------------
//	MProjectFile

class MProjectFile : public MProjectItem
{
  public:
					MProjectFile(
						const std::string&	inName,
						MProjectGroup*		inParent,
						const fs::path&		inParentDir);
	
	virtual void	UpdatePaths(
						const fs::path&		inObjectDir);

	void			SetParentDir(
						const fs::path&		inParentDir);

	virtual MProjectFile*
					GetProjectFileForPath(
						const fs::path&		inPath) const;

	virtual void	CheckCompilationResult();

	virtual void	CheckIsOutOfDate(
						MModDateCache&		ioModDateCache);

	virtual bool	IsOutOfDate() const						{ return mIsOutOfDate; }
	virtual void	SetOutOfDate(
						bool				inIsOutOfDate);

	virtual bool	IsCompilable() const					{ return FileNameMatches("*.cpp;*.c", mName); }

	virtual bool	IsCompiling() const						{ return mIsCompiling; }
	void			SetCompiling(
						bool				inIsCompiling);
	
	fs::path		GetPath() const							{ return mParentDir / mName; }
	const fs::path&	GetObjectPath() const					{ return mObjectPath; }
	const fs::path&	GetDependsPath() const					{ return mDependsPath; }

	virtual uint32	GetTextSize() const						{ return mTextSize; }
	virtual uint32	GetDataSize() const						{ return mDataSize; }

  protected:
	fs::path		mParentDir;
	fs::path		mObjectPath;
	fs::path		mDependsPath;
	std::vector<std::string>
					mIncludedFiles;
	uint32			mTextSize;
	uint32			mDataSize;
	bool			mIsCompiling;
	bool			mIsOutOfDate;
};

// ---------------------------------------------------------------------------
//	MProjectGroup

class MProjectGroup : public MProjectItem
{
  public:
					MProjectGroup(
						const std::string&	inName,
						MProjectGroup*		inParent);
	virtual			~MProjectGroup();

	virtual MProjectFile*
					GetProjectFileForPath(
						const fs::path&		inPath) const;
	
	virtual void	UpdatePaths(
						const fs::path&		inObjectDir);

	virtual void	CheckCompilationResult();

	virtual void	CheckIsOutOfDate(
						MModDateCache&		ioModDateCache);
	
	virtual bool	IsOutOfDate() const;
	virtual void	SetOutOfDate(
						bool				inIsOutOfDate);

	virtual bool	IsCompiling() const;

	virtual int32	GetItemPosition(
						const MProjectItem*	inItem) const;

	void			AddProjectItem(
						MProjectItem*		inItem,
						int32				inPosition = -1);
	
	void			RemoveProjectItem(
						MProjectItem*		inItem);

	virtual uint32	GetTextSize() const;
	virtual uint32	GetDataSize() const;

	virtual void	Flatten(
						std::vector<MProjectItem*>&
											outItems);

	class iterator : public boost::iterator_facade<iterator, MProjectItem, boost::forward_traversal_tag>
	{
	  public:
						iterator(const iterator& inOther);
						iterator(MProjectGroup& inGroup, uint32 inOffset);
		
	  private:
		friend class boost::iterator_core_access;

		void			increment();
		MProjectItem&	dereference() const;
		bool			equal(const iterator& inOther) const;
		
		MProjectGroup*	mBegin;
		MProjectGroup*	mGroup;
		uint32			mOffset;
	};
	
	iterator		begin()									{ return iterator(*this, 0); }
	iterator		end()									{ return iterator(*this, mItems.size()); }

	virtual int32	Count() const							{ return mItems.size(); }

	std::vector<MProjectItem*>&
					GetItems()								{ return mItems; }

	MProjectItem*	GetItem(
						uint32				inIndex) const;

	MProjectItem*	GetItem(
						const fs::path&		inPath) const;

	virtual bool	Contains(
						MProjectItem*		inItem) const;

	MProjectGroup*	GetGroupForPath(
						const fs::path&		inPath);

	fs::path		GetGroupPath() const;
	
	virtual uint32	GetDepth() const;

  private:
	std::vector<MProjectItem*>
					mItems;
};

// ---------------------------------------------------------------------------
//	MProjectCpFile

class MProjectCpFile : public MProjectItem
{
  public:
					MProjectCpFile(
						const std::string&	inName,
						MProjectGroup*		inParent,
						const fs::path&		inPath)
						: MProjectItem(inName, inParent)
						, mPath(inPath) {}

	const fs::path&	GetSourcePath() const					{ return mPath; }

	fs::path		GetDestPath(
						const fs::path& inPackageDir) const;

	virtual uint32	GetDataSize() const;

  private:
	fs::path			mPath;
};

// ---------------------------------------------------------------------------
//	MProjectLib

class MProjectLib : public MProjectItem
{
  public:
					MProjectLib(
						const std::string&	inName,
						bool				inStatic,
						bool				inOptional,
						MProjectGroup*		inParent)
						: MProjectItem(inName, inParent)
						, mStatic(inStatic)
						, mOptional(inOptional) {}

	virtual std::string
					GetDisplayName() const;

	virtual std::string
					GetLibraryName() const;

	bool			IsOptional() const				{ return mOptional; }
	void			SetOptional(
						bool		inOptional)		{ mOptional = inOptional; }

	bool			IsStatic() const				{ return mStatic; }
	void			SetStatic(
						bool		inStatic)		{ mStatic = inStatic; }

  private:
	bool			mStatic, mOptional;
};

// ---------------------------------------------------------------------------
//	MProjectResource

class MProjectResource : public MProjectFile
{
  public:
					MProjectResource(
						const std::string&	inName,
						MProjectGroup*		inParent,
						fs::path			inParentDir)
						: MProjectFile(inName, inParent, inParentDir) {}

	virtual bool	IsCompilable() const					{ return true; }

	virtual void	UpdatePaths(
						const fs::path&		inObjectDir);

	virtual void	CheckIsOutOfDate(
						MModDateCache&		ioModDateCache);
	
	std::string		GetResourceName() const;

	virtual uint32	GetDataSize() const;
};

#endif
