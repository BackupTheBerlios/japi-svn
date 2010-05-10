//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef MOBJECTFILEIMP_MACHO_H
#define MOBJECTFILEIMP_MACHO_H

#include "MObjectFile.h"

struct MMachoObjectFileImp : public MObjectFileImp
{
	template<class SWAPPER>
	void			Read(
						struct mach_header&	mh,
						std::istream&		inData);
	
	virtual void	Read(
						const fs::path&		inFile);

	virtual void	Write(
						const fs::path&		inFile);
};

#endif
