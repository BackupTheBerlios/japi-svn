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

#include "MJapi.h"

#include <set>
#include <memory>
#include <gdk/gdkkeysyms.h>
#include <cstring>

#include "MCommands.h"
#include "MAcceleratorTable.h"

using namespace std;

const uint32
//	kValidModifiersMask = GDK_CONTROL_MASK | GDK_SHIFT_MASK | GDK_MOD1_MASK;
	kValidModifiersMask = gtk_accelerator_get_default_mod_mask();

namespace {

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

struct MAccelCombo
{
	int64		key;
	uint32		keyval;
	uint32		modifiers;
	uint32		command;
	
	bool		operator<(const MAccelCombo& rhs) const
					{ return key < rhs.key; }
};


struct MAcceleratorTableImp
{
	set<MAccelCombo>	mTable;
};

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
		sInstance->RegisterAcceleratorKey(cmd_Uncomment, GDK_apostrophe, GDK_CONTROL_MASK | GDK_SHIFT_MASK);
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
		sInstance->RegisterAcceleratorKey(cmd_CompleteLookingFwd, GDK_Tab, GDK_CONTROL_MASK | GDK_SHIFT_MASK);
		sInstance->RegisterAcceleratorKey(cmd_GoToLine, GDK_comma, GDK_CONTROL_MASK);
		sInstance->RegisterAcceleratorKey(cmd_JumpToNextMark, GDK_F2, 0);
		sInstance->RegisterAcceleratorKey(cmd_JumpToPrevMark, GDK_F2, GDK_SHIFT_MASK);
		sInstance->RegisterAcceleratorKey(cmd_MarkLine, GDK_F1, 0);
	
		sInstance->RegisterAcceleratorKey(cmd_BringUpToDate, GDK_U, GDK_CONTROL_MASK);
		sInstance->RegisterAcceleratorKey(cmd_Compile, GDK_K, GDK_CONTROL_MASK);
		sInstance->RegisterAcceleratorKey(cmd_CheckSyntax, GDK_semicolon, GDK_CONTROL_MASK);
		sInstance->RegisterAcceleratorKey(cmd_Make, GDK_M, GDK_CONTROL_MASK | GDK_SHIFT_MASK);
	
		sInstance->RegisterAcceleratorKey(cmd_Worksheet, GDK_0, GDK_CONTROL_MASK);

		sInstance->RegisterAcceleratorKey(cmd_Stop, GDK_period, GDK_CONTROL_MASK);
		
		sInstance->RegisterAcceleratorKey(cmd_Menu, GDK_Menu, 0);
		sInstance->RegisterAcceleratorKey(cmd_Menu, GDK_F10, GDK_SHIFT_MASK);
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

		sInstance->RegisterAcceleratorKey(kcmd_ScrollOneLineUp, GDK_Down, GDK_CONTROL_MASK);
		sInstance->RegisterAcceleratorKey(kcmd_ScrollOneLineDown, GDK_Up, GDK_CONTROL_MASK);
		sInstance->RegisterAcceleratorKey(kcmd_ScrollPageUp, GDK_Page_Up, GDK_CONTROL_MASK);
		sInstance->RegisterAcceleratorKey(kcmd_ScrollPageDown, GDK_Page_Down, GDK_CONTROL_MASK);
//		sInstance->RegisterAcceleratorKey(kcmd_ScrollToStartOfFile, GDK_Home, GDK_CONTROL_MASK);
//		sInstance->RegisterAcceleratorKey(kcmd_ScrollToEndOfFile, GDK_End, GDK_CONTROL_MASK);

		sInstance->RegisterAcceleratorKey(kcmd_MoveLineUp, GDK_Up, GDK_CONTROL_MASK | GDK_SHIFT_MASK);
		sInstance->RegisterAcceleratorKey(kcmd_MoveLineDown, GDK_Down, GDK_CONTROL_MASK | GDK_SHIFT_MASK);
	}
	
	return *sInstance.get();
}

MAcceleratorTable::MAcceleratorTable()
	: mImpl(new MAcceleratorTableImp)
{
}

void MAcceleratorTable::RegisterAcceleratorKey(
	uint32			inCommand,
	uint32			inKeyValue,
	uint32			inModifiers)
{
	gint nkeys;
	GdkKeymapKey* keys;
	
	if (gdk_keymap_get_entries_for_keyval(gdk_keymap_get_default(),
		inKeyValue, &keys, &nkeys))
	{
		for (int32 k = 0; k < nkeys; ++k)
		{
			guint keyval;
			GdkModifierType consumed;
			
			if (gdk_keymap_translate_keyboard_state(nil,
				keys[k].keycode, GdkModifierType(inModifiers), 0, &keyval, nil, nil, &consumed))
			{
				uint32 modifiers = inModifiers & ~consumed;
				if (inModifiers & GDK_SHIFT_MASK)
					modifiers |= GDK_SHIFT_MASK;				
				
				int64 key = (int64(keyval) << 32) | modifiers;
				
				MAccelCombo kc = {
					key, inKeyValue, inModifiers, inCommand
				};
				
				mImpl->mTable.insert(kc);
			}

			if (gdk_keymap_translate_keyboard_state(nil,
				keys[k].keycode, GdkModifierType(inModifiers | GDK_LOCK_MASK), 0, &keyval, nil, nil, &consumed))
			{
				uint32 modifiers = inModifiers & ~consumed;
				if (inModifiers & GDK_SHIFT_MASK)
					modifiers |= GDK_SHIFT_MASK;				
				
				int64 key = (int64(keyval) << 32) | modifiers;
				
				MAccelCombo kc = {
					key, inKeyValue, inModifiers, inCommand
				};
				
				mImpl->mTable.insert(kc);
			}
		}

		g_free(keys);
	}
}

bool MAcceleratorTable::GetAcceleratorKeyForCommand(
	uint32			inCommand,
	uint32&			outKeyValue,
	uint32&			outModifiers)
{
	bool result = false;
	
	for (set<MAccelCombo>::iterator a = mImpl->mTable.begin(); a != mImpl->mTable.end(); ++a)
	{
		if (a->command == inCommand)
		{
			outKeyValue = a->keyval;
			outModifiers = a->modifiers;
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

	int keyval = inEvent->keyval;
	int modifiers = inEvent->state & kValidModifiersMask;

//	PRINT(("IsAcceleratorKey for %10.10s %c-%c-%c-%c (%x - %x)",
//		gdk_keyval_name(keyval),
//		inEvent->state & GDK_CONTROL_MASK ? 'C' : '_',
//		inEvent->state & GDK_SHIFT_MASK ? 'S' : '_',
//		inEvent->state & GDK_MOD1_MASK ? '1' : '_',
//		inEvent->state & GDK_MOD2_MASK ? '2' : '_',
//		inEvent->state, modifiers));

	int64 key = (int64(keyval) << 32) | modifiers;

	MAccelCombo kc;
	kc.key = key;

	set<MAccelCombo>::iterator a = mImpl->mTable.find(kc);	
	if (a != mImpl->mTable.end())
	{
		outCommand = a->command;
		result = true;
	}
	
//	if (result)
//		PRINT(("cmd is %s", (const char*)MCommandToString(outCommand)));
	
	return result;
}

bool MAcceleratorTable::IsNavigationKey(
	uint32			inKeyValue,
	uint32			inModifiers,
	MKeyCommand&	outCommand)
{
	bool result = false;
	
//	PRINT(("IsNavigationKey for %10.10s %c-%c-%c-%c (%x - %x)",
//		gdk_keyval_name(inKeyValue),
//		inModifiers & GDK_CONTROL_MASK ? 'C' : '_',
//		inModifiers & GDK_SHIFT_MASK ? 'S' : '_',
//		inModifiers & GDK_MOD1_MASK ? '1' : '_',
//		inModifiers & GDK_MOD2_MASK ? '2' : '_',
//		inModifiers, inModifiers & kValidModifiersMask));

	inModifiers &= kValidModifiersMask;

	MAccelCombo kc;
	kc.key = (int64(inKeyValue) << 32) | inModifiers;

	set<MAccelCombo>::iterator a = mImpl->mTable.find(kc);	
	if (a != mImpl->mTable.end())
	{
		outCommand = MKeyCommand(a->command);
		result = true;
	}

//	if (result)
//		PRINT(("cmd is %s", (const char*)MCommandToString(outCommand)));
//	
	return result;
}


