#include "MJapieG.h"

#include "MHandler.h"

MHandler* MHandler::sFocus = nil;

MHandler::MHandler(
	MHandler*		inSuper)
	: mSuper(inSuper)
{
}

MHandler::~MHandler()
{
	ReleaseFocus();
}

bool MHandler::UpdateCommandStatus(
	uint32			inCommand,
	bool&			outEnabled,
	bool&			outChecked)
{
	bool result = false;
	
	if (mSuper != nil)
		result = mSuper->UpdateCommandStatus(inCommand, outEnabled, outChecked);
	
	return result;
}

bool MHandler::ProcessCommand(
	uint32			inCommand)
{
	bool result = false;
	
	if (mSuper != nil)
		result = mSuper->ProcessCommand(inCommand);
	
	return result;
}

void MHandler::SetSuper(
	MHandler*		inSuper)
{
	mSuper = inSuper;
}

void MHandler::TakeFocus()
{
	sFocus = this;
}

void MHandler::ReleaseFocus()
{
	if (sFocus == this)
		sFocus = mSuper;
}

