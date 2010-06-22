//          Copyright Maarten L. Hekkelman 2006-2010
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef MLISTIMPL_H
#define MLISTIMPL_H

#include "MListView.h"

class MListViewImpl
{
public:
							MListViewImpl(MListView* inListView)
								: mListView(inListView) {}
	virtual					~MListViewImpl() {}

	virtual void			AddedToWindow() {}
	virtual void			FrameResized() {}

	virtual void			AppendColumn(
								const std::string&	inLabel,
								int					inWidth) = 0;

	static MListViewImpl*	Create(MListView* inListView);

protected:
	MListView*				mListView;
};

#endif
