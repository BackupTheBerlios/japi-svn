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

//#include "mrsrc.h"

#include "MObjectFile.h"

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


// access to resources

namespace mrsrc {

struct rsrc_imp;

class rsrc_not_found_exception : public std::exception
{
  public:
    virtual const char* what() const throw()		{ return "resource not found"; }
};

class rsrc;
typedef std::list<rsrc> rsrc_list;

class rsrc
{
  public:
						rsrc();

						rsrc(
							const rsrc&			other);

	rsrc&				operator=(
							const rsrc&			other);
	
						rsrc(
							const std::string&	path);

	std::string			name() const;

	const char*			data() const;
	
	unsigned long		size() const;

						operator bool () const;

	rsrc_list			children() const;

  private:

						rsrc(
							const rsrc_imp*		imp);

	const rsrc_imp*		m_impl;
};

}

#endif
