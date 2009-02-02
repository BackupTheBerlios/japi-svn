//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef MPROJECTITEM_H
#define MPROJECTITEM_H

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
	
	uint32			GetLevel() const;

	virtual void	Flatten(
						std::vector<MProjectItem*>&
											outItems)		{ outItems.push_back(this); }

	virtual bool	Contains(
						MProjectItem*		inItem) const	{ return this == inItem; }

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

	virtual bool	IsOptional() const						{ return mOptional; }
	virtual void	SetOptional(
						bool				inOptional)		{ mOptional = inOptional; }

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
	bool			mOptional;
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

	virtual int32	Count() const							{ return mItems.size(); }

	std::vector<MProjectItem*>&
					GetItems()								{ return mItems; }

	MProjectItem*	GetItem(
						uint32				inIndex) const;

	virtual bool	Contains(
						MProjectItem*		inItem) const;

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
						MProjectGroup*		inParent)
						: MProjectItem(inName, inParent) {}
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
