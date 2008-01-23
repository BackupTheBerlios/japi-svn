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

#ifndef MPROJECTTARGET_H
#define MPROJECTTARGET_H

#include <string>

// ---------------------------------------------------------------------------
//	MProjectTarget

enum MTargetCPU {
	eCPU_Unknown,
	eCPU_native,
	eCPU_386,
	eCPU_x86_64,
	eCPU_PowerPC_32,
	eCPU_PowerPC_64
};

enum MTargetKind
{
	eTargetNone,
	eTargetApplicationPackage,
	eTargetBundlePackage,
	eTargetExecutable,
	eTargetSharedLibrary,
	eTargetBundle,
	eTargetStaticLibrary
};

class MProjectTarget
{
  public:
						MProjectTarget(
							const std::string&	inLinkTarget,
							const std::string&	inName,
							MTargetKind			inKind,
							MTargetCPU			inTargetCPU)
							: mLinkTarget(inLinkTarget)
							, mBundleName(inLinkTarget)
							, mName(inName)
							, mKind(inKind)
							, mTargetCPU(inTargetCPU)
							, mAnsiStrict(false)
							, mPedantic(false)
							, mDebug(false) {}

//						MProjectTarget(
//							const MProjectTarget& inOther);
//
//	MProjectTarget&		operator=(
//							const MProjectTarget& rhs);

	static MTargetCPU	GetNativeCPU();

	void				AddCFlag(
							const char*		inFlag)
						{
							mCFlags.push_back(inFlag); 
						}

	void				AddLDFlag(
							const char*		inFlag)
						{
							mLDFlags.push_back(inFlag); 
						}

	void				AddWarning(
							const char*		inWarning)
						{
							mWarnings.push_back(inWarning);
						}

	void				AddDefine(
							const char*		inDefine)
						{
							mDefines.push_back(inDefine);
						}

	void				AddFramework(
							const char*		inFramework)
						{
							mFrameworks.push_back(inFramework);
						}

	void				SetBundleName(
							const std::string&	inBundleName)
						{
							mBundleName = inBundleName;
						}
	
	void				SetName(
							const std::string&	inName)
						{
							mName = inName;
						}
	
	void				SetLinkTarget(
							const std::string&	inName)
						{
							mLinkTarget = inName;
						}
	
	void				SetKind(
							MTargetKind			inKind)
						{
							mKind = inKind;
						}
	
	void				SetTargetCPU(
							MTargetCPU			inTargetCPU)
						{
							mTargetCPU = inTargetCPU;
						}
	
	std::string			GetLinkTarget() const	{ return mLinkTarget; }
	std::string			GetBundleName() const	{ return mBundleName; }
	std::string			GetName() const			{ return mName; }
	MTargetKind			GetKind() const			{ return mKind; }
	MTargetCPU			GetTargetCPU() const	{ return mTargetCPU; }
	
	const std::vector<std::string>&
						GetCFlags() const		{ return mCFlags; }
	const std::vector<std::string>&
						GetLDFlags() const		{ return mLDFlags; }
	const std::vector<std::string>&
						GetWarnings() const		{ return mWarnings; }
	const std::vector<std::string>&
						GetDefines() const		{ return mDefines; }
	const std::vector<std::string>&
						GetFrameworks() const	{ return mFrameworks; }
	
	bool				IsAnsiStrict() const	{ return mAnsiStrict; }
	void				SetAnsiStrict(
							bool inAnsiStrict)	{ mAnsiStrict = inAnsiStrict; }

	bool				IsPedantic() const	{ return mPedantic; }
	void				SetPedantic(
							bool inPedantic)	{ mPedantic = inPedantic; }

	bool				GetDebugFlag() const	{ return mDebug; }
	void				SetDebugFlag(
							bool inDebug)		{ mDebug = inDebug; }

	void				SetCreator(const std::string& inCreator)	{ mCreator = inCreator; }
	std::string			GetCreator() const							{ return mCreator; }

	void				SetType(const std::string& inType)			{ mType = inType; }
	std::string			GetType() const								{ return mType; }

  private:
	std::string			mLinkTarget;
	std::string			mBundleName;
	std::string			mName;
	std::string			mCreator;
	std::string			mType;
	MTargetKind			mKind;
	MTargetCPU			mTargetCPU;
	std::vector<std::string>
						mCFlags;
	std::vector<std::string>
						mLDFlags;
	std::vector<std::string>
						mWarnings;
	std::vector<std::string>
						mDefines;
	std::vector<std::string>
						mFrameworks;
	bool				mAnsiStrict;
	bool				mPedantic;
	bool				mDebug;
};

#endif
