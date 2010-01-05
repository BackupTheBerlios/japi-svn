//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef MRESOURCES_H
#define MRESOURCES_H

#include <istream>

#include <boost/bind.hpp>
#include <boost/iterator/iterator_facade.hpp>
#include <boost/filesystem/operations.hpp>

#include "MObjectFile.h"

/*
	Resources are data sources for the application.
	
	They are retrieved by name.

	Basic usage:
	
	MResource rsrc = MResource::root().find("dialogs/my-cool-dialog.glade");
	
	if (rsrc)
	{
		GladeXML* glade = glade_xml_new_from_buffer(rsrc.data(), rsrc.size(), nil, "japi");
		
		...
	}

*/

struct MResourceImp
{
	uint32			mNext;
	uint32			mChild;
	uint32			mName;
	uint32			mSize;
	uint32			mData;
};

extern const MResourceImp	gResourceIndex[];
extern const char			gResourceData[];
extern const char			gResourceName[];

class MResource
{
  public:
						MResource()
							: mImpl(nil) {}

						MResource(
							const MResource&	rhs)
							: mImpl(rhs.mImpl) {}
	
	MResource&			operator=(
							const MResource& rhs)
						{
							mImpl = rhs.mImpl;
							return *this;
						}

	static MResource	root()
						{
							return MResource(gResourceIndex);
						}

	MResource			find(
							const std::string&	inPath);

	class iterator : public boost::iterator_facade<iterator, MResource, boost::forward_traversal_tag>
	{
		friend class boost::iterator_core_access;
		friend class MResource;
	  public:

		MResource&		dereference() const
						{
							return *mRsrc;
						}

		void			increment()
						{
							if (mRsrc->mImpl->mNext)
								mRsrc->mImpl = gResourceIndex + mRsrc->mImpl->mNext;
							else
								mRsrc->mImpl = nil;
						}

		bool			equal(const iterator& rhs) const
						{
							return mRsrc->mImpl == rhs.mRsrc->mImpl;
						}
	
						iterator(
							const iterator&	rhs)
							: mRsrc(new MResource(rhs.mRsrc->mImpl))
						{
						}
						
						~iterator()
						{
							delete mRsrc;
						}
		
		iterator&		operator=(
							const iterator&	rhs)
						{
							mRsrc->mImpl = rhs.mRsrc->mImpl;
							return *this;
						}

	  private:
						iterator(
							const MResourceImp*	inImpl)
							: mRsrc(new MResource(inImpl)) {}

		MResource*		mRsrc;
	};

	iterator			begin() const
						{
							const MResourceImp* imp = nil;
							if (mImpl->mChild != 0)
								imp = gResourceIndex + mImpl->mChild;
							return iterator(imp);
						}
						
	iterator			end() const
						{
							return iterator(nil);
						}

	std::string			name() const			{ return gResourceName + mImpl->mName; }

	const char*			data() const			{ return gResourceData + mImpl->mData; }
	
	uint32				size() const			{ return mImpl->mSize; }

						operator bool () const	{ return mImpl != nil; }

  private:
						MResource(
							const MResourceImp*	inImpl)
							: mImpl(inImpl) {}

	const MResourceImp*	mImpl;
};

// inlines

inline MResource MResource::find(
	const std::string&	inPath)
{
	MResource rsrc(*this);
	
	fs::path path(inPath);
	
	for (fs::path::iterator p = path.begin(); p != path.end(); ++p)
	{
		MResource::iterator child = std::find_if(
			rsrc.begin(), rsrc.end(), boost::bind(&MResource::name, _1) == *p);
		
		if (child == rsrc.end())
		{
			rsrc = MResource(nil);
			break;
		}
		
		rsrc = *child;
	}
	
	return rsrc;
}

// building resource files:

class MResourceFile
{
  public:
			MResourceFile(
				MTargetCPU			inTarget);
			
			~MResourceFile();
	
	void	Add(
				const std::string&	inPath,
				const void*			inData,
				uint32				inSize);

	void	Add(
				const std::string&	inPath,
				const fs::path&		inFile);
	
	void	Write(
				const fs::path&		inFile);

  private:

	struct MResourceFileImp*		mImpl;
};

#endif
