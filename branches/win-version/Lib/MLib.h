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
#pragma warning (disable : 4996)	// stl copy is unsafe?
#include <ciso646>
#include <sdkddkver.h>
#endif

#if defined(_DEBUG) && ! defined(DEBUG)
#define DEBUG _DEBUG
#endif

#if DEBUG && NDEBUG
#error "Cannot be defined both"
#elif ! defined(NDEBUG) && ! defined(DEBUG)
#pragma message("Neither NDEBUG nor DEBUG is defined, falling back to DEBUG")
#define DEBUG	1
#endif

#include "MTypes.h"

#endif
