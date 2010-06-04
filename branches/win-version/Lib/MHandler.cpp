//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "MLib.h"

#include "MHandler.h"

using namespace std;

MHandler::MHandler(
	MHandler*		inSuper)
	: mSuper(inSuper)
{
}

MHandler::~MHandler()
{
	SetSuper(nil);
}

bool MHandler::UpdateCommandStatus(
	uint32			inCommand,
	MMenu*			inMenu,
	uint32			inItemIndex,
	bool&			outEnabled,
	bool&			outChecked)
{
	bool result = false;
	
	if (mSuper != nil)
	{
		result = mSuper->UpdateCommandStatus(
			inCommand, inMenu, inItemIndex, outEnabled, outChecked);
	}
	else
	{
		outEnabled = false;
		outChecked = false;
	}
	
	return result;
}

bool MHandler::ProcessCommand(
	uint32			inCommand,
	const MMenu*	inMenu,
	uint32			inItemIndex,
	uint32			inModifiers)
{
	bool result = false;
	
	if (mSuper != nil)
		result = mSuper->ProcessCommand(inCommand, inMenu, inItemIndex, inModifiers);
	
	return result;
}

bool MHandler::HandleKeydown(
	uint32			inKeyCode,
	uint32			inModifiers,
	const string&	inText)
{
	bool result = false;
	if (mSuper != nil)
		result = mSuper->HandleKeydown(inKeyCode, inModifiers, inText);
	return result;
}

void MHandler::SetSuper(
	MHandler*		inSuper)
{
	mSuper = inSuper;
}

