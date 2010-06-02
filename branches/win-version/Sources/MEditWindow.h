//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEDITWINDOW_H
#define MEDITWINDOW_H

#include "MDocWindow.h"
#include "MSelection.h"

class MParsePopup;
class MSSHProgress;
class MTextView;

class MEditWindow : public MDocWindow
{
  public:
						MEditWindow();

	virtual				~MEditWindow();
	
	static MEditWindow*	DisplayDocument(
							MDocument*		inDocument);

	static MEditWindow*	FindWindowForDocument(
							MDocument*		inDocument);

	virtual void		SetDocument(
							MDocument*		inDocument);

	virtual void		SaveState();
	
	MEventIn<void(MSelection,std::string)>	eSelectionChanged;
	MEventIn<void(bool)>					eShellStatus;
	MEventIn<void(float,std::string)>		eSSHProgress;

	virtual void		AddRoutes(
							MDocument*		inDocument);
	
	virtual void		RemoveRoutes(
							MDocument*		inDocument);

  protected:

	void				SelectionChanged(
							MSelection		inNewSelection,
							std::string		inRangeName);

	void				ShellStatus(
							bool			inActive);

	void				SSHProgress(
							float			inFraction,
							std::string		inMessage);
	
	MTextView*			mTextView;
	MParsePopup*		mParsePopup;
	MParsePopup*		mIncludePopup;
	MSSHProgress*		mSSHProgress;
};

#endif
