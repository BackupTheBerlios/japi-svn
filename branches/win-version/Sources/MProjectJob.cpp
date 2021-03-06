//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "MJapi.h"

#include <vector>
#include <deque>
#include <cerrno>
#include <signal.h>
#include <fcntl.h>
//#include <sys/wait.h>

#include "MResources.h"

#undef check
#ifndef BOOST_DISABLE_ASSERTS
#define BOOST_DISABLE_ASSERTS
#endif

#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/ptr_container/ptr_deque.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/bind.hpp>

#include "MFile.h"
#include "MMessageWindow.h"
#include "MProject.h"
#include "MProjectJob.h"
#include "MObjectFile.h"
#include "MError.h"

extern char** environ;
extern int VERBOSE;

namespace ba = boost::algorithm;

using namespace std;

// ---------------------------------------------------------------------------
//	MProjectExecJob::Execute

void MProjectExecJob::Execute()
{
#if DEBUG
	if (VERBOSE)
	{
		cout << "About to execute:" << endl;
		copy(mArgv.begin(), mArgv.end(), ostream_iterator<string>(cout, "\n  "));
		cout << endl;
	}
#endif

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
//		THROW(("fork failed"));
//	}
//	
//	if (pid == 0)	// the child
//	{
//		try
//		{
//			setpgid(0, 0);		// detach from the process group, create new
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
//			vector<const char*> argv;
//			for (vector<string>::iterator a = mArgv.begin(); a != mArgv.end(); ++a)
//				argv.push_back(a->c_str());
//			argv.push_back(nil);
//
//			(void)execve(argv[0],
//				const_cast<char* const*>(&argv[0]),
//				const_cast<char* const*>(environ));
//			cerr << "execution of " << argv[0] << " failed: " << strerror(errno) << endl;
//		}
//		catch (...)
//		{
//			cerr << "Exception catched" << endl;
//		}
//		exit(-1);
//	}
//
//	mPID = pid;
//	
//	close(ifd[0]);
//	close(ifd[1]);
//
//	close(ofd[1]);
//	int flags = fcntl(ofd[0], F_GETFL, 0);
//	fcntl(ofd[0], F_SETFL, flags | O_NONBLOCK);
//
//	mStdOutDone = false;
//	mStdOut = ofd[0];
//
//	close(efd[1]);
//	flags = fcntl(efd[0], F_GETFL, 0);
//	fcntl(efd[0], F_SETFL, flags | O_NONBLOCK);
//
//	mStdErrDone = false;
//	mStdErr = efd[0];
//		
//	mProject->SetStatus(mTitle, true);
}

// ---------------------------------------------------------------------------
//	MProjectExecJob::Kill

void MProjectExecJob::Kill()
{
//	// kill all the processes in the process group
//	kill(-mPID, SIGINT);
//	mPID = -1;
//
//	// avoid the creation of zombies
//	waitpid(mPID, &mStatus, 0);
//
//	mPID = -1;
//
//	close(mStdOut);
//	mStdOut = -1;
}

// ---------------------------------------------------------------------------
//	MProjectExecJob::IsDone

bool MProjectExecJob::IsDone()
{
//	if (not mStdOutDone)
//	{
//		char buffer[10240];
//		int r, n;
//		
//		n = 0;
//		while (not mStdOutDone)
//		{
//			r = read(mStdOut, buffer, sizeof(buffer));
//
//			if (r > 0)
//				eStdOut(buffer, r);
//			else if (r == 0 or errno != EAGAIN)
//			{
//				if (mStdOut >= 0)
//					close(mStdOut);
//				mStdOut = -1;
//				mStdOutDone = true;
//			}
//			else
//				break;
//		}
//	}
//	
//	string stderr;
//	
//	if (not mStdErrDone)
//	{
//		char buffer[10240];
//		int r, n;
//		
//		n = 0;
//		while (not mStdErrDone)
//		{
//			r = read(mStdErr, buffer, sizeof(buffer));
//
//			if (r > 0)
//				stderr.append(buffer, buffer + r);
//			else if (r == 0 or errno != EAGAIN)
//			{
//				if (mStdErr >= 0)
//					close(mStdErr);
//				mStdErr = -1;
//				mStdErrDone = true;
//			}
//			else
//				break;
//		}
//	}
//	
//	if (stderr.length())
//		eStdErr(stderr.c_str(), stderr.length());
//
//	if (mStdOutDone and mStdErrDone)
//	{
////		if (mPID >= 0)
////		{
////			waitpid(mPID, &mStatus, 0);	// used to pass in WNOHANG...
////			mPID = -1;
////		}
////		else
//			mStatus = 0;
//	}
//	
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
//	MProjectDoAllJob::AddJob

void MProjectDoAllJob::AddJob(
	MProjectJob*			inJob)
{
	mCompileJobs.push_back(inJob);
}

// ---------------------------------------------------------------------------
//	MProjectDoAllJob::Execute

void MProjectDoAllJob::Execute()
{
	while (not mCompileJobs.empty() and mCurrentJobs.size() < gConcurrentJobs)
	{
		mCurrentJobs.transfer(mCurrentJobs.end(), mCompileJobs.begin(), mCompileJobs);
		mCurrentJobs.back().Execute();
	}
}

// ---------------------------------------------------------------------------
//	MProjectDoAllJob::Kill

void MProjectDoAllJob::Kill()
{
	for_each(mCurrentJobs.begin(), mCurrentJobs.end(),
		boost::bind(&MProjectJob::Kill, _1));
}

// ---------------------------------------------------------------------------
//	MProjectDoAllJob::Execute

bool MProjectDoAllJob::IsDone()
{
	boost::ptr_deque<MProjectJob>::iterator job = mCurrentJobs.begin();
	while (job != mCurrentJobs.end())
	{
		if (job->IsDone())
		{
			mStatus = mStatus or job->mStatus;
			job = mCurrentJobs.erase(job);
			continue;
		}
		++job;
	}
	
	if (mCurrentJobs.size() < gConcurrentJobs)
		Execute();
	
	return mCurrentJobs.empty() and mCompileJobs.empty();
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
	const fs::path&	inSrcDir,
	const fs::path&	inDstDir)
{
	if (not exists(inDstDir))
	{
		fs::path dstDir(inDstDir);
		fs::create_directory(dstDir);
	}
	
	MFileIterator iter(inSrcDir, kFileIter_ReturnDirectories);
	fs::path p;
	
	while (iter.Next(p))
	{
		if (p.filename() == ".svn" or FileNameMatches("*~.nib", p))
			continue;
		
		mProject->SetStatus(string("Copying ") + p.filename(), true);

		if (is_directory(p))
			CopyFilesRecursive(p, inDstDir / p.filename());
		else
		{
			fs::path dstFile = inDstDir / p.filename();
			
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
		mProject->SetStatus(string("Copying ") + mSrcFile.filename(), true);

		if (fs::exists(mDstFile))
			fs::remove(mDstFile);

		fs::copy_file(mSrcFile, mDstFile);
	}
}

// ---------------------------------------------------------------------------
//	MProjectCreateResourceJob::Execute

void MProjectCreateResourceJob::Execute()
{
//	MResourceFile rsrcFile(mTargetCPU);
//	
//	for (vector<MProjectItem*>::iterator p = mSrcFiles.begin(); p != mSrcFiles.end(); ++p)
//	{
//		MProjectResource* rsrc = dynamic_cast<MProjectResource*>(*p);
//		if (rsrc == nil)
//			continue;
//		
//		fs::ifstream f(rsrc->GetPath());
//
//		string name = rsrc->GetResourceName();
//		
//		if (not f.is_open())
//			THROW(("Could not open resource data file %s", name.c_str()));
//		
//		filebuf* b = f.rdbuf();
//	
//		uint32 size = b->pubseekoff(0, ios::end);
//		b->pubseekpos(0);
//	
//		char* data = new char[size + 1];
//		
//		try
//		{
//			b->sgetn(data, size);
//			data[size] = 0;
//		
//			f.close();
//			
//			rsrcFile.Add(name, data, size);
//		}
//		catch (...)
//		{
//			delete[] data;
//			throw;
//		}
//		
//		delete[] data;
//	}
//	
//	rsrcFile.Write(mDstFile);
}

