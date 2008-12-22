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

#include <glade/glade-xml.h>
#include <vector>

#include "MWindow.h"
#include "MColor.h"

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
	
	void			SetCloseImmediatelyFlag(
						bool				inCloseImmediately);

	void			SetFocus(
						uint32				inID);
	
	void			GetText(
						uint32				inID,
						std::string&		outText) const;

	void			SetText(
						uint32				inID,
						const std::string&	inText);

	void			SetPasswordField(
						uint32				inID,
						bool				isVisible);

	int32			GetValue(
						uint32				inID) const;

	void			SetValue(
						uint32				inID,
						int32				inValue);

	// for comboboxes
	void			GetValues(
						uint32				inID,
						std::vector<std::string>& 
											outValues) const;

	void			SetValues(
						uint32				inID,
						const std::vector<std::string>&
											inValues);

	void			SetColor(
						uint32				inID,
						MColor				inColor);

	MColor			GetColor(
						uint32				inID) const;

	bool			IsChecked(
						uint32				inID) const;

	void			SetChecked(
						uint32				inID,
						bool				inOn);

	bool			IsVisible(
						uint32				inID) const;

	void			SetVisible(
						uint32				inID,
						bool				inVisible);

	bool			IsEnabled(
						uint32				inID) const;
	
	void			SetEnabled(
						uint32				inID,
						bool				inEnabled);

	bool			IsExpanded(
						uint32				inID) const;
	
	void			SetExpanded(
						uint32				inID,
						bool				inExpanded);

	void			SetProgressFraction(
						uint32				inID,
						float				inFraction);

	virtual void	ValueChanged(
						uint32				inID);

  protected:

					MDialog(
						const char*			inDialogResource);

  private:

	static void		ChangedCallBack(
						GtkWidget*			inWidget,
						gpointer			inUserData);

	static void		StdBtnClickedCallBack(
						GtkWidget*			inWidget,
						gpointer			inUserData);

	MWindow*		mParentWindow;
	MDialog*		mNext;						// for the close all
	static MDialog*	sFirst;
	bool			mCloseImmediatelyOnOK;
};

#endif // MDIALOG_H
