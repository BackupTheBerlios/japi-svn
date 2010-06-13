//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

/*	$Id: MDialog.h 133 2007-05-01 08:34:48Z maarten $
	Copyright Maarten L. Hekkelman
	Created Sunday August 15 2004 13:51:09
*/

#ifndef MDIALOG_H
#define MDIALOG_H

#include <vector>

#include "MWindow.h"

class MDialog : public MWindow
{
  public:
					~MDialog();

	virtual bool	OKClicked();

	virtual bool	CancelClicked();

	void			Show(
						MWindow*			inParent);

	void			SavePosition(
						const char*			inName);

	void			RestorePosition(
						const char*			inName);

	MWindow*		GetParentWindow() const				{ return mParentWindow; }
	
	std::string		GetText(uint32 inID) const;
	void			SetText(uint32 inID, const std::string& inText);

	bool			IsChecked(uint32 inID) const;
	void			SetChecked(uint32 inID, bool inChecked);

	void			SetChoices(uint32 inID,
						std::vector<std::string>& inChoices);
	
	void			SetEnabled(uint32 inID, bool inEnabled);
	void			SetVisible(uint32 inID, bool inEnabled);

  protected:

					MDialog(
						const std::string&	inDialogResource);

  private:

	//MSlot<void()>	mStdBtnClicked;
	//void			StdBtnClicked();

	MWindow*		mParentWindow;
	MDialog*		mNext;						// for the close all
	static MDialog*	sFirst;
};

#endif // MDIALOG_H
