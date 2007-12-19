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

/*	$Id: MDialog.h 133 2007-05-01 08:34:48Z maarten $
	Copyright Maarten L. Hekkelman
	Created Sunday August 15 2004 13:51:09
*/

#ifndef MDIALOG_H
#define MDIALOG_H

#include <glade/glade-xml.h>
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
