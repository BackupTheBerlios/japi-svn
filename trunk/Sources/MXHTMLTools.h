//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef MXHTMLTOOLS_H
#define MXHTMLTOOLS_H

#include <string>
#include <vector>

namespace MXHTMLTools
{

enum Level {
	info, error, warning
};

struct Problem
{
	Level			kind;
	uint32			line, column;
	std::string		message;
};

typedef std::vector<Problem>	Problems;

void ValidateXHTML(
	const std::string&	inText,
	Problems&			outProblems);

void ConvertAnyToXHTML(
	std::string&		ioText,
	Problems&			outProblems);

}

#endif
