//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef MWINLIB_H
#define MWINLIB_H

#pragma warning (disable : 4355)	// this is used in Base Initializer list
#pragma warning (disable : 4996)	// unsafe function or variable
#pragma warning (disable : 4068)	// unknown pragma
#include <ciso646>

#include <Windows.h>
#include <CommCtrl.h>
#include <d2d1.h>
#include <d2d1helper.h>
#include <dwrite.h>
#include <Uxtheme.h>
#include <vssym32.h>
#include <shlobj.h>
#include <objbase.h>      // For COM headers
#include <shobjidl.h>     // for IFileDialogEvents and IFileDialogControlEvents
#include <shlwapi.h>

#undef GetNextWindow
#undef GetWindow
#undef PlaySound

#include "MTypes.h"

#include <boost/filesystem/path.hpp>

extern const char kAppName[], kVersionString[];

// ===========================================================================
//	general globals

extern boost::filesystem::path	gPrefsDir;

#endif
