#ifndef MSTRINGS_H
#define MSTRINGS_H

#include <string>
#include <vector>
#include <sstream>

#define _(s)	(GetLocalisedString(s))

const char* GetLocalisedString(
				const char* 		inString);

std::string GetLocalisedString(
				const std::string&	inString);

std::string GetFormattedLocalisedStringWithArguments(
				const std::string&	inString,
				const std::vector<std::string>&
									inArgs);

template<class T1>
std::string FormatString(
				const char*			inString,
				const T1&			inArg1);

template<class T1, class T2>
std::string FormatString(
				const char*			inString,
				const T1&			inArg1,
				const T2&			inArg2);

template<class T1, class T2, class T3>
std::string FormatString(
				const char*			inString,
				const T1&			inArg1,
				const T2&			inArg2,
				const T3&			inArg3);

// --------------------------------------------------------------------

template<class T>
inline 
void PushArgument(
	std::vector<std::string>&	inArgs,
	const T&					inArg)
{
	std::stringstream s;
	s << inArg;
	inArgs.push_back(s.str());
}

inline
void PushArgument(
	std::vector<std::string>&	inArgs,
	const char*					inArg)
{
	inArgs.push_back(GetLocalisedString(inArg));
}

template<>
inline
void PushArgument(
	std::vector<std::string>&	inArgs,
	const std::string&			inArg)
{
	inArgs.push_back(GetLocalisedString(inArg.c_str()));
}

template<class T1>
std::string FormatString(
	const char*		inString,
	const T1&		inArg1)
{
	std::vector<std::string> args;
	PushArgument(args, inArg1);
	return GetFormattedLocalisedStringWithArguments(inString, args);
}

template<class T1, class T2>
std::string FormatString(
	const char*		inString,
	const T1&		inArg1,
	const T2&		inArg2)
{
	std::vector<std::string> args;
	PushArgument(args, inArg1);
	PushArgument(args, inArg2);
	return GetFormattedLocalisedStringWithArguments(inString, args);
}

template<class T1, class T2, class T3>
std::string FormatString(
	const char*		inString,
	const T1&		inArg1,
	const T2&		inArg2,
	const T3&		inArg3)
{
	std::vector<std::string> args;
	PushArgument(args, inArg1);
	PushArgument(args, inArg2);
	PushArgument(args, inArg3);
	return GetFormattedLocalisedStringWithArguments(inString, args);
}

#endif
