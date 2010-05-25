//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <Windows.h>

#include "MLib.h"

#include <sstream>
#include <list>
#include <limits>

#include <boost/filesystem/fstream.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/bind.hpp>

#include "MObjectFile.h"
#include "MResources.h"
#include "MPatriciaTree.h"
#include "MError.h"

using namespace std;

namespace MWinRSRC
{

using namespace mrsrc;

rsrc_imp* Load()
{
	//HRSRC hres = ::FindResourceA(nil, "ResourceData", "MYDATA");
	//ThrowIfNil(hres);
	//
	//HGLOBAL hmem = ::LoadResource(nil, hres);
	//ThrowIfNil(hmem);
	//
	//gResData = reinterpret_cast<char*>(::LockResource(hmem));
	//ThrowIfNil(gResData);
	return nil;
}

}

const mrsrc::rsrc_imp gResourceIndex[1] = {};
const char gResourceData[] = "\0\0\0\0";
const char gResourceName[] = "\0\0\0\0";

