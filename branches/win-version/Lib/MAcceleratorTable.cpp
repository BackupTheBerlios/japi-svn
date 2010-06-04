//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "MLib.h"

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
	static MAcceleratorTable sInstance;
	return sInstance;
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

bool MAcceleratorTable::IsAcceleratorKey(
	uint32			inKeyCode,
	uint32			inModifiers,
	uint32&			outCommand)
{
	bool result = false;

//	PRINT(("IsAcceleratorKey for %10.10s %c-%c-%c-%c (%x - %x)",
//		gdk_keyval_name(keyval),
//		inEvent->state & kControlKey ? 'C' : '_',
//		inEvent->state & kShiftKey ? 'S' : '_',
//		inEvent->state & kOptionKey ? '1' : '_',
//		inEvent->state & GDK_MOD2_MASK ? '2' : '_',
//		inEvent->state, modifiers));

	int64 key = (int64(inKeyCode) << 32) | inModifiers;

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


