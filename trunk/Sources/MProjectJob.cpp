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

#include <vector>
#include <deque>
#include <cerrno>
#include <signal.h>
#if HAVE_WAIT_H
#include <wait.h>
#endif

#undef check
#ifndef BOOST_DISABLE_ASSERTS
#define BOOST_DISABLE_ASSERTS
#endif

#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/ptr_container/ptr_deque.hpp>

#include "MFile.h"
#include "MMessageWindow.h"
#include "MProjectTarget.h"
#include "MProject.h"
#include "MProjectJob.h"

extern char** environ;

using namespace std;

// ---------------------------------------------------------------------------
//	MProjectExecJob::Execute

void MProjectExecJob::Execute()
{
//cout << "About to execute:" << endl;
//copy(mArgv.begin(), mArgv.end(), ostream_iterator<string>(cout, " "));
//cout << endl;
//
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
		
		THROW(("fork failed"));
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

			vector<const char*> argv;
			for (vector<string>::iterator a = mArgv.begin(); a != mArgv.end(); ++a)
				argv.push_back(a->c_str());
			argv.push_back(nil);

			(void)execve(argv[0],
				const_cast<char* const*>(&argv[0]),
				const_cast<char* const*>(environ));
			cerr << "execution of " << argv[0] << " failed: " << strerror(errno) << endl;
		}
		catch (...)
		{
			cerr << "Exception catched" << endl;
		}
		exit(-1);
	}

	mPID = pid;
	
	close(ifd[0]);
	close(ifd[1]);

	close(ofd[1]);
	int flags = fcntl(ofd[0], F_GETFL, 0);
	fcntl(ofd[0], F_SETFL, flags | O_NONBLOCK);

	mStdOutDone = false;
	mStdOut = ofd[0];

	close(efd[1]);
	flags = fcntl(efd[0], F_GETFL, 0);
	fcntl(efd[0], F_SETFL, flags | O_NONBLOCK);

	mStdErrDone = false;
	mStdErr = efd[0];
		
	mProject->SetStatus(mTitle, true);
}

// ---------------------------------------------------------------------------
//	MProjectExecJob::Kill

void MProjectExecJob::Kill()
{
	// kill all the processes in the process group
	kill(-mPID, SIGINT);
	mPID = -1;

	// avoid the creation of zombies
	waitpid(mPID, &mStatus, 0);

	mPID = -1;

	close(mStdOut);
	mStdOut = -1;
}

// ---------------------------------------------------------------------------
//	MProjectExecJob::IsDone

bool MProjectExecJob::IsDone()
{
	if (not mStdOutDone)
	{
		char buffer[10240];
		int r, n;
		
		n = 0;
		while (not mStdOutDone)
		{
			r = read(mStdOut, buffer, sizeof(buffer));

			if (r > 0)
				eStdOut(buffer, r);
			else if (r == 0 or errno != EAGAIN)
			{
				if (mStdOut >= 0)
					close(mStdOut);
				mStdOut = -1;
				mStdOutDone = true;
			}
			else
				break;
		}
	}
	
	if (not mStdErrDone)
	{
		char buffer[10240];
		int r, n;
		
		n = 0;
		while (not mStdErrDone)
		{
			r = read(mStdErr, buffer, sizeof(buffer));

			if (r > 0)
			{
				MMessageWindow* messageWindow = mProject->GetMessageWindow();
				messageWindow->AddStdErr(buffer, r);
			}
			else if (r == 0 or errno != EAGAIN)
			{
				if (mStdErr >= 0)
					close(mStdErr);
				mStdErr = -1;
				mStdErrDone = true;
			}
			else
				break;
		}
	}

	if (mStdOutDone and mStdErrDone)
	{
		if (mPID >= 0)
		{
			waitpid(mPID, &mStatus, WNOHANG);
			mPID = -1;
		}
		else
			mStatus = 0;
	}
	
	return mStdOutDone and mStdErrDone;
}

// ---------------------------------------------------------------------------
//	MProjectCompileJob::Execute

void MProjectCompileJob::Execute()
{
	MProjectExecJob::Execute();

	mProjectFile->SetCompiling(true);
}

// ---------------------------------------------------------------------------
//	MProjectCompileJob::IsDone

bool MProjectCompileJob::IsDone()
{
	bool result = MProjectExecJob::IsDone();
	if (result)
	{
		mProjectFile->SetCompiling(false);
	
		if (mStatus == 0)
			mProjectFile->CheckCompilationResult();
	}
	
	return result;
}

// ---------------------------------------------------------------------------
//	MProjectCompileAllJob::AddJob

void MProjectCompileAllJob::AddJob(
	MProjectJob*			inJob)
{
	mCompileJobs.push_back(inJob);
}

// ---------------------------------------------------------------------------
//	MProjectCompileAllJob::Execute

void MProjectCompileAllJob::Execute()
{
	if (mCompileJobs.size() > 0)
		mCompileJobs.front().Execute();
}

// ---------------------------------------------------------------------------
//	MProjectCompileAllJob::Kill

void MProjectCompileAllJob::Kill()
{
	if (mCompileJobs.size() > 0)
		mCompileJobs.front().Kill();
}

// ---------------------------------------------------------------------------
//	MProjectCompileAllJob::Execute

bool MProjectCompileAllJob::IsDone()
{
	bool result = true;
	
	if (mCompileJobs.size() > 0)
	{
		result = mCompileJobs.front().IsDone();
		
		if (result)
		{
			if (mStatus == 0 and mCompileJobs.front().mStatus != 0)
				mStatus = mCompileJobs.front().mStatus;
			
			mCompileJobs.pop_front();
			
			if (mCompileJobs.size() > 0)
			{
				mCompileJobs.front().Execute();
				result = false;
			}
		}
	}
	
	return result;
}

// ---------------------------------------------------------------------------
//	MProjectIfJob::Execute

void MProjectIfJob::Execute()
{
	if (mFirstJob.get() != nil)
		mFirstJob->Execute();
	else if (mSecondJob.get() != nil)
		mSecondJob->Execute();
}

// ---------------------------------------------------------------------------
//	MProjectIfJob::Kill

void MProjectIfJob::Kill()
{
	if (mFirstJob.get() != nil)
		mFirstJob->Kill();
	else if (mSecondJob.get() != nil)
		mSecondJob->Kill();
}

// ---------------------------------------------------------------------------
//	MProjectIfJob::IsDone

bool MProjectIfJob::IsDone()
{
	bool result = true;
	
	if (mFirstJob.get() != nil)
	{
		result = mFirstJob->IsDone();
		if (result)
		{
			mStatus = mFirstJob->mStatus;
			
			mFirstJob.release();
			if (mStatus == 0 and mSecondJob.get())
			{
				mSecondJob->Execute();
				result = false;
			}
		}
	}
	else if (mSecondJob.get() != nil)
	{
		result = mSecondJob->IsDone();
		if (result)
			mStatus = mSecondJob->mStatus;
	}
	
	return result;
}

// ---------------------------------------------------------------------------
//	MProjectCopyFileJob::CopyFilesRecursive

void MProjectCopyFileJob::CopyFilesRecursive(
	const MPath&	inSrcDir,
	const MPath&	inDstDir)
{
	if (not exists(inDstDir))
	{
		MPath dstDir(inDstDir);
		fs::create_directory(dstDir);
	}
	
	MFileIterator iter(inSrcDir, kFileIter_ReturnDirectories);
	MPath p;
	
	while (iter.Next(p))
	{
		if (p.leaf() == ".svn" or FileNameMatches("*~.nib", p))
			continue;
		
		mProject->SetStatus(string("Copying ") + p.leaf(), true);

		if (is_directory(p))
			CopyFilesRecursive(p, inDstDir / p.leaf());
		else
		{
			MPath dstFile = inDstDir / p.leaf();
			
			if (fs::exists(dstFile))
				fs::remove(dstFile);

			fs::copy_file(p, dstFile);
		}
	}
}

// ---------------------------------------------------------------------------
//	MProjectCopyFileJob::Execute

void MProjectCopyFileJob::Execute()
{
	if (is_directory(mSrcFile))
	{
		if (exists(mDstFile) and not is_directory(mDstFile))
			THROW(("Cannot overwrite a file with a directory"));
		
		CopyFilesRecursive(mSrcFile, mDstFile);
	}	
	else
	{
		mProject->SetStatus(string("Copying ") + mSrcFile.leaf(), true);

		if (fs::exists(mDstFile))
			fs::remove(mDstFile);

		fs::copy_file(mSrcFile, mDstFile);
	}
}

