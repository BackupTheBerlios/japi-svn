//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef MCOMMANDS_H
#define MCOMMANDS_H

// stock commands

enum MStdCommand {
	cmd_New =					'new ',
	cmd_Open =					'open',
	cmd_Quit =					'quit',
	cmd_Close =					'clos',
	cmd_Save =					'save',
	cmd_SaveAs =				'sava',
	cmd_Revert =				'reve',
	cmd_PageSetup =				'pgsu',
	cmd_Print =					'prnt',
	cmd_Undo =					'undo',
	cmd_Redo =					'redo',
	cmd_Cut =					'cut ',
	cmd_Copy =					'copy',
	cmd_Paste =					'past',
	cmd_Clear =					'clea',
	
	cmd_About =					'abou',
	cmd_Help =					'help',
	
	cmd_Menu =					'menu',		// Shift-F10 or Menu key
};

// private commands

const uint32
	cmd_NewProject =			'NewP',
	cmd_Balance =				'Bala',
	cmd_ShiftLeft =				'ShiL',
	cmd_ShiftRight =			'ShiR',
	cmd_Comment =				'Comm',
	cmd_Uncomment =				'Unco',
	cmd_Entab =					'Enta',
	cmd_Detab =					'Deta',
	cmd_PasteNext =				'PstN',
	cmd_CopyAppend =			'CopA',
	cmd_CutAppend =				'CutA',
	cmd_SelectAll =				'sall',
	cmd_FastFind =				'FFnd',
	cmd_FastFindBW =			'FFnb',
	cmd_MarkLine =				'Mark',
	cmd_ClearMarks =			'ClAM',
	cmd_CutMarkedLines =		'CuML',
	cmd_CopyMarkedLines =		'CoML',
	cmd_ClearMarkedLines =		'ClML',
	cmd_JumpToNextMark =		'NxtM',
	cmd_JumpToPrevMark =		'PrvM',
	cmd_Find = 					'Find',
	cmd_FindNext =				'FndN',
	cmd_FindPrev =				'FndP',
	cmd_FindInNextFile =		'FnND',
	cmd_MarkMatching =			'F&Mk',
	cmd_EnterSearchString =		'EntF',
	cmd_EnterReplaceString =	'EntR',
	cmd_Replace =				'Repl',
	cmd_ReplaceAll =			'RplA',
	cmd_ReplaceFindNext =		'RpFN',
	cmd_ReplaceFindPrev =		'RpFP',
	cmd_CompleteLookingBack =	'ComB',
	cmd_CompleteLookingFwd =	'ComF',
	cmd_OpenIncludeFile =		'OInc',
	cmd_GoToLine =				'GoTo',
	cmd_SwitchHeaderSource =	'SHdS',
	cmd_Softwrap = 				'SftW',
	cmd_ChangeCaseUpper =		'CsUP',
	cmd_ChangeCaseLower =		'Cslw',
	cmd_ChangeCaseMixed =		'CsMx',
	cmd_ShowDiffWindow =		'diff',
	cmd_ShowDocInfoDialog = 	'DocI',
	cmd_AddFileToProject =		'Padd',
	cmd_CheckSyntax =			'Psyn',
	cmd_Preprocess =			'Ppre',
	cmd_Compile =				'Pcmp',
	cmd_Disassemble =			'Pdis',
	cmd_BringUpToDate =			'Pupd',
	cmd_Make =					'Pmak',
	cmd_MakeClean =				'Pcln',
	cmd_Run =					'Prun',
	cmd_RecheckFiles =			'Rchk',
	cmd_NewGroup =				'Pnwg',
	cmd_OpenIncludeFromMenu =	'OMic',
	cmd_SelectFunctionFromMenu ='OMfu',
	cmd_Preferences	=			'pref',
	cmd_CloseAll =				'Clos',
	cmd_SaveAll =				'Save',
	cmd_OpenRecent =			'Recd',
	cmd_OpenTemplate =			'Tmpd',
	cmd_ClearRecent =			'ClrR',
	cmd_Worksheet =				'Wrks',
	cmd_SelectWindowFromMenu =	'WSel',
	
	cmd_2CharsPerTab =			'Tw_2',
	cmd_4CharsPerTab =			'Tw_4',
	cmd_8CharsPerTab =			'Tw_8',
	cmd_16CharsPerTab =			'Tw16',
	
	cmd_SyntaxNone =			'St_N',
	cmd_SyntaxLanguage =		'St_L',
	
	cmd_Stop =					'stop',
	
	cmd_ApplyScript =			'scri',
	cmd_QuotedRewrap =			'rewr',
	
	cmd_ShowHideWhiteSpace =	'SWit';

#ifndef NDEBUG
const uint32 cmd_Test =					'Test';
#endif

// ---------------------------------------------------------------------------
//
// edit key commands
//

enum MKeyCommand
{
	kcmd_None,
	
	kcmd_MoveCharacterLeft,
	kcmd_MoveCharacterRight,
	kcmd_MoveWordLeft,
	kcmd_MoveWordRight,
	kcmd_MoveToBeginningOfLine,
	kcmd_MoveToEndOfLine,
	kcmd_MoveToPreviousLine,
	kcmd_MoveToNextLine,
	kcmd_MoveToTopOfPage,
	kcmd_MoveToEndOfPage,
	kcmd_MoveToBeginningOfFile,
	kcmd_MoveToEndOfFile,

	kcmd_MoveLineUp,
	kcmd_MoveLineDown,

	kcmd_DeleteCharacterLeft,
	kcmd_DeleteCharacterRight,
	kcmd_DeleteWordLeft,
	kcmd_DeleteWordRight,
	kcmd_DeleteToEndOfLine,
	kcmd_DeleteToEndOfFile,

	kcmd_ExtendSelectionWithCharacterLeft,
	kcmd_ExtendSelectionWithCharacterRight,
	kcmd_ExtendSelectionWithPreviousWord,
	kcmd_ExtendSelectionWithNextWord,
	kcmd_ExtendSelectionToCurrentLine,
	kcmd_ExtendSelectionToPreviousLine,
	kcmd_ExtendSelectionToNextLine,
	kcmd_ExtendSelectionToBeginningOfLine,
	kcmd_ExtendSelectionToEndOfLine,
	kcmd_ExtendSelectionToBeginningOfPage,
	kcmd_ExtendSelectionToEndOfPage,
	kcmd_ExtendSelectionToBeginningOfFile,
	kcmd_ExtendSelectionToEndOfFile,

	kcmd_ScrollOneLineUp,
	kcmd_ScrollOneLineDown,
	kcmd_ScrollPageUp,
	kcmd_ScrollPageDown,
	kcmd_ScrollToStartOfFile,
	kcmd_ScrollToEndOfFile
};

#endif
