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
	
	void			SetFocus(const std::string& inID);

	std::string		GetText(const std::string& inID) const;
	void			SetText(const std::string& inID, const std::string& inText);

	int32			GetValue(const std::string& inID) const;
	void			SetValue(const std::string& inID, int32 inValue);

	bool			IsChecked(const std::string& inID) const;
	void			SetChecked(const std::string& inID, bool inChecked);

	void			SetChoices(const std::string& inID,
						std::vector<std::string>& inChoices);
	
	void			SetEnabled(const std::string& inID, bool inEnabled);
	void			SetVisible(const std::string& inID, bool inVisible);

	void			SetPasswordChar(const std::string& inID, uint32 inUnicode = 0x2022);

	virtual void	ButtonClicked(const std::string& inID);
	virtual void	CheckboxChanged(const std::string& inID, bool inValue);
	virtual void	TextChanged(const std::string& inID, const std::string& inText);

	MEventIn<void(const std::string&)>						eButtonClicked;
	MEventIn<void(const std::string&,bool)>					eCheckboxClicked;
	MEventIn<void(const std::string&,const std::string&)>	eTextChanged;

  protected:

					MDialog(
						const std::string&	inDialogResource);

  private:

	MWindow*		mParentWindow;
	MDialog*		mNext;						// for the close all
	static MDialog*	sFirst;
};

#endif // MDIALOG_H
