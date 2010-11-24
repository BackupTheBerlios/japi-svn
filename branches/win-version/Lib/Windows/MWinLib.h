//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef MWINLIB_H
#define MWINLIB_H

#include <SDKDDKVer.h>

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers

#include <Windows.h>
#include <d2d1.h>
#include <dwrite.h>
#include <CommCtrl.h>
#include <ShlObj.h>
#include <Shlwapi.h>
#include <ShellAPI.h>
#include <Uxtheme.h>
#include <vssym32.h>

#undef GetNextWindow
#undef PlaySound
#undef GetWindow
#undef ClipRegion
#undef CreateDialog

#pragma warning (disable : 4355)	// this is used in Base Initializer list
#pragma warning (disable : 4996)	// unsafe function or variable
#pragma warning (disable : 4068)	// unknown pragma
#pragma warning (disable : 4996)	// stl copy()

#include <ciso646>

#include "MLib.h"

#include "MTypes.h"

#endif
