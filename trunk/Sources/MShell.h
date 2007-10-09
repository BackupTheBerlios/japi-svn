/*
	Copyright (c) 2006, Maarten L. Hekkelman
	All rights reserved.

	Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions
	are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in
      the documentation and/or other materials provided with the
      distribution.
    * Neither the name of the Maarten L. Hekkelman nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
	"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
	LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
	FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
	COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
	INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
	(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
	SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
	HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
	STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
	ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
	OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/*	$Id$

	This file contains my shell implementation.
	
	It all started with a extremely simplistic parser...
	
	


*/


#ifndef MSHELL_H
#define MSHELL_H

#include <map>
#include <vector>
#include <string>

#include "MP2PEvents.h"

class MShell
{
  public:
							MShell(bool inRedirectStdErr = false);
	virtual					~MShell();					
	
	void					Execute(std::string inCommand);
	void					Kill();
	bool					IsRunning() const				{ return mPid > 0; }
	
	void					SetCWD(std::string inCWD);
	std::string				GetCWD() const					{ return mCurDir; }

	MEventOut<void(const char*, uint32)>	eStdOut;
	MEventOut<void(const char*, uint32)>	eStdErr;
	MEventOut<void(bool)>					eShellStatus;

	MEventIn<void()>						ePoll;

  private:

	typedef	std::map<std::string,std::string>	Env;
	typedef std::vector<std::string>			Args;

	char					GetNextChar();
	void					Restart(int& ioStart, int& ioState);
	void					Retract();
	std::string				ParseSQString();
	std::string				ParseDQString();
	std::string				ParseCmd();
	int						GetNextToken();
	void					Parse();
	void					ExecuteSubCommand(Args& inArgs, Env& inEnv);

	void					Poll();
	
	std::string				NextPath(std::string& ioPathVar, std::string inName);

	// built-in
	int						Cd(int argc, char* const argv[], char* const env[]);
	int						Pwd(int argc, char* const argv[], char* const env[]);
	int						Echo(int argc, char* const argv[], char* const env[]);
	int						Exec(int argc, char* const argv[], char* const env[]);

	int						mPid;
	int						mStdInFD;
	int						mStdOutFD;
	int						mStdErrFD;
	bool					mRedirectStdErr, mOutDone, mErrDone;
	Env						mEnvironment;
	std::string				mCurDir, mPrevDir;
	std::string				mToken;
	
	std::string				mCommand;
	std::string::iterator	mCommandBegin;
	std::string::iterator	mCommandPtr;
	std::string::iterator	mCommandEnd;
	int						mCommandOverflow;
	
};

#endif
