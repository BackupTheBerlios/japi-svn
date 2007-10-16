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

/*	$Id: MFindDialog.h 158 2007-05-28 10:08:18Z maarten $
	Copyright Maarten L. Hekkelman
*/

#ifndef MFINDDIALOG_H
#define MFINDDIALOG_H

#include <deque>
#include <boost/thread.hpp>

//#include "MDialog.h"
#include "MFile.h"
#include "MP2PEvents.h"
//#include "MScrap.h"

class MDocument;
class MMessageList;

const uint32
	kFindStringCount = 10;

enum MMultiMethod
{
	eMMDirectory,
	eMMOpenWindows,
	eMMIncludes
};	

class MFindDialog // : public MDialog
{
  public:

	static MFindDialog&	Instance();
	
	void			Select();
	virtual void	Close();

	void			FindNext();
	void			FindAll(
						const std::string&	inWhat,
						bool				inIgnoreCase,
						bool				inRegex);

	bool			GetInSelection() const;
	bool			GetWrap() const;
	bool			GetIgnoreCase() const;
	bool			GetRegex() const;

	std::string		GetFindString();
	void			SetFindString(
						const std::string&	inString,
						bool				inReplaceFirst = false);
	
	std::string		GetReplaceString();
	void			SetReplaceString(
						const std::string&	inString);

  private:

	typedef std::deque<std::string>			StringArray;
	typedef std::deque<MPath>				FileArray;

					MFindDialog();

	void			Initialize();

	virtual bool	OKClicked();

	void			ShowHideMultiPanel(
						bool				inShow);

//	virtual OSStatus
//					DoControlHit(EventRef inEvent);
//
//	OSStatus		DoWindowActivate(
//						EventRef			inEvent);

	void			StoreComboText(
						uint32				inID,
						StringArray&		inArray);
	
	void			DoFindCommand(
						uint32				inCommand);

	void			FindAll(
						const std::string&	inWhat,
						bool				inIgnoreCase,
						bool				inRegex,
						MMultiMethod		inMethod,
						MPath				inDirectory,
						bool				inRecursive,
						bool				inTextFilesOnly,
						const std::string&	inFileNameFilter);
	
	void			GetFilesForFindAll(
						MMultiMethod		inMethod,
						const MPath&			inDirectory,
						bool				inRecursive,
						bool				inTextFilesOnly,
						const std::string&	inFileNameFilter,
						FileArray&			outFiles);
	
	void			SetStatusString(
						const std::string&	inMessage);

	MEventIn<void()>	eIdle;
	void			Idle();

	bool			mMultiMode;
	bool			mInSelection;
	StringArray		mFindStrings;
	StringArray		mReplaceStrings;
	StringArray		mStartDirectories;
	FileArray		mMultiFiles;

	boost::thread*	mFindAllThread;
	boost::mutex	mFindDialogMutex;
	std::string		mCurrentMultiFile;
	MMessageList*	mFindAllResult;
};

#endif // MFINDDIALOG_H
