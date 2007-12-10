#ifndef MPKGCONFIG_H
#define MPKGCONFIG_H

#include <vector>
#include "MFile.h"

void GetPkgConfigResult(
	const std::string&			inPackage,
	const char*					inInfo,
	std::vector<std::string>&	outFlags);

#endif
