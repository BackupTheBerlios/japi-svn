//          Copyright Maarten L. Hekkelman 2006-2010
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef MWINUTILS_H
#define MWINUTILS_H

#include <string>

std::wstring c2w(std::string& s);
std::string w2c(std::wstring& s);

#endif