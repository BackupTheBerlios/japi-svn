#include "MJapieG.h"

#include "MHandler.h"

MHandler::MHandler(
	MHandler*		inSuper)
	: mSuper(inSuper)
{
}

MHandler::~MHandler()
{
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
