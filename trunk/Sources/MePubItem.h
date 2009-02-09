//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEPUBITEM_H
#define MEPUBITEM_H

#include "MProjectItem.h"

class MePubItem : public MProjectItem
{
  public:
					MePubItem(
						const std::string&	inName,
						MProjectGroup*		inParent);
	
	virtual bool	IsOutOfDate() const						{ return mIsOutOfDate; }
	virtual void	SetOutOfDate(
						bool				inIsOutOfDate);

	fs::path		GetPath() const							{ return mParent->GetGroupPath() / mName; }

	virtual uint32	GetDataSize() const						{ return mData.length(); }

	std::string		GetID() const							{ return mID; }
	void			SetID(
						const std::string&	inID)			{ mID = inID; }

	std::string		GetData() const							{ return mData; }
	void			SetData(
						const std::string&	inData)			{ mData = inData; }

	std::string		GetMediaType() const					{ return mMediaType; }
	void			SetMediaType(
						const std::string&	inMediaType)	{ mMediaType = inMediaType; }

	void			GuessMediaType();

  protected:
	bool			mIsOutOfDate;
	std::string		mID;
	std::string		mData;
	std::string		mMediaType;
};

class MePubTOCItem : public MProjectGroup
{
  public:
					MePubTOCItem(
						const std::string&	inName,
						MProjectGroup*		inParent)
						: MProjectGroup(inName, inParent) {}
	
	std::string		GetClass() const						{ return mClass; }
	void			SetClass(
						const std::string&	inClass)		{ mClass = inClass; }

	std::string		GetSrc() const							{ return mSrc; }
	void			SetSrc(
						const std::string&	inSrc)			{ mSrc = inSrc; }

  protected:
	std::string		mClass, mSrc;
};

#endif

