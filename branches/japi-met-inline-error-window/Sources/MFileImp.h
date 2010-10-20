//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef MFILEIMP_H
#define MFILEIMP_H

struct MFileImp
{
	virtual bool			Equivalent(const MFileImp* rhs) const = 0;
	virtual std::string		GetURI() const = 0;
	virtual fs::path		GetPath() const = 0;
	virtual std::string		GetScheme() const = 0;
	virtual std::string		GetFileName() const = 0;
	virtual bool			IsLocal() const = 0;
	virtual MFileImp*		GetParent() const = 0;
	virtual MFileImp*		GetChild(const fs::path& inSubPath) const = 0;
	
	virtual MFileLoader*	Load(MFile& inFile) = 0;
	virtual MFileSaver*		Save(MFile& inFile) = 0;

							MFileImp()
								: mRefCount(1)
							{
							}

	MFileImp*				Reference()
							{
								++mRefCount;
								return this;
							}
	
	void					Release()
							{
								if (--mRefCount <= 0)
									delete this;
							}

  protected:
	virtual					~MFileImp()
							{
								assert(mRefCount == 0);
							}
  private:
	int32					mRefCount;
};

#endif
