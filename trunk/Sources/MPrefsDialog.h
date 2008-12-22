//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef MPREFSDIALOG_H
#define MPREFSDIALOG_H

#include "MDialog.h"
#include "MP2PEvents.h"

class MPrefsDialog : public MDialog
{
  public:
	static void		Create();
	
	static MEventOut<void()>
					ePrefsChanged;
	
  private:
	
	enum
	{
		kKeywordSwatchNr,
		kPreprocessorSwatchNr,
		kCharConstSwatchNr,
		kCommentSwatchNr,
		kStringSwatchNr,
		kHTMLTagSwatchNr,
		kHTMLAttributeSwatchNr,
		kSwatchCount
	};


					MPrefsDialog();

	virtual bool	DoClose();
	
	void			SelectPage(
						uint32			inPageID);
	
	virtual bool	OKClicked();

	virtual void	ValueChanged(
						uint32				inID);

	uint32			mCurrentPage;

	static MPrefsDialog*
					sInstance;
};

#endif
