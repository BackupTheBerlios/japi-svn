//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "MJapi.h"

#include <map>

#include "MShellImpl.h"
#include "MError.h"
#include "MPreferences.h"
#include "MP2PEvents.h"
#include "MJapiApp.h"

using namespace std;

MShellImpl::~MShellImpl()
{
	//Abort();
}



// --------------------------------------------------------------------

MShell::MShell(
	bool			inRedirectStdErr)
	: mImpl(MShellImpl::Create(eStdOut, eStdErr, eShellStatus))
{
}

MShell::~MShell()
{
	delete mImpl;
}

void MShell::Execute(
	const string&	inScript)
{
	mImpl->Execute(inScript);
}

void MShell::ExecuteScript(
	const string&	inScript,
	const string&	inText)
{
	mImpl->ExecuteScript(inScript, inText);
}

void MShell::Kill()
{
	mImpl->Abort();
}

bool MShell::IsRunning() const
{
	//mImpl->Poll(0);
	//return mImpl->mPID > 0;
	return false;
}

void MShell::SetCWD(
	const string&	inCWD)
{
	mImpl->SetCWD(inCWD);
}

string MShell::GetCWD() const
{
	return mImpl->GetCWD();
}
