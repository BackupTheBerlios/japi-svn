//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

/*
	Model = MDocument
	View = MTextView
	Controller = MController

*/


#ifndef MTEXTCONTROLLER_H
#define MTEXTCONTROLLER_H

#include "MController.h"
#include <list>

class MTextView;

class MTextController : public MController
{
  public:
						MTextController(
							MHandler*		inSuper);

						~MTextController();

	MTextView*			GetTextView();

	void				AddTextView(
							MTextView*		inTextView);

	void				RemoveTextView(
							MTextView*		inTextView);

	virtual bool		UpdateCommandStatus(
							uint32			inCommand,
							MMenu*			inMenu,
							uint32			inItemIndex,
							bool&			outEnabled,
							bool&			outChecked);

	virtual bool		ProcessCommand(
							uint32			inCommand,
							const MMenu*	inMenu,
							uint32			inItemIndex,
							uint32			inModifiers);

	bool				OpenInclude(
							std::string		inFileName);

	void				OpenCounterpart();

  protected:

	virtual void		Print();

	virtual bool		TryCloseDocument(
							MCloseReason	inAction);

  private:

	void				DoGoToLine();
	void				DoOpenIncludeFile();
	void				DoOpenCounterpart();
	void				DoMarkMatching();

	typedef std::list<MTextView*>	TextViewArray;

	TextViewArray		mTextViews;
};

#endif
