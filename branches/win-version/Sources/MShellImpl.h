//          Copyright Maarten L. Hekkelman 2006-2010
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef MSHELLIMPL_H
#define MSHELLIMPL_H

#include <map>

#include "MShell.h"

typedef std::map<std::string,std::string>		Env;

struct MShellImpl
{
	static MShellImpl*
					Create(
						MCallback<void(const char*, uint32)>&	inStdOut,
						MCallback<void(const char*, uint32)>&	inStdErr,
						MCallback<void(bool)>&					inShellStatus);

					MShellImpl(
						MCallback<void(const char*, uint32)>&	inStdOut,
						MCallback<void(const char*, uint32)>&	inStdErr,
						MCallback<void(bool)>&					inShellStatus)
						: eStdOut(inStdOut)
						, eStdErr(inStdErr)
						, eShellStatus(inShellStatus)			{}
		
	virtual 		~MShellImpl();
	
	virtual void	SetCWD(
						const std::string&		inCWD)			{ mENV["PWD"] = inCWD; }

	virtual std::string
					GetCWD()									{ return mENV["PWD"]; }

	virtual void	Execute(
						std::vector<char*>&	argv) = 0;

	virtual void	Execute(
						const std::string&	inCommand) = 0;

	virtual void	ExecuteScript(
						const std::string&	inScript,
						const std::string&	inText) = 0;

	virtual void	Abort() = 0;

	MCallback<void(const char*, uint32)>&	eStdOut;
	MCallback<void(const char*, uint32)>&	eStdErr;
	MCallback<void(bool)>&					eShellStatus;

	Env				mENV;
};

#endif
