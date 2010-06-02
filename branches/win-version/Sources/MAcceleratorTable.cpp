//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "MJapi.h"

#include <set>
#include <memory>
#include <cstring>

#include "MCommands.h"
#include "MAcceleratorTable.h"

using namespace std;

const uint32
//	kValidModifiersMask = kControlKey | kShiftKey | kOptionKey;
	//kValidModifiersMask = gtk_accelerator_get_default_mod_mask();
	kValidModifiersMask = kControlKey | kShiftKey | kOptionKey;

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
	static unique_ptr<MAcceleratorTable>	sInstance;
	
	if (sInstance.get() == nil)
	{
		sInstance.reset(new MAcceleratorTable);

		//sInstance->RegisterAcceleratorKey(cmd_New, GDK_N, kControlKey);
		//sInstance->RegisterAcceleratorKey(cmd_Open, GDK_O, kControlKey);
		//sInstance->RegisterAcceleratorKey(cmd_OpenIncludeFile, GDK_D, kControlKey | kShiftKey);
		//sInstance->RegisterAcceleratorKey(cmd_SwitchHeaderSource, GDK_1, kControlKey);
		//sInstance->RegisterAcceleratorKey(cmd_Close, GDK_W, kControlKey);
		//sInstance->RegisterAcceleratorKey(cmd_CloseAll, GDK_W, kControlKey | kShiftKey);
		//sInstance->RegisterAcceleratorKey(cmd_Save, GDK_S, kControlKey);
		//sInstance->RegisterAcceleratorKey(cmd_SaveAll, GDK_S, kControlKey | kShiftKey);
		//sInstance->RegisterAcceleratorKey(cmd_Quit, GDK_Q, kControlKey);
		//sInstance->RegisterAcceleratorKey(cmd_Undo, GDK_Z, kControlKey);
		//sInstance->RegisterAcceleratorKey(cmd_Redo, GDK_Z, kControlKey | kShiftKey);
		//sInstance->RegisterAcceleratorKey(cmd_Cut, GDK_X, kControlKey);
		//sInstance->RegisterAcceleratorKey(cmd_CutAppend, GDK_X, kControlKey | kShiftKey);
		//sInstance->RegisterAcceleratorKey(cmd_Copy, GDK_C, kControlKey);
		//sInstance->RegisterAcceleratorKey(cmd_CopyAppend, GDK_C, kControlKey | kShiftKey);
		//sInstance->RegisterAcceleratorKey(cmd_Paste, GDK_V, kControlKey);
		//sInstance->RegisterAcceleratorKey(cmd_PasteNext, GDK_V, kControlKey | kShiftKey);
		//sInstance->RegisterAcceleratorKey(cmd_SelectAll, GDK_A, kControlKey);
		//sInstance->RegisterAcceleratorKey(cmd_Balance, GDK_B, kControlKey);
		//sInstance->RegisterAcceleratorKey(cmd_ShiftLeft, GDK_bracketleft, kControlKey);
		//sInstance->RegisterAcceleratorKey(cmd_ShiftRight, GDK_bracketright, kControlKey);
		//sInstance->RegisterAcceleratorKey(cmd_Comment, GDK_apostrophe, kControlKey);
		//sInstance->RegisterAcceleratorKey(cmd_Uncomment, GDK_apostrophe, kControlKey | kShiftKey);
		//sInstance->RegisterAcceleratorKey(cmd_FastFind, GDK_I, kControlKey);
		//sInstance->RegisterAcceleratorKey(cmd_FastFindBW, GDK_i, kControlKey | kShiftKey);
		//sInstance->RegisterAcceleratorKey(cmd_Find, GDK_F, kControlKey);
		//sInstance->RegisterAcceleratorKey(cmd_FindNext, GDK_G, kControlKey);
		//sInstance->RegisterAcceleratorKey(cmd_FindPrev, GDK_g, kControlKey | kShiftKey);
		//sInstance->RegisterAcceleratorKey(cmd_FindInNextFile, GDK_J, kControlKey);
		//sInstance->RegisterAcceleratorKey(cmd_EnterSearchString, GDK_E, kControlKey);
		//sInstance->RegisterAcceleratorKey(cmd_EnterReplaceString, GDK_e, kControlKey | kShiftKey);
		//sInstance->RegisterAcceleratorKey(cmd_Replace, GDK_equal, kControlKey);
		//sInstance->RegisterAcceleratorKey(cmd_ReplaceAll, GDK_equal, kControlKey | kShiftKey);
		//sInstance->RegisterAcceleratorKey(cmd_ReplaceFindNext, GDK_T, kControlKey);
		//sInstance->RegisterAcceleratorKey(cmd_ReplaceFindPrev, GDK_t, kControlKey | kShiftKey);
		//sInstance->RegisterAcceleratorKey(cmd_CompleteLookingBack, GDK_Tab, kControlKey);
		//sInstance->RegisterAcceleratorKey(cmd_CompleteLookingFwd, GDK_Tab, kControlKey | kShiftKey);
		//sInstance->RegisterAcceleratorKey(cmd_GoToLine, GDK_comma, kControlKey);
		//sInstance->RegisterAcceleratorKey(cmd_JumpToNextMark, GDK_F2, 0);
		//sInstance->RegisterAcceleratorKey(cmd_JumpToPrevMark, GDK_F2, kShiftKey);
		//sInstance->RegisterAcceleratorKey(cmd_MarkLine, GDK_F1, 0);
	
		//sInstance->RegisterAcceleratorKey(cmd_BringUpToDate, GDK_U, kControlKey);
		//sInstance->RegisterAcceleratorKey(cmd_Compile, GDK_K, kControlKey);
		//sInstance->RegisterAcceleratorKey(cmd_CheckSyntax, GDK_semicolon, kControlKey);
		//sInstance->RegisterAcceleratorKey(cmd_Make, GDK_M, kControlKey | kShiftKey);
	
		//sInstance->RegisterAcceleratorKey(cmd_Worksheet, GDK_0, kControlKey);

		//sInstance->RegisterAcceleratorKey(cmd_Stop, GDK_period, kControlKey);
		//
		//sInstance->RegisterAcceleratorKey(cmd_Menu, GDK_Menu, 0);
		//sInstance->RegisterAcceleratorKey(cmd_Menu, GDK_F10, kShiftKey);
	}

	return *sInstance.get();
}

MAcceleratorTable&
MAcceleratorTable::EditKeysInstance()
{
	static unique_ptr<MAcceleratorTable>	sInstance;
	
	if (sInstance.get() == nil)
	{
		sInstance.reset(new MAcceleratorTable);

		sInstance->RegisterAcceleratorKey(kcmd_MoveCharacterLeft, kLeftArrowKeyCode, 0);
		sInstance->RegisterAcceleratorKey(kcmd_MoveCharacterRight, kRightArrowKeyCode, 0);
		sInstance->RegisterAcceleratorKey(kcmd_MoveWordLeft, kLeftArrowKeyCode, kControlKey);
		sInstance->RegisterAcceleratorKey(kcmd_MoveWordRight, kRightArrowKeyCode, kControlKey);
		sInstance->RegisterAcceleratorKey(kcmd_MoveToBeginningOfLine, kHomeKeyCode, 0);
		sInstance->RegisterAcceleratorKey(kcmd_MoveToEndOfLine, kEndKeyCode, 0);
		sInstance->RegisterAcceleratorKey(kcmd_MoveToPreviousLine, kUpArrowKeyCode, 0);
		sInstance->RegisterAcceleratorKey(kcmd_MoveToNextLine, kDownArrowKeyCode, 0);
		sInstance->RegisterAcceleratorKey(kcmd_MoveToTopOfPage, kPageUpKeyCode, 0);
		sInstance->RegisterAcceleratorKey(kcmd_MoveToEndOfPage, kPageDownKeyCode, 0);
		sInstance->RegisterAcceleratorKey(kcmd_MoveToBeginningOfFile, kHomeKeyCode, kControlKey);
		sInstance->RegisterAcceleratorKey(kcmd_MoveToEndOfFile, kEndKeyCode, kControlKey);

		sInstance->RegisterAcceleratorKey(kcmd_MoveLineUp, kUpArrowKeyCode, kOptionKey);
		sInstance->RegisterAcceleratorKey(kcmd_MoveLineDown, kDownArrowKeyCode, kOptionKey);

		sInstance->RegisterAcceleratorKey(kcmd_DeleteCharacterLeft, kBackspaceKeyCode, 0);
		sInstance->RegisterAcceleratorKey(kcmd_DeleteCharacterRight, kDeleteKeyCode, 0);
		sInstance->RegisterAcceleratorKey(kcmd_DeleteWordLeft, kBackspaceKeyCode, kControlKey);
		sInstance->RegisterAcceleratorKey(kcmd_DeleteWordRight, kDeleteKeyCode, kControlKey);
		sInstance->RegisterAcceleratorKey(kcmd_DeleteToEndOfLine, kDeleteKeyCode, kShiftKey);
		sInstance->RegisterAcceleratorKey(kcmd_DeleteToEndOfFile, kDeleteKeyCode, kControlKey | kShiftKey);

		sInstance->RegisterAcceleratorKey(kcmd_ExtendSelectionWithCharacterLeft, kLeftArrowKeyCode, kShiftKey);
		sInstance->RegisterAcceleratorKey(kcmd_ExtendSelectionWithCharacterRight, kRightArrowKeyCode, kShiftKey);
		sInstance->RegisterAcceleratorKey(kcmd_ExtendSelectionWithPreviousWord, kLeftArrowKeyCode, kShiftKey | kControlKey);
		sInstance->RegisterAcceleratorKey(kcmd_ExtendSelectionWithNextWord, kRightArrowKeyCode, kShiftKey | kControlKey);
		sInstance->RegisterAcceleratorKey(kcmd_ExtendSelectionToCurrentLine, 'L', kControlKey);
		sInstance->RegisterAcceleratorKey(kcmd_ExtendSelectionToPreviousLine, kUpArrowKeyCode, kShiftKey);
		sInstance->RegisterAcceleratorKey(kcmd_ExtendSelectionToNextLine, kDownArrowKeyCode, kShiftKey);
		sInstance->RegisterAcceleratorKey(kcmd_ExtendSelectionToBeginningOfLine, kHomeKeyCode, kShiftKey);
		sInstance->RegisterAcceleratorKey(kcmd_ExtendSelectionToEndOfLine, kEndKeyCode, kShiftKey);
		sInstance->RegisterAcceleratorKey(kcmd_ExtendSelectionToBeginningOfPage, kPageUpKeyCode, kShiftKey);
		sInstance->RegisterAcceleratorKey(kcmd_ExtendSelectionToEndOfPage, kPageDownKeyCode, kShiftKey);
		sInstance->RegisterAcceleratorKey(kcmd_ExtendSelectionToBeginningOfFile, kHomeKeyCode, kShiftKey | kControlKey);
		sInstance->RegisterAcceleratorKey(kcmd_ExtendSelectionToEndOfFile, kEndKeyCode, kShiftKey | kControlKey);

		sInstance->RegisterAcceleratorKey(kcmd_ScrollOneLineUp, kDownArrowKeyCode, kControlKey);
		sInstance->RegisterAcceleratorKey(kcmd_ScrollOneLineDown, kUpArrowKeyCode, kControlKey);
		sInstance->RegisterAcceleratorKey(kcmd_ScrollPageUp, kPageUpKeyCode, kControlKey);
		sInstance->RegisterAcceleratorKey(kcmd_ScrollPageDown, kPageDownKeyCode, kControlKey);
//		sInstance->RegisterAcceleratorKey(kcmd_ScrollToStartOfFile, GDK_Home, kControlKey);
//		sInstance->RegisterAcceleratorKey(kcmd_ScrollToEndOfFile, GDK_End, kControlKey);

		sInstance->RegisterAcceleratorKey(kcmd_MoveLineUp, kUpArrowKeyCode, kControlKey | kShiftKey);
		sInstance->RegisterAcceleratorKey(kcmd_MoveLineDown, kDownArrowKeyCode, kControlKey | kShiftKey);
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
	int64 key = (int64(inKeyValue) << 32) | inModifiers;
	MAccelCombo kc = {
		key, inKeyValue, inModifiers, inCommand
	};

	mImpl->mTable.insert(kc);

	//gint nkeys;
	//GdkKeymapKey* keys;
	//
	//if (gdk_keymap_get_entries_for_keyval(gdk_keymap_get_default(),
	//	inKeyValue, &keys, &nkeys))
	//{
	//	for (int32 k = 0; k < nkeys; ++k)
	//	{
	//		guint keyval;
	//		GdkModifierType consumed;
	//		
	//		if (gdk_keymap_translate_keyboard_state(nil,
	//			keys[k].keycode, GdkModifierType(inModifiers), 0, &keyval, nil, nil, &consumed))
	//		{
	//			uint32 modifiers = inModifiers & ~consumed;
	//			if (inModifiers & kShiftKey)
	//				modifiers |= kShiftKey;				
	//			
	//			int64 key = (int64(keyval) << 32) | modifiers;
	//			
	//			MAccelCombo kc = {
	//				key, inKeyValue, inModifiers, inCommand
	//			};
	//			
	//			mImpl->mTable.insert(kc);
	//		}

	//		if (gdk_keymap_translate_keyboard_state(nil,
	//			keys[k].keycode, GdkModifierType(inModifiers | GDK_LOCK_MASK), 0, &keyval, nil, nil, &consumed))
	//		{
	//			uint32 modifiers = inModifiers & ~consumed;
	//			if (inModifiers & kShiftKey)
	//				modifiers |= kShiftKey;				
	//			
	//			int64 key = (int64(keyval) << 32) | modifiers;
	//			
	//			MAccelCombo kc = {
	//				key, inKeyValue, inModifiers, inCommand
	//			};
	//			
	//			mImpl->mTable.insert(kc);
	//		}
	//	}

	//	g_free(keys);
	//}
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

//bool MAcceleratorTable::IsAcceleratorKey(
//	GdkEventKey*	inEvent,
//	uint32&			outCommand)
//{
//	bool result = false;
//
//	int keyval = inEvent->keyval;
//	int modifiers = inEvent->state & kValidModifiersMask;
//
////	PRINT(("IsAcceleratorKey for %10.10s %c-%c-%c-%c (%x - %x)",
////		gdk_keyval_name(keyval),
////		inEvent->state & kControlKey ? 'C' : '_',
////		inEvent->state & kShiftKey ? 'S' : '_',
////		inEvent->state & kOptionKey ? '1' : '_',
////		inEvent->state & GDK_MOD2_MASK ? '2' : '_',
////		inEvent->state, modifiers));
//
//	int64 key = (int64(keyval) << 32) | modifiers;
//
//	MAccelCombo kc;
//	kc.key = key;
//
//	set<MAccelCombo>::iterator a = mImpl->mTable.find(kc);	
//	if (a != mImpl->mTable.end())
//	{
//		outCommand = a->command;
//		result = true;
//	}
//	
////	if (result)
////		PRINT(("cmd is %s", (const char*)MCommandToString(outCommand)));
//	
//	return result;
//}

bool MAcceleratorTable::IsNavigationKey(
	uint32			inKeyValue,
	uint32			inModifiers,
	MKeyCommand&	outCommand)
{
	bool result = false;
	
//	PRINT(("IsNavigationKey for %10.10s %c-%c-%c-%c (%x - %x)",
//		gdk_keyval_name(inKeyValue),
//		inModifiers & kControlKey ? 'C' : '_',
//		inModifiers & kShiftKey ? 'S' : '_',
//		inModifiers & kOptionKey ? '1' : '_',
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


