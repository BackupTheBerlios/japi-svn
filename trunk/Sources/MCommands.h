/*
	Copyright (c) 2006, Maarten L. Hekkelman
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
	cmd_Undo =					'undo',
	cmd_Redo =					'redo',
	cmd_Cut =					'cut ',
	cmd_Copy =					'copy',
	cmd_Paste =					'past',
	cmd_Clear =					'clea',
	
};

// private commands

const uint32 cmd_Balance =				'Bala';
const uint32 cmd_ShiftLeft =			'ShiL';
const uint32 cmd_ShiftRight =			'ShiR';
const uint32 cmd_Comment =				'Comm';
const uint32 cmd_Uncomment =			'Unco';
const uint32 cmd_Entab =				'Enta';
const uint32 cmd_Detab =				'Deta';
const uint32 cmd_PasteNext =			'PstN';
const uint32 cmd_CopyAppend =			'CopA';
const uint32 cmd_CutAppend =			'CutA';
const uint32 cmd_SelectAll =			'sall';
const uint32 cmd_FastFind =				'FFnd';
const uint32 cmd_FastFindBW =			'FFnb';
const uint32 cmd_MarkLine =				'Mark';
const uint32 cmd_ClearMarks =			'ClAM';
const uint32 cmd_CutMarkedLines =		'CuML';
const uint32 cmd_CopyMarkedLines =		'CoML';
const uint32 cmd_ClearMarkedLines =		'ClML';
const uint32 cmd_JumpToNextMark =		'NxtM';
const uint32 cmd_JumpToPrevMark =		'PrvM';
const uint32 cmd_Find = 				'Find';
const uint32 cmd_FindNext =				'FndN';
const uint32 cmd_FindPrev =				'FndP';
const uint32 cmd_FindInNextFile =		'FnND';
const uint32 cmd_MarkMatching =			'F&Mk';
const uint32 cmd_EnterSearchString =	'EntF';
const uint32 cmd_EnterReplaceString =	'EntR';
const uint32 cmd_Replace =				'Repl';
const uint32 cmd_ReplaceAll =			'RplA';
const uint32 cmd_ReplaceFindNext =		'RpFN';
const uint32 cmd_ReplaceFindPrev =		'RpFP';
const uint32 cmd_CompleteLookingBack =	'ComB';
const uint32 cmd_CompleteLookingFwd =	'ComF';
const uint32 cmd_OpenIncludeFile =		'OInc';
const uint32 cmd_GoToLine =				'GoTo';
const uint32 cmd_SwitchHeaderSource =	'SHdS';
const uint32 cmd_Softwrap = 			'SftW';

const uint32 cmd_ChangeCaseUpper =		'CsUP';
const uint32 cmd_ChangeCaseLower =		'Cslw';
const uint32 cmd_ChangeCaseMixed =		'CsMx';

const uint32 cmd_ShowDiffWindow =		'diff';

const uint32 cmd_ShowDocInfoDialog = 	'DocI';

const uint32 cmd_AddFileToProject =		'Padd';
const uint32 cmd_CheckSyntax =			'Psyn';
const uint32 cmd_Precompile =			'Ppre';
const uint32 cmd_Compile =				'Pcmp';
const uint32 cmd_BringUpToDate =		'Pupd';
const uint32 cmd_Make =					'Pmak';
const uint32 cmd_MakeClean =			'Pcln';
const uint32 cmd_Run =					'Prun';
const uint32 cmd_NewGroup =				'Pnwg';

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

//// for emacs users:
//	kcmd_OpenLine,
//
//	kcmd_Mark,
//	kcmd_MarkAll,
//	kcmd_MarkWord,
//	kcmd_ExchangeMarkAndPoint,
//
//	kcmd_CutRegion,
//	kcmd_CopyRegion,
//	kcmd_ClearRegion,
//
//	kcmd_CuttoEndOfLine,
//	kcmd_CutWord,
//	kcmd_CutWordBackward,
//	kcmd_CutSentence,
//
//	kcmd_AppendNextCut,
//
//	kcmd_Recenter,

//	kcmd_NrPrefix,

//	kcmd_SwitchActivePart,
//	kcmd_SplitWindow,
//	kcmd_UnsplitWindow,
//
//	kcmd_StartBlockSelect,
//	kcmd_CopyLine,
//	kcmd_CutLine,
//	kcmd_ClearLine
};

#endif
