//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef MLIB_H
#define MLIB_H

#if defined(_MSC_VER)
#pragma warning (disable : 4355)	// this is used in Base Initializer list
#pragma warning (disable : 4996)	// unsafe function or variable
#pragma warning (disable : 4068)	// unknown pragma
#include <ciso646>
#endif

#if defined(_DEBUG) && ! defined(DEBUG)
#define DEBUG _DEBUG
#endif

#if DEBUG && NDEBUG
#error "Cannot be defined both"
#elif ! defined(NDEBUG) && ! defined(DEBUG)
#pragma message("Neither NDEBUG nor DEBUG is defined, falling back to DEBUG")
#define DEBUG	1
#elif ! defined(NDEBUG)
#define NDEBUG	(! DEBUG)
#elif ! defined(DEBUG)
#define DEBUG	(! NDEBUG)
#endif

#include "MTypes.h"

#include <boost/filesystem/path.hpp>

extern const char kAppName[], kVersionString[];

// ===========================================================================
//	general globals

extern boost::filesystem::path	gPrefsDir;

#endif
