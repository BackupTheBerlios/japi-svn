//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef MALERTS_H
#define MALERTS_H

#include <sstream>
#include <vector>

void DisplayError(
	const std::exception&	inException);

GtkWidget* CreateAlertWithArgs(
	const char* 				inResourceName,
	std::vector<std::string>&	inArgs);

int32 DisplayAlertWithArgs(
	const char*					inResourceName,
	std::vector<std::string>&	inArgs);

template<class T>
inline 
void AddArgument(
	std::vector<std::string>&	inArgs,
	const T&					inArg)
{
	std::stringstream s;
	s << inArg;
	inArgs.push_back(s.str());
}

inline
int32 DisplayAlert(
	const char*			inResourceName)
{
	std::vector<std::string> args;
	return DisplayAlertWithArgs(inResourceName, args);
}

template<class T1>
int32 DisplayAlert(
	const char*			inResourceName,
	const T1&			inArg1)
{
	std::vector<std::string> args;
	AddArgument(args, inArg1);
	return DisplayAlertWithArgs(inResourceName, args);
}

template<class T1, class T2>
int32 DisplayAlert(
	const char*			inResourceName,
	const T1&			inArg1,
	const T2&			inArg2)
{
	std::vector<std::string> args;
	AddArgument(args, inArg1);
	AddArgument(args, inArg2);
	return DisplayAlertWithArgs(inResourceName, args);
}

template<class T1, class T2, class T3>
int32 DisplayAlert(
	const char*			inResourceName,
	const T1&			inArg1,
	const T2&			inArg2,
	const T3&			inArg3)
{
	std::vector<std::string> args;
	AddArgument(args, inArg1);
	AddArgument(args, inArg2);
	AddArgument(args, inArg3);
	return DisplayAlertWithArgs(inResourceName, args);
}

template<class T1>
GtkWidget* CreateAlert(
	const char*			inResourceName,
	const T1&			inArg1)
{
	std::vector<std::string> args;
	AddArgument(args, inArg1);
	return CreateAlertWithArgs(inResourceName, args);
}

#endif
