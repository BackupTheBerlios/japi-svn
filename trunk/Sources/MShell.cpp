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

#include "MJapieG.h"

#include <fcntl.h>
#include <unistd.h>

#include <sys/param.h>
#include <sys/sysctl.h>
#include <sys/user.h>
#include <sys/stat.h>
#include <sys/types.h>
#if __MWERKS__
#define _ANSI_SOURCE 1
#endif
#include <sys/wait.h>
#include <cerrno>

#include <iostream>
#include <sstream>
#include <getopt.h>

#include "MShell.h"
#include "MError.h"
#include "MPreferences.h"

extern char** environ;

using namespace std;

namespace
{

enum TOKEN {
	T_EOF = 0,
	T_EOLN,
	T_CMD,			// mToken contains the builtin command name
	T_ENV,			// an environment pair VAR=VAL
	T_SQ_STR,		// single quoted string
	T_DQ_STR,		// double quoted string
	T_PIPE,
	T_REDIRECT,		// mToken contains destination
	
	T_UNKNOWN
};

struct MShellException : public MException
{
	MShellException(const char* inErr, ...);
};

MShellException::MShellException(const char* inErr, ...)
{
	va_list vl;
	va_start(vl, inErr);
	vsnprintf(mMessage, sizeof(mMessage), inErr, vl);
	va_end(vl);
}

#define error(x)	do { StOKToThrow ok; throw MShellException x; } while (false)
	
}

MShell::MShell(bool inRedirectStdErr)
	: ePoll(this, &MShell::Poll)
	, mPid(0)
	, mStdOutFD(-1)
	, mStdErrFD(-1)
	, mRedirectStdErr(inRedirectStdErr)
{
	for (char** e = environ; *e != NULL; ++e)
	{
		string v = *e;
		string::size_type n = v.find('=');
		string k = v.substr(0, n);
		v.erase(0, n + 1);

		mEnvironment[k] = v;
		
		if (k == "HOME")
			mCurDir = v;
	}
}

MShell::~MShell()
{
	if (mPid > 0)
		Kill();

	RemoveRoute(ePoll, gApp->eIdle);
}

void MShell::SetCWD(string inCWD)
{
	mCurDir = inCWD;
}

char MShell::GetNextChar()
{
	char result = 0;
	if (mCommandPtr != mCommandEnd)
	{
		result = *mCommandPtr++;
		mToken += result;
	}
	else
		++mCommandOverflow;
	return result;
}

void MShell::Restart(int& ioStart, int& ioState)
{
	switch (ioStart)
	{
		case 0:		ioStart = 100;	break;
//		case 10:	ioStart = 20;	break;
//		case 20:	ioStart = 100;	break;
		case 100:	ioStart = 200;	break;
		default:	error(("internal error"));
	}

	ioState = ioStart;

	mToken.clear();
	mCommandPtr = mCommandBegin;
	mCommandOverflow = 0;
}

void MShell::Retract()
{
	if (mCommandOverflow > 0)
		--mCommandOverflow;
	else
		--mCommandPtr;

	mToken.erase(mToken.length() - 1, 1);
}

string MShell::ParseSQString()
{
	string result;
	
	for (;;)
	{
		char ch = GetNextChar();
		
		if (ch == 0)
			error(("unterminated string, expected '"));
		
		if (ch == '\'')
			break;
		
		result += ch;
	}
	
	return result;
}

string MShell::ParseDQString()
{
	string result, var;
	bool done = false;
	int state = 0;
	
	while (not done)
	{
		char ch = GetNextChar();
		
		switch (state)
		{
			case 0:
				if (ch == '"')
					done = true;
				else if (ch == '$')
					state = 10;
				else if (ch == '\\')
					state = 20;
				else if (ch == 0)
					error(("unterminated string, missing double quote"));
				else
					result += ch;
				break;
			
			case 10:
				if (isalpha(ch) or ch == '_')
				{
					var = ch;
					state = 11;
				}
				else
				{
					Retract();
					result += '$';
					state = 0;
				}
				break;
			
			case 11:
				if (not isalnum(ch) and ch != '_')
				{
					Retract();
					result += mEnvironment[var];
					state = 0;
				}
				else
					var += ch;
				break;
				
			case 20:
				if (ch == '$' or ch == '`' or ch == '"' or ch == '\\' or ch == '\n')
				{
					result.erase(result.length() - 1, 1);
					result[result.length() - 1] = ch;
				}
				else
				{
					Retract();
					result += '\\';
				}
				state = 0;
				break;
		}
	}
	
	return result;
}

string MShell::ParseCmd()
{
	string result;
	bool done = false;

	while (not done and mCommandBegin != mCommandEnd)
	{
		char ch = GetNextChar();
		
		switch (ch)
		{
			case '"':
				result += ParseDQString();
				break;
			
			case '\'':
				result += ParseSQString();
				break;
			
			case '$':
				ch = GetNextChar();
				if (isalpha(ch) or ch == '_')
				{
					string var;
					
					do
					{
						var += ch;
						ch = GetNextChar();
					}
					while (isalnum(ch) or ch == '_');

					Retract();
					result += mEnvironment[var];
				}
				else
				{
					Retract();
					result += ch;
				}
				break;
			
			case '\\':
				if (mCommandBegin != mCommandEnd)
				{
					ch = GetNextChar();

					if (ch != '\n')
						result += ch;
				}
				break;
			
			case 0:
			case '<':
			case '>':
			case '(':
			case ')':
			case ':':
			case '&':
			case '|':
			case ' ':
			case '\t':	
				Retract();
				done = true;
				break;
			
			default:
				result += ch;
				break;
		}
	}
	
	return result;
}

int MShell::GetNextToken()
{
	mCommandBegin = mCommandPtr = mCommand.begin();
	mCommandEnd = mCommand.end();
	mCommandOverflow = 0;
	
	mToken.clear();

	int state = 0;
	int start = 0;
	int token = T_UNKNOWN;
	
	string::iterator c = mCommand.begin();
	do
	{
		char ch = GetNextChar();
		
		switch (state)
		{
			case 0:
				switch (ch)
				{
					case 0:
						token = T_EOF;
						break;
					
					case ' ':
					case '\t':
					case '\n':
						mCommand.erase(0, 1);
						mCommandBegin = mCommandPtr = mCommand.begin();
						mCommandEnd = mCommand.end();
						mCommandOverflow = 0;
						break;
					
					case ';':
						token = T_EOLN;
						break;
					
					case '\\':
						state = 1;
						break;
					
					case '|':
						token = T_PIPE;
						break;
					
					default:
						Restart(start, state);
						break;
				}
				break;
			
			case 1:	// backslash
				switch (ch)
				{
					case '\n':	// treat as white space
						mCommandPtr += 2;
						state = 0;
						break;
					
					default:
						Restart(start, state);
						break;
				}
				break;

			case 100:
				if (isalpha(ch) or ch == '_')
					state = 101;
				else
					Restart(start, state);
				break;
			
			case 101:
				if (ch == '=')
				{
					string s = mToken;
					mToken = s + ParseCmd();
					token = T_ENV;
				}
				else if (not isalnum(ch) and ch != '_')
					Restart(start, state);
				break;
			
			case 200:	// command or path
				Retract();
				mToken = ParseCmd();
				token = T_CMD;
				break;		
			
		}
	}
	while (token == T_UNKNOWN);
	
	mCommand.erase(mCommand.begin(), mCommandPtr);
	
	return token;
}

void MShell::Parse()
{
	for (;;)
	{
		Args args;
		Env env;
		
		int token = GetNextToken();
		
		while (token == T_ENV)
		{
			string::size_type n = mToken.find('=');
			string var = mToken.substr(0, n);
			mToken.erase(0, n + 1);
			env[var] = mToken;

			token = GetNextToken();
		}
		
		while (token == T_CMD)
		{
			args.push_back(mToken);
			
			token = GetNextToken();
		}
		
		if (args.size() > 0)
		{
			ExecuteSubCommand(args, env);
			args.clear();
		}
		else
			error(("Null command"));
		
//		if (token != T_PIPE)
			break;
	}
}

void MShell::ExecuteSubCommand(Args& inArgs, Env& inEnv)
{
	// set up the environment and argv vectors for the exec calls
	
	vector<char*> argv;
	for (Args::iterator a = inArgs.begin(); a != inArgs.end(); ++a)
		argv.push_back(const_cast<char*>((*a).c_str()));
	argv.push_back(nil);
	
	vector<std::string> envBuffer;
	vector<char*> envv;
	
	for (Env::iterator e = mEnvironment.begin(); e != mEnvironment.end(); ++e)
	{
		if (inEnv.find((*e).first) == inEnv.end())
		{
			envBuffer.push_back((*e).first + "=" + (*e).second);
			envv.push_back(const_cast<char*>(envBuffer.back().c_str()));
		}
	}
	
	for (Env::iterator e = inEnv.begin(); e != inEnv.end(); ++e)
	{
		envBuffer.push_back((*e).first + "=" + (*e).second);
		envv.push_back(const_cast<char*>(envBuffer.back().c_str()));
	}

	envv.push_back(nil);

	if (inArgs[0] == "cd" or inArgs[0] == "chdir")
		Cd(inArgs.size(), &argv[0], &envv[0]);
	else if (inArgs[0] == "pwd")
		Pwd(inArgs.size(), &argv[0], &envv[0]);
	else if (inArgs[0] == "echo")
		Echo(inArgs.size(), &argv[0], &envv[0]);
	else
		Exec(inArgs.size(), &argv[0], &envv[0]);
}

void MShell::Execute(string inCommand)
{
	eShellStatus(true);
	
	try
	{
		// setup the cwd
		if (mCurDir.length())
			chdir(mCurDir.c_str());
		
		mCommand = inCommand;
		mStdOutFD = mStdErrFD = -1;
		
		Parse();
	}
	catch (std::exception& e)
	{
		eStdOut(e.what(), strlen(e.what()));
	}
	
	if (mPid == 0)
		eShellStatus(false);
}

string MShell::NextPath(string& ioPathVar, string inName)
{
	string result = inName;
	
	if (ioPathVar.length())
	{
		if (ioPathVar[0] == ':')
			ioPathVar.erase(0, 1);
	
		string::size_type n = ioPathVar.find(':');
		if (n != string::npos)
		{
			result = ioPathVar.substr(0, n) + '/' + inName;
			ioPathVar.erase(0, n + 1);
		}
		else if (ioPathVar.length() > 0)
		{
			result = ioPathVar + '/' + inName;
			ioPathVar.clear();
		}
	}
	
	return result;
}

int MShell::Cd(int argc, char* const argv[], char* const env[])
{
	string dest;
	bool print = false;
	
	int c;
	optind = 1; opterr = 1;
	while ((c = getopt(argc, argv, "LP")) != -1)
	{
		switch (c)
		{
			case 'L':
				break;
			case 'P':
				break;
			default:
				error(("unknown option: %c", optopt));
		}
	}
	
	argc -= optind;
	argv += optind;
	
	if (argc > 1)
		error(("too many arguments"));
	
	if (*argv == NULL)
	{
		if (mEnvironment.find("HOME") == mEnvironment.end())
			error(("HOME not set"));
		dest = mEnvironment["HOME"];
	}
	else
		dest = *argv;
	
	if (dest == "-")
	{
		if (mPrevDir.length())
			dest = mPrevDir;
		else
			dest = mCurDir;
	}
	
	if (dest.length() == 0)
		dest = ".";
	
//	if (dest[0] != '/'

	string cdpath = mEnvironment["CDPATH"];
	for (;;)
	{
		string path = NextPath(cdpath, dest);
		
		struct stat statb;
		if (stat(path.c_str(), &statb) >= 0 and S_ISDIR(statb.st_mode))
		{
			if (not print)
			{
				if (path.length() > 2 and path.substr(0, 2) == "./")
					path.erase(0, 2);
				print = path != dest;
			}
			
			if (chdir(path.c_str()) == 0)
			{
				char buf[1024];
				if (getcwd(buf, sizeof(buf)) != nil)
				{
					mPrevDir = mCurDir;
					mCurDir = buf;
					break;
				}
			}
		}
		
		if (path == dest)
			error(("can't cd to %s", dest.c_str()));
	}
	
	return 0;
}

int MShell::Pwd(int argc, char* const argv[], char* const env[])
{
	if (argc != 1)
		error(("pwd does not take arguments"));
	
	eStdOut(mCurDir.c_str(), mCurDir.length());
	eStdOut("\n", 1);

	return 0;
}

int MShell::Echo(int argc, char* const argv[], char* const env[])
{
	bool printNL = true;
	
	int c;
	optind = 1; opterr = 1;
	while ((c = getopt(argc, argv, "n")) != -1)
	{
		switch (c)
		{
			case 'n':
				printNL = false;
				break;
			default:
				error(("unknown option: %c", optopt));
				break;
		}
	}
	
	argc -= optind;
	argv += optind;
	
	for (int i = 0; i < argc; ++i)
	{
		eStdOut(argv[i], strlen(argv[i]));
		if (i + 1 < argc)
			eStdOut(" ", 1);
	}

	if (printNL)
		eStdOut("\n", 1);
	
	return 0;
}

int MShell::Exec(int argc, char* const argv[], char* const env[])
{
	// OK, so we're supposed to run the thing in argv[0]
	// find out if it exists first...
	
	string path = argv[0];
	bool found = true;
	
	// If the path contains slashes we don't bother
	if (path.find('/') == string::npos)
	{
		found = false;
		
		struct stat statb;
		
		string PATH = mEnvironment["PATH"];
		
		while ((path = NextPath(PATH, argv[0])) != argv[0])
		{
			if (stat(path.c_str(), &statb) >= 0 and S_ISREG(statb.st_mode))
			{
				found = true;
				break;
			}
		}
	}
	
	if (not found)
		error(("command not found: %s", path.c_str()));
	
	try
	{
		int ifd[2], ofd[2], efd[2];
		
		pipe(ifd);
		pipe(ofd);
		pipe(efd);
		
		int pid = fork();
		
		if (pid == -1)
		{
			close(ifd[0]);
			close(ifd[1]);
			close(ofd[0]);
			close(ofd[1]);
			close(efd[0]);
			close(efd[1]);
			
//			THROW_IF_POSIX_ERROR(-1);
			error(("fork failed: %s", strerror(errno)));
		}
		
		if (pid == 0)	// the child
		{
			try
			{
				setpgid(0, 0);		// detach from the process group, create new

				dup2(ifd[0], STDIN_FILENO);
				close(ifd[0]);
				close(ifd[1]);

				dup2(ofd[1], STDOUT_FILENO);
				close(ofd[0]);
				close(ofd[1]);
								
				dup2(efd[1], STDERR_FILENO);
				close(efd[0]);
				close(efd[1]);

				(void)execve(path.c_str(), argv, env);
				cerr << "execution of " << path << " failed: " << strerror(errno) << endl;
			}
			catch (...)
			{
				cerr << "Exception catched" << endl;
			}
			exit(-1);
		}
		
		mPid = pid;
		
		close(ifd[0]);
		mStdInFD = ifd[1];

		int flags;
	
		flags = fcntl(mStdInFD, F_GETFL, 0);
		fcntl(mStdInFD, F_SETFL, flags | O_NONBLOCK);
//		// since we're not using stdin:
//		close(ifd[1]);
		
		close(ofd[1]);
		mStdOutFD = ofd[0];

		flags = fcntl(mStdOutFD, F_GETFL, 0);
		fcntl(mStdOutFD, F_SETFL, flags | O_NONBLOCK);
		mOutDone = false;

		close(efd[1]);
		mStdErrFD = efd[0];

		flags = fcntl(mStdErrFD, F_GETFL, 0);
		fcntl(mStdErrFD, F_SETFL, flags | O_NONBLOCK);
		mErrDone = false;
		
		AddRoute(ePoll, gApp->eIdle);
	}
	catch (std::exception& e)
	{
		MError::DisplayError(e);

		mPid = 0;
		mStdOutFD = -1;
		mStdErrFD = -1;
	}
	
	return 0;
}

void MShell::Kill()
{
	// kill all the processes in the process group
	kill(-mPid, SIGINT);

	// avoid the creation of zombies
	int status;
	waitpid(mPid, &status, 0);

	mPid = 0;

	close(mStdOutFD);
	mStdOutFD = -1;
	
	if (mStdErrFD >= 0)
		close(mStdErrFD);
	mStdErrFD = -1;
}

void MShell::Poll(
	double			inSystemTime)
{
	if (not mErrDone or not mOutDone)
	{
		char buffer[1024];
		int r, n;
		
		n = 0;
		while (not mOutDone /*and n++ < 100*/)
		{
			r = read(mStdOutFD, buffer, sizeof(buffer));

			if (r > 0)
				eStdOut(buffer, r);
			else if (r == 0 or errno != EAGAIN)
				mOutDone = true;
			else
				break;
		}
		
		n = 0;
		while (not mErrDone /*and n++ < 100*/)
		{
			r = read(mStdErrFD, buffer, sizeof(buffer));

			if (r > 0)
				eStdErr(buffer, r);
			else if (r == 0 or errno != EAGAIN)
				mErrDone = true;
			else
				break;
		}
	}
	
	// if both are done, we're done
	if (mOutDone and mErrDone)
	{
		close(mStdOutFD);
		mStdOutFD = -1;

		if (mRedirectStdErr)
			close(mStdErrFD);
		mStdErrFD = -1;
		
		// avoid the creation of zombies
		int status;
		waitpid(mPid, &status, WNOHANG);

		mPid = 0;
		eShellStatus(false);

		RemoveRoute(ePoll, gApp->eIdle);
	}
}
