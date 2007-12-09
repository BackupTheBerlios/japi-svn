#ifndef MPKGCONFIG_H
#define MPKGCONFIG_H

#include <vector>
#include "MFile.h"

void PkgConfigGetCFlags(
	const std::string&			inPackage,
	std::vector<std::string>&	outCFlags,
	std::vector<std::string>&	outIncludeDirs);

#endif
