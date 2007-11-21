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

	virtual MProjectFile*
					GetProjectFileForPath(
						const MPath&		inPath) const	{ return nil; }

	virtual void	UpdatePaths(
						const MPath&		inObjectDir)	{}

	virtual void	CheckCompilationResult()				{}

	virtual void	CheckIsOutOfDate(
						MModDateCache&		ioModDateCache)	{}

	virtual bool	IsCompilable() const					{ return false; }
	virtual bool	IsCompiling() const						{ return false; }
	
	virtual bool	IsOutOfDate() const						{ return false; }
	virtual void	SetOutOfDate(
						bool				inIsOutOfDate)	{}

	MEventOut<void()>						eStatusChanged;
	
	virtual uint32	GetTextSize() const						{ return 0; }
	virtual uint32	GetDataSize() const						{ return 0; }

	std::string		GetName() const							{ return mName; }
	
	uint32			GetLevel() const;

	virtual void	Flatten(
						std::vector<MProjectItem*>&
											outItems)		{ outItems.push_back(this); }

	virtual uint32	Count() const							{ return 1; }

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
						const MPath&		inParentDir);
	
	virtual void	UpdatePaths(
						const MPath&		inObjectDir);

	void			SetParentDir(
						const MPath&		inParentDir);

	virtual MProjectFile*
					GetProjectFileForPath(
						const MPath&		inPath) const;

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
	
	MPath			GetPath() const							{ return mParentDir / mName; }
	const MPath&	GetObjectPath() const					{ return mObjectPath; }
	const MPath&	GetDependsPath() const					{ return mDependsPath; }

	virtual uint32	GetTextSize() const						{ return mTextSize; }
	virtual uint32	GetDataSize() const						{ return mDataSize; }

  protected:
	MPath			mParentDir;
	MPath			mObjectPath;
	MPath			mDependsPath;
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
						const MPath&		inPath) const;
	
	virtual void	UpdatePaths(
						const MPath&		inObjectDir);

	virtual void	CheckCompilationResult();

	virtual void	CheckIsOutOfDate(
						MModDateCache&		ioModDateCache);
	
	virtual bool	IsOutOfDate() const;
	virtual void	SetOutOfDate(
						bool				inIsOutOfDate);

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

	virtual uint32	Count() const;

	std::vector<MProjectItem*>&
					GetItems()								{ return mItems; }

  private:
	std::vector<MProjectItem*>
					mItems;
};

// ---------------------------------------------------------------------------
//	MProjectMkDir

class MProjectMkDir : public MProjectGroup
{
  public:
					MProjectMkDir(
						const std::string&	inName,
						MProjectGroup*		inParent)
						: MProjectGroup(inName, inParent) {}
};

// ---------------------------------------------------------------------------
//	MProjectCpFile

class MProjectCpFile : public MProjectItem
{
  public:
					MProjectCpFile(
						const std::string&	inName,
						MProjectGroup*		inParent,
						const MPath&		inPath)
						: MProjectItem(inName, inParent)
						, mPath(inPath) {}

	const MPath&	GetSourcePath() const					{ return mPath; }

	MPath			GetDestPath(
						const MPath& inPackageDir) const;

	virtual uint32	GetDataSize() const;

  private:
	MPath			mPath;
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
						MPath				inParentDir)
						: MProjectFile(inName, inParent, inParentDir) {}

	virtual bool	IsCompilable() const					{ return true; }

	virtual void	UpdatePaths(
						const MPath&		inObjectDir);

	virtual void	CheckIsOutOfDate(
						MModDateCache&		ioModDateCache);
	
	std::string		GetResourceName() const;
};

#endif
