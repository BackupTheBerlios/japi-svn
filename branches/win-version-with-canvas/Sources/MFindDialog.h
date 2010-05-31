//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

/*	$Id: MFindDialog.h 158 2007-05-28 10:08:18Z maarten $
	Copyright Maarten L. Hekkelman
*/

#ifndef MFINDDIALOG_H
#define MFINDDIALOG_H

#include <deque>
#include <set>
#include <vector>

#include <boost/thread.hpp>

#include "MDialog.h"
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

class MFindDialog : public MDialog
{
  public:

	static MFindDialog&	Instance();

	void			Select();

	virtual bool	ProcessCommand(
						uint32				inCommand,
						const MMenu*		inMenu,
						uint32				inItemIndex,
							uint32			inModifiers);

	bool			FindNext();
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

	std::string		GetStartDirectory();

	virtual bool	DoClose();

  private:

					MFindDialog();

					~MFindDialog();

	typedef std::vector<std::string>		StringArray;
	typedef std::deque<fs::path>			FileArray;
	typedef std::set<fs::path>				FileSet;

	virtual bool	OKClicked();

	void			ShowHideMultiPanel(
						bool				inShow);

	virtual void	ValueChanged(
						uint32				inButonID);

	void			DoFindCommand(
						uint32				inCommand);

	void			ReplaceAll(
						bool				inSaveToDisk);

	void			FindAll(
						const std::string&	inWhat,
						bool				inIgnoreCase,
						bool				inRegex,
						MMultiMethod		inMethod,
						fs::path			inDirectory,
						bool				inRecursive,
						const std::string&	inFileNameFilter);
	
	void			GetFilesForFindAll(
						MMultiMethod		inMethod,
						const fs::path&		inDirectory,
						bool				inRecursive,
						const std::string&	inFileNameFilter,
						FileSet&			outFiles);
	
	void			SetStatusString(
						const std::string&	inMessage);

	void			SelectSearchDir();

	MEventIn<void(double)>	eIdle;
	void			Idle(
						double				inSystemTime);

	bool			Stop();

	bool			mMultiMode;
	bool			mInSelection;
	bool			mStopFindAll;
	bool			mUpdatingComboBox;
	bool			mFindStringChanged, mReplaceStringChanged, mStartDirectoriesChanged;
	bool			mVisible;
	StringArray		mFindStrings;
	StringArray		mReplaceStrings;
	StringArray		mStartDirectories;
	FileArray		mMultiFiles;

	boost::thread*	mFindAllThread;
	boost::mutex	mFindDialogMutex;
	std::string		mCurrentMultiFile;
	MMessageList*	mFindAllResult;
	
	static MFindDialog*
					sInstance;
};

#endif // MFINDDIALOG_H
