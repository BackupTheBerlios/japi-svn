//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef MSHELL_H
#define MSHELL_H

#include "MCallbacks.h"

class MShell
{
  public:
							MShell(
								bool				inRedirectStdErr = false);

	virtual					~MShell();
	
	void					Execute(
								const std::string&	inScript);

	void					ExecuteScript(
								const std::string&	inScript,
								const std::string&	inText);

	void					Kill();

	bool					IsRunning() const;
	
	void					SetCWD(
								const std::string&	inCWD);

	std::string				GetCWD() const;

	MCallback<void(const char*, uint32)>	eStdOut;
	MCallback<void(const char*, uint32)>	eStdErr;
	MCallback<void(bool)>					eShellStatus;

  private:
	struct MShellImp*		mImpl;
};

#endif
