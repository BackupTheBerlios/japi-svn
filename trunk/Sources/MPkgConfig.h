//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef MPKGCONFIG_H
#define MPKGCONFIG_H

#include <vector>
#include "MFile.h"

void GetPkgConfigResult(
	const std::string&			inPackage,
	const char*					inInfo,
	std::vector<std::string>&	outFlags);

void GetPkgConfigPackagesList(
	std::vector<std::pair<std::string,std::string> >&
								outPackages);

void GetCompilerPaths(
	const std::string&			inCompiler,
	std::string&				outCppIncludeDir,
	std::string&				outSysIncludeDir,
	std::vector<fs::path>&		outLibDirs);

void GetToolConfigResult(
	const std::string&			inTool,
	const char*					inArgs[],
	std::vector<std::string>&	outFlags);

#endif
