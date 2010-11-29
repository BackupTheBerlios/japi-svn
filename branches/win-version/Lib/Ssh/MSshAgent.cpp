//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "MLib.h"

#include "MSshAgent.h"

MSshAgent::MSshAgent()
	: mImpl(MSshAgentImpl::Create())
{
}

MSshAgent::~MSshAgent()
{
	delete mImpl;
}
