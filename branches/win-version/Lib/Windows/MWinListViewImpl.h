//          Copyright Maarten L. Hekkelman 2006-2010
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef MWINLISTIMPL_H
#define MWINLISTIMPL_H

#include "MListViewImpl.h"
#include "MWinProcMixin.h"
#include "MControls.h"

class MWinListViewImpl : public MListViewImpl
{
  public:
					MWinListViewImpl(
						MListView*		inListView);

					~MWinListViewImpl();

	virtual void	AppendColumn(
						const std::string&	inLabel,
						int					inWidth);

//	virtual void	AddedToWindow();

	MEventIn<void(uint32,uint32)>
					eColumnResized;

	void			ColumnResized(uint32 inItemNr, uint32 inWidth);

  protected:
	MListHeader*	mListHeader;
	std::vector<uint32>
					mColumnWidths;
};

#endif
