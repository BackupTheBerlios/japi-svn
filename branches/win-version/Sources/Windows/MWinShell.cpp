//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "MJapi.h"

#include "MShellImpl.h"
#include "MError.h"
#include "MPreferences.h"
#include "MP2PEvents.h"
#include "MJapiApp.h"

//extern char** environ;

using namespace std;

const char
	kSetDelimiterStr[]	= "--:@:----:@:--",
	kSetDelimiter[]		= "--:@:----:@:--\n";

const int32
	kSetDelimiterLength = sizeof(kSetDelimiter) - 1;

typedef map<string,string>		Env;

// --------------------------------------------------------------------

struct MWinShellImpl : public MShellImpl
{
					MWinShellImpl(
						MCallback<void(const char*, uint32)>&	inStdOut,
						MCallback<void(const char*, uint32)>&	inStdErr,
						MCallback<void(bool)>&					inShellStatus);

	virtual void	Execute(
				vector<char*>&		argv);

	virtual void	Execute(
				const string&		inCommand);

	virtual void	ExecuteScript(
				const string&		inScript,
				const string&		inText);

//	virtual void	Poll(
//				double				inTime);
//
	virtual void	Abort();

//	MEventIn<void(double)>			ePoll;
	
//	int		mPID;
//	int		mStdInFD, mStdOutFD, mStdErrFD;
//	
//	int		mState;
//	string	mSetString;
};

MWinShellImpl::MWinShellImpl(
	MCallback<void(const char*, uint32)>&	inStdOut,
	MCallback<void(const char*, uint32)>&	inStdErr,
	MCallback<void(bool)>&					inShellStatus)
	: MShellImpl(inStdOut, inStdErr, inShellStatus)
//	, ePoll(this, &MShellImp::Poll)
//	, mPID(-1)
//	, mStdInFD(-1)
//	, mStdOutFD(-1)
//	, mStdErrFD(-1)
//	, mState(0)
{
//	for (char** e = environ; *e != nil; ++e)
//	{
//		string v = *e;
//		string::size_type p;
//
//		if ((p = v.find('=')) != string::npos)
//			mENV[v.substr(0, p)] = v.substr(p + 1);
//	}
}

void MWinShellImpl::Execute(
	vector<char*>&	argv)
{
//	vector<string> envBuffer;
//	vector<char*> envv;
//	
//	string cwd, home;
//	
//	for (Env::iterator e = mENV.begin(); e != mENV.end(); ++e)
//	{
//		envBuffer.push_back(e->first + "=" + e->second);
//		envv.push_back(const_cast<char*>(envBuffer.back().c_str()));
//		
//		if (e->first == "PWD")
//			cwd = e->second;
//		
//		if (e->first == "HOME")
//			home = e->second;
//	}
//
//	envv.push_back(nil);
//	
//	int ifd[2], ofd[2], efd[2];
//	
//	(void)pipe(ifd);
//	(void)pipe(ofd);
//	(void)pipe(efd);
//	
//	int pid = fork();
//	
//	if (pid == -1)
//	{
//		close(ifd[0]);
//		close(ifd[1]);
//		close(ofd[0]);
//		close(ofd[1]);
//		close(efd[0]);
//		close(efd[1]);
//		
//		THROW(("fork failed: %s", strerror(errno)));
//	}
//	
//	if (pid == 0)	// the child
//	{
//		try
//		{
//			// detach from the process group, create new
//			setpgid(0, 0);
//			
//			if (cwd.length() > 0)
//				(void)chdir(cwd.c_str());
//			else if (home.length() > 0)
//				(void)chdir(home.c_str());
//				
//			dup2(ifd[0], STDIN_FILENO);
//			close(ifd[0]);
//			close(ifd[1]);
//
//			dup2(ofd[1], STDOUT_FILENO);
//			close(ofd[0]);
//			close(ofd[1]);
//							
//			dup2(efd[1], STDERR_FILENO);
//			close(efd[0]);
//			close(efd[1]);
//			
//			(void)execve(argv[0], &argv[0], &envv[0]);
//			cerr << "execution of " << argv[0] << " failed: " << strerror(errno) << endl;
//			_exit(1);
//		}
//		catch (...)
//		{
//			cerr << "Exception caught" << endl;
//		}
//		_exit(-1);
//	}
//	
//	mPID = pid;
//	
//	close(ifd[0]);
//	mStdInFD = ifd[1];
//	int flags = fcntl(mStdInFD, F_GETFL, 0);
//	fcntl(mStdInFD, F_SETFL, flags | O_NONBLOCK);
//	
//	close(ofd[1]);
//	mStdOutFD = ofd[0];
//	flags = fcntl(mStdOutFD, F_GETFL, 0);
//	fcntl(mStdOutFD, F_SETFL, flags | O_NONBLOCK);
//
//	close(efd[1]);
//	mStdErrFD = efd[0];
//	flags = fcntl(mStdErrFD, F_GETFL, 0);
//	fcntl(mStdErrFD, F_SETFL, flags | O_NONBLOCK);
//	
//	if (gApp != nil)
//		AddRoute(ePoll, gApp->eIdle);
//	
//	mState = 0;
//	mSetString.clear();
}

void MWinShellImpl::Execute(
	const string&		inCommand)
{
//	string cmd = inCommand;
//	cmd += " ; echo '";
//	cmd += kSetDelimiterStr;
//	cmd += "' ; env ;";
//	
//	vector<char*> argv;
//
//	argv.push_back(const_cast<char*>("/bin/sh"));
//	argv.push_back(const_cast<char*>("-c"));
//	argv.push_back(const_cast<char*>(cmd.c_str()));
//
//	argv.push_back(nil);
//	
//	Execute(argv);
}

void MWinShellImpl::ExecuteScript(
	const string&		inScript,
	const string&		inText)
{
//	vector<char*> argv;
//
//	argv.push_back(const_cast<char*>(inScript.c_str()));
//	argv.push_back(nil);
//	
//	Execute(argv);
//	
//	// make this a thread
//	
//	uint32 l = inText.length();
//	const char* p = inText.c_str();
//	
//	while (l > 0)
//	{
//		uint32 k = 512;
//		if (k > l)
//			k = l;
//		
//		int r = write(mStdInFD, p, k);
//		if (r > 0)
//		{
//			p += r;
//			l -= r;
//		}
//		
//		Poll(0);
//	}
//		
//	close(mStdInFD);
}

//void MWinShellImpl::Poll(
//	double				inTime)
//{
//	char buffer[10240];
//	int r;
//	
//	string out;
//	
//	while (mStdOutFD >= 0)
//	{
//		r = read(mStdOutFD, buffer, sizeof(buffer));
//		
//		if (r > 0)
//		{
//			// process the buffer block just read
//			
//			for (char* p = buffer; p < buffer + r; ++p)
//			{
//				// if we've already encountered the set delimiter, we simply cat
//				// output to the set string.
//				
//				if (mState == kSetDelimiterLength)
//				{
//					mSetString += *p;
//					continue;
//				}
//				
//				// if this is a character in the set string increase state
//				if (*p == kSetDelimiter[mState])
//				{
//					++mState;
//					continue;
//				}
//				
//				// if we've seen some characters that start the set delimiter
//				// add those to the output buffer first and reset mState
//				
//				if (mState > 0)
//				{
//					for (int32 d = 0; d < mState; ++d)
//						out += kSetDelimiter[d];
//					
//					mState = 0;
//				}
//				
//				// and now simply add the character to the output
//				out += *p;
//			}
//		}
//		else if (r == 0 or errno != EAGAIN)
//		{
//			close(mStdOutFD);
//			mStdOutFD = -1;
//		}
//		else
//			break;
//	}
//	
//	if (not out.empty())
//		eStdOut(out.c_str(), out.length());
//
//	while (mStdErrFD >= 0)
//	{
//		r = read(mStdErrFD, buffer, sizeof(buffer));
//
//		if (r > 0)
//			eStdErr(buffer, r);
//		else if (r == 0 or errno != EAGAIN)
//		{
//			close(mStdErrFD);
//			mStdErrFD = -1;
//		}
//		else
//			break;
//	}
//	
//	// if both are done, we're done
//	if (mStdOutFD < 0 and mStdErrFD < 0)
//	{
//		// avoid the creation of zombies
//		int status;
//		waitpid(mPID, &status, WNOHANG);
//
//		mPID = -1;
//		eShellStatus(false);
//
//		if (gApp != nil)
//			RemoveRoute(ePoll, gApp->eIdle);
//		
//		stringstream s(mSetString);
//
//		Env nenv;		
//		while (not s.eof())
//		{
//			string line;
//			getline(s, line);
//			
//			string::size_type p;
//	
//			if ((p = line.find('=')) != string::npos)
//			{
//				string key = line.substr(0, p);
//				string value = line.substr(p + 1);
//				
//				if (key != "SHELLOPTS")
//					nenv[key] = value;
//			}
//		}
//		
//		if (nenv.size() > 0)
//			mENV = nenv;
//	}
//}

void MWinShellImpl::Abort()
{
//	if (mPID)
//		kill(-mPID, SIGINT);
//	mPID = -1;
}

MShellImpl* MShellImpl::Create(
	MCallback<void(const char*, uint32)>&	inStdOut,
	MCallback<void(const char*, uint32)>&	inStdErr,
	MCallback<void(bool)>&					inShellStatus)
{
	return new MWinShellImpl(inStdOut, inStdErr, inShellStatus);
}