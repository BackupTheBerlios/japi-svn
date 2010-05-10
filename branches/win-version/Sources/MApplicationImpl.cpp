//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

// $Id$

#include "MLib.h"

#include "MApplicationImpl.h"


MApplicationImpl::MApplicationImpl(
	MApplication*		inApp)
	: mApp(inApp)
{
}

MApplicationImpl::~MApplicationImpl()
{
}

void MApplicationImpl::Pulse()
{
	mApp->Pulse();
}
