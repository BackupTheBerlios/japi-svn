/*
	Copyright (c) 2007, Maarten L. Hekkelman
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

#include <map>
#include <gdk/gdkkeysyms.h>

#include "MCommands.h"
#include "MAcceleratorTable.h"

using namespace std;

namespace {

const uint32
	kValidModifiersMask = GDK_CONTROL_MASK | GDK_SHIFT_MASK | GDK_MOD1_MASK;

struct MCommandToString
{
	char mCommandString[10];
	
	MCommandToString(uint32 inCommand)
	{
		strcpy(mCommandString, "MCmd_xxxx");
		
		mCommandString[5] = ((inCommand & 0xff000000) >> 24) & 0x000000ff;
		mCommandString[6] = ((inCommand & 0x00ff0000) >> 16) & 0x000000ff;
		mCommandString[7] = ((inCommand & 0x0000ff00) >>  8) & 0x000000ff;
		mCommandString[8] = ((inCommand & 0x000000ff) >>  0) & 0x000000ff;
	}
	
	operator const char*() const	{ return mCommandString; }
};

}

struct MAcceleratorTableImp
{
	map<int64,uint32>		mTable;
	map<int64,MKeyCommand>	mNavTable;
};

inline
int64
MakeKey(
	uint32	inKeyValue,
	uint32	inModifiers)
{
	return (static_cast<int64>(inKeyValue) << 32) | (inModifiers);
}	

// -------------------------------------

MAcceleratorTable& MAcceleratorTable::Instance()
{
	static auto_ptr<MAcceleratorTable>	sInstance;
	
	if (sInstance.get() == nil)
	{
		sInstance.reset(new MAcceleratorTable);

		sInstance->RegisterAcceleratorKey(cmd_New, GDK_N, GDK_CONTROL_MASK);
		sInstance->RegisterAcceleratorKey(cmd_Open, GDK_O, GDK_CONTROL_MASK);
		sInstance->RegisterAcceleratorKey(cmd_OpenIncludeFile, GDK_D, GDK_CONTROL_MASK | GDK_SHIFT_MASK);
		sInstance->RegisterAcceleratorKey(cmd_SwitchHeaderSource, GDK_1, GDK_CONTROL_MASK);
		sInstance->RegisterAcceleratorKey(cmd_Close, GDK_W, GDK_CONTROL_MASK);
		sInstance->RegisterAcceleratorKey(cmd_CloseAll, GDK_W, GDK_CONTROL_MASK | GDK_SHIFT_MASK);
		sInstance->RegisterAcceleratorKey(cmd_Save, GDK_S, GDK_CONTROL_MASK);
		sInstance->RegisterAcceleratorKey(cmd_SaveAll, GDK_S, GDK_CONTROL_MASK | GDK_SHIFT_MASK);
		sInstance->RegisterAcceleratorKey(cmd_Quit, GDK_Q, GDK_CONTROL_MASK);
		sInstance->RegisterAcceleratorKey(cmd_Undo, GDK_Z, GDK_CONTROL_MASK);
		sInstance->RegisterAcceleratorKey(cmd_Redo, GDK_Z, GDK_CONTROL_MASK | GDK_SHIFT_MASK);
		sInstance->RegisterAcceleratorKey(cmd_Cut, GDK_X, GDK_CONTROL_MASK);
		sInstance->RegisterAcceleratorKey(cmd_CutAppend, GDK_X, GDK_CONTROL_MASK | GDK_SHIFT_MASK);
		sInstance->RegisterAcceleratorKey(cmd_Copy, GDK_C, GDK_CONTROL_MASK);
		sInstance->RegisterAcceleratorKey(cmd_CopyAppend, GDK_C, GDK_CONTROL_MASK | GDK_SHIFT_MASK);
		sInstance->RegisterAcceleratorKey(cmd_Paste, GDK_V, GDK_CONTROL_MASK);
		sInstance->RegisterAcceleratorKey(cmd_PasteNext, GDK_V, GDK_CONTROL_MASK | GDK_SHIFT_MASK);
		sInstance->RegisterAcceleratorKey(cmd_SelectAll, GDK_A, GDK_CONTROL_MASK);
		sInstance->RegisterAcceleratorKey(cmd_Balance, GDK_B, GDK_CONTROL_MASK);
		sInstance->RegisterAcceleratorKey(cmd_ShiftLeft, GDK_bracketleft, GDK_CONTROL_MASK);
		sInstance->RegisterAcceleratorKey(cmd_ShiftRight, GDK_bracketright, GDK_CONTROL_MASK);
		sInstance->RegisterAcceleratorKey(cmd_Comment, GDK_apostrophe, GDK_CONTROL_MASK);
		sInstance->RegisterAcceleratorKey(cmd_Uncomment, GDK_quotedbl, GDK_CONTROL_MASK | GDK_SHIFT_MASK);
		sInstance->RegisterAcceleratorKey(cmd_FastFind, GDK_I, GDK_CONTROL_MASK);
		sInstance->RegisterAcceleratorKey(cmd_FastFindBW, GDK_i, GDK_CONTROL_MASK | GDK_SHIFT_MASK);
		sInstance->RegisterAcceleratorKey(cmd_Find, GDK_F, GDK_CONTROL_MASK);
		sInstance->RegisterAcceleratorKey(cmd_FindNext, GDK_G, GDK_CONTROL_MASK);
		sInstance->RegisterAcceleratorKey(cmd_FindPrev, GDK_g, GDK_CONTROL_MASK | GDK_SHIFT_MASK);
		sInstance->RegisterAcceleratorKey(cmd_FindInNextFile, GDK_J, GDK_CONTROL_MASK);
		sInstance->RegisterAcceleratorKey(cmd_EnterSearchString, GDK_E, GDK_CONTROL_MASK);
		sInstance->RegisterAcceleratorKey(cmd_EnterReplaceString, GDK_e, GDK_CONTROL_MASK | GDK_SHIFT_MASK);
		sInstance->RegisterAcceleratorKey(cmd_Replace, GDK_equal, GDK_CONTROL_MASK);
		sInstance->RegisterAcceleratorKey(cmd_ReplaceAll, GDK_equal, GDK_CONTROL_MASK | GDK_SHIFT_MASK);
		sInstance->RegisterAcceleratorKey(cmd_ReplaceFindNext, GDK_T, GDK_CONTROL_MASK);
		sInstance->RegisterAcceleratorKey(cmd_ReplaceFindPrev, GDK_t, GDK_CONTROL_MASK | GDK_SHIFT_MASK);
		sInstance->RegisterAcceleratorKey(cmd_CompleteLookingBack, GDK_Tab, GDK_CONTROL_MASK);
		sInstance->RegisterAcceleratorKey(cmd_CompleteLookingFwd, GDK_ISO_Left_Tab, GDK_CONTROL_MASK | GDK_SHIFT_MASK);
		sInstance->RegisterAcceleratorKey(cmd_GoToLine, GDK_comma, GDK_CONTROL_MASK);
		sInstance->RegisterAcceleratorKey(cmd_JumpToNextMark, GDK_F2, 0);
		sInstance->RegisterAcceleratorKey(cmd_JumpToPrevMark, GDK_F2, GDK_SHIFT_MASK);
		sInstance->RegisterAcceleratorKey(cmd_MarkLine, GDK_F1, 0);
	
		sInstance->RegisterAcceleratorKey(cmd_BringUpToDate, GDK_U, GDK_CONTROL_MASK);
		sInstance->RegisterAcceleratorKey(cmd_Compile, GDK_K, GDK_CONTROL_MASK);
		sInstance->RegisterAcceleratorKey(cmd_CheckSyntax, GDK_semicolon, GDK_CONTROL_MASK);
		sInstance->RegisterAcceleratorKey(cmd_Make, GDK_M, GDK_CONTROL_MASK | GDK_MOD1_MASK);
	
		sInstance->RegisterAcceleratorKey(cmd_Worksheet, GDK_0, GDK_CONTROL_MASK);
	}

	return *sInstance.get();
}

MAcceleratorTable&
MAcceleratorTable::EditKeysInstance()
{
	static auto_ptr<MAcceleratorTable>	sInstance;
	
	if (sInstance.get() == nil)
	{
		sInstance.reset(new MAcceleratorTable);

		sInstance->RegisterAcceleratorKey(kcmd_MoveCharacterLeft, GDK_Left, 0);
		sInstance->RegisterAcceleratorKey(kcmd_MoveCharacterRight, GDK_Right, 0);
		sInstance->RegisterAcceleratorKey(kcmd_MoveWordLeft, GDK_Left, GDK_CONTROL_MASK);
		sInstance->RegisterAcceleratorKey(kcmd_MoveWordRight, GDK_Right, GDK_CONTROL_MASK);
		sInstance->RegisterAcceleratorKey(kcmd_MoveToBeginningOfLine, GDK_Home, 0);
		sInstance->RegisterAcceleratorKey(kcmd_MoveToEndOfLine, GDK_End, 0);
		sInstance->RegisterAcceleratorKey(kcmd_MoveToPreviousLine, GDK_Up, 0);
		sInstance->RegisterAcceleratorKey(kcmd_MoveToNextLine, GDK_Down, 0);
		sInstance->RegisterAcceleratorKey(kcmd_MoveToTopOfPage, GDK_Page_Up, 0);
		sInstance->RegisterAcceleratorKey(kcmd_MoveToEndOfPage, GDK_Page_Down, 0);
		sInstance->RegisterAcceleratorKey(kcmd_MoveToBeginningOfFile, GDK_Home, GDK_CONTROL_MASK);
		sInstance->RegisterAcceleratorKey(kcmd_MoveToEndOfFile, GDK_End, GDK_CONTROL_MASK);

		sInstance->RegisterAcceleratorKey(kcmd_MoveLineUp, GDK_Up, GDK_MOD1_MASK);
		sInstance->RegisterAcceleratorKey(kcmd_MoveLineDown, GDK_Down, GDK_MOD1_MASK);

		sInstance->RegisterAcceleratorKey(kcmd_DeleteCharacterLeft, GDK_BackSpace, 0);
		sInstance->RegisterAcceleratorKey(kcmd_DeleteCharacterRight, GDK_Delete, 0);
		sInstance->RegisterAcceleratorKey(kcmd_DeleteWordLeft, GDK_BackSpace, GDK_CONTROL_MASK);
		sInstance->RegisterAcceleratorKey(kcmd_DeleteWordRight, GDK_Delete, GDK_CONTROL_MASK);
		sInstance->RegisterAcceleratorKey(kcmd_DeleteToEndOfLine, GDK_Delete, GDK_SHIFT_MASK);
		sInstance->RegisterAcceleratorKey(kcmd_DeleteToEndOfFile, GDK_Delete, GDK_CONTROL_MASK | GDK_SHIFT_MASK);

		sInstance->RegisterAcceleratorKey(kcmd_ExtendSelectionWithCharacterLeft, GDK_Left, GDK_SHIFT_MASK);
		sInstance->RegisterAcceleratorKey(kcmd_ExtendSelectionWithCharacterRight, GDK_Right, GDK_SHIFT_MASK);
		sInstance->RegisterAcceleratorKey(kcmd_ExtendSelectionWithPreviousWord, GDK_Left, GDK_SHIFT_MASK | GDK_CONTROL_MASK);
		sInstance->RegisterAcceleratorKey(kcmd_ExtendSelectionWithNextWord, GDK_Right, GDK_SHIFT_MASK | GDK_CONTROL_MASK);
		sInstance->RegisterAcceleratorKey(kcmd_ExtendSelectionToCurrentLine, GDK_L, GDK_CONTROL_MASK);
		sInstance->RegisterAcceleratorKey(kcmd_ExtendSelectionToPreviousLine, GDK_Up, GDK_SHIFT_MASK);
		sInstance->RegisterAcceleratorKey(kcmd_ExtendSelectionToNextLine, GDK_Down, GDK_SHIFT_MASK);
		sInstance->RegisterAcceleratorKey(kcmd_ExtendSelectionToBeginningOfLine, GDK_Home, GDK_SHIFT_MASK);
		sInstance->RegisterAcceleratorKey(kcmd_ExtendSelectionToEndOfLine, GDK_End, GDK_SHIFT_MASK);
		sInstance->RegisterAcceleratorKey(kcmd_ExtendSelectionToBeginningOfPage, GDK_Page_Up, GDK_SHIFT_MASK);
		sInstance->RegisterAcceleratorKey(kcmd_ExtendSelectionToEndOfPage, GDK_Page_Down, GDK_SHIFT_MASK);
		sInstance->RegisterAcceleratorKey(kcmd_ExtendSelectionToBeginningOfFile, GDK_Home, GDK_SHIFT_MASK | GDK_CONTROL_MASK);
		sInstance->RegisterAcceleratorKey(kcmd_ExtendSelectionToEndOfFile, GDK_End, GDK_SHIFT_MASK | GDK_CONTROL_MASK);

		sInstance->RegisterAcceleratorKey(kcmd_ScrollOneLineUp, GDK_Up, GDK_CONTROL_MASK);
		sInstance->RegisterAcceleratorKey(kcmd_ScrollOneLineDown, GDK_Down, GDK_CONTROL_MASK);
		sInstance->RegisterAcceleratorKey(kcmd_ScrollPageUp, GDK_Page_Up, GDK_CONTROL_MASK);
		sInstance->RegisterAcceleratorKey(kcmd_ScrollPageDown, GDK_Page_Down, GDK_CONTROL_MASK);
//		sInstance->RegisterAcceleratorKey(kcmd_ScrollToStartOfFile, GDK_Home, GDK_CONTROL_MASK);
//		sInstance->RegisterAcceleratorKey(kcmd_ScrollToEndOfFile, GDK_End, GDK_CONTROL_MASK);
	}
	
	return *sInstance.get();
}

MAcceleratorTable::MAcceleratorTable()
	: mImpl(new MAcceleratorTableImp)
{
//	// debug code, dump the current table
//	for (map<int64,uint32>::iterator a = mImpl->mTable.begin(); a != mImpl->mTable.end(); ++a)
//	{
//		uint32 key = static_cast<uint32>(a->first >> 32);
//		uint32 mod = static_cast<uint32>(a->first) & kValidModifiersMask;
//		
//		cout << gdk_keyval_name(key);
//
//		if (mod & GDK_CONTROL_MASK)
//			cout << " CTRL";
//		
//		if (mod & GDK_SHIFT_MASK)
//			cout << " SHIFT";
//		
//		if (mod & GDK_MOD1_MASK)
//			cout << " ALT";
//		
//		if (mod & GDK_MOD2_MASK)
//			cout << " MOD2";
//
//		cout << " mapped to " << MCommandToString(a->second) << endl;
//	}
}

void MAcceleratorTable::RegisterAcceleratorKey(
	uint32			inCommand,
	uint32			inKeyValue,
	uint32			inModifiers)
{
	int64 key = (int64(gdk_keyval_to_upper(inKeyValue)) << 32) | (inModifiers & kValidModifiersMask);
//	int64 key = int64(inKeyValue) << 32 | (inModifiers & kValidModifiersMask);
	
	mImpl->mTable[key] = inCommand;
}

bool MAcceleratorTable::GetAcceleratorKeyForCommand(
	uint32			inCommand,
	uint32&			outKeyValue,
	uint32&			outModifiers)
{
	bool result = false;
	
	for (map<int64,uint32>::iterator a = mImpl->mTable.begin(); a != mImpl->mTable.end(); ++a)
	{
		if (a->second == inCommand)
		{
			outKeyValue = static_cast<uint32>(a->first >> 32);
			outModifiers = static_cast<uint32>(a->first) & kValidModifiersMask;
			result = true;
			break;
		}
	}
	
	return result;
}

bool MAcceleratorTable::IsAcceleratorKey(
	GdkEventKey*	inEvent,
	uint32&			outCommand)
{
	bool result = false;

//cout << "IsAcceleratorKey for " << gdk_keyval_name(inEvent->keyval);
//
//if (inEvent->state & GDK_CONTROL_MASK)
//	cout << " CTRL";
//
//if (inEvent->state & GDK_SHIFT_MASK)
//	cout << " SHIFT";
//
//if (inEvent->state & GDK_MOD1_MASK)
//	cout << " ALT";
//
//if (inEvent->state & GDK_MOD2_MASK)
//	cout << " MOD2";

	int keyval = gdk_keyval_to_upper(inEvent->keyval);
//	int keyval = inEvent->keyval;
	int64 key = (int64(keyval) << 32) | (inEvent->state & kValidModifiersMask);

	map<int64,uint32>::iterator a = mImpl->mTable.find(key);	
	if (a != mImpl->mTable.end())
	{
		outCommand = a->second;
		result = true;
		
//cout << " mapped to " << MCommandToString(outCommand);
	}

//cout << endl;
	
	return result;
}

bool MAcceleratorTable::IsNavigationKey(
	uint32			inKeyValue,
	uint32			inModifiers,
	MKeyCommand&	outCommand)
{
	bool result = false;

	int keyval = gdk_keyval_to_upper(inKeyValue);
	int64 key = (int64(keyval) << 32) | (inModifiers & kValidModifiersMask);

	map<int64,uint32>::iterator a = mImpl->mTable.find(key);	
	if (a != mImpl->mTable.end())
	{
		outCommand = MKeyCommand(a->second);
		result = true;
	}
	
	return result;
}


