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

#ifndef MPROJECTJOB_H
#define MPROJECTJOB_H

struct MProject;
class MProjectFile;

// ---------------------------------------------------------------------------
//	MProjectJob

struct MProjectJob
{
							MProjectJob(
								const std::string&		inTitle,
								MProject*				inProject)
								: mProject(inProject)
								, mTitle(inTitle)
								, mStatus(0) {}

	virtual					~MProjectJob() {}

	virtual void			Execute() = 0;
	virtual bool			IsDone() = 0;
	virtual void			Kill() {}

	MProject*				mProject;
	std::string				mTitle;
	int						mStatus;		// zero indicates success
};

typedef std::deque<MProjectJob*>	MProjectJobQueue;

// ---------------------------------------------------------------------------
//	MProjectExecJob

struct MProjectExecJob : public MProjectJob
{
							MProjectExecJob(
								const std::string&		inTitle,
								MProject*				inProject,
								const std::vector<std::string>&
														inArgv)
								: MProjectJob(inTitle, inProject)
								, mArgv(inArgv)
								, mPID(-1)
								, mStdOut(-1)
								, mStdOutDone(false)
								, mStdErr(-1)
								, mStdErrDone(false) {}

	virtual void			Execute();
	virtual void			Kill();
	virtual bool			IsDone();

	MCallBack<void(const char* inText, uint32 inSize)>	eStdOut;
	MCallBack<void(const char* inText, uint32 inSize)>	eStdErr;

	std::vector<std::string>
							mArgv;
	int						mPID;
	int						mStdOut;
	bool					mStdOutDone;
	int						mStdErr;
	bool					mStdErrDone;
};

// ---------------------------------------------------------------------------
//	MProjectCompileJob

struct MProjectCompileJob : public MProjectExecJob
{
							MProjectCompileJob(
								const std::string&		inTitle,
								MProject*				inProject,
								const std::vector<std::string>&
														inArgs,
								MProjectFile*			inProjectFile)
								: MProjectExecJob(inTitle, inProject, inArgs)
								, mProjectFile(inProjectFile) {}

	virtual void			Execute();
	virtual bool			IsDone();

	MProjectFile*			mProjectFile;
};

// ---------------------------------------------------------------------------
//	MProjectCompileAllJob

struct MProjectCompileAllJob : public MProjectJob
{
							MProjectCompileAllJob(
								const std::string&		inTitle,
								MProject*				inProject)
								: MProjectJob(inTitle, inProject) {}

	void					AddJob(
								MProjectJob*			inJob);

	virtual void			Execute();
	virtual void			Kill();
	virtual bool			IsDone();

	boost::ptr_deque<MProjectJob>
							mCompileJobs;
	boost::ptr_deque<MProjectJob>
							mCurrentJobs;
};

// ---------------------------------------------------------------------------
//	MProjectIfJob

struct MProjectIfJob : public MProjectJob
{
							MProjectIfJob(
								const std::string&		inTitle,
								MProject*				inProject,
								MProjectJob*			inFirstJob,
								MProjectJob*			inSecondJob)
								: MProjectJob(inTitle, inProject)
								, mFirstJob(inFirstJob)
								, mSecondJob(inSecondJob) {}

	virtual void			Execute();
	virtual void			Kill();
	virtual bool			IsDone();

	std::auto_ptr<MProjectJob>
							mFirstJob;
	std::auto_ptr<MProjectJob>
							mSecondJob;
};

// ---------------------------------------------------------------------------
//	MProjectCopyFileJob

struct MProjectCopyFileJob : public MProjectJob
{
							MProjectCopyFileJob(
								const std::string&	inTitle,
								MProject*			inProject,
								const fs::path&		inSrcFile,
								const fs::path&		inDstFile,
								bool				inRecursive = true)		// in the package dir
								: MProjectJob(inTitle, inProject)
								, mSrcFile(inSrcFile)
								, mDstFile(inDstFile)
								, mRecursive(inRecursive) {}

	virtual void			Execute();
	virtual bool			IsDone()			{ return true; }

	void					CopyFilesRecursive(
								const fs::path&	inSrcDir,
								const fs::path&	inDstDir);

	fs::path					mSrcFile;
	fs::path					mDstFile;
	bool					mRecursive;
};

// ---------------------------------------------------------------------------
//	MProjectCreateResourceJob

struct MProjectCreateResourceJob : public MProjectJob
{
							MProjectCreateResourceJob(
								const std::string&	inTitle,
								MProject*			inProject,
								const std::vector<MProjectItem*>&
													inSrcFiles,
								const fs::path&		inDstFile,		// in the package dir
								MTargetCPU			inTargetCPU)
								: MProjectJob(inTitle, inProject)
								, mSrcFiles(inSrcFiles)
								, mDstFile(inDstFile)
								, mTargetCPU(inTargetCPU) {}

	virtual void			Execute();
	virtual bool			IsDone()			{ return true; }

	std::vector<MProjectItem*>
							mSrcFiles;
	fs::path					mDstFile;
	MTargetCPU				mTargetCPU;
};

#endif
